#include "display.hpp"

Display display;

static float sin_lut[256];
static float cos_lut[256];
static bool lut_initialized = false;

static void initTrigLUT()
{
    if (lut_initialized)
        return;
    for (int i = 0; i < 256; ++i)
    {
        float angle = (i * 2.0f * M_PI) / 256.0f;
        sin_lut[i] = sinf(angle);
        cos_lut[i] = cosf(angle);
    }
    lut_initialized = true;
}

static inline float fast_sin(float rad)
{
    float normalized = fmodf(rad, 2.0f * M_PI);
    if (normalized < 0.0f)
        normalized += 2.0f * M_PI;
    int idx = static_cast<int>(normalized * (256.0f / (2.0f * M_PI))) & 255;
    return sin_lut[idx];
}

static inline float fast_cos(float rad)
{
    float normalized = fmodf(rad, 2.0f * M_PI);
    if (normalized < 0.0f)
        normalized += 2.0f * M_PI;
    int idx = static_cast<int>(normalized * (256.0f / (2.0f * M_PI))) & 255;
    return cos_lut[idx];
}

Display::Display() : sprite(&canvas), initialized(false), isV2Format(false)
{
    initTrigLUT();
};

void Display::init()
{
    canvas.init();
    canvas.setBrightness(255);
    canvas.setRotation(1);
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);

    int w = canvas.width();
    int h = canvas.height();

    size_t bufferSize = w * h * sizeof(uint16_t);
    void *internalBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (internalBuffer)
    {
        sprite.setBuffer(internalBuffer, w, h);
    }
    else
    {
        sprite.createSprite(w, h);
    }

    m_scratchNodeX.reserve(5000); // Pre-allocate scanline scratchpad buffer
    initialized = true;
}

void Display::showNoRoute()
{
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextDatum(CC_DATUM);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("NO ROUTE", canvas.width() / 2, (canvas.height() / 2) - 20);
    canvas.drawString("AVAILABLE!", canvas.width() / 2, (canvas.height() / 2) + 25);

    canvas.setFont(&FreeSans7pt7b);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("SWIPE RIGHT TO LOAD", canvas.width() / 2, 305);
}

bool Display::loadMapData(const char *filePath)
{
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS mount failed");
        return false;
    }

    File file = LittleFS.open(filePath, "r");
    if (!file)
    {
        Serial.printf("Failed to open binary map file: %s\n", filePath);
        return false;
    }

    size_t fileSize = file.size();
    if (fileSize < 24)
    {
        Serial.println("File too small to contain header");
        file.close();
        return false;
    }

    m_mapData.resize(fileSize);
    file.read(m_mapData.data(), fileSize);
    file.close();

    char magic[4];
    memcpy(magic, m_mapData.data(), 4);

    if (memcmp(magic, "BMA2", 4) == 0)
    {
        isV2Format = true;
        Serial.println("Loaded V2 Map format with precomputed bounding boxes.");
    }
    else if (memcmp(magic, "BMAP", 4) == 0)
    {
        isV2Format = false;
        Serial.println("Loaded legacy Map format (using node fallback check).");
    }
    else
    {
        Serial.println("Invalid map magic header");
        return false;
    }

    renderMap(0.0, 0.0);
    return true;
}

void Display::renderMap(double offsetX_mm, double offsetY_mm, double heading_rad)
{
    if (m_mapData.empty())
        return;

    const uint8_t *ptr = m_mapData.data();
    size_t totalSize = m_mapData.size();

    ptr += 4; // Skip magic
    double originLat, originLon;
    memcpy(&originLat, ptr, sizeof(double));
    ptr += sizeof(double);
    memcpy(&originLon, ptr, sizeof(double));
    ptr += sizeof(double);

    uint32_t featureCount;
    memcpy(&featureCount, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    sprite.fillScreen(MapTheme::background());
    int32_t screenW = sprite.width();
    int32_t screenH = sprite.height();
    int32_t centerX = screenW / 2;
    int32_t centerY = screenH / 2;

    float maxExtentMm = 100000.0f;
    float scale = (static_cast<float>(screenW) / 2.0f) / maxExtentMm;
    int32_t scale_fixed = static_cast<int32_t>(scale * 65536.0f);

    char nearestStreet[128] = {0};
    double minCenterDistSq_mm = 20000.0 * 20000.0;

    auto drawWideLine = [&](int x0, int y0, int x1, int y1, uint16_t color, int width)
    {
        if (width <= 1)
        {
            sprite.drawLine(x0, y0, x1, y1, color);
            return;
        }

        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 0.001f)
            return;

        float nx = -dy / len;
        float ny = dx / len;
        float hw = width / 2.0f;

        int xA = round(x0 - nx * hw);
        int yA = round(y0 - ny * hw);
        int xB = round(x0 + nx * hw);
        int yB = round(y0 + ny * hw);
        int xC = round(x1 + nx * hw);
        int yC = round(y1 + ny * hw);
        int xD = round(x1 - nx * hw);
        int yD = round(y1 - ny * hw);

        sprite.fillTriangle(xA, yA, xB, yB, xC, yC, color);
        sprite.fillTriangle(xA, yA, xC, yC, xD, yD, color);
    };

    auto fillPolygon = [&](const std::vector<int16_t> &pxs, const std::vector<int16_t> &pys, uint16_t fillColor)
    {
        if (pxs.size() < 3)
            return;
        int16_t minY = pys[0], maxY = pys[0];
        for (size_t i = 1; i < pys.size(); ++i)
        {
            if (pys[i] < minY)
                minY = pys[i];
            if (pys[i] > maxY)
                maxY = pys[i];
        }
        minY = std::max((int16_t)0, minY);
        maxY = std::min((int16_t)(sprite.height() - 1), maxY);

        for (int y = minY; y <= maxY; ++y)
        {
            m_scratchNodeX.clear();
            size_t j = pxs.size() - 1;
            for (size_t i = 0; i < pxs.size(); ++i)
            {
                if ((pys[i] < y && pys[j] >= y) || (pys[j] < y && pys[i] >= y))
                {
                    if (pys[j] != pys[i])
                    {
                        m_scratchNodeX.push_back(pxs[i] + (y - pys[i]) * (pxs[j] - pxs[i]) / (pys[j] - pys[i]));
                    }
                }
                j = i;
            }

            size_t numNodes = m_scratchNodeX.size();
            for (size_t i = 1; i < numNodes; ++i)
            {
                int16_t key = m_scratchNodeX[i];
                int k = (int)i - 1;
                while (k >= 0 && m_scratchNodeX[k] > key)
                {
                    m_scratchNodeX[k + 1] = m_scratchNodeX[k];
                    k--;
                }
                m_scratchNodeX[k + 1] = key;
            }

            for (size_t i = 0; i + 1 < m_scratchNodeX.size(); i += 2)
            {
                int x0 = std::max((int16_t)0, m_scratchNodeX[i]);
                int x1 = std::min((int16_t)(sprite.width() - 1), m_scratchNodeX[i + 1]);
                if (x0 <= x1)
                {
                    sprite.drawFastHLine(x0, y, x1 - x0 + 1, fillColor);
                }
            }
        }
    };

    std::vector<int16_t> pxList;
    std::vector<int16_t> pyList;
    pxList.reserve(128);
    pyList.reserve(128);

    for (uint32_t r = 0; r < featureCount; ++r)
    {
        uint8_t category, subType;
        uint16_t nodeCount;
        uint8_t nameLength = 0;
        char nameBuf[128] = {0};
        bool visible = false;

        if (isV2Format)
        {
            if ((size_t)(ptr - m_mapData.data() + 21) > totalSize)
                break;

            category = *ptr++;
            subType = *ptr++;
            memcpy(&nodeCount, ptr, 2);
            ptr += 2;

            int32_t minX_mm, maxX_mm, minY_mm, maxY_mm;
            memcpy(&minX_mm, ptr, 4);
            ptr += 4;
            memcpy(&maxX_mm, ptr, 4);
            ptr += 4;
            memcpy(&minY_mm, ptr, 4);
            ptr += 4;
            memcpy(&maxY_mm, ptr, 4);
            ptr += 4;

            nameLength = *ptr++;
            if (nameLength > 0)
            {
                if ((size_t)(ptr - m_mapData.data() + nameLength) > totalSize)
                    break;
                uint8_t len = std::min(nameLength, (uint8_t)127);
                memcpy(nameBuf, ptr, len);
                nameBuf[len] = '\0';
                ptr += nameLength;
            }

            if (nodeCount == 0 || nodeCount > 5000)
                break;
            if ((size_t)(ptr - m_mapData.data() + nodeCount * sizeof(BinNode)) > totalSize)
                break;

            // Instant precomputed bounding box check in millimeters
            int32_t spanX = static_cast<int32_t>(maxExtentMm * 1.5f);
            int32_t spanY = static_cast<int32_t>(maxExtentMm * 1.5f * ((float)screenH / (float)screenW));

            if (maxX_mm >= offsetX_mm - spanX && minX_mm <= offsetX_mm + spanX &&
                maxY_mm >= offsetY_mm - spanY && minY_mm <= offsetY_mm + spanY)
            {
                visible = true;
            }

            const BinNode *nodes = reinterpret_cast<const BinNode *>(ptr);
            ptr += nodeCount * sizeof(BinNode);

            if (!visible && category != 1)
            {
                continue;
            }

            pxList.clear();
            pyList.clear();
            pxList.reserve(nodeCount);

            for (uint16_t n = 0; n < nodeCount; ++n)
            {
                int32_t adjustedX = nodes[n].relX_mm - static_cast<int32_t>(offsetX_mm);
                int32_t adjustedY = nodes[n].relY_mm - static_cast<int32_t>(offsetY_mm);

                if (category == 1 && nameLength > 0)
                {
                    int64_t distSq = (int64_t)adjustedX * adjustedX + (int64_t)adjustedY * adjustedY;
                    if (distSq < minCenterDistSq_mm)
                    {
                        minCenterDistSq_mm = distSq;
                        strncpy(nearestStreet, nameBuf, sizeof(nearestStreet) - 1);
                        nearestStreet[sizeof(nearestStreet) - 1] = '\0';
                    }
                }

                int32_t px = centerX + ((adjustedX * scale_fixed) >> 16);
                int32_t py = centerY - ((adjustedY * scale_fixed) >> 16);

                pxList.push_back(static_cast<int16_t>(px));
                pyList.push_back(static_cast<int16_t>(py));
            }

            if (!visible)
                continue;
        }
        else
        {
            // Legacy Fallback ("BMAP") using node-by-node inspection
            if ((size_t)(ptr - m_mapData.data() + 5) > totalSize)
                break;

            category = *ptr++;
            subType = *ptr++;
            memcpy(&nodeCount, ptr, 2);
            ptr += 2;
            nameLength = *ptr++;

            if (nameLength > 0)
            {
                if ((size_t)(ptr - m_mapData.data() + nameLength) > totalSize)
                    break;
                uint8_t len = std::min(nameLength, (uint8_t)127);
                memcpy(nameBuf, ptr, len);
                nameBuf[len] = '\0';
                ptr += nameLength;
            }

            if (nodeCount == 0 || nodeCount > 5000)
                break;
            if ((size_t)(ptr - m_mapData.data() + nodeCount * sizeof(BinNode)) > totalSize)
                break;

            int32_t minPx = 9999, maxPx = -9999, minPy = 9999, maxPy = -9999;
            const BinNode *nodes = reinterpret_cast<const BinNode *>(ptr);

            for (uint16_t n = 0; n < nodeCount; ++n)
            {
                int32_t adjustedX = nodes[n].relX_mm - static_cast<int32_t>(offsetX_mm);
                int32_t adjustedY = nodes[n].relY_mm - static_cast<int32_t>(offsetY_mm);

                int32_t px = centerX + ((adjustedX * scale_fixed) >> 16);
                int32_t py = centerY - ((adjustedY * scale_fixed) >> 16);

                if (px < minPx)
                    minPx = px;
                if (px > maxPx)
                    maxPx = px;
                if (py < minPy)
                    minPy = py;
                if (py > maxPy)
                    maxPy = py;
            }

            if (maxPx >= -50 && minPx <= screenW + 50 && maxPy >= -50 && minPy <= screenH + 50)
            {
                visible = true;
            }

            if (!visible && category != 1)
            {
                ptr += nodeCount * sizeof(BinNode);
                continue;
            }

            pxList.clear();
            pyList.clear();
            pxList.reserve(nodeCount);

            for (uint16_t n = 0; n < nodeCount; ++n)
            {
                int32_t adjustedX = nodes[n].relX_mm - static_cast<int32_t>(offsetX_mm);
                int32_t adjustedY = nodes[n].relY_mm - static_cast<int32_t>(offsetY_mm);

                if (category == 1 && nameLength > 0)
                {
                    int64_t distSq = (int64_t)adjustedX * adjustedX + (int64_t)adjustedY * adjustedY;
                    if (distSq < minCenterDistSq_mm)
                    {
                        minCenterDistSq_mm = distSq;
                        strncpy(nearestStreet, nameBuf, sizeof(nearestStreet) - 1);
                        nearestStreet[sizeof(nearestStreet) - 1] = '\0';
                    }
                }

                int32_t px = centerX + ((adjustedX * scale_fixed) >> 16);
                int32_t py = centerY - ((adjustedY * scale_fixed) >> 16);

                pxList.push_back(static_cast<int16_t>(px));
                pyList.push_back(static_cast<int16_t>(py));
            }

            ptr += nodeCount * sizeof(BinNode);
            if (!visible)
                continue;
        }

        if (category >= 4 && category <= 6 && pxList.size() > 2)
        {
            PolygonStyle polyStyle = MapTheme::getPolygonStyle(category);
            fillPolygon(pxList, pyList, polyStyle.fillColor);

            for (size_t i = 0; i < pxList.size() - 1; ++i)
            {
                sprite.drawLine(pxList[i], pyList[i], pxList[i + 1], pyList[i + 1], polyStyle.borderColor);
            }
            sprite.drawLine(pxList.back(), pyList.back(), pxList[0], pyList[0], polyStyle.borderColor);
        }
        else if (pxList.size() > 1)
        {
            LineStyle lineStyle = MapTheme::getLineStyle(category, subType);

            for (size_t i = 0; i < pxList.size() - 1; ++i)
            {
                drawWideLine(pxList[i], pyList[i], pxList[i + 1], pyList[i + 1], lineStyle.color, lineStyle.width);
            }

            if (lineStyle.width > 2)
            {
                int radius = lineStyle.width / 2;
                for (size_t i = 0; i < pxList.size(); ++i)
                {
                    if (i > 0 && pxList[i] == pxList[i - 1] && pyList[i] == pyList[i - 1])
                        continue;
                    sprite.fillCircle(pxList[i], pyList[i], radius, lineStyle.color);
                }
            }
        }
    }

    int bannerH = 30;
    int bannerY = sprite.height() - bannerH;
    sprite.fillRect(0, bannerY, screenW, bannerH, MapTheme::bannerFill());
    sprite.drawRect(0, bannerY, screenW, bannerH, MapTheme::bannerBorder());

    sprite.setFont(&FreeSans7pt7b);
    sprite.setTextColor(TFT_WHITE);
    sprite.drawString("Where am I?", 10, bannerY + 8);

    sprite.setFont(&FreeSans7pt7b);
    sprite.setTextColor(MapTheme::bannerTextSec());
    sprite.drawString(nearestStreet, 100, bannerY + 8);

    float adjusted_heading = heading_rad;

    float s = fast_sin(adjusted_heading);
    float c = fast_cos(adjusted_heading);
    float s_left = fast_sin(adjusted_heading - 2.4f);
    float c_left = fast_cos(adjusted_heading - 2.4f);
    float s_right = fast_sin(adjusted_heading + 2.4f);
    float c_right = fast_cos(adjusted_heading + 2.4f);

    float arrowLen = 12.0f;
    float baseLen = 7.0f;

    float tipX = centerX + s * arrowLen;
    float tipY = centerY - c * arrowLen;

    float leftX = centerX + s_left * baseLen;
    float leftY = centerY - c_left * baseLen;

    float rightX = centerX + s_right * baseLen;
    float rightY = centerY - c_right * baseLen;

    sprite.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, MapTheme::reticleColor());

    sprite.pushSprite(0, 0);
}

void Display::gotoSleep()
{
    canvas.setBrightness(0);
    canvas.getPanel()->setSleep(true);
}
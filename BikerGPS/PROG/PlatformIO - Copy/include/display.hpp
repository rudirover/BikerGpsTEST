#pragma once

#include "debug.hpp"
#include <LittleFS.h>
#include <memory>
#include <math.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <LovyanGFX.hpp>
#include "FreeSans7pt7b.h"
#include "mapdatastructs.hpp"
#include "route.hpp"


#define RGB565(r, g, b) ( \
    (((uint16_t)((r) & 0xF8)) << 8) | \
    (((uint16_t)((g) & 0xFC)) << 3) | \
    (((uint16_t)(b)) >> 3) \
)


#define LCD_SCLK_PIN    42      // IO42_TFT_SCK
#define LCD_MOSI_PIN    39      // IO39_TFT_SDA
#define LCD_MISO_PIN    -1      // not connected
#define LCD_DC_PIN      41      // IO41_TFT_RS
#define LCD_CS_PIN      40      // IO40_TFT_CS 
#define LCD_RST_PIN     -1      // not connected
#define LCD_BUSY_PIN    -1      // not connected
#define LCD_BK_PIN      38      // IO38_LED_BK
#define LCD_SDA_PIN     15      // IO15_SDA  
#define LCD_SCL_PIN     16      // IO15_SCL


#define BACKLIGHT_ON_LEVEL          200
#define BACKLIGHT_DIM_LEVEL         30
#define BACKLIGHT_OFF_LEVEL         0
#define BACKLIGHT_FADE_TIME         3000
#define CIRCLE_SIZE                 52
#define CIRCLE_PEN_SIZE             6
#define CIRCLE_CENTER_OFFSET        160
#define CIRCLE_CENTER_X_OFFSET      -20
#define ROUTE_CIRCLE_COLOR          RGB565(0, 220, 0)
#define ROUTE_CIRCLE_SELECT_COLOR   RGB565(0, 255, 0)
#define BLINKER_TIME                500
#define OUTER_DOT_SIZE              8
#define INNER_DOT_SIZE              6
#define WPT_DISTANCE_BAR_WIDTH      15
#define WPT_DISTANCE_BAR_X_OFFSET   5
#define SATELLITE_FIX_BAR_HEIGHT    5
#define SLOW_SCROLL_SPEED           10 // pixels per scroll
#define FAST_SCROLL_SPEED           20

#define DARK_THEME

// --- DARK MAP & UI THEME CONFIGURATION ---
#define THEME_DARK_COLOR_BG RGB565(24, 28, 36)
#define THEME_DARK_BANNER_FILL RGB565(15, 18, 24)
#define THEME_DARK_BANNER_BORDER RGB565(50, 60, 75)
#define THEME_DARK_BANNER_TEXT_SEC RGB565(150, 160, 175)
#define THEME_DARK_RETICLE_COLOR RGB565(240, 60, 60)

#define THEME_DARK_WATER_FILL RGB565(35, 60, 85)
#define THEME_DARK_WATER_BORDER RGB565(50, 90, 125)
#define THEME_DARK_GREEN_FILL RGB565(30, 50, 38)
#define THEME_DARK_GREEN_BORDER RGB565(45, 75, 55)
#define THEME_DARK_BUILDING_FILL RGB565(42, 45, 52)
#define THEME_DARK_BUILDING_BORDER RGB565(65, 70, 80)

#define THEME_DARK_HIGHWAY_PATH_COLOR RGB565(90, 150, 80)
#define THEME_DARK_HIGHWAY_PATH_WIDTH 3
#define THEME_DARK_HIGHWAY_MAJOR_COLOR RGB565(230, 120, 40)
#define THEME_DARK_HIGHWAY_MAJOR_WIDTH 5
#define THEME_DARK_HIGHWAY_RES_COLOR RGB565(180, 180, 180)
#define THEME_DARK_HIGHWAY_RES_WIDTH 3
#define THEME_DARK_HIGHWAY_MOTOR_COLOR RGB565(220, 70, 60)
#define THEME_DARK_HIGHWAY_MOTOR_WIDTH 5
#define THEME_DARK_HIGHWAY_DEF_COLOR RGB565(90, 95, 105)
#define THEME_DARK_HIGHWAY_DEF_WIDTH 2

#define THEME_DARK_RAILWAY_COLOR RGB565(120, 120, 120)
#define THEME_DARK_RAILWAY_WIDTH 1
#define THEME_DARK_WATERWAY_COLOR RGB565(70, 130, 180)
#define THEME_DARK_WATERWAY_WIDTH 2
#define THEME_DARK_FALLBACK_LINE_COLOR RGB565(80, 80, 80)
#define THEME_DARK_FALLBACK_LINE_WIDTH 1

struct LineStyle
{
    uint16_t color;
    int width;
};

struct PolygonStyle
{
    uint16_t fillColor;
    uint16_t borderColor;
};

class MapTheme
{
public:
    static uint16_t background() { return THEME_DARK_COLOR_BG; }

    static PolygonStyle getPolygonStyle(uint8_t category)
    {
        switch (category)
        {
        case 4: return {THEME_DARK_WATER_FILL, THEME_DARK_WATER_BORDER};
        case 5: return {THEME_DARK_GREEN_FILL, THEME_DARK_GREEN_BORDER};
        case 6: 
        default: return {THEME_DARK_BUILDING_FILL, THEME_DARK_BUILDING_BORDER};
        }
    }

    static LineStyle getLineStyle(uint8_t category, uint8_t subType)
    {
        if (category == 1)
        {
            switch (subType)
            {
            case 1: return {THEME_DARK_HIGHWAY_PATH_COLOR, THEME_DARK_HIGHWAY_PATH_WIDTH};
            case 2: return {THEME_DARK_HIGHWAY_MAJOR_COLOR, THEME_DARK_HIGHWAY_MAJOR_WIDTH};
            case 3: return {THEME_DARK_HIGHWAY_RES_COLOR, THEME_DARK_HIGHWAY_RES_WIDTH};
            case 4: return {THEME_DARK_HIGHWAY_MOTOR_COLOR, THEME_DARK_HIGHWAY_MOTOR_WIDTH};
            default: return {THEME_DARK_HIGHWAY_DEF_COLOR, THEME_DARK_HIGHWAY_DEF_WIDTH};
            }
        }
        else if (category == 2) return {THEME_DARK_RAILWAY_COLOR, THEME_DARK_RAILWAY_WIDTH};
        else if (category == 3) return {THEME_DARK_WATERWAY_COLOR, THEME_DARK_WATERWAY_WIDTH};
        return {THEME_DARK_FALLBACK_LINE_COLOR, THEME_DARK_FALLBACK_LINE_WIDTH};
    }

    static uint16_t bannerFill() { return THEME_DARK_BANNER_FILL; }
    static uint16_t reticleColor() { return THEME_DARK_RETICLE_COLOR; }
    static uint16_t bannerBorder() { return THEME_DARK_BANNER_BORDER; }
    static uint16_t bannerTextSec() { return THEME_DARK_BANNER_TEXT_SEC; }       
};

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Bus_SPI _bus;
    lgfx::Panel_ST7789 _panel;
    lgfx::Light_PWM _backlight;
    lgfx::Touch_FT5x06 _touch;

public:
    LGFX()
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = LCD_SCLK_PIN;
            cfg.pin_mosi = LCD_MOSI_PIN;
            cfg.pin_miso = LCD_MISO_PIN;
            cfg.pin_dc = LCD_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = LCD_CS_PIN;
            cfg.pin_rst = LCD_RST_PIN;
            cfg.pin_busy = LCD_BUSY_PIN;
            cfg.memory_width = 240;
            cfg.memory_height = 320;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_rotation = 3;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.bus_shared = true;
            _panel.config(cfg);
        }
        {
            auto cfg = _backlight.config();
            cfg.freq = 5000;
            cfg.pin_bl = LCD_BK_PIN;
            cfg.pwm_channel = 1;
            _backlight.config(cfg);
            _panel.setLight(&_backlight);
        }
        {
            auto cfg = _touch.config();
            cfg.x_min = 0; cfg.x_max = 239;
            cfg.y_min = 0; cfg.y_max = 319;
            cfg.pin_int = 47;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port = 0;
            cfg.i2c_addr = 0x38;
            cfg.pin_sda = LCD_SDA_PIN;
            cfg.pin_scl = LCD_SCL_PIN;
            cfg.freq = 400000;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }
};

class Display
{
public:
    Display();
    void init();
    void run();
    void showNoRoute();
    void showReceiveRoute();
    void showRoute();
    //void showMap();
    void fadeBackLight(uint8_t backLightLevel);
    void setBackLight(uint8_t backLightLevel);
    void scrollUp(int speed = SLOW_SCROLL_SPEED);
    void scrollDown(int speed = SLOW_SCROLL_SPEED);
    void scrollIntoView();
    bool inView();
    void sleep();
    bool initDone();
    LGFX& getCanvas() { return canvas; }    

private:
    LGFX canvas;
    LGFX_Sprite sprite;
    uint8_t oldBackLightLevel;
    uint8_t newBackLigthLevel;
    void drawRoute();
    void drawWpts(int offset=0);
    void onFadeBackLight();
    void drawBluetoothIcon(int centerX, int centerY, int size, uint16_t color);
    void drawRouteCircle(int centerX, int centerY, uint16_t color);
    void drawDistanceLabel(std::string distance, int centerX, int centerY);
    void drawDirectionDot();
    void drawWptDistanceBar();
    void drawSatelliteFixBar();
    unsigned long blinkerTime;
    bool blinker = false;
    bool showingRoute = false;
    unsigned long fadeTimer;
    unsigned long fadeTime;
    bool initialized = false;
    bool loadMap(const char *filePath);
    void drawMap(double offsetX_mm, double offsetY_mm, double heading_rad = 0.0);
    std::vector<uint8_t> m_mapData;
    std::vector<int16_t> m_scratchNodeX; // Pre-allocated buffer to eliminate polygon rasterization heap thrashing
    bool isV2Format = false;    
};

extern Display display;
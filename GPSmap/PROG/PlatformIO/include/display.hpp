#pragma once

#include <LittleFS.h>
#include <memory>
#include <math.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <LovyanGFX.hpp>
#include "FreeSans7pt7b.h"
#include "mapdatastructs.hpp"

// RGB565 Color Conversion Macro
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b >> 3)))

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
            cfg.pin_sclk = 42;
            cfg.pin_mosi = 39;
            cfg.pin_miso = -1;
            cfg.pin_dc = 41;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs = 40;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
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
            cfg.pin_bl = 38;
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
            cfg.pin_sda = 15;
            cfg.pin_scl = 16;
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
    void showNoRoute();
    bool loadMapData(const char *filePath);
    void renderMap(double offsetX_mm, double offsetY_mm, double heading_rad = 0.0);
    void gotoSleep();

    LGFX& getCanvas() { return canvas; }


private:
        LGFX canvas;
    LGFX_Sprite sprite;
    bool initialized = false;
    std::vector<uint8_t> m_mapData;
    std::vector<int16_t> m_scratchNodeX; // Pre-allocated buffer to eliminate polygon rasterization heap thrashing
    bool isV2Format = false;
};

extern Display display;
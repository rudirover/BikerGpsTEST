#pragma once

#include "debug.hpp"
#include <LovyanGFX.hpp>
#include <math.h>
#include "route.hpp"
#include "gps.hpp"
#include "sdcard.hpp"

#define RGB565(r, g, b) ( \
    (((uint16_t)((r) & 0xF8)) << 8) | \
    (((uint16_t)((g) & 0xFC)) << 3) | \
    (((uint16_t)(b)) >> 3) \
)
/*
            cfg.pin_sclk = 42;
            cfg.pin_mosi = 39;
            cfg.pin_miso = -1;
            cfg.pin_dc = 41;
            cfg.pin_cs = 40;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;            
*/

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
    //void showPowerOff();
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

};

extern Display display;
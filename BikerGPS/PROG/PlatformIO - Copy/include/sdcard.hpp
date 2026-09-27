#pragma once

#include <SPI.h>
#include <FS.h>
#include <SD.h>

#define SD_MOSI_PIN 6
#define SD_MISO_PIN 4
#define SD_SCK_PIN 5
#define SD_CS_PIN 7

class SDCard
{
public:
    SDCard();
    void init();

private:
    SPIClass SDSpi = SPIClass(HSPI);
    void listDir(fs::FS & fs, const char *dirname, uint8_t levels);
};

extern SDCard sdcard;
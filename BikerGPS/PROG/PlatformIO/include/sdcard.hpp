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
    void write(const std::string &fileName, const std::string &payLoad);    
private:
    SPIClass SDSpi = SPIClass(HSPI);
    void read(String path, String data);


    void listDir(fs::FS & fs, const char *dirname, uint8_t levels);
};

extern SDCard sdcard;
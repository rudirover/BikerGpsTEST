#include "sdcard.hpp"

SDCard sdcard;

SDCard::SDCard() {};

void SDCard::init()
{
    SDSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
    if (!SD.begin(SD_CS_PIN, SDSpi, 80000000))
    {
        Serial.println(F("ERROR: File system mount failed!"));
        SDSpi.end();
    }
    else
    {
        Serial.println("Card Mount Successed");
        Serial.printf("SD Size： %lluMB \n", SD.cardSize() / (1024 * 1024));
        listDir(SD, "/", 2);
    }


    Serial.println("**** TF Card init finished ****.");
    return;
}

void SDCard::read(String path, String data){

}

void SDCard::write(const std::string &fileName, const std::string &payLoad){
    String path = fileName.c_str();
    path = "/" + path;
    File file = SD.open(path.c_str(), FILE_WRITE);

    size_t bytesWritten = file.write(reinterpret_cast<const uint8_t*>(payLoad.data()), payLoad.size());
    Serial.print("Bytes Written: ");
    Serial.println(bytesWritten);
}

void SDCard::listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
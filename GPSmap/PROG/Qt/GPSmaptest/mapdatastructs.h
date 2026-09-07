#pragma once
#include <cstdint>

#pragma pack(1)

struct BinHeader {
    char magic[4];          // "BMA2" for precomputed bounding boxes, "BMAP" for legacy fallback
    double originLat;       // Center/Origin point latitude
    double originLon;       // Center/Origin point longitude
    uint32_t featureCount;  // Total number of features in file
};

struct BinFeatureLegacy {
    uint8_t category;       // 1 = Highway, 2 = Railway, 3 = Waterway, 4 = Water Body, 5 = Green Zone, 6 = Building
    uint8_t subType;        // Highway subtype: 1=Cycle/Path, 2=Major, 3=Residential, 4=Motorway/Trunk, 5=Service
    uint16_t nodeCount;     // Number of coordinate points in this feature
    uint8_t nameLength;     // Length of the following UTF-8 name string (0 if unnamed)
};

struct BinFeatureV2 {
    uint8_t category;
    uint8_t subType;
    uint16_t nodeCount;
    int32_t minX_mm;        // Precomputed minimum X offset from origin in millimeters
    int32_t maxX_mm;        // Precomputed maximum X offset from origin in millimeters
    int32_t minY_mm;        // Precomputed minimum Y offset from origin in millimeters
    int32_t maxY_mm;        // Precomputed maximum Y offset from origin in millimeters
    uint8_t nameLength;
};

struct BinNode {
    int32_t relX_mm;        // X offset from origin in millimeters
    int32_t relY_mm;        // Y offset from origin in millimeters
};

#pragma pack()
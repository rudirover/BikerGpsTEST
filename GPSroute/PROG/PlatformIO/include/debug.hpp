#pragma once

#include <Arduino_DebugUtils.h>

inline const char* get_debug_timestamp() {
  static char buf[16];
  unsigned long ms = millis();
  
  unsigned long secs = ms / 1000;
  unsigned long mins = secs / 60;
  unsigned long hrs  = mins / 60;
  
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu.%03lu", 
           hrs % 24, mins % 60, secs % 60, ms % 1000);
           
  return buf;
}

#define DBG_EXT(level, fmt, ...) \
  Debug.print(level, "[%s] [%s:%d (%s)] " fmt, get_debug_timestamp(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define IF_DBG(level) if (Debug.getDebugLevel() >= (level))
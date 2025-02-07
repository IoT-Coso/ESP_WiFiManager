/****************************************************************************************************************************
  ESP_WiFiManager.h
  For ESP8266 / ESP32 boards

  ESP_WiFiManager is a library for the ESP8266/Arduino platform
  (https://github.com/esp8266/Arduino) to enable easy
  configuration and reconfiguration of WiFi credentials using a Captive Portal
  inspired by:
  http://www.esp8266.com/viewtopic.php?f=29&t=2520
  https://github.com/chriscook8/esp-arduino-apboot
  https://github.com/esp8266/Arduino/blob/master/libraries/DNSServer/examples/CaptivePortalAdvanced/

  Modified from Tzapu https://github.com/tzapu/WiFiManager
  and from Ken Taylor https://github.com/kentaylor

  Built by Khoi Hoang https://github.com/khoih-prog/ESP_WiFiManager
  Licensed under MIT license
  
  Version: 1.12.0

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      07/10/2019 Initial coding
  ...
  1.8.0   K Hoang      29/12/2021 Fix `multiple-definitions` linker error and weird bug related to src_cpp
  1.9.0   K Hoang      17/01/2022 Enable compatibility with old code to include only ESP_WiFiManager.h
  1.10.0  K Hoang      10/02/2022 Add support to new ESP32-S3
  1.10.1  K Hoang      11/02/2022 Add LittleFS support to ESP32-C3. Use core LittleFS instead of Lorol's LITTLEFS for v2.0.0+
  1.10.2  K Hoang      13/03/2022 Send CORS header in handleWifiSave() function
  1.11.0  K Hoang      09/09/2022 Fix ESP32 chipID and add ESP_getChipOUI()
  1.12.0  K Hoang      07/10/2022 Optional display Credentials (SSIDs, PWDs) in Config Portal
 *****************************************************************************************************************************/

#pragma once

#ifndef ESP_WiFiManager_h
#define ESP_WiFiManager_h

#include "ESP_WiFiManager.hpp"              //https://github.com/khoih-prog/ESP_WiFiManager
#include "ESP_WiFiManager-Impl.h"          //https://github.com/khoih-prog/ESP_WiFiManager

#endif    // ESP_WiFiManager_h


/****************************************************************************************************************************
  ESP_WiFiManager.hpp
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

#ifndef ESP_WiFiManager_hpp
#define ESP_WiFiManager_hpp

////////////////////////////////////////////////////

#if !( defined(ESP8266) ||  defined(ESP32) )
  #error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#elif ( ARDUINO_ESP32S2_DEV || ARDUINO_FEATHERS2 || ARDUINO_ESP32S2_THING_PLUS || ARDUINO_MICROS2 || \
        ARDUINO_METRO_ESP32S2 || ARDUINO_MAGTAG29_ESP32S2 || ARDUINO_FUNHOUSE_ESP32S2 || \
        ARDUINO_ADAFRUIT_FEATHER_ESP32S2_NOPSRAM )
  #if (_WIFIMGR_LOGLEVEL_ > 3)
		#warning Using ESP32_S2. To follow library instructions to install esp32-s2 core and WebServer Patch
		#warning You have to select HUGE APP or 1.9-2.0 MB APP to be able to run Config Portal. Must use PSRAM
  #endif
  #define USING_ESP32_S2        true
#elif ( ARDUINO_ESP32C3_DEV )
	#if (_WIFIMGR_LOGLEVEL_ > 3)
		#if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
		  #warning Using ESP32_C3 using core v2.0.0+. Either LittleFS, SPIFFS or EEPROM OK.
		#else
		  #warning Using ESP32_C3 using core v1.0.6-. To follow library instructions to install esp32-c3 core. Only SPIFFS and EEPROM OK.
		#endif
		
		#warning You have to select Flash size 2MB and Minimal APP (1.3MB + 700KB) for some boards
  #endif
    
  #define USING_ESP32_C3        true
#elif ( defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32_S3_BOX) || defined(ARDUINO_TINYS3) || \
        defined(ARDUINO_PROS3) || defined(ARDUINO_FEATHERS3) )
  #if (_WIFIMGR_LOGLEVEL_ > 3)
  	#warning Using ESP32_S3. To install esp32-s3-support branch if using core v2.0.2-
  #endif
  #define USING_ESP32_S3        true   
#endif

////////////////////////////////////////////////////

#define ESP_WIFIMANAGER_VERSION           "ESP_WiFiManager v1.12.0"

#define ESP_WIFIMANAGER_VERSION_MAJOR     1
#define ESP_WIFIMANAGER_VERSION_MINOR     12
#define ESP_WIFIMANAGER_VERSION_PATCH     0

#define ESP_WIFIMANAGER_VERSION_INT      1012000

////////////////////////////////////////////////////

#include "ESP_WiFiManager_Debug.h"

////////////////////////////////////////////////////

#if ( defined(HTTP_PORT) && (HTTP_PORT < 65536) && (HTTP_PORT > 0) )
  #if (_WIFIMGR_LOGLEVEL_ > 3)
    #warning Using custom HTTP_PORT
  #endif
    
  #define HTTP_PORT_TO_USE     HTTP_PORT
#else
  #if (_WIFIMGR_LOGLEVEL_ > 3)
    #warning Using default HTTP_PORT = 80
  #endif
  
  #define HTTP_PORT_TO_USE     80
#endif

////////////////////////////////////////////////////

//KH, for ESP32
#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#else		//ESP32
  #include <WiFi.h>
  #include <WebServer.h>
#endif

#include <DNSServer.h>
#include <memory>
#undef min
#undef max
#include <algorithm>

////////////////////////////////////////////////////

//KH, for ESP32
#ifdef ESP8266
  extern "C"
  {
    #include "user_interface.h"
  }
  
  #define ESP_getChipId()   (ESP.getChipId())

  
#else		//ESP32

  #include <esp_wifi.h>
  
  uint32_t getChipID();
  uint32_t getChipOUI();

   
  #if defined(ESP_getChipId)
    #undef ESP_getChipId
  #endif
  
  #define ESP_getChipId()  	getChipID()			// ((uint32_t)ESP.getEfuseMac())
  #define ESP_getChipOUI() 	getChipOUI()		// ((uint32_t)ESP.getEfuseMac())

#endif


////////////////////////////////////////////////////

typedef struct
{
  IPAddress _ap_static_ip;
  IPAddress _ap_static_gw;
  IPAddress _ap_static_sn;

}  WiFi_AP_IPConfig;

////////////////////////////////////////////////////

// Thanks to @Amorphous for the feature and code
// (https://community.blynk.cc/t/esp-wifimanager-for-esp32-and-esp8266/42257/13)
// To enable to configure from sketch
#if !defined(USE_CONFIGURABLE_DNS)
  #define USE_CONFIGURABLE_DNS        false
#endif

////////////////////////////////////////////////////

typedef struct
{
  IPAddress _sta_static_ip;
  IPAddress _sta_static_gw;
  IPAddress _sta_static_sn;
  IPAddress _sta_static_dns1;
  IPAddress _sta_static_dns2;
}  WiFi_STA_IPConfig;

////////////////////////////////////////////////////

#define WFM_LABEL_BEFORE			1
#define WFM_LABEL_AFTER				2
#define WFM_NO_LABEL          0

////////////////////////////////////////////////////



/** Handle CORS in pages */
// Default false for using only whenever necessary to avoid security issue when using CORS (Cross-Origin Resource Sharing)
#ifndef USING_CORS_FEATURE
  // Contributed by AlesSt (https://github.com/AlesSt) to solve AJAX CORS protection problem of API redirects on client side
  // See more in https://github.com/khoih-prog/ESP_WiFiManager/issues/27 and https://en.wikipedia.org/wiki/Cross-origin_resource_sharing
  #define USING_CORS_FEATURE     false
#endif

////////////////////////////////////////////////////



//KH
//Mofidy HTTP_HEAD to WM_HTTP_HEAD_START to avoid conflict in Arduino esp8266 core 2.6.0+
const char WM_HTTP_200[] PROGMEM = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const char WM_HTTP_HEAD_START[] PROGMEM = "<!DOCTYPE html><html lang='en'><head><meta name='viewport' content='width=device-width, initial-scale=1, user-scalable=no'/><title>{v}</title>";

////////////////////////////////////////////////////

const char WM_HTTP_STYLE[] PROGMEM = "<style>div{padding:2px;font-size:1em;}body,textarea,input,select{background: 0;border-radius: 0;font: 16px sans-serif;margin: 0}textarea,input,select{outline: 0;font-size: 14px;border: 1px solid #ccc;padding: 8px;width: 90%}.btn a{text-decoration: none}.container{margin: auto;width: 90%}@media(min-width:1200px){.container{margin: auto;width: 30%}}@media(min-width:768px) and (max-width:1200px){.container{margin: auto;width: 50%}}.btn,h2{font-size: 1.3em}h1{font-size: 2em}.btn{background: #E71D73;border-radius: 4px;border: 0px solid #A31A5C;color: #fff;cursor: pointer;display: inline-block;margin: 2px 0;padding: 10px 14px 11px;width: 100%}.btn:hover{background: #F9B334}.btn:active,.btn:focus{background: #8B205A}label>*{display: inline}form>*{display: block;margin-bottom: 10px}textarea:focus,input:focus,select:focus{border-color: #5ab}.msg{background: #FBF0FA;border-left: 5px solid #8B205A;padding: 1.1em}.q{float: right;width: 64px;text-align: right}.l{background: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==') no-repeat left center;background-size: 1em}input[type='checkbox']{float: left;width: 20px}.table td{padding:.5em;text-align:left}.table tbody>:nth-child(2n-1){background:#ddd}fieldset{border-radius:0.5rem;margin:0px;}</style>";

////////////////////////////////////////////////////

const char WM_HTTP_SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();document.getElementById('s1').value=l.innerText||l.textContent;document.getElementById('p1').focus();document.getElementById('timezone').value=timezone.name();}</script>";

////////////////////////////////////////////////////
////////////////////////////////////////////////////

// To permit disable or configure NTP from sketch
#ifndef USE_ESP_WIFIMANAGER_NTP
  // To enable NTP config
  #define USE_ESP_WIFIMANAGER_NTP     true
#endif

////////////////////////////////////////////////////

#if USE_ESP_WIFIMANAGER_NTP

#include "utils/TZ.h"

const char WM_HTTP_SCRIPT_NTP_MSG[] PROGMEM = "<p style='display: none;'>Your Timezone is : <b><label id='timezone' name='timezone'></b><script>document.getElementById('timezone').innerHTML=timezone.name();document.getElementById('timezone').value=timezone.name();</script></p>";
const char WM_HTTP_SCRIPT_NTP_HIDDEN[] PROGMEM = "<p><input type='hidden' id='timezone' name='timezone'><script>document.getElementById('timezone').innerHTML=timezone.name();document.getElementById('timezone').value=timezone.name();</script></p>";

#if ESP8266
  #if !(USE_CLOUDFLARE_NTP)
    #undef USE_CLOUDFLARE_NTP
    #define USE_CLOUDFLARE_NTP      true
    
    #if (_WIFIMGR_LOGLEVEL_ > 3)
    	#warning Forcing USE_CLOUDFLARE_NTP for ESP8266 as low memory can cause blank page
    #endif
  #endif
#endif

// To permit disable or configure NTP from sketch
#ifndef USE_CLOUDFLARE_NTP
  #define USE_CLOUDFLARE_NTP          false
#endif

#if USE_CLOUDFLARE_NTP
const char WM_HTTP_SCRIPT_NTP[] PROGMEM = "<script src='https://cdnjs.cloudflare.com/ajax/libs/jstimezonedetect/1.0.7/jstz.min.js'></script><script>var timezone=jstz.determine();console.log('Your CloudFlare timezone is:' + timezone.name());document.getElementById('timezone').innerHTML = timezone.name();</script>";
#else
const char WM_HTTP_SCRIPT_NTP[] PROGMEM = "<script>(function(e){var t=function(){'use strict';var e='s',n=function(e){var t=-e.getTimezoneOffset();return t!==null?t:0},r=function(e,t,n){var r=new Date;return e!==undefined&&r.setFullYear(e),r.setDate(n),r.setMonth(t),r},i=function(e){return n(r(e,0,2))},s=function(e){return n(r(e,5,2))},o=function(e){var t=e.getMonth()>7?s(e.getFullYear()):i(e.getFullYear()),r=n(e);return t-r!==0},u=function(){var t=i(),n=s(),r=i()-s();return r<0?t+',1':r>0?n+',1,'+e:t+',0'},a=function(){var e=u();return new t.TimeZone(t.olson.timezones[e])},f=function(e){var t=new Date(2010,6,15,1,0,0,0),n={'America/Denver':new Date(2011,2,13,3,0,0,0),'America/Mazatlan':new Date(2011,3,3,3,0,0,0),'America/Chicago':new Date(2011,2,13,3,0,0,0),'America/Mexico_City':new Date(2011,3,3,3,0,0,0),'America/Asuncion':new Date(2012,9,7,3,0,0,0),'America/Santiago':new Date(2012,9,3,3,0,0,0),'America/Campo_Grande':new Date(2012,9,21,5,0,0,0),'America/Montevideo':new Date(2011,9,2,3,0,0,0),'America/Sao_Paulo':new Date(2011,9,16,5,0,0,0),'America/Los_Angeles':new Date(2011,2,13,8,0,0,0),'America/Santa_Isabel':new Date(2011,3,5,8,0,0,0),'America/Havana':new Date(2012,2,10,2,0,0,0),'America/New_York':new Date(2012,2,10,7,0,0,0),'Asia/Beirut':new Date(2011,2,27,1,0,0,0),'Europe/Helsinki':new Date(2011,2,27,4,0,0,0),'Europe/Istanbul':new Date(2011,2,28,5,0,0,0),'Asia/Damascus':new Date(2011,3,1,2,0,0,0),'Asia/Jerusalem':new Date(2011,3,1,6,0,0,0),'Asia/Gaza':new Date(2009,2,28,0,30,0,0),'Africa/Cairo':new Date(2009,3,25,0,30,0,0),'Pacific/Auckland':new Date(2011,8,26,7,0,0,0),'Pacific/Fiji':new Date(2010,11,29,23,0,0,0),'America/Halifax':new Date(2011,2,13,6,0,0,0),'America/Goose_Bay':new Date(2011,2,13,2,1,0,0),'America/Miquelon':new Date(2011,2,13,5,0,0,0),'America/Godthab':new Date(2011,2,27,1,0,0,0),'Europe/Moscow':t,'Asia/Yekaterinburg':t,'Asia/Omsk':t,'Asia/Krasnoyarsk':t,'Asia/Irkutsk':t,'Asia/Yakutsk':t,'Asia/Vladivostok':t,'Asia/Kamchatka':t,'Europe/Minsk':t,'Australia/Perth':new Date(2008,10,1,1,0,0,0)};return n[e]};return{determine:a,date_is_dst:o,dst_start_for:f}}();t.TimeZone=function(e){'use strict';var n={'America/Denver':['America/Denver','America/Mazatlan'],'America/Chicago':['America/Chicago','America/Mexico_City'],'America/Santiago':['America/Santiago','America/Asuncion','America/Campo_Grande'],'America/Montevideo':['America/Montevideo','America/Sao_Paulo'],'Asia/Beirut':['Asia/Beirut','Europe/Helsinki','Europe/Istanbul','Asia/Damascus','Asia/Jerusalem','Asia/Gaza'],'Pacific/Auckland':['Pacific/Auckland','Pacific/Fiji'],'America/Los_Angeles':['America/Los_Angeles','America/Santa_Isabel'],'America/New_York':['America/Havana','America/New_York'],'America/Halifax':['America/Goose_Bay','America/Halifax'],'America/Godthab':['America/Miquelon','America/Godthab'],'Asia/Dubai':['Europe/Moscow'],'Asia/Dhaka':['Asia/Yekaterinburg'],'Asia/Jakarta':['Asia/Omsk'],'Asia/Shanghai':['Asia/Krasnoyarsk','Australia/Perth'],'Asia/Tokyo':['Asia/Irkutsk'],'Australia/Brisbane':['Asia/Yakutsk'],'Pacific/Noumea':['Asia/Vladivostok'],'Pacific/Tarawa':['Asia/Kamchatka'],'Africa/Johannesburg':['Asia/Gaza','Africa/Cairo'],'Asia/Baghdad':['Europe/Minsk']},r=e,i=function(){var e=n[r],i=e.length,s=0,o=e[0];for(;s<i;s+=1){o=e[s];if(t.date_is_dst(t.dst_start_for(o))){r=o;return}}},s=function(){return typeof n[r]!='undefined'};return s()&&i(),{name:function(){return r}}},t.olson={},t.olson.timezones={'-720,0':'Etc/GMT+12','-660,0':'Pacific/Pago_Pago','-600,1':'America/Adak','-600,0':'Pacific/Honolulu','-570,0':'Pacific/Marquesas','-540,0':'Pacific/Gambier','-540,1':'America/Anchorage','-480,1':'America/Los_Angeles','-480,0':'Pacific/Pitcairn','-420,0':'America/Phoenix','-420,1':'America/Denver','-360,0':'America/Guatemala','-360,1':'America/Chicago','-360,1,s':'Pacific/Easter','-300,0':'America/Bogota','-300,1':'America/New_York','-270,0':'America/Caracas','-240,1':'America/Halifax','-240,0':'America/Santo_Domingo','-240,1,s':'America/Santiago','-210,1':'America/St_Johns','-180,1':'America/Godthab','-180,0':'America/Argentina/Buenos_Aires','-180,1,s':'America/Montevideo','-120,0':'Etc/GMT+2','-120,1':'Etc/GMT+2','-60,1':'Atlantic/Azores','-60,0':'Atlantic/Cape_Verde','0,0':'Etc/UTC','0,1':'Europe/London','60,1':'Europe/Berlin','60,0':'Africa/Lagos','60,1,s':'Africa/Windhoek','120,1':'Asia/Beirut','120,0':'Africa/Johannesburg','180,0':'Asia/Baghdad','180,1':'Europe/Moscow','210,1':'Asia/Tehran','240,0':'Asia/Dubai','240,1':'Asia/Baku','270,0':'Asia/Kabul','300,1':'Asia/Yekaterinburg','300,0':'Asia/Karachi','330,0':'Asia/Kolkata','345,0':'Asia/Kathmandu','360,0':'Asia/Dhaka','360,1':'Asia/Omsk','390,0':'Asia/Rangoon','420,1':'Asia/Krasnoyarsk','420,0':'Asia/Jakarta','480,0':'Asia/Shanghai','480,1':'Asia/Irkutsk','525,0':'Australia/Eucla','525,1,s':'Australia/Eucla','540,1':'Asia/Yakutsk','540,0':'Asia/Tokyo','570,0':'Australia/Darwin','570,1,s':'Australia/Adelaide','600,0':'Australia/Brisbane','600,1':'Asia/Vladivostok','600,1,s':'Australia/Sydney','630,1,s':'Australia/Lord_Howe','660,1':'Asia/Kamchatka','660,0':'Pacific/Noumea','690,0':'Pacific/Norfolk','720,1,s':'Pacific/Auckland','720,0':'Pacific/Tarawa','765,1,s':'Pacific/Chatham','780,0':'Pacific/Tongatapu','780,1,s':'Pacific/Apia','840,0':'Pacific/Kiritimati'},typeof exports!='undefined'?exports.jstz=t:e.jstz=t})(this);</script><script>var timezone=jstz.determine();console.log('Your Timezone is:' + timezone.name());document.getElementById('timezone').innerHTML = timezone.name();</script>";
#endif

#else
  const char WM_HTTP_SCRIPT_NTP_MSG[]     PROGMEM   = "";
  const char WM_HTTP_SCRIPT_NTP_HIDDEN[]  PROGMEM   = "";
  const char WM_HTTP_SCRIPT_NTP[]         PROGMEM   = "";
#endif

////////////////////////////////////////////////////
////////////////////////////////////////////////////

const char WM_HTTP_HEAD_END[] PROGMEM = "</head><body><div class='container'><div style='text-align:left;display:inline-block;min-width:260px;'>";

const char WM_FLDSET_START[]  PROGMEM = "<fieldset>";
const char WM_FLDSET_END[]    PROGMEM = "</fieldset>";

////////////////////////////////////////////////////

const char WM_HTTP_PORTAL_OPTIONS[] PROGMEM = "<form action='/wifi' method='get'><button class='btn'>WiFi Configuration</button></form><br/><form action='/i' method='get'><button class='btn'>Information</button></form><br/><form action='/close' method='get'><button class='btn'>Exit</button></form><br/>";
const char WM_HTTP_ITEM[] PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char JSON_ITEM[] PROGMEM    = "{\"SSID\":\"{v}\", \"Encryption\":{i}, \"Quality\":\"{r}\"}";

////////////////////////////////////////////////////

// KH, update from v1.12.0
// To permit display stored Credentials on CP
#ifndef DISPLAY_STORED_CREDENTIALS_IN_CP   
  #define DISPLAY_STORED_CREDENTIALS_IN_CP          true
#endif

#if ( (_WIFIMGR_LOGLEVEL_ > 3) && DISPLAY_STORED_CREDENTIALS_IN_CP )
  #warning Enable DISPLAY_STORED_CREDENTIALS_IN_CP
#endif

#if DISPLAY_STORED_CREDENTIALS_IN_CP
const char WM_HTTP_FORM_START[] PROGMEM = "<form method='get' action='wifisave'><fieldset><div><label>SSID</label><input value='[[ssid]]' id='s' name='s' length=32 placeholder='SSID'><div></div></div><div><label>Password</label><input value='[[pwd]]' id='p' name='p' length=64 placeholder='password'><div></div></div><div><label>SSID1</label><input value='[[ssid1]]' id='s1' name='s1' length=32 placeholder='SSID1'><div></div></div><div><label>Password</label><input value='[[pwd1]]' id='p1' name='p1' length=64 placeholder='password1'><div></div></div></fieldset>";
#else
const char WM_HTTP_FORM_START[] PROGMEM = "<form method='get' action='wifisave'><fieldset><div><label>SSID</label><input id='s' name='s' length=32 placeholder='SSID'><div></div></div><div><label>Password</label><input id='p' name='p' length=64 placeholder='password'><div></div></div><div><label>SSID1</label><input id='s1' name='s1' length=32 placeholder='SSID1'><div></div></div><div><label>Password</label><input id='p1' name='p1' length=64 placeholder='password1'><div></div></div></fieldset>";
#endif

////////////////////////////////////////////////////

const char WM_HTTP_FORM_LABEL_BEFORE[] PROGMEM = "<div><label for='{i}'>{p}</label><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}><div></div></div>";
const char WM_HTTP_FORM_LABEL_AFTER[] PROGMEM = "<div><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}><label for='{i}'>{p}</label><div></div></div>";

////////////////////////////////////////////////////

const char WM_HTTP_FORM_LABEL[] PROGMEM = "<label for='{i}'>{p}</label>";
const char WM_HTTP_FORM_PARAM[] PROGMEM = "<input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}>";

////////////////////////////////////////////////////

const char WM_HTTP_FORM_END[] PROGMEM = "<button class='btn' type='submit'>Save</button></form>";

////////////////////////////////////////////////////

const char WM_HTTP_SAVED[] PROGMEM = "<div class='msg'><b>Credentials Saved</b><br>Try connecting ESP to the {x}/{x1} network. Wait around 10 seconds then check <a href='/'>if it's OK.</a> <p/>The {v} AP will run on the same WiFi channel of the {x}/{x1} AP. You may have to manually reconnect to the {v} AP.</div>";

////////////////////////////////////////////////////

const char WM_HTTP_END[] PROGMEM = "</div></body></html>";

////////////////////////////////////////////////////

const char WM_HTTP_HEAD_CL[]         PROGMEM = "Content-Length";
const char WM_HTTP_HEAD_CT[]         PROGMEM = "text/html";
const char WM_HTTP_HEAD_CT2[]        PROGMEM = "text/plain";

//KH Add repeatedly used const
const char WM_HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char WM_HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char WM_HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char WM_HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char WM_HTTP_EXPIRES[]         PROGMEM = "Expires";
const char WM_HTTP_CORS[]            PROGMEM = "Access-Control-Allow-Origin";
const char WM_HTTP_CORS_ALLOW_ALL[]  PROGMEM = "*";

////////////////////////////////////////////////////

#if USE_AVAILABLE_PAGES
const char WM_HTTP_AVAILABLE_PAGES[] PROGMEM = "<h3>Available Pages</h3><table class='table'><thead><tr><th>Page</th><th>Function</th></tr></thead><tbody><tr><td><a href='/'>/</a></td><td>Menu page.</td></tr><tr><td><a href='/wifi'>/wifi</a></td><td>Show WiFi scan results and enter WiFi configuration.</td></tr><tr><td><a href='/wifisave'>/wifisave</a></td><td>Save WiFi configuration information and configure device. Needs variables supplied.</td></tr><tr><td><a href='/close'>/close</a></td><td>Close the configuration server and configuration WiFi network.</td></tr><tr><td><a href='/i'>/i</a></td><td>This page.</td></tr><tr><td><a href='/r'>/r</a></td><td>Delete WiFi configuration and reboot. ESP device will not reconnect to a network until new WiFi configuration data is entered.</td></tr><tr><td><a href='/state'>/state</a></td><td>Current device state in JSON format. Interface for programmatic WiFi configuration.</td></tr><tr><td><a href='/scan'>/scan</a></td><td>Run a WiFi scan and return results in JSON format. Interface for programmatic WiFi configuration.</td></tr></table>";
#else
const char WM_HTTP_AVAILABLE_PAGES[] PROGMEM = "";
#endif

//KH
#define WIFI_MANAGER_MAX_PARAMS 20

////////////////////////////////////////////////////

typedef struct
{
  const char *_id;
  const char *_placeholder;
  char       *_value;
  int         _length;
  int         _labelPlacement;

}  WMParam_Data;

////////////////////////////////////////////////////
////////////////////////////////////////////////////

class ESP_WMParameter 
{
  public:
    ESP_WMParameter(const char *custom);
    ESP_WMParameter(const char *id, const char *placeholder, const char *defaultValue, const int& length, 
                    const char *custom = "", const int& labelPlacement = WFM_LABEL_BEFORE);
              
    ESP_WMParameter(const WMParam_Data& WMParam_data);                   
                   
    ~ESP_WMParameter();
    
    void setWMParam_Data(const WMParam_Data& WMParam_data);
    void getWMParam_Data(WMParam_Data& WMParam_data);

    const char *getID();
    const char *getValue();
    const char *getPlaceholder();
    int         getValueLength();
    int         getLabelPlacement();
    const char *getCustomHTML();
    
  private:
  
    WMParam_Data _WMParam_data;
    
    const char *_customHTML;

    void init(const char *id, const char *placeholder, const char *defaultValue, const int& length, 
              const char *custom, const int& labelPlacement);

    friend class ESP_WiFiManager;
};

////////////////////////////////////////////////////

#define USE_DYNAMIC_PARAMS				true
#define DEFAULT_PORTAL_TIMEOUT  	60000L

// From v1.0.10 to permit disable/enable StaticIP configuration in Config Portal from sketch. Valid only if DHCP is used.
// You have to explicitly specify false to disable the feature.
#ifndef USE_STATIC_IP_CONFIG_IN_CP
  #define USE_STATIC_IP_CONFIG_IN_CP          true
#endif

////////////////////////////////////////////////////
////////////////////////////////////////////////////

class ESP_WiFiManager
{
  public:

    ESP_WiFiManager(const char *iHostname = "");

    ~ESP_WiFiManager();

    // Can use with STA staticIP now
    bool          autoConnect();
    bool          autoConnect(char const *apName, char const *apPassword = NULL);

    //if you want to start the config portal
    bool          startConfigPortal();
    bool          startConfigPortal(char const *apName, char const *apPassword = NULL);

    // get the AP name of the config portal, so it can be used in the callback
    String        getConfigPortalSSID();
    
    
    // get the AP password of the config portal, so it can be used in the callback
    String        getConfigPortalPW();

    void          resetSettings();

    //sets timeout before webserver loop ends and exits even if there has been no setup.
    //usefully for devices that failed to connect at some point and got stuck in a webserver loop
    //in seconds setConfigPortalTimeout is a new name for setTimeout
    void          setConfigPortalTimeout(const unsigned long& seconds);
    void          setTimeout(const unsigned long& seconds);

    //sets timeout for which to attempt connecting, usefull if you get a lot of failed connects
    void          setConnectTimeout(const unsigned long& seconds);

    void          setDebugOutput(bool debug);
    //defaults to not showing anything under 8% signal quality if called
    void          setMinimumSignalQuality(const int& quality = 8);
    
    // KH, To enable dynamic/random channel
    int           setConfigPortalChannel(const int& channel = 1);
    //////
    
    //sets a custom ip /gateway /subnet configuration
    void          setAPStaticIPConfig(const IPAddress& ip, const IPAddress& gw, const IPAddress& sn);
    
    void          setAPStaticIPConfig(const WiFi_AP_IPConfig& WM_AP_IPconfig);
    void          getAPStaticIPConfig(WiFi_AP_IPConfig& WM_AP_IPconfig);
    
    //sets config for a static IP
    void          setSTAStaticIPConfig(const IPAddress& ip, const IPAddress& gw, const IPAddress& sn);
    
    void          setSTAStaticIPConfig(const WiFi_STA_IPConfig&  WM_STA_IPconfig);
    void          getSTAStaticIPConfig(WiFi_STA_IPConfig& WM_STA_IPconfig);
    
#if USE_CONFIGURABLE_DNS
    void          setSTAStaticIPConfig(const IPAddress& ip, const IPAddress& gw, const IPAddress& sn,
                                       const IPAddress& dns_address_1, const IPAddress& dns_address_2);
#endif

    //called when AP mode and config portal is started
    void          setAPCallback(void(*func)(ESP_WiFiManager*));
    
    //called when settings have been changed and connection was successful
    void          setSaveConfigCallback(void(*func)());

#if USE_DYNAMIC_PARAMS
    //adds a custom parameter
    bool 				  addParameter(ESP_WMParameter *p);
#else
    //adds a custom parameter
    void 				  addParameter(ESP_WMParameter *p);
#endif

    //if this is set, it will exit after config, even if connection is unsucessful.
    void          setBreakAfterConfig(bool shouldBreak);
    
    //if this is set, try WPS setup when starting (this will delay config portal for up to 2 mins)
    //TODO
    //if this is set, customise style
    void          setCustomHeadElement(const char* element);
    
    //if this is true, remove duplicated Access Points - defaut true
    void          setRemoveDuplicateAPs(bool removeDuplicates);
    
    //Scan for WiFiNetworks in range and sort by signal strength
    //space for indices array allocated on the heap and should be freed when no longer required
    int           scanWifiNetworks(int **indicesptr);

////////////////////////////////////////////////////

    // KH add to display SSIDs and PWDs in CP   
    void				  setCredentials(const char* ssid, const char* pwd, const char* ssid1, const char* pwd1)
    {
      _ssid   = String(ssid);
      _pass   = String(pwd);
      _ssid1  = String(ssid1);
      _pass1  = String(pwd1);
    }
    
    inline void	  setCredentials(String & ssid, String & pwd, String & ssid1, String & pwd1)
    {
      _ssid   = ssid;
      _pass   = pwd;
      _ssid1  = ssid1;
      _pass1  = pwd1;
    }

////////////////////////////////////////////////////

    // return SSID of router in STA mode got from config portal. NULL if no user's input //KH
    inline String	getSSID() 
    {
      return _ssid;
    }

    // return password of router in STA mode got from config portal. NULL if no user's input //KH
    inline String	getPW() 
    {
      return _pass;
    }
    
    // New from v1.1.0
    // return SSID of router in STA mode got from config portal. NULL if no user's input //KH
    inline String	getSSID1() 
    {
      return _ssid1;
    }

    // return password of router in STA mode got from config portal. NULL if no user's input //KH
    inline String	getPW1() 
    {
      return _pass1;
    }
    
    #define MAX_WIFI_CREDENTIALS        2
    
    String				getSSID(const uint8_t& index) 
    {
      if (index == 0)
        return _ssid;
      else if (index == 1)
        return _ssid1;
      else     
        return String("");
    }
    
    String				getPW(const uint8_t& index) 
    {
      if (index == 0)
        return _pass;
      else if (index == 1)
        return _pass1;
      else     
        return String("");
    }

////////////////////////////////////////////////////
    
    // New For configuring CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"
#if USING_CORS_FEATURE
    void setCORSHeader(const char* CORSHeaders)
    {     
      _CORS_Header = CORSHeaders;

      LOGWARN1(F("Set CORS Header to : "), _CORS_Header);
    }
    
    inline const char* getCORSHeader()
    {
      return _CORS_Header;
    }
#endif   

////////////////////////////////////////////////////
    
    //returns the list of Parameters
    ESP_WMParameter** getParameters();
    // returns the Parameters Count
    int           getParametersCount();

    const char*   getStatus(const int& status);

#ifdef ESP32
    String getStoredWiFiSSID();
    String getStoredWiFiPass();
#endif

    inline String WiFi_SSID()
    {
#ifdef ESP8266
      return WiFi.SSID();
#else
      return getStoredWiFiSSID();
#endif
    }

    inline String WiFi_Pass()
    {
#ifdef ESP8266
      return WiFi.psk();
#else
      return getStoredWiFiPass();
#endif
    }

    void setHostname()
    {
      if (RFC952_hostname[0] != 0)
      {
#if ESP8266      
        WiFi.hostname(RFC952_hostname);
#else

  // Check cores/esp32/esp_arduino_version.h and cores/esp32/core_version.h
  //#if ( ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2, 0, 0) )  //(ESP_ARDUINO_VERSION_MAJOR >= 2)
  #if ( defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 2) )
      WiFi.setHostname(RFC952_hostname);
  #else     
      // Still have bug in ESP32_S2 for old core. If using WiFi.setHostname() => WiFi.localIP() always = 255.255.255.255
      if ( String(ARDUINO_BOARD) != "ESP32S2_DEV" )
      {
        // See https://github.com/espressif/arduino-esp32/issues/2537
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        WiFi.setHostname(RFC952_hostname);
      } 
   #endif    
#endif        
      }
    }

////////////////////////////////////////////////////

#if USE_ESP_WIFIMANAGER_NTP
    
    inline String getTimezoneName() 
    {  
      return _timezoneName;
    }

    inline void setTimezoneName(const String& inTimezoneName) 
    {  
      _timezoneName = inTimezoneName;
    }
    
    //See: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    // EST5EDT,M3.2.0,M11.1.0 (for America/New_York)
    // EST5EDT is the name of the time zone
    // EST is the abbreviation used when DST is off
    // 6 hours is the time difference from GMT
    // EDT is the abbreviation used when DST is on
    // ,M3 is the third month
    // .2 is the second occurrence of the day in the month
    // .0 is Sunday
    // ,M11 is the eleventh month
    // .1 is the first occurrence of the day in the month
    // .0 is Sunday   
    
    const char * getTZ(const char * timezoneName)
    {               
      //const char TZ_NAME[][TIMEZONE_MAX_LEN]
      for (uint16_t index = 0; index < sizeof(TZ_NAME) / TIMEZONE_MAX_LEN; index++)
      {
        if ( !strncmp(timezoneName, (TZ_NAME[index]), strlen((TZ_NAME[index])) ) )
        {
          yield();
          return (ESP_TZ_NAME[index]);            
        }    
      }
      
      return "";      
    }
    
    
    const char * getTZ(const String& timezoneName)
    {
      return getTZ(timezoneName.c_str());      
    }
#endif    

////////////////////////////////////////////////////

  private:
    std::unique_ptr<DNSServer>        dnsServer;

    //KH, for ESP32
#ifdef ESP8266
    std::unique_ptr<ESP8266WebServer> server;
#else		//ESP32
    std::unique_ptr<WebServer>        server;
#endif

#define RFC952_HOSTNAME_MAXLEN      24
    char RFC952_hostname[RFC952_HOSTNAME_MAXLEN + 1];

    char* getRFC952_hostname(const char* iHostname);

    void          setupConfigPortal();
    void          startWPS();
    //const char*   getStatus(const int& status);

    const char*   _apName = "no-net";
    const char*   _apPassword = NULL;
    
    String        _ssid   = "";
    String        _pass   = "";
    
    String        _ssid1  = "";
    String        _pass1  = "";

    ////////////////////////////////////////////////////
    
#if USE_ESP_WIFIMANAGER_NTP
    // Timezone info
    String        _timezoneName         = "";
#endif

    ////////////////////////////////////////////////////

    unsigned long _configPortalTimeout  = 0;

    unsigned long _connectTimeout       = 0;
    unsigned long _configPortalStart    = 0;

    int           numberOfNetworks;
    int           *networkIndices;
    
    // KH, To enable dynamic/random channel
    // default to channel 1
    #define MIN_WIFI_CHANNEL      1
    #define MAX_WIFI_CHANNEL      11    // Channel 12,13 is flaky, because of bad number 13 ;-)

    int _WiFiAPChannel = 1;

    WiFi_AP_IPConfig  _WiFi_AP_IPconfig;
    
    WiFi_STA_IPConfig _WiFi_STA_IPconfig = { IPAddress(0, 0, 0, 0), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
                                             IPAddress(192, 168, 2, 1), IPAddress(8, 8, 8, 8) };

    ////////////////////////////////////////////////////
    
    int           _paramsCount            = 0;
    int           _minimumQuality         = -1;
    bool          _removeDuplicateAPs     = true;
    bool          _shouldBreakAfterConfig = false;
    bool          _tryWPS                 = false;

    const char*   _customHeadElement      = "";

    int           status                  = WL_IDLE_STATUS;
    
    // For configuring CORS Header, default to WM_HTTP_CORS_ALLOW_ALL = "*"
#if USING_CORS_FEATURE
    const char*     _CORS_Header          = WM_HTTP_CORS_ALLOW_ALL;   //"*";
#endif   

    void          setWifiStaticIP();   
    int           reconnectWifi();
    int           connectWifi(const String& ssid = "", const String& pass = "");
   
    uint8_t       waitForConnectResult();

    void          handleRoot();
    void          handleWifi();
    void          handleWifiSave();
    void          handleServerClose();
    void          handleInfo();
    void          handleState();
    void          handleScan();
    void          handleReset();
    void          handleNotFound();
    bool          captivePortal();
    
    void          reportStatus(String& page);

    // DNS server
    const byte    DNS_PORT = 53;

    //helpers
    int           getRSSIasQuality(const int& RSSI);
    bool          isIp(const String& str);
    String        toStringIp(const IPAddress& ip);

    bool          connect;
    bool          stopConfigPortal = false;
    
    bool          _debug = false;     //true;

    void(*_apcallback)  (ESP_WiFiManager*)  = NULL;
    void(*_savecallback)()                  = NULL;

    ////////////////////////////////////////////////////

#if USE_DYNAMIC_PARAMS
    int                    _max_params;
    ESP_WMParameter** _params;
#else
    ESP_WMParameter* _params[WIFI_MANAGER_MAX_PARAMS];
#endif

    ////////////////////////////////////////////////////

    template <typename Generic>
    void          DEBUG_WM(Generic text);

    template <class T>
    auto optionalIPFromString(T *obj, const char *s) -> decltype(obj->fromString(s)) 
    {
      return  obj->fromString(s);
    }
    
    auto optionalIPFromString(...) -> bool 
    {
      LOGINFO("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
      
      return false;
    }
};

////////////////////////////////////////////////////

#endif    // ESP_WiFiManager_h


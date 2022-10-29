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

const char WM_HTTP_HEAD_END[] PROGMEM = "</head><body><div style='text-align: center'><img src='data:image/png;base64,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'></div><div class='container'><div style='text-align:left;display:inline-block;min-width:260px;'>";

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


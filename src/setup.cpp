// Project: ESP32-Doorbell
// Programmers: Jos van der Zande
//              Paul Hermans
//
// Setup module
//
#include <ESPAsyncWebServer.h>  // Local WebServer used to server the configuration portal
#include <SPIFFS.h>
#include <esp_camera.h>
#include "ArduinoJson.h"
#include "setup.h"
#include "WebServer.h"
#include "Main.h"
#include "domoticz.h"
//#include "camera_pins.h"
#ifndef VERSION
#define VERSION 2.0.0
#endif

// Define private used funcs
bool GetJsonField(String key, DynamicJsonDocument doc, char *variable);
bool GetJsonField(String key, DynamicJsonDocument doc, uint *variable);

extern AsyncWebServer webserver;

// Get Variable info from JSON input CHAR STRINGS
bool GetJsonField(String key, DynamicJsonDocument doc, char *variable) {
    const char *value = doc[key];
    String msg = F("Key ");
    msg += key;
    if (value) {
        strcpy(variable, value);
        msg += F("=");
        msg += String(value);
        msg += F("\n");
        AddLogMessageD(msg);
        return true;
    }
    msg += F(" not in configfile, using default\n");
    AddLogMessageW(msg);
    return false;
}

// Get Variable info from JSON input unsigned INT Values
bool GetJsonField(String key, DynamicJsonDocument doc, uint *variable) {
    const char *value = doc[key];
    String msg = F("Key ");
    msg += key;
    if (value) {
        *variable = atoi(value);
        msg += F("=");
        msg += String(*variable);
        msg += F("\n");
        AddLogMessageD(msg);
        return true;
    }
    msg += F(" not in configfile, using default\n");
    AddLogMessageW(msg);
    return false;
}

// Restore ESP settings From SPIFFS
bool Restore_ESPConfig_from_SPIFFS() {
    // SPIFFS
    AddLogMessageI(F("Restore configuration from SPIFF\n"));
    File file = SPIFFS.open("/ESP_CAM_CONFIG.json", "r");
    if (!file || file.isDirectory()) {
        AddLogMessageD(F("- empty file or failed to open file\n"));
        return false;
    }
    String fileContent;
    while (file.available()) {
        fileContent += String((char)file.read());
        if (fileContent.length() > 5000) {
            AddLogMessageE(F("- file too large, assume it is corrupt and use defaults\n"));
            fileContent = "";
            file.close();
            SPIFFS.remove("/ESP_CAM_CONFIG.json");
            break;
        }
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error) {
        AddLogMessageD(F("Config Parsing failed\n"));
        return false;
    }
    GetJsonField("webloglevel", doc, &webloglevel);
    GetJsonField("esp_board", doc, esp_board);
    GetJsonField("esp_name", doc, esp_name);
    GetJsonField("esp_uname", doc, esp_uname);
    GetJsonField("esp_pass", doc, esp_pass);
    GetJsonField("IPsetting", doc, IPsetting);
    GetJsonField("IPaddr", doc, IPaddr);
    GetJsonField("SubNetMask", doc, SubNetMask);
    GetJsonField("GatewayAddr", doc, GatewayAddr);
    GetJsonField("SendProtocol", doc, SendProtocol);
    GetJsonField("ServerIP", doc, ServerIP);
    GetJsonField("ServerPort", doc, ServerPort);
    GetJsonField("ServerUser", doc, ServerUser);
    GetJsonField("ServerPass", doc, ServerPass);
    GetJsonField("DomoticzIDX", doc, DomoticzIDX);
    GetJsonField("MQTTsubscriber", doc, MQTTsubscriber);
    GetJsonField("MQTTtopicin", doc, MQTTtopicin);
    GetJsonField("Flashcount", doc, &Flashcount);
    GetJsonField("Flashduration", doc, &Flashduration);
    GetJsonField("Rotation", doc, Rotation);
    return true;
}

// Save the information from SETUP.HTM to SPIFFS
void Save_NewESPConfig_to_SPIFFS(AsyncWebServerRequest *request) {
    static char json_response[1024];
    char *p = json_response;
    *p++ = '{';
    p += sprintf(p, "\"webloglevel\":\"%s\",", urlDecode(request->arg("webloglevel")).c_str());
    p += sprintf(p, "\"esp_board\":\"%s\",", urlDecode(request->arg("esp_board")).c_str());
    p += sprintf(p, "\"esp_name\":\"%s\",", urlDecode(request->arg("esp_name")).c_str());
    p += sprintf(p, "\"esp_uname\":\"%s\",", urlDecode(request->arg("esp_uname")).c_str());
    p += sprintf(p, "\"esp_pass\":\"%s\",", urlDecode(request->arg("esp_pass")).c_str());
    p += sprintf(p, "\"IPsetting\":\"%s\",", urlDecode(request->arg("IPsetting")).c_str());
    p += sprintf(p, "\"IPaddr\":\"%s\",", urlDecode(request->arg("IPaddr")).c_str());
    p += sprintf(p, "\"SubNetMask\":\"%s\",", urlDecode(request->arg("SubNetMask")).c_str());
    p += sprintf(p, "\"GatewayAddr\":\"%s\",", urlDecode(request->arg("GatewayAddr")).c_str());
    p += sprintf(p, "\"SendProtocol\":\"%s\",", urlDecode(request->arg("Send_Protocol")).c_str());
    p += sprintf(p, "\"ServerIP\":\"%s\",", urlDecode(request->arg("ServerIP")).c_str());
    p += sprintf(p, "\"ServerPort\":\"%s\",", urlDecode(request->arg("ServerPort")).c_str());
    p += sprintf(p, "\"ServerUser\":\"%s\",", urlDecode(request->arg("ServerUser")).c_str());
    p += sprintf(p, "\"ServerPass\":\"%s\",", urlDecode(request->arg("ServerPass")).c_str());
    p += sprintf(p, "\"DomoticzIDX\":\"%s\",", urlDecode(request->arg("DomoticzIDX")).c_str());
    p += sprintf(p, "\"MQTTsubscriber\":\"%s\",", urlDecode(request->arg("MQTTsubscriber")).c_str());
    p += sprintf(p, "\"MQTTtopicin\":\"%s\",", urlDecode(request->arg("MQTTtopicin")).c_str());
    p += sprintf(p, "\"Flashcount\":\"%s\",", urlDecode(request->arg("Flashcount")).c_str());
    p += sprintf(p, "\"Flashduration\":\"%s\",", urlDecode(request->arg("Flashduration")).c_str());
    p += sprintf(p, "\"Rotation\":\"%s\",", urlDecode(request->arg("Rotation")).c_str());
    p += sprintf(p, "\"dummy\":\"\"");
    *p++ = '}';
    *p++ = 0;
    File file = SPIFFS.open("/ESP_CAM_CONFIG.json", "w");
    String msg = F("Saving ESPCAM configuration to SPIFF, ");
    if (!file) {
        msg += F(" failed to open file for writing\n");
        AddLogMessageE(msg);
        return;
    }
    if (file.print(json_response)) {
        msg += F("- config saved\n");
        AddLogMessageI(msg);
    } else {
        msg += F("- config save failed!!!!");
        AddLogMessageE(msg);
    }
    file.close();
}

// Add possible template variables for the webpages
String TranslateTemplateVars(const String &var) {
    if (var == "VERSION") {
        char tmp[40];
        sprintf(tmp, "%s - %s", VERSION, BUILD_MAIN);
        return tmp;
    }
    if (var == "VERSION_MAJOR")
        return VERSION;
    if (var == "esp_board")
        return esp_board;
    if (var == "BUTTON_GPIO_NUM")
        return String(BUTTON_GPIO_NUM);
    if (var == "BUTTONLED_GPIO_NUM")
        return String(BUTTONLED_GPIO_NUM);
    if (var == "webloglevel")
        return String(webloglevel);
    if (var == "esp_board")
        return esp_board;
    if (var == "esp_name")
        return esp_name;
    if (var == "esp_uname")
        return esp_uname;
    if (var == "esp_pass")
        return esp_pass;
    if (var == "IPsetting")
        return IPsetting;
    if (var == "IPaddr")
        return IPaddr;
    if (var == "SubNetMask")
        return SubNetMask;
    if (var == "GatewayAddr")
        return GatewayAddr;
    if (var == "SendProtocol")
        return SendProtocol;
    if (var == "ServerIP")
        return ServerIP;
    if (var == "ServerPort")
        return ServerPort;
    if (var == "ServerUser")
        return ServerUser;
    if (var == "ServerPass")
        return ServerPass;
    if (var == "DomoticzIDX")
        return DomoticzIDX;
    if (var == "MQTTsubscriber")
        return MQTTsubscriber;
    if (var == "MQTTtopicin")
        return MQTTtopicin;
    if (var == "Flashcount")
        return String(Flashcount);
    if (var == "Flashduration")
        return String(Flashduration);
    if (var == "Rotation")
        return Rotation;
    if (var == "ipaddr")
        return WiFi.localIP().toString();
    if (var == "ipgate")
        return WiFi.gatewayIP().toString();
    if (var == "ipnetm")
        return WiFi.subnetMask().toString();
    if (var == "wifi_ssid")
        return String(WiFi.SSID());
    if (var == "wifi_rssi")
        return String(WiFi.RSSI());
    if (var == "SPIFFS_tot")
        return String(SPIFFS.totalBytes() / 1000);
    if (var == "SPIFFS_used")
        return String(SPIFFS.usedBytes() / 1000);
    if (var == "SPIFFS_free")
        return String((SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1000);
    if (var == "CamStatus") {
        if (strcmp(esp_board, "none") == 0) {
            return F("Camera not yet defined in ESP Settings!");
        }
        sensor_t *cst = esp_camera_sensor_get();
        if (cst == NULL) {
            return F("not detected");
        } else {
            return F("working");
        }
    }
    return String();
}
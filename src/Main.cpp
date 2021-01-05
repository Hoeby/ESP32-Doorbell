// Project: ESP32-Doorbell          V2.0 (jan-2021)
// Programmers: Jos van der Zande
//              Paul Hermans
//
// Main ESP32-Doorbell module
//
#include <WiFi.h>
#include <SPIFFS.h>
#include "Main.h"
#include "WebServer.h"
#include "domoticz.h"
#include "setup.h"
#include "cam.h"

const char BUILD_MAIN[] = __DATE__ " " __TIME__;
//**************************************************************************************************************************************************
//**                                                           Setting Wifi credentials                                                           **
//**************************************************************************************************************************************************
uint webloglevel = 3;  //loglevel to show in WebConsole. 0-5

char esp_board[20] = "none";         //ESP type selection
char esp_name[20] = "ESP Doorbell";  //Wifi SSID waarop ESP32-cam zich moet aanmelden.
char esp_uname[10] = "admin";        //Username voor weblogin.
char esp_pass[20] = "espadmin";      //Bijbehorend wachtwoord voor SSID & Weblogin, moet min 8 characters zijn voor WifiManager

//**************************************************************************************************************************************************
//**                                                              IP settings device                                                              **
//**************************************************************************************************************************************************
char IPsetting[6] = "DHCP";             // DHCP/Fixed
char IPaddr[16] = "192.168.0.0";        // IP adres
char SubNetMask[16] = "255.255.255.0";  // subnet mask
char GatewayAddr[16] = "192.168.0.1";   // Gateway adres
//Als device geen internet nodig heeft, voer een fake gateway in.

//**************************************************************************************************************************************************
//**                                                          Setting Server credentials                                                         **
//**************************************************************************************************************************************************
char SendProtocol[5] = "none";               //Define protocol to use
char ServerIP[16] = "192.168.0.0";           //Domoticz Server IP adres.
char ServerPort[5] = "8080";                 //Domoticz Server poort adres.
char ServerUser[16] = "";                    //MQTT username
char ServerPass[16] = "";                    //MQTT password
char DomoticzIDX[5] = "999";                 //Domoticz IDX nummer welke geschakeld moet worden.
char MQTTsubscriber[20] = "ESP32CAM/Input";  //MQTT MQTTsubscriber name
char MQTTtopicin[20] = "domoticz/in";        //MQTT Topic name

//**************************************************************************************************************************************************
//**                                                              Setting PUSH button                                                             **
//**************************************************************************************************************************************************
uint Flashcount = 5;       //How many time has led to flash (1 time equals 1 sec)
uint Flashduration = 500;  //length of one On/Off
//**************************************************************************************************************************************************
//**                                                              Setting CAMERA                                                             **
//**************************************************************************************************************************************************
char Rotation[4] = "0";  //Define Capture/Stream WebPage camera rotation in degrees (-)0-180
//**************************************************************************************************************************************************
//**       Static ESP-BOARD Settings variables, use _ESP_Board_Settings.h to define all these per ESP_BOARD type.                                                           **
//**************************************************************************************************************************************************
int PWDN_GPIO_NUM = 0;
int RESET_GPIO_NUM = 0;
int XCLK_GPIO_NUM = 0;
int SIOD_GPIO_NUM = 0;
int SIOC_GPIO_NUM = 0;
int Y9_GPIO_NUM = 0;
int Y8_GPIO_NUM = 0;
int Y7_GPIO_NUM = 0;
int Y6_GPIO_NUM = 0;
int Y5_GPIO_NUM = 0;
int Y4_GPIO_NUM = 0;
int Y3_GPIO_NUM = 0;
int Y2_GPIO_NUM = 0;
int VSYNC_GPIO_NUM = 0;
int HREF_GPIO_NUM = 0;
int PCLK_GPIO_NUM = 0;
int BUTTON_GPIO_NUM = 12;     // Set the BUTTON GPIO pin
int BUTTONLED_GPIO_NUM = 13;  // Set the LED GPIO pin
int ON_LED_STATE = HIGH;      // Set the default State of the LED GPIO pin

//**************************************************************************************************************************************************
//**                                                                END SETTINGS END                                                              **
//**************************************************************************************************************************************************

// Define private used funcs
void start_ssdp_service();

AsyncWebServer webserver(80);

DNSServer dns;
ssdpAWS mySSDP(&webserver);

// Define whether the pushbutton changes to LOW or HIGH when pushed
#if defined(BUTTOM_PUSH_STATE)
const int buttonPushedState = BUTTOM_PUSH_STATE;
#else
const int buttonPushedState = HIGH;
#endif

unsigned long MQTT_lasttime;  // MQTT check lasttime
bool WifiOK = false;          // WiFi status
bool mqtt_initdone = false;   // MQTT status
bool reboot = false;          // Pending reboot status
long rebootdelay = 0;         // used to calculate the delay

void setup() {
    //EEPROM.begin(200);
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
        ESP_LOGE(TAG, "An Error has occurred while mounting SPIFFS");

    // restore previous ESP saved settings
    Restore_ESPConfig_from_SPIFFS();

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&webserver, &dns);
    //reset saved settings
    //  wifiManager.resetSettings();
    // Previous line doesn't always work so this is another option to erase the EEPROM and all saved settings
    //  pio run --target erase

    // Set hardcoded IP Stettings when Fixed IP is defined
    if (strcmp(IPsetting, "Fixed") == 0) {
        AddLogMessageI(F("==>Set Static IP\n"));
        IPAddress ip;
        IPAddress nm;
        IPAddress gw;
        ip.fromString(IPaddr);
        nm.fromString(SubNetMask);
        gw.fromString(GatewayAddr);
        wifiManager.setSTAStaticIPConfig(ip, gw, nm);
    }
    // Try connecting to previous saved WiFI settings or else start as AP
    wifiManager.autoConnect(esp_name, esp_pass);
    //if you get here you have connected to the WiFi
    WifiOK = true;

    // Get Network time
    const char *NTPpool = "nl.pool.ntp.org";
    const char *defaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
    configTzTime(defaultTimezone, NTPpool);  //sets TZ and starts NTP sync
    // wait max 5 secs till time is synced
    AddLogMessageI("Wifi connected " + WiFi.SSID() + "  IP:" + WiFi.localIP().toString() + "  RSSI:" + String(WiFi.RSSI()) + "\n");
    struct tm timeinfo;
    for (uint i = 0; i < 10; i++) {
        if (getLocalTime(&timeinfo,500)) {
            AddLogMessageI(F("Time synced.\n"));
            break;
        }
    }
    // Init Camera
    initcamera();

    // Set Button GPIO for INPUT and PULL UP/DOWN depending of the definition
    pinMode(BUTTON_GPIO_NUM, (buttonPushedState ? INPUT_PULLDOWN : INPUT_PULLUP));
    // Set LED output and optional the same flashes for the defined LED_BUILTIN
    pinMode(BUTTONLED_GPIO_NUM, OUTPUT);
    digitalWrite(BUTTONLED_GPIO_NUM, ON_LED_STATE);
    //Necessary for ESP_EYE (Jos:Not sure about this as this is for the other onboard buttons)
    if (strcmp(esp_board, "ESP_EYE") == 0) {
        pinMode(13, INPUT_PULLUP);
        pinMode(14, INPUT_PULLUP);
    }

    // start mqtt
    if (!strcmp(SendProtocol, "mqtt")) {
        Mqtt_begin();
        mqtt_initdone = true;
    }
    MQTT_lasttime = millis();

    // Init WebServer
    WebServerInit(&webserver);

    // Make ESP-CAM "known" in the network under it's ESP_NAME
    start_ssdp_service();
    AddLogMessageI(F("ESP Doorbell initialized\n"));
}

void loop() {
    // Don't do anything when shutting down and wait 1 second before rebooting
    if (reboot) {
        if (rebootdelay + 1000 > millis()) {
            Serial.flush();
            SPIFFS.end();
            delay(500);
            ESP.restart();
            delay(2000);
        }
        return;
    }

    // Check button or open actions for it
    Button_Check();

    // Flash LED when WiFi is lost
    if (!WiFi.isConnected()) {
        // Add a WiFI log record when WiFI goes down
        if (WifiOK) {
            WifiOK = false;
            AddLogMessageE(F("WiFi connection lost!\n"));
        }
        LedToggle();
        delay(100);
        LedToggle();
        delay(100);
    } else {
        // Add a WiFI log record when WiFI is restored
        if (!WifiOK) {
            WifiOK = true;
            AddLogMessageW(F("WiFi connection restored\n"));
        }
        // Process MQTT when selected
        if (mqtt_initdone && (millis() > MQTT_lasttime + 500)) {
            Mqtt_Loop();
            MQTT_lasttime = millis();
        }
        // Send any logmessages to the browser
        SendNextLogMessage();
    }

    // short pause
    delay(10);
}

void start_ssdp_service() {
    //initialize mDNS service
    //Define SSDP and model name
    const char *SSDP_Name = esp_name;
    const char *modelName = esp_board;
    const char *nVersion = BUILD_MAIN;
    const char *SerialNumber = "";
    const char *Manufacturer = "ESP32CAM";
    const char *ManufacturerURL = "https://github.com/jvanderzande/ESPCAM";
    mySSDP.begin(SSDP_Name, SerialNumber, modelName, nVersion, Manufacturer, ManufacturerURL);
    AddLogMessageI(F("SSDP started\n"));
}
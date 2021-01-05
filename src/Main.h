#include <Arduino.h>

#include <ssdpAWS.h>              // SSDP for ESPAsyncWebServer
#include <DNSServer.h>            // Local DNS Server used for redirecting all requests to the configuration portal
#include <ESPAsyncWebServer.h>    // Local WebServer used to server the configuration portal
#include <ESPAsyncWiFiManager.h>  // https://github.com/alanswx/ESPAsyncWiFiManager

//**************************************************************************************************************************************************
//**                                                              Setting Camera type                                                             **
//**************************************************************************************************************************************************
//-------------------------------------------------------
// Global variables defined in Main.cpp
//-------------------------------------------------------
extern uint webloglevel;
extern char esp_board[20];
extern char esp_name[20];
extern char esp_uname[10];
extern char esp_pass[20];
extern char IPsetting[6];
extern char IPaddr[16];
extern char SubNetMask[16];
extern char GatewayAddr[16];
extern char SendProtocol[5];
extern char ServerIP[16];
extern char ServerPort[5];
extern char ServerUser[16];
extern char ServerPass[16];
extern char DomoticzIDX[5];
extern char MQTTsubscriber[20];
extern char MQTTtopicin[20];
extern const int buttonPushedState;
extern uint Flashcount;
extern uint Flashduration;
extern char Rotation[4];
extern const char BUILD_MAIN[];
extern bool reboot;
extern long rebootdelay;
extern bool mqtt_initdone;

extern int PWDN_GPIO_NUM;
extern int RESET_GPIO_NUM;
extern int XCLK_GPIO_NUM;
extern int SIOD_GPIO_NUM;
extern int SIOC_GPIO_NUM;
extern int Y9_GPIO_NUM;
extern int Y8_GPIO_NUM;
extern int Y7_GPIO_NUM;
extern int Y6_GPIO_NUM;
extern int Y5_GPIO_NUM;
extern int Y4_GPIO_NUM;
extern int Y3_GPIO_NUM;
extern int Y2_GPIO_NUM;
extern int VSYNC_GPIO_NUM;
extern int HREF_GPIO_NUM;
extern int PCLK_GPIO_NUM;
extern int BUTTON_GPIO_NUM;
extern int BUTTONLED_GPIO_NUM;
extern int ON_LED_STATE;

//-------------------------------------------------------
// Global Functions


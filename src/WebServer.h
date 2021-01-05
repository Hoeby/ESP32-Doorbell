#include <Arduino.h>

// pio run -t uploadfs
// will upload all files of /data into SPIFFS

String TranslateTemplateVars(const String& var);
void WebServerInit(AsyncWebServer * server);

void ESPShowPagewithTemplate(AsyncWebServerRequest *request);

bool _webAuth(AsyncWebServerRequest *request);
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len);
void Web_messageReceived(AsyncWebServerRequest *request);
void ESPSaveSettings(AsyncWebServerRequest *request);
void ApplyCamSettings(AsyncWebServerRequest *request);

void LogClean(AsyncWebServerRequest *request);
void LogDump(AsyncWebServerRequest *request);
void ConfigDump(AsyncWebServerRequest *request);
void ConfigCamDump(AsyncWebServerRequest *request);
void ConfigCamCurrent(AsyncWebServerRequest *request);

void ConfigFileUploads(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);

void WrongPage(AsyncWebServerRequest *request);

    /*
#define ARDUHAL_LOG_LEVEL_NONE       (0)
#define ARDUHAL_LOG_LEVEL_ERROR      (1)
#define ARDUHAL_LOG_LEVEL_WARN       (2)
#define ARDUHAL_LOG_LEVEL_INFO       (3)
#define ARDUHAL_LOG_LEVEL_DEBUG      (4)
#define ARDUHAL_LOG_LEVEL_VERBOSE    (5)
*/

void AddLogMessage(String msg, String Module, String Function, String Severity, int Line);
#define AddLogMessageE( message ) AddLogMessage(message, __FILE__, __FUNCTION__, "E", __LINE__)
#define AddLogMessageW( message ) AddLogMessage(message, __FILE__, __FUNCTION__, "W", __LINE__)
#define AddLogMessageI( message ) AddLogMessage(message, __FILE__, __FUNCTION__, "I", __LINE__)
#define AddLogMessageD( message ) AddLogMessage(message, __FILE__, __FUNCTION__, "D", __LINE__)
#define AddLogMessageV( message ) AddLogMessage(message, __FILE__, __FUNCTION__, "V", __LINE__)

// Define public used funcs
void SendNextLogMessage();
String urlDecode(String input);

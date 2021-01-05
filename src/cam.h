#include <Arduino.h>

// Define public used funcs
void initcamera();
void sendJpg(AsyncWebServerRequest *request);
void streamJpg(AsyncWebServerRequest *request);
bool Restore_CamSettings_from_SPIFFS();
void Save_NewCAMConfig_to_SPIFFS(AsyncWebServerRequest *request);
bool Set_Cam_Settings_from_JSON(char *JSONCamSetting);
char *GetCurrentCamSettings();

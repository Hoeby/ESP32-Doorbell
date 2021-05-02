// Project: ESP32-Doorbell
// Programmers: Jos van der Zande
//              Paul Hermans
//
// Domoticz functions sourcefile
//
#include "domoticz.h"
#include "Main.h"
#include "WebServer.h"
#include <base64.h>
#include <MQTT.h>
#include "ArduinoJson.h"

WiFiClient client;  // wifi client object
MQTTClient MqttClient;

uint Flash_done = 0;               //How many time has led has flashed
long Flash_timer = 0;              //Flash timer
bool ButtonProcessActive = false;  //Button pressed process started
int ledState = ON_LED_STATE;       //the current state of LED High/Low
bool ledon_bydefault = true;       //is the default state for the LED ON? True/False

uint MotionDisable_done = 0;       //How long is motion disabled
long Motion_timer = 0;              //Motion timer
bool MotionProcessActive = false;  //Motion Active process started

//-4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
//-3 : MQTT_CONNECTION_LOST - the network connection was broken
//-2 : MQTT_CONNECT_FAILED - the network connection failed
//-1 : MQTT_DISCONNECTED - the client is disconnected cleanly
// 0 : MQTT_CONNECTED - the client is connected
// 1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
// 2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
// 3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
// 4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
// 5 : MQTT_CONNECT_UNAUTHORIZED -

void Mqtt_begin() {
    if (strcmp(SendProtocol, "mqtt") != 0)
        return;

    AddLogMessageI("Init MQTT\n");
    MqttClient.begin(ServerIP, atoi(ServerPort), client);
    MqttClient.onMessage(Mqtt_messageReceived);  // subscribe to mqtt for input messages
    Mqtt_Connect();
}

bool Mqtt_Connect() {
    if (strcmp(SendProtocol, "mqtt") != 0)
        return false;

    if (MqttClient.connected())
        return true;

    if (MqttClient.connect(esp_name, ServerUser, ServerPass)) {
        AddLogMessageI("MQTT connected, subscribing to:" + String(MQTTsubscriber) + "\n");
        MqttClient.subscribe(MQTTsubscriber);
        return true;
    } else {
        AddLogMessageE("MQTT failed to connect! Err:" + String(MqttClient.lastError()) + "\n");
    }
    return false;
}

// Check for new MQTT messages
bool Mqtt_Loop() {
    // return immediately when not using mqtt
    if (strcmp(SendProtocol, "mqtt") != 0)
        return false;
    // Check connection to mqtt and messages
    if (Mqtt_Connect()) {
        // check for queued messages
        return MqttClient.loop();
    }
    return false;
}

void Mqtt_messageReceived(String &topic, String &payload) {
    AddLogMessageI("MQTT incoming msg: " + payload + "\n");
    process_messageReceived(payload);
}

//==================================================================
// Put here the received tasks logic from either MQTT or Webserver
String process_messageReceived(String payload) {
    DynamicJsonDocument doc(1024);
    // translate JSON payload into doc
    DeserializationError error = deserializeJson(doc, urlDecode(payload));
    if (error) {
        String msg = "{\"status\":\"Error\",\"Message:\":\"";
        msg += error.c_str();
        msg += "\"}\n";
        AddLogMessageE("Command Parsing failed for payload:" + payload + "\n");
        AddLogMessageE(msg);
        return msg;
    }
    // Loop through provided keywords
    AddLogMessageI("Processing command: " + payload + "\n");
    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        const char *key = kv.key().c_str();
        const char *value = kv.value().as<char *>();
        Serial.printf("key:%s  value:%s", key, value);
        if (strcasecmp(key, "led") == 0) {
            AddLogMessageI(String("Switch LED to ") + String(value) + "\n");
            if (strcasecmp(value, "on") == 0)
                ledon_bydefault = true;
            else
                ledon_bydefault = false;
            // Set the led to the new default
            SetLedtoDefault(true);
        } else if (strcasecmp(key, "reboot") == 0) {
            AddLogMessageI("Rebooting ESP now.\n");
            reboot = true;
            rebootdelay = millis();
        } else {
            AddLogMessageE("Invalid Key=" + String(key) + "  value=" + String(value) + "\n");
        }
    }

    return "{\"status\":\"Ok\"}";
}

// Check the button state and process the Flash & Switch action
void Button_Check() {
    // Check if button is activated
    if (digitalRead(BUTTON_GPIO_NUM) == buttonPushedState && !ButtonProcessActive) {
        ButtonProcessActive = true;
        AddLogMessageI(F("Button Pressed.\n"));
        // Perform Domoticz action when button is pressed
        Button_Pressed("On");
        AddLogMessageI(F("LED"));
        Flash_done = 1;
        // Switch led on before Sending command to Domoticz
        LedToggle();
        Flash_timer = millis();
        return;
    }
    // perform flashes and switch Off
    if (ButtonProcessActive) {
        if (Flash_done > Flashcount * 2) {
            LedToggle();
            AddLogMessageI(" Done.\n");
            Button_Pressed("Off");
            ButtonProcessActive = false;
            // ensure the LED is off again at the end of the cycle
            SetLedtoDefault(false);
        } else {
            if (Flash_timer + (Flashduration / 2) < millis()) {
                LedToggle();
                Flash_timer = millis();
                Flash_done++;
            }
        }
    }
}

// Function to process when button is pressed
void Button_Pressed(const char *State) {
    AddLogMessageI("Button: " + String(State) + "\n");
    if (!strcmp(SendProtocol, "json")) {
        Domoticz_JSON_Switch(DomoticzIDX, State);
    } else if (!strcmp(SendProtocol, "mqtt")) {
        Domoticz_MQTT_Switch(DomoticzIDX, State);
    } else {
        AddLogMessageW(F("SendProtocol = \"none\", No command to send\n"));
    }
}

// function to switch domoticz switch on/off
bool Domoticz_JSON_Switch(const char *Idx, const char *State) {
    client.stop();  // Clear any current connections
    bool respok = true;
    if (!client.connect(ServerIP, atoi(ServerPort))) {
        AddLogMessageE(F("Domoticz JSON Connection failed\n"));
        return false;
    }
    // Set UserVarible to button pressed
    String url = F("/json.htm?type=command&param=switchlight&idx=");
    url += String(Idx);
    url += F("&switchcmd=");
    url += State;
    client.print(F("GET "));
    client.print(url);
    // add header
    client.print(F(" HTTP/1.1\r\n"));
    // Add Authentication to the HTTP header when USER or Password is defined
    if (!(strcmp(ServerUser, "") == 0) || !(strcmp(ServerPass, "") == 0)) {
        String auth = base64::encode(String(ServerUser) + ":" + String(ServerPass));
        AddLogMessageI("  -> Use basic Authentication: " + auth + "\n");
        client.printf("Authorization: Basic %s\r\n", auth.c_str());
    }
    client.print(F("\r\n\r\n Connection: close\r\n\r\n"));
    unsigned long timeout = millis();
    AddLogMessageD("Domoticz URL " + url + "\n");
    while (client.available() == 0) {
        if (millis() - timeout > 2000) {
            AddLogMessageE(F("Domoticz JSON Connection timeout\n"));
            client.stop();
            return false;
        }
    }
    String response = client.readString();
    if ((response.indexOf("200 OK") > 0) && (response.indexOf("\"ERR\"") < 0)) {
        AddLogMessageI(F("Domoticz Switch command send\n"));
        respok = true;
    } else {
        AddLogMessageE("Domoticz Switch command failed:" + response + "\n");
        respok = false;
    }
    client.stop();
    return respok;
}

bool Domoticz_MQTT_Switch(const char *Idx, const char *State) {
    String MqttMessage = F("{\"command\": \"switchlight\", \"idx\": ");
    MqttMessage += String(Idx);
    MqttMessage += F(", \"switchcmd\": \"");
    MqttMessage += State;
    MqttMessage += F("\"}");
    if (Mqtt_Connect()) {
        String msg = F("mqtt publish t= ");
        msg += MQTTtopicin;
        msg += F(" m=");
        msg += MqttMessage;
        msg += F("\n");
        AddLogMessageI(msg);
        MqttClient.publish(MQTTtopicin, ((char *)MqttMessage.c_str()));
        return true;
    } else {
        AddLogMessageE(F("Mqtt not connected so Switch message not send!\n"));
        return false;
    }
}

void LedToggle() {
    ledState = !ledState;
    if (ledState)
        AddLogMessageI(".On");
    else
        AddLogMessageI(".Off ");
    digitalWrite(BUTTONLED_GPIO_NUM, ledState);
#if defined(LED_BUILTIN)
    digitalWrite(LED_BUILTIN, ledState);
#endif
}

// Set LED to the requested default in case the state is different
void SetLedtoDefault(bool warn) {
    if (ledon_bydefault) {
        if (ledState != ON_LED_STATE) {
            ledState = ON_LED_STATE;
            digitalWrite(BUTTONLED_GPIO_NUM, ledState);
            AddLogMessageI(F("Led changed to default On\n"));
        } else {
            if (warn) AddLogMessageI(F("Led already On\n"));
        }
    } else {
        if (ledState == ON_LED_STATE) {
            ledState = !ON_LED_STATE;
            digitalWrite(BUTTONLED_GPIO_NUM, ledState);
            AddLogMessageI(F("Led changed to default Off\n"));
        } else {
            if (warn) AddLogMessageI(F("Led already Off\n"));
        }
    }
}

// Check the motion state and process the switch action
void Motion_Check() {
    // Check if motion is activated
    if (digitalRead(MOTION_GPIO_NUM) == MotionActiveState && !MotionProcessActive) {
        MotionProcessActive = true;
        MotionDisable_done = 1;
        Motion_Active("On");
        return;
    }
    // perform and switch Off
    if (MotionProcessActive) {
        if (MotionDisable_done > (MotionDisable / 10)) {
            AddLogMessageD("Motion timer done.\n");
            Motion_Active("Off");
            MotionProcessActive = false;
        } else {
            if ((MotionDisable / 1000) < millis()) {
                Motion_timer = millis();
                MotionDisable_done++;
            }
        }
    }
}

// Function to process when motion is active
void Motion_Active(const char *State) {
    AddLogMessageI("Motion: " + String(State) + "\n");
    if (!strcmp(SendProtocol, "json")) {
        Domoticz_JSON_Switch(MotionIDX, State);
    } else if (!strcmp(SendProtocol, "mqtt")) {
        Domoticz_MQTT_Switch(MotionIDX, State);
    } else {
        AddLogMessageW(F("SendProtocol = \"none\", No command to send\n"));
    }
}

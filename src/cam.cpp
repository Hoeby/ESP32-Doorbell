// Project: ESP32-Doorbell
// Programmers: Jos van der Zande
//              Paul Hermans
//
// Camera module for ESP-CAM
// Partly based on: // https://gist.github.com/me-no-dev/d34fba51a8f059ac559bf62002e61aa3
//
#include <ESPAsyncWiFiManager.h>  // https://github.com/alanswx/ESPAsyncWiFiManager
#include <esp_camera.h>
#include <SPIFFS.h>
#include "ArduinoJson.h"

#include "cam.h"
#include "Main.h"
#include "Webserver.h"
#include "setup.h"
#include "_ESP_Board_Settings.h"

// Define private used funcs
bool GetJsonField_UpdateCam(sensor_t *s, DynamicJsonDocument doc, char *variable);
void ESP_Standard_Settings();

typedef struct
{
    camera_fb_t *fb;
    size_t index;
} camera_frame_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART = "Content-Type: %s\r\nContent-Length: %u\r\n\r\n";

static const char *JPG_CONTENT_TYPE = "image/jpeg";
//static const char *BMP_CONTENT_TYPE = "image/x-windows-bmp";

class AsyncBufferResponse : public AsyncAbstractResponse {
   private:
    uint8_t *_buf;
    size_t _len;
    size_t _index;

   public:
    AsyncBufferResponse(uint8_t *buf, size_t len, const char *contentType) {
        _buf = buf;
        _len = len;
        _callback = nullptr;
        _code = 200;
        _contentLength = _len;
        _contentType = contentType;
        _index = 0;
    }
    ~AsyncBufferResponse() {
        if (_buf != nullptr) {
            free(_buf);
        }
    }
    bool _sourceValid() const { return _buf != nullptr; }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t ret = _content(buf, maxLen, _index);
        if (ret != RESPONSE_TRY_AGAIN) {
            _index += ret;
        }
        return ret;
    }
    size_t _content(uint8_t *buffer, size_t maxLen, size_t index) {
        memcpy(buffer, _buf + index, maxLen);
        if ((index + maxLen) == _len) {
            free(_buf);
            _buf = nullptr;
        }
        return maxLen;
    }
};

class AsyncFrameResponse : public AsyncAbstractResponse {
   private:
    camera_fb_t *fb;
    size_t _index;

   public:
    AsyncFrameResponse(camera_fb_t *frame, const char *contentType) {
        _callback = nullptr;
        _code = 200;
        _contentLength = frame->len;
        _contentType = contentType;
        _index = 0;
        fb = frame;
    }
    ~AsyncFrameResponse() {
        if (fb != nullptr) {
            esp_camera_fb_return(fb);
        }
    }
    bool _sourceValid() const { return fb != nullptr; }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t ret = _content(buf, maxLen, _index);
        if (ret != RESPONSE_TRY_AGAIN) {
            _index += ret;
        }
        return ret;
    }
    size_t _content(uint8_t *buffer, size_t maxLen, size_t index) {
        memcpy(buffer, fb->buf + index, maxLen);
        if ((index + maxLen) == fb->len) {
            esp_camera_fb_return(fb);
            fb = nullptr;
        }
        return maxLen;
    }
};

class AsyncJpegStreamResponse : public AsyncAbstractResponse {
   private:
    camera_frame_t _frame;
    size_t _index;
    size_t _jpg_buf_len;
    uint8_t *_jpg_buf;
    long lastAsyncRequest;

   public:
    AsyncJpegStreamResponse() {
        _callback = nullptr;
        _code = 200;
        _contentLength = 0;
        _contentType = STREAM_CONTENT_TYPE;
        _sendContentLength = false;
        _chunked = true;
        _index = 0;
        _jpg_buf_len = 0;
        _jpg_buf = NULL;
        lastAsyncRequest = 0;
        memset(&_frame, 0, sizeof(camera_frame_t));
    }
    ~AsyncJpegStreamResponse() {
        if (_frame.fb) {
            if (_frame.fb->format != PIXFORMAT_JPEG) {
                free(_jpg_buf);
            }
            esp_camera_fb_return(_frame.fb);
        }
    }
    bool _sourceValid() const {
        return true;
    }
    virtual size_t _fillBuffer(uint8_t *buf, size_t maxLen) override {
        size_t ret = _content(buf, maxLen, _index);
        if (ret != RESPONSE_TRY_AGAIN) {
            _index += ret;
        }
        return ret;
    }
    size_t _content(uint8_t *buffer, size_t maxLen, size_t index) {
        if (!_frame.fb || _frame.index == _jpg_buf_len) {
            if (index && _frame.fb) {
                long end = millis();
                int fp = (end - lastAsyncRequest);
                log_d("Size: %uKB, Time: %ums (%ifps)\n", _jpg_buf_len / 1024, fp, 1000 / fp);
                lastAsyncRequest = end;
                if (_frame.fb->format != PIXFORMAT_JPEG) {
                    free(_jpg_buf);
                }
                esp_camera_fb_return(_frame.fb);
                _frame.fb = NULL;
                _jpg_buf_len = 0;
                _jpg_buf = NULL;
            }
            if (maxLen < (strlen(STREAM_BOUNDARY) + strlen(STREAM_PART) + strlen(JPG_CONTENT_TYPE) + 8)) {
                //log_w("Not enough space for headers");
                return RESPONSE_TRY_AGAIN;
            }
            //get frame
            _frame.index = 0;

            _frame.fb = esp_camera_fb_get();
            if (_frame.fb == NULL) {
                log_e("Camera frame failed");
                return 0;
            }

            if (_frame.fb->format != PIXFORMAT_JPEG) {
                unsigned long st = millis();
                bool jpeg_converted = frame2jpg(_frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
                if (!jpeg_converted) {
                    log_e("JPEG compression failed");
                    esp_camera_fb_return(_frame.fb);
                    _frame.fb = NULL;
                    _jpg_buf_len = 0;
                    _jpg_buf = NULL;
                    return 0;
                }
                log_i("JPEG: %lums, %uB", millis() - st, _jpg_buf_len);
            } else {
                _jpg_buf_len = _frame.fb->len;
                _jpg_buf = _frame.fb->buf;
            }

            //send boundary
            size_t blen = 0;
            if (index) {
                blen = strlen(STREAM_BOUNDARY);
                memcpy(buffer, STREAM_BOUNDARY, blen);
                buffer += blen;
            }
            //send header
            size_t hlen = sprintf((char *)buffer, STREAM_PART, JPG_CONTENT_TYPE, _jpg_buf_len);
            buffer += hlen;
            //send frame
            hlen = maxLen - hlen - blen;
            if (hlen > _jpg_buf_len) {
                maxLen -= hlen - _jpg_buf_len;
                hlen = _jpg_buf_len;
            }
            memcpy(buffer, _jpg_buf, hlen);
            _frame.index += hlen;
            return maxLen;
        }

        size_t available = _jpg_buf_len - _frame.index;
        if (maxLen > available) {
            maxLen = available;
        }
        memcpy(buffer, _jpg_buf + _frame.index, maxLen);
        _frame.index += maxLen;

        return maxLen;
    }
};

// ----------------------------------------------------------------------------------------------------------------
// ------  Camera Webfunctions ------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

void sendJpg(AsyncWebServerRequest *request) {
    if (!_webAuth(request))
        return;
    if (strcmp(esp_board, "none") == 0) {
        AddLogMessageE(F("Webrequest: \"/capture\" -> No ESP Board type selected yet.\n"));
        String s = F("No ESP Board type selected yet.<script>setTimeout(function() {window.parent.location.href= \"/\";s}, 3000);</script>");
        request->send(200, "text/html", s);
        return;
    }
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        AddLogMessageE(F("Webrequest: \"/capture\" -> Camera not Detected.\n"));
        String s = F("Camera not Detected.<script>setTimeout(function() {window.parent.location.href= \"/\";s}, 3000);</script>");
        request->send(200, "text/html", s);
        return;
    }
    AddLogMessageI(F("Start JPG Capture\n"));
    if (fb->format == PIXFORMAT_JPEG) {
        AsyncFrameResponse *response = new AsyncFrameResponse(fb, JPG_CONTENT_TYPE);
        if (response == NULL) {
            log_e("Response alloc failed");
            request->send(501);
            return;
        }
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
        return;
    }

    size_t jpg_buf_len = 0;
    uint8_t *jpg_buf = NULL;
    unsigned long st = millis();
    bool jpeg_converted = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
    esp_camera_fb_return(fb);
    if (!jpeg_converted) {
        log_e("JPEG compression failed: %lu", millis());
        request->send(501);
        return;
    }
    log_i("JPEG: %lums, %uB", millis() - st, jpg_buf_len);

    AsyncBufferResponse *response = new AsyncBufferResponse(jpg_buf, jpg_buf_len, JPG_CONTENT_TYPE);
    if (response == NULL) {
        log_e("Response alloc failed");
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void streamJpg(AsyncWebServerRequest *request) {
    if (!_webAuth(request))
        return;
    if (strcmp(esp_board, "none") == 0) {
        AddLogMessageE(F("Webrequest: \"/stream\" -> No ESP Board type selected yet.\n"));
        String s = F("No ESP Board type selected yet.<script>setTimeout(function() {window.parent.location.href= \"/\";s}, 3000);</script>");
        request->send(200, "text/html", s);
        return;
    }
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        AddLogMessageE(F("Webrequest: \"/stream\" -> Camera not Detected.\n"));
        String s = F("Camera not Detected.<script>setTimeout(function() {window.parent.location.href= \"/\";s}, 3000);</script>");
        request->send(200, "text/html", s);
        return;
    }
    AddLogMessageI(F("Start JPG streaming\n"));
    if (strcmp(esp_board, "none") == 0) {
        AddLogMessageE(F("No ESP Board type selected yet"));
        request->send(501);
        return;
    }
    AsyncJpegStreamResponse *response = new AsyncJpegStreamResponse();
    if (!response) {
        request->send(501);
        return;
    }
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

// ----------------------------------------------------------------------------------------------------------------
// ------  Camera other functions  ------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------------

// Initialise cameras
void initcamera() {
    // No ESP Board type selected yet.
    if (strcmp(esp_board, "none") == 0)
        return;
    //Set Camera config - from camera_pins.h
    ESP_Standard_Settings();
    //
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    //Setup size of image
    if (psramFound()) {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_VGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        AddLogMessageE(F("Camera init failed!\n"));

        return;
    }

    // restore the saved settings from SPIFFS
    Restore_CamSettings_from_SPIFFS();

    AddLogMessageI(F("Camera initialised!\n"));
}

// Update the settings in the CAM when changed
bool GetJsonField_UpdateCam(sensor_t *s, DynamicJsonDocument doc, char *variable) {
    int ncharvalue = doc[variable];
    //if (!ncharvalue)
    if (!doc.containsKey(variable)) {
        AddLogMessageW("!! Config key " + String(variable) + " doesn't exist in ESP_CAM_CONFIG.json!!\n");
        return false;
    }
    int nvalue = ncharvalue;
    log_d(" -> key  %s   new=%i", variable, (int)nvalue);

    // Update requested parameter
    if (!strcmp(variable, "framesize"))
        s->set_framesize(s, (framesize_t)nvalue);
    else if (!strcmp(variable, "quality"))
        s->set_quality(s, nvalue);
    else if (!strcmp(variable, "contrast"))
        s->set_contrast(s, nvalue);
    else if (!strcmp(variable, "brightness"))
        s->set_brightness(s, nvalue);
    else if (!strcmp(variable, "saturation"))
        s->set_saturation(s, nvalue);
    else if (!strcmp(variable, "sharpness"))
        s->set_sharpness(s, nvalue);
    else if (!strcmp(variable, "gainceiling"))
        s->set_gainceiling(s, (gainceiling_t)nvalue);
    else if (!strcmp(variable, "colorbar"))
        s->set_colorbar(s, nvalue);
    else if (!strcmp(variable, "awb"))
        s->set_whitebal(s, nvalue);
    else if (!strcmp(variable, "agc"))
        s->set_gain_ctrl(s, nvalue);
    else if (!strcmp(variable, "aec"))
        s->set_exposure_ctrl(s, nvalue);
    else if (!strcmp(variable, "hmirror"))
        s->set_hmirror(s, nvalue);
    else if (!strcmp(variable, "vflip"))
        s->set_vflip(s, nvalue);
    else if (!strcmp(variable, "awb_gain"))
        s->set_awb_gain(s, nvalue);
    else if (!strcmp(variable, "agc_gain"))
        s->set_agc_gain(s, nvalue);
    else if (!strcmp(variable, "aec_value"))
        s->set_aec_value(s, nvalue);
    else if (!strcmp(variable, "aec2"))
        s->set_aec2(s, nvalue);
    else if (!strcmp(variable, "denoise"))
        s->set_denoise(s, nvalue);
    else if (!strcmp(variable, "dcw"))
        s->set_dcw(s, nvalue);
    else if (!strcmp(variable, "bpc"))
        s->set_bpc(s, nvalue);
    else if (!strcmp(variable, "wpc"))
        s->set_wpc(s, nvalue);
    else if (!strcmp(variable, "raw_gma"))
        s->set_raw_gma(s, nvalue);
    else if (!strcmp(variable, "lenc"))
        s->set_lenc(s, nvalue);
    else if (!strcmp(variable, "special_effect"))
        s->set_special_effect(s, nvalue);
    else if (!strcmp(variable, "wb_mode"))
        s->set_wb_mode(s, nvalue);
    else if (!strcmp(variable, "ae_level"))
        s->set_ae_level(s, nvalue);
    else {
        AddLogMessageW("skipping unknown setting:" + String(variable) + "\n");
        return false;
    }

    return true;
}

// Restore Camera settings From SPIFFS
bool Restore_CamSettings_from_SPIFFS() {
    // SPIFFS
    File file = SPIFFS.open("/ESP_CAM_SETTINGS.json", "r");
    if (!file || file.isDirectory()) {
        log_e("- empty file or failed to open file");
        return false;
    }
    static char json_response[1024];
    char *p = json_response;
    while (file.available()) {
        *p++ = file.read();
        if (*p > 1022) {
            log_e("---ERROR: file larger than 1022 char so assume the config is corrupt.");
            json_response[0] = '\0';  // reset content
            file.close();
            break;
        }
    }
    if (Set_Cam_Settings_from_JSON(json_response)) {
        AddLogMessageI(F("Camera settings loaded from SPIFFS\n"));
        return false;
    } else {
        AddLogMessageE(F("Camera settings load failed!\n"));
    }
    return true;
}

// Read updated values from camsetup.htm webpage and set the Camera settings
void Save_NewCAMConfig_to_SPIFFS(AsyncWebServerRequest *request) {
    static char json_response[1024];
    char *p = json_response;
    *p++ = '{';
    p += sprintf(p, "\"framesize\":%u,", atoi(request->arg("framesize").c_str()));
    p += sprintf(p, "\"quality\":%u,", atoi(request->arg("quality").c_str()));
    p += sprintf(p, "\"brightness\":%d,", atoi(request->arg("brightness").c_str()));
    p += sprintf(p, "\"contrast\":%d,", atoi(request->arg("contrast").c_str()));
    p += sprintf(p, "\"saturation\":%d,", atoi(request->arg("saturation").c_str()));
    //p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", atoi(request->arg("special_effect").c_str()));
    p += sprintf(p, "\"wb_mode\":%u,", atoi(request->arg("wb_mode").c_str()));
    p += sprintf(p, "\"awb\":%u,", atoi(request->arg("awb").c_str()));
    p += sprintf(p, "\"awb_gain\":%u,", atoi(request->arg("awb_gain").c_str()));
    p += sprintf(p, "\"aec\":%u,", atoi(request->arg("aec").c_str()));
    p += sprintf(p, "\"aec2\":%u,", atoi(request->arg("aec2").c_str()));
    //p += sprintf(p, "\"denoise\":%u,", s->status.denoise);
    p += sprintf(p, "\"ae_level\":%d,", atoi(request->arg("aec2").c_str()));
    p += sprintf(p, "\"aec_value\":%u,", atoi(request->arg("aec_value").c_str()));
    p += sprintf(p, "\"agc\":%u,", atoi(request->arg("agc").c_str()));
    p += sprintf(p, "\"agc_gain\":%u,", atoi(request->arg("agc_gain").c_str()));
    p += sprintf(p, "\"gainceiling\":%u,", atoi(request->arg("gainceiling").c_str()));
    p += sprintf(p, "\"bpc\":%u,", atoi(request->arg("bpc").c_str()));
    p += sprintf(p, "\"wpc\":%u,", atoi(request->arg("wpc").c_str()));
    //p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", atoi(request->arg("lenc").c_str()));
    p += sprintf(p, "\"hmirror\":%u,", atoi(request->arg("hmirror").c_str()));
    p += sprintf(p, "\"vflip\":%u,", atoi(request->arg("vflip").c_str()));
    p += sprintf(p, "\"dcw\":%u,", atoi(request->arg("dcw").c_str()));
    p += sprintf(p, "\"colorbar\":%u", atoi(request->arg("colorbar").c_str()));
    *p++ = '}';
    *p++ = 0;

    File file = SPIFFS.open("/ESP_CAM_SETTINGS.json", "w");
    String msg = F("Saving Camera Settings to SPIFF, ");
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

// Update Camera settings from JSON input
bool Set_Cam_Settings_from_JSON(char *JSONCamSetting) {
    String fileContent = JSONCamSetting;
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, fileContent);
    if (error) {
        AddLogMessageE(F("Config not set: JSON Parsing failed\n"));
        return false;
    }
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        AddLogMessageE(F("Config not set: Camera init failed\n"));
        return false;
    } else {
        //UINT
        GetJsonField_UpdateCam(s, doc, (char *)"framesize");
        GetJsonField_UpdateCam(s, doc, (char *)"quality");
        GetJsonField_UpdateCam(s, doc, (char *)"special_effect");
        GetJsonField_UpdateCam(s, doc, (char *)"wb_mode");
        GetJsonField_UpdateCam(s, doc, (char *)"awb");
        GetJsonField_UpdateCam(s, doc, (char *)"awb_gain");
        GetJsonField_UpdateCam(s, doc, (char *)"aec");
        GetJsonField_UpdateCam(s, doc, (char *)"aec2");
        //GetJsonField_UpdateCam(s, doc, (char *)"denoise");
        GetJsonField_UpdateCam(s, doc, (char *)"aec_value");
        GetJsonField_UpdateCam(s, doc, (char *)"agc");
        GetJsonField_UpdateCam(s, doc, (char *)"agc_gain");
        GetJsonField_UpdateCam(s, doc, (char *)"gainceiling");
        GetJsonField_UpdateCam(s, doc, (char *)"bpc");
        GetJsonField_UpdateCam(s, doc, (char *)"wpc");
        //GetJsonField_UpdateCam(s, doc, (char *)"raw_gma");
        GetJsonField_UpdateCam(s, doc, (char *)"lenc");
        GetJsonField_UpdateCam(s, doc, (char *)"hmirror");
        GetJsonField_UpdateCam(s, doc, (char *)"vflip");
        GetJsonField_UpdateCam(s, doc, (char *)"dcw");
        GetJsonField_UpdateCam(s, doc, (char *)"colorbar");
        //Double
        GetJsonField_UpdateCam(s, doc, (char *)"brightness");
        GetJsonField_UpdateCam(s, doc, (char *)"contrast");
        GetJsonField_UpdateCam(s, doc, (char *)"saturation");
        //GetJsonField_UpdateCam(s, doc, (char *)"sharpness");
        GetJsonField_UpdateCam(s, doc, (char *)"ae_level");
    }

    return true;
}

// Read Current CAM settings from the Camera itself.
char *GetCurrentCamSettings() {
    static char json_response[1024];
    char *p = json_response;
    *p++ = '{';
    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL) {
        AddLogMessageW(F("Could get current setting: Camera init failed\n"));
        p += sprintf(p, "\"Error\":\"Camera failed\"");
    } else {
        p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
        p += sprintf(p, "\"quality\":%u,", s->status.quality);
        p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
        p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
        p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
        p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
        p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
        p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
        p += sprintf(p, "\"awb\":%u,", s->status.awb);
        p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
        p += sprintf(p, "\"aec\":%u,", s->status.aec);
        p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
        p += sprintf(p, "\"denoise\":%u,", s->status.denoise);
        p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
        p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
        p += sprintf(p, "\"agc\":%u,", s->status.agc);
        p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
        p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
        p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
        p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
        p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
        p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
        p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
        p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
        p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
        p += sprintf(p, "\"colorbar\":%u", s->status.colorbar);
    }
    *p++ = '}';
    *p++ = 0;
    return json_response;
}

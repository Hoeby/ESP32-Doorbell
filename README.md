# ESPCAM V2-Dev

# Index:
- <a href="assets/readme_first_file_upload/README_First_file_upload.md">First time; How to upload files to ESP</a>
- <a href="assets/readme_first_wifi_setup/README_First_wifi_setup.md">First time; First time wifi setup</a>
- Camera setup (has to be made)
- MQTT setup (has to be made)
- HTTP setup (has to be made)
- Upload new firmware (has to be made)
- Download config (has to be made)
- Download logfile (has to be made)


# RoadMap ESP Doorbell

Project CAM logic is based on:  <https://gist.github.com/me-no-dev/d34fba51a8f059ac559bf62002e61aa3>

- Added Wifi support and WifiManager -> <https://github.com/alanswx/ESPAsyncWiFiManager>
- Added Webpages: Home page; Camera options; Setup dummy en OTA bin update
- Added Camera support
- Added SSDP to "see" the ESP device in Windows/Network and can double click it to show the HomePage.
- Added basics for all variables to start building the domoticz functionality and make the Web Setup page
- Added Loginpage. default defined in esp_uname & esp_pass.
- Setup WebPage and store info to SPIFFS done
- Domoticz integration, with json/mqtt (none secure or secure).
- Static/DHCP network settings.
- Json/mqtt incomming-command, to activate a script part.
- Multi-board builds, which uses fixed camera, gpio-in, gpio-out settings
- Upload/download config.
- LED Flashcounter and Time.
- Both serial logging in Webbrowser and SPIFFS download.
- On wifi loss LED will flash in a certain way
- Info page which shows, nothing can be changed here:
  - Device name
  - Device dhcp/static
  - wifi credentials
  - network credentials
  - build number
  - maybe settings, to have everything on 1 page
- Camera variable:
  - Framesize (s, SVGA)                // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  - brightness(s, 0);                  // -2 to 2
  - contrast(s, 0);                    // -2 to 2
  - saturation(s, 0);                  // -2 to 2
  - special_effect(s, 0);              // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  - whitebal(s, 1);                    // 0 = disable , 1 = enable
  - awb_gain(s, 1);                    // 0 = disable , 1 = enable
  - wb_mode(s, 0);                     // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  - exposure_ctrl(s, 1);               // 0 = disable , 1 = enable
  - aec2(s, 0);                        // 0 = disable , 1 = enable
  - ae_level(s, 0);                    // -2 to 2
  - aec_value(s, 300);                 // 0 to 1200
  - gain_ctrl(s, 1);                   // 0 = disable , 1 = enable
  - agc_gain(s, 0);                    // 0 to 30
  - gainceiling(s, (gainceiling_t)0);  // 0 to 6
  - bpc(s, 0);                         // 0 = disable , 1 = enable
  - wpc(s, 1);                         // 0 = disable , 1 = enable
  - raw_gma(s, 1);                     // 0 = disable , 1 = enable
  - lenc(s, 1);                        // 0 = disable , 1 = enable
  - hmirror(s, 0);                     // 0 = disable , 1 = enable
  - vflip(s, 0);                       // 0 = disable , 1 = enable
  - dcw(s, 1);                         // 0 = disable , 1 = enable
  - colorbar(s, 0);                    // 0 = disable , 1 = enable
- Added rotation to setup options.

TBD:

- Onboard Wiki/Help pages
- GitHub Wiki pages for:
  - Installation
  - Setup

Optional:

- Motion detection
- Telegram, sending images when button pushed
- cliënt ip filtering

Excluded:

- Face detection. This is in conflict with AVG.

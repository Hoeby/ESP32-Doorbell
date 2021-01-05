// Set camera options
#include "Main.h"
// Define your settings for the specified ESP Board
void ESP_Standard_Settings() {
    if (strcmp(esp_board, "WROVER_KIT") == 0) {
        PWDN_GPIO_NUM = -1;
        RESET_GPIO_NUM = -1;
        XCLK_GPIO_NUM = 21;
        SIOD_GPIO_NUM = 26;
        SIOC_GPIO_NUM = 27;
        Y9_GPIO_NUM = 35;
        Y8_GPIO_NUM = 34;
        Y7_GPIO_NUM = 39;
        Y6_GPIO_NUM = 36;
        Y5_GPIO_NUM = 19;
        Y4_GPIO_NUM = 18;
        Y3_GPIO_NUM = 5;
        Y2_GPIO_NUM = 4;
        VSYNC_GPIO_NUM = 25;
        HREF_GPIO_NUM = 23;
        PCLK_GPIO_NUM = 22;
        BUTTON_GPIO_NUM = 12;
        BUTTONLED_GPIO_NUM = 13;
        ON_LED_STATE = HIGH;

    } else if (strcmp(esp_board, "ESP_EYE") == 0) {
        PWDN_GPIO_NUM = -1;
        RESET_GPIO_NUM = -1;
        XCLK_GPIO_NUM = 4;
        SIOD_GPIO_NUM = 18;
        SIOC_GPIO_NUM = 23;
        Y9_GPIO_NUM = 36;
        Y8_GPIO_NUM = 37;
        Y7_GPIO_NUM = 38;
        Y6_GPIO_NUM = 39;
        Y5_GPIO_NUM = 35;
        Y4_GPIO_NUM = 14;
        Y3_GPIO_NUM = 13;
        Y2_GPIO_NUM = 34;
        VSYNC_GPIO_NUM = 5;
        HREF_GPIO_NUM = 27;
        PCLK_GPIO_NUM = 25;
        BUTTON_GPIO_NUM = 15;
        BUTTONLED_GPIO_NUM = 21;
        ON_LED_STATE = HIGH;

    } else if (strcmp(esp_board, "M5STACK_PSRAM") == 0) {
        PWDN_GPIO_NUM = -1;
        RESET_GPIO_NUM = 15;
        XCLK_GPIO_NUM = 27;
        SIOD_GPIO_NUM = 25;
        SIOC_GPIO_NUM = 23;
        Y9_GPIO_NUM = 19;
        Y8_GPIO_NUM = 36;
        Y7_GPIO_NUM = 18;
        Y6_GPIO_NUM = 39;
        Y5_GPIO_NUM = 5;
        Y4_GPIO_NUM = 34;
        Y3_GPIO_NUM = 35;
        Y2_GPIO_NUM = 32;
        VSYNC_GPIO_NUM = 22;
        HREF_GPIO_NUM = 26;
        PCLK_GPIO_NUM = 21;
        BUTTON_GPIO_NUM = 12;
        BUTTONLED_GPIO_NUM = 13;
        ON_LED_STATE = HIGH;

    } else if (strcmp(esp_board, "M5STACK_WIDE") == 0) {
        PWDN_GPIO_NUM = -1;
        RESET_GPIO_NUM = 15;
        XCLK_GPIO_NUM = 27;
        SIOD_GPIO_NUM = 22;
        SIOC_GPIO_NUM = 23;
        Y9_GPIO_NUM = 19;
        Y8_GPIO_NUM = 36;
        Y7_GPIO_NUM = 18;
        Y6_GPIO_NUM = 39;
        Y5_GPIO_NUM = 5;
        Y4_GPIO_NUM = 34;
        Y3_GPIO_NUM = 35;
        Y2_GPIO_NUM = 32;
        VSYNC_GPIO_NUM = 25;
        HREF_GPIO_NUM = 26;
        PCLK_GPIO_NUM = 21;
        BUTTON_GPIO_NUM = 12;
        BUTTONLED_GPIO_NUM = 13;
        ON_LED_STATE = HIGH;
        
    } else if (strcmp(esp_board, "AI_THINKER") == 0) {
        PWDN_GPIO_NUM = 32;
        RESET_GPIO_NUM = -1;
        XCLK_GPIO_NUM = 0;
        SIOD_GPIO_NUM = 26;
        SIOC_GPIO_NUM = 27;
        Y9_GPIO_NUM = 35;
        Y8_GPIO_NUM = 34;
        Y7_GPIO_NUM = 39;
        Y6_GPIO_NUM = 36;
        Y5_GPIO_NUM = 21;
        Y4_GPIO_NUM = 19;
        Y3_GPIO_NUM = 18;
        Y2_GPIO_NUM = 5;
        VSYNC_GPIO_NUM = 25;
        HREF_GPIO_NUM = 23;
        PCLK_GPIO_NUM = 22;
        BUTTON_GPIO_NUM = 12;
        BUTTONLED_GPIO_NUM = 13;
        ON_LED_STATE = HIGH;
    }
}
#include "neo_blinky.h"

extern float glob_temperature;
extern float glob_humidity;
void neo_blinky(void *pvParameters){
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();

    while(1) {                          
        uint32_t current_color;
        int delay_time;

        if (glob_temperature > 35.0 && glob_humidity > 80.0) {
            current_color = strip.Color(255, 0, 0); 
            delay_time = 200;
        }
        else if (glob_temperature > 35.0 || glob_humidity > 80.0) {
            current_color = strip.Color(255, 255, 0); 
            delay_time = 500;
        }
        else {
            current_color = strip.Color(0, 255, 255); 
            delay_time = 1000;
        }

        strip.setPixelColor(0, current_color);
        strip.show();
        vTaskDelay(delay_time);

        strip.setPixelColor(0, strip.Color(0, 0, 0));
        strip.show();
        vTaskDelay(delay_time);
    }
}
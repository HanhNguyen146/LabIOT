#include "led_blinky.h"
extern float glob_temperature;
extern float glob_humidity;
void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);
  
  while(1) {                        
    if (glob_temperature > 35.0 || glob_humidity > 80.0) {
      digitalWrite(LED_GPIO, HIGH);
      vTaskDelay(100);
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(100);
    } 
    else {
      digitalWrite(LED_GPIO, HIGH);
      vTaskDelay(1000);
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(1000);
    }
  }
}
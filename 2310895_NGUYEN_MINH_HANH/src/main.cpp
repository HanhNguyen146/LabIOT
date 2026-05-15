#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
// #include "mainserver.h"
// #include "tinyml.h"
#include "coreiot.h"
// #include "mainserver.h"
// include task
// #include "task_check_info.h"
// #include "task_toogle_boot.h"
// #include "task_wifi.h"
// #include "task_webserver.h"
// #include "task_core_iot.h"

void setup()
{
  Serial.begin(115200);
  //check_info_File(0);

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 2048, NULL, 2, NULL);
  // xTaskCreate(main_server_task, "Task Main Server" ,8192  ,NULL  ,2 , NULL);
  // xTaskCreate( tiny_ml_task, "Tiny ML Task" ,2048  ,NULL  ,2 , NULL);
  //xTaskCreate(coreiot_init, "CoreIOT Task" ,4096  ,NULL  ,2 , NULL);
  coreiot_init();
  // xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
  xTaskCreate(task_wifi_check,      "WiFi_Check",      4096, NULL, 3, NULL);
  
  // Priority 2 — MQTT connect & loop cùng mức
  xTaskCreate(task_mqtt_connect,    "MQTT_Connect",    4096, NULL, 2, NULL);
  xTaskCreate(task_mqtt_loop,       "MQTT_Loop",       4096, NULL, 2, NULL);
  
  // Priority 1 — Gửi data sau khi đã có kết nối
  xTaskCreate(task_send_telemetry,  "Send_Telemetry",  4096, NULL, 1, NULL);
  xTaskCreate(task_send_attributes, "Send_Attributes", 4096, NULL, 1, NULL);
}

void loop()
{
  // if (check_info_File(1))
  // {
  //   if (!Wifi_reconnect())
  //   {
  //     Webserver_stop();
  //   }
  //   else
  //   {
  //     //CORE_IOT_reconnect();
  //   }
  // }
  // Webserver_reconnect();
}
#include <Arduino.h>
#include <WiFi.h>
#include "glob.h"
extern String wifi_ssid;
extern String wifi_password;
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("[WiFi] Đã kết nối tới Access Point.");
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.printf("[WiFi] Thành công! Địa chỉ IP: %s\n", WiFi.localIP().toString().c_str());
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("[WiFi] Mất kết nối! Đang tự động kết nối lại...");
      WiFi.reconnect();
      break;

    default:
      break;
  }
}

// 3. Hàm thiết lập chế độ và bắt đầu kết nối
void connectToWiFi() {
  // Ngắt kết nối cũ để làm sạch bộ nhớ mạng
  WiFi.disconnect(true);
  delay(100);

  // Chuyển sang chế độ Trạm (Station) để bắt WiFi
  WiFi.mode(WIFI_STA);

  // Gửi yêu cầu kết nối
  if (wifi_password.isEmpty()) {
    WiFi.begin(wifi_ssid.c_str());
  } else {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  }

  Serial.print("Đang khởi động kết nối tới: ");
  Serial.println(wifi_ssid);
}

// 4. Task chạy ngầm để quản lý WiFi
void wifi_connect_task(void *pvParameters) {
  // Đăng ký bộ lắng nghe sự kiện
  WiFi.onEvent(WiFiEvent);

  // Gọi hàm kết nối
  connectToWiFi();

  // Vòng lặp vô hạn của Task
  while (1) {
    // Nếu chưa kết nối được thì in ra dấu chấm cho dễ nhìn
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
    }
    
    // Tạm dừng task 1 giây (1000ms) để nhường CPU cho các task khác
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}
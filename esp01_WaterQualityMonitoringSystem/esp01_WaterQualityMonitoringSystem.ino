#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "index.h"
#include "wifi_config.h"

#define EEPROM_SIZE 96

bool uartActive = false;
unsigned long lastUartTime = 0;


// Wi-Fi mặc định trong code
const char* hardcoded_ssid = "American Study //HD";
const char* hardcoded_pass = "66668888";

// Thông tin AP mặc định khi không kết nối được Wi-Fi đã lưu
const char* my_default_ssid = "ESP01_AQM";
const char* my_default_pass = "12345678";

// IP tĩnh khi chạy ở chế độ AP
IPAddress local_IP(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

ESP8266WebServer server(80);

float tempValue = 0, phValue = 0, tdsValue = 0, turbValue = 0, doValue = 0;
bool dataReceived = false;
unsigned long lastDataTime = 0;
const unsigned long dataTimeout = 3000;

String storedSSID = "";
String storedPASS = "";

void saveWiFiConfig(String ssid, String pass) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) EEPROM.write(i, (i < ssid.length()) ? ssid[i] : 0);
  EEPROM.write(31, 0); // đảm bảo chuỗi kết thúc
  for (int i = 0; i < 64; i++) EEPROM.write(32 + i, (i < pass.length()) ? pass[i] : 0);
  EEPROM.write(95, 0); // đảm bảo chuỗi kết thúc
  EEPROM.commit();
}


void loadWiFiConfig() {
  EEPROM.begin(EEPROM_SIZE);
  char ssid[33], pass[65];
  for (int i = 0; i < 32; i++) ssid[i] = EEPROM.read(i);
  for (int i = 0; i < 64; i++) pass[i] = EEPROM.read(32 + i);
  ssid[32] = 0;
  pass[64] = 0;
  storedSSID = String(ssid);
  storedPASS = String(pass);
}

bool connectWiFi(String ssid, String pass) {
  WiFi.disconnect(true);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Lỗi cấu hình IP tĩnh");
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("Đang kết nối tới Wi-Fi: %s\n", ssid.c_str());

  unsigned long start = millis();
  while (millis() - start < 20000) {
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      Serial.printf("Kết nối thành công! IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nKết nối thất bại hoặc IP không hợp lệ.");
  return false;
}

void startAPConfig() {
  // IP cho chế độ AP → phải cùng lớp mạng 192.168.4.x
  IPAddress apIP(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  WiFi.softAP(my_default_ssid, my_default_pass);

  Serial.printf("Chế độ AP Config. Kết nối tới SSID: %s, Pass: %s\n", 
                my_default_ssid, my_default_pass);
  Serial.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());

    // Trang config Wi-Fi
  server.on("/", []() {
    server.send(200, "text/html", configPage);
  });

  // API quét Wi-Fi
  server.on("/scanwifi", []() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i) json += ",";
      json += "\"" + WiFi.SSID(i) + "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
  });

  // API lưu Wi-Fi
  server.on("/savewifi", []() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
      saveWiFiConfig(server.arg("ssid"), server.arg("pass"));
      server.send(200, "text/plain", "Lưu thành công. ESP sẽ khởi động lại.");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/plain", "Thiếu tham số");
    }
  });
}


void setup() {
  Serial.begin(9600);
  loadWiFiConfig();

  bool connected = false;

  // 1. Thử Wi-Fi lưu trong EEPROM
  if (storedSSID.length() > 0) {
    connected = connectWiFi(storedSSID, storedPASS);
  }

  // 2. Nếu không được, thử Wi-Fi mặc định hard-code
  if (!connected) {
    connected = connectWiFi(hardcoded_ssid, hardcoded_pass);
  }

  // 3. Nếu vẫn không được → phát AP config
  if (!connected) {
    startAPConfig();
  }

  // API realtime
  server.on("/data", []() {
    String json = "{";
    json += "\"ph\":" + String(phValue, 2) + ",";
    json += "\"tds\":" + String(tdsValue, 0) + ",";
    json += "\"temp\":" + String(tempValue, 1) + ",";
    json += "\"turb\":" + String(turbValue, 2) + ",";
    json += "\"do\":" + String(doValue, 2) + ",";
    json += "\"uart\":" + String(uartActive ? 1 : 0);
    json += "}";
    server.send(200, "application/json", json);
  });

    // Trang chính
  server.on("/", []() {
    String html = MAIN_page;
    html.replace("%PH%", String(phValue, 2));
    html.replace("%TDS%", String(tdsValue, 0));
    html.replace("%TEMP%", String(tempValue, 1));
    html.replace("%TURB%", String(turbValue, 2));
    html.replace("%DO%", String(doValue, 2));
    server.send(200, "text/html", html);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  readSerialData();
  if (millis() - lastDataTime > dataTimeout) {
    dataReceived = false;
  }
}

void readSerialData() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    lastUartTime = millis();
uartActive = true;

    line.trim();
    if (line.length() > 0) {
      float values[5] = { 0 };
      int idx = 0;
      int lastComma = 0;
      for (int i = 0; i < line.length(); i++) {
        if (line.charAt(i) == ',' || i == line.length() - 1) {
          String val = line.substring(lastComma, (i == line.length() - 1) ? i + 1 : i);
          val.trim();
          values[idx++] = val.toFloat();
          lastComma = i + 1;
          if (idx >= 5) break;
        }
      }
      if (idx == 5) {
        tempValue = values[0];
        phValue = values[1];
        tdsValue = values[2];
        turbValue = values[3];
        doValue = values[4];
        dataReceived = true;
        lastDataTime = millis();
        Serial.printf("Updated -> Temp: %.2f, pH: %.2f, TDS: %.0f, Turb: %.2f\n, DO: %.2f\n",
                      tempValue, phValue, tdsValue, turbValue, doValue);
      }
    }
  }
  if (millis() - lastUartTime > 5000) {
  uartActive = false;
}

}

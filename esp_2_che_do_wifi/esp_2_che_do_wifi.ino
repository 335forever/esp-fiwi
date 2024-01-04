#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "SPIFFS.h"

/*********************************************************************
  Vùng khởi tạo dữ liệu */

// Tạo AsyncWebServer trên cổng 80
AsyncWebServer server(80);

const char *ap_ssid = "ESP32 của Quân";
const char *ap_password = "12345678";

// Biến lưu dữ liệu từ HTML form
String ssid;
String pass;

// Đường dẫn lưu dữ liệu
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";

/*********************************************************************
  Vùng khởi tạo hàm xử lí */

void initSPIFFS() {
  // Khởi động bộ nhớ SPIFFS
  if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred mounting SPIFFS");
        return;
  }
  Serial.println("SPIFFS ready");
  delay(1000);
}

void readSPIFFS() {
  // Lấy dữ liệu từ SPIFFS
  ssid    = readFile(SPIFFS, ssidPath);
  pass    = readFile(SPIFFS, passPath);
}

bool initWiFi() {
  if (ssid == "") {
    Serial.println("No networks configured");
    delay(1000);
    return false;
  }
  
  Serial.println("Scanning for WiFi networks...");
  int networkCount = WiFi.scanNetworks();
  
  if (networkCount == 0) {
    Serial.println("No WiFi networks found");
    return false;
  }

  for (int i = 0; i < networkCount; ++i) 
    if (strcmp(WiFi.SSID(i).c_str(), ssid.c_str()) == 0) {
      Serial.print("Found ");Serial.println(ssid);
      Serial.print("Attempting to connect");
      WiFi.begin(ssid.c_str(), pass.c_str());

      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
      }
      Serial.println();
    
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected to ");  
        Serial.print(ssid);
        Serial.print(" with IP: ");
        Serial.println(WiFi.localIP());
        delay(1000);
        return true;
      }
      else {
        Serial.println("Connection failed.");
        delay(1000);
        return false;  
      }
    } else Serial.println(WiFi.SSID(i));
  
  Serial.print(ssid);Serial.println(" not found ");
  return false; 
}

void initAP() {
  // Bật chế độ Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Access Point started");
  Serial.print("Connect to " + String(ap_ssid) + " with IP Address: ");
  Serial.println(WiFi.softAPIP());
  delay(1000);  
}

void initSERVER() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/wifimanager.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
  });

  // Xử lí dữ liệu nhận về từ client
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
      ssid = request->arg("ssid");
      pass = request->arg("pass");
      
      saveConfig(ssid, pass);

      request->send(200, "text/plain", "Configuration saved! ESP will restart to connect Wifi");
      delay(3000);
      ESP.restart();
  });

  // Khởi động server
  server.begin();  
}

void saveConfig(String new_ssid, String new_pass) {
    // Mở file để ghi
    File ssidFile = SPIFFS.open(ssidPath, "w+");
    File passFile = SPIFFS.open(passPath, "w+");

    if (!ssidFile || !passFile) {
        Serial.println("Failed to open config files for writing");
        return;
    }

    // Ghi dữ liệu vào file
    ssidFile.println(new_ssid);
    passFile.println(new_pass);

    // Thông báo hoàn tất
    Serial.println("Configuration saved!");

    // Đóng file
    ssidFile.close();
    passFile.close();
    
    delay(1000);
}

String readFile(fs::FS& fs, const char* path) {

  Serial.printf("Reading file: %s\r\n", path);
  delay(1000);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    fileContent.trim();
    break;
  }
  return fileContent;
}

//*********************************************************************

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  delay(3000);
  
  initSPIFFS();
  
  readSPIFFS();
  
  if (initWiFi()) {
  
  } 
  
  else {
    
    initAP();
    initSERVER();
       
  }
  
}

//*********************************************************************
void loop() {
 }

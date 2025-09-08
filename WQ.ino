#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "index1.h" // Your login/dashboard HTML

// -------------------- WiFi & MQTT --------------------
const char* ssid = "BabyAncestoriNteli";
const char* password = "bb2020$!@@";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "waterQuality/data";
const char* mqtt_alert_topic = "waterQuality/alert";

const char* BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* CHAT_ID = "YOUR_CHAT_ID";
const char* logUploadURL = "http://192.168.200.106/water_monitoring/log.php";


// -------------------- Sensor Pins --------------------
#define PH_PIN 34
#define TDS_PIN 35
#define TURBIDITY_PIN 32
#define ONE_WIRE_BUS 4
#define SD_CS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer server(80);

// -------------------- Users --------------------
struct User {
  String username;
  String password;
  String role; // "admin" or "user"
};
User users[10];
int userCount = 0;

// -------------------- Usage Areas & Thresholds --------------------
enum UsageArea { DRINKING=0, AGRICULTURE, INDUSTRIAL, IRRIGATION };
UsageArea selectedArea = DRINKING;

struct Thresholds {
  float ph_min, ph_max;
  float tds_min, tds_max;
  float turbidity_min, turbidity_max;
  float temp_min, temp_max;
};

Thresholds drinking = {6.5, 8.5, 0, 500, 0, 5, 5, 25};
Thresholds agriculture = {6, 9, 0, 450, 0, 10, 15, 30};
Thresholds industrial = {5, 11, 50, 1000, 0, 20, 0, 60};
Thresholds irrigation = {6.5, 8.5, 300, 500, 0, 25, 20, 32};

Thresholds currentThresholds;
Thresholds userThresholds;
bool manualThresholdActive = false;



// -------------------- Sensor Data --------------------
float lastPh=NAN, lastTds=NAN, lastTemp=NAN, lastTurb=NAN;
String lastAlert="";
bool alertActive=false;

// -------------------- Timing --------------------
const unsigned long readInterval = 5000; // 5 sec
unsigned long lastReadTime = 0;
const unsigned long wifiReconnectInterval = 10000;
unsigned long lastWifiCheck = 0;

// -------------------- Function Prototypes --------------------
void setupWiFi();
void reconnectMQTT();
void publishSensorData(float ph, float tds, float turbidity, float temp);
String checkAlerts(float ph, float tds, float turbidity, float temp);
void logToSD(float ph, float tds, float turbidity, float temp);
float readPH();
float readTDS();
float readTurbidity();
float readTemperature();
void setUsageArea(const String& area);
void setupTime();
String getCurrentTimestamp();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void uploadSDLog(float temp, float tds, float turbidity, float ph);
void readAndProcessSensors();
void loadThresholds();
void setManualThresholds(float ph_min, float ph_max, float tds_min, float tds_max,
                         float turbidity_min, float turbidity_max, float temp_min, float temp_max);
void resetToAutoThresholds();
void sendTelegramMessage(const String& message);

// -------------------- Web Handlers --------------------
void handleRoot() {
// server.on("/", []() { server.send_P(200, "text/html", MAIN_page); });
server.send_P(200, "text/html", MAIN_page);
}


// Login endpoint
/*void handleLogin() {
  String user = server.arg("user");
  String pass = server.arg("pass");
  bool success = false;
  String role = "user";
  for(int i=0;i<userCount;i++){
    if(users[i].username==user && users[i].password==pass){
      success=true;
      role = users[i].role;
      break;
    }
  }
  server.send(200,"application/json",
    "{\"success\":" + String(success?"true":"false") + ",\"role\":\""+role+"\"}"
  );
}

// Register endpoint
void handleRegister() {
  String user = server.arg("user");
  String pass = server.arg("pass");
  String role = server.arg("role"); // "admin" or "user"
  bool success = false;
  if(userCount<10){
    users[userCount++] = {user, pass, role};
    success = true;
  }
  server.send(200,"application/json", "{\"success\":" + String(success?"true":"false") + "}");
}
*/
// -------------------- User Login/Register --------------------
void handleLogin() {
  String user = server.arg("user");
  String pass = server.arg("pass");

  if(WiFi.status() != WL_CONNECTED){
    server.send(200, "application/json", "{\"success\":false,\"role\":\"\"}");
    return;
  }

  HTTPClient http;
  String url = "http://192.168.200.106/water_monitoring/users.php"; // fixed URL
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "action=login&user=" + user + "&pass=" + pass;
  int httpCode = http.POST(postData);

  String payload = "{\"success\":false,\"role\":\"\"}";
  if(httpCode > 0){
    payload = http.getString();  // server response
    Serial.println("Login response: " + payload);
  } else {
    Serial.println("❌ Login HTTP failed, code: " + String(httpCode));
  }

  http.end();
  server.send(200, "application/json", payload);
}

void handleRegister() {
  String user = server.arg("user");
  String pass = server.arg("pass");
  String role = server.arg("role"); // "admin" or "user"

  if(WiFi.status() != WL_CONNECTED){
    server.send(200,"application/json","{\"success\":false}");
    return;
  }

  HTTPClient http;
  String url = "http://192.168.200.106/water_monitoring/users.php"; // fixed URL
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "action=register&user=" + user + "&pass=" + pass + "&role=" + role;
  int httpCode = http.POST(postData);

  String payload = "{\"success\":false}";
  if(httpCode > 0){
    payload = http.getString();  // server response
    Serial.println("Register response: " + payload);
  } else {
    Serial.println("❌ Register HTTP failed, code: " + String(httpCode));
  }

  http.end();
  server.send(200, "application/json", payload);
}

// Sensor data endpoint
void handleData(){
  String area_name = 
    selectedArea == DRINKING ? "drinking" :
    selectedArea == AGRICULTURE ? "agriculture" :
    selectedArea == INDUSTRIAL ? "industrial" :
    selectedArea == IRRIGATION ? "irrigation" : "unknown";

  String json="{";
  json += "\"ph\":" + String(isnan(lastPh)?-1:lastPh,2) + ",";
  json += "\"tds\":" + String(isnan(lastTds)?-1:lastTds,1) + ",";
  json += "\"temp\":" + String(isnan(lastTemp)?-1:lastTemp,1) + ",";
  json += "\"turb\":" + String(isnan(lastTurb)?-1:lastTurb,1) + ",";
  json += "\"alert\":\"" + lastAlert + "\",";
  json += "\"mode\":\"" + String(manualThresholdActive?"manual":"auto") + "\",";
  json += "\"area\":" + String((int)selectedArea) + ",";
  json += "\"area_name\":\"" + area_name + "\",";
  json += "\"ph_min\":" + String(currentThresholds.ph_min,2) + ",";
  json += "\"ph_max\":" + String(currentThresholds.ph_max,2) + ",";
  json += "\"tds_min\":" + String(currentThresholds.tds_min,1) + ",";
  json += "\"tds_max\":" + String(currentThresholds.tds_max,1) + ",";
  json += "\"turbidity_min\":" + String(currentThresholds.turbidity_min,1) + ",";
  json += "\"turbidity_max\":" + String(currentThresholds.turbidity_max,1) + ",";
  json += "\"temp_min\":" + String(currentThresholds.temp_min,1) + ",";
  json += "\"temp_max\":" + String(currentThresholds.temp_max,1);
  json += "}";
  server.send(200,"application/json",json);
}
/*
// Set mode endpoint
void handleSetMode() {
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  deserializeJson(doc, body);
  String mode = doc["mode"];
  if(mode=="manual") manualThresholdActive=true;
  else resetToAutoThresholds();
  server.send(200,"application/json","{\"success\":true}");
}

// Update thresholds endpoint
void handleUpdateThresholds() {
  String body = server.arg("plain");
  StaticJsonDocument<300> doc;
  deserializeJson(doc, body);
  JsonObject th = doc["thresholds"];

  setManualThresholds(
    th["ph"][0], th["ph"][1],
    th["tds"][0], th["tds"][1],
    th["turbidity"][0], th["turbidity"][1],
    th["temp"][0], th["temp"][1]
  );
  server.send(200,"application/json","{\"success\":true}");
}

// Set usage area endpoint
void handleSetArea() {
  String area = server.arg("area");
  setUsageArea(area);
  resetToAutoThresholds();
  server.send(200,"application/json","{\"success\":true}");
}
*/
// Set mode (manual or auto)
void handleSetMode() {
  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error || !doc.containsKey("mode")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  String mode = doc["mode"];
  if (mode == "manual") {
    manualThresholdActive = true;
  } else {
    resetToAutoThresholds();
  }

  server.send(200, "application/json", "{\"success\":true}");
}

// Update thresholds (sync with server DB)
void handleUpdateThresholds() {
  String body = server.arg("plain");
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error || !doc.containsKey("thresholds")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonObject th = doc["thresholds"];

  float ph_min   = th["ph"][0];
  float ph_max   = th["ph"][1];
  float tds_min  = th["tds"][0];
  float tds_max  = th["tds"][1];
  float turb_min = th["turbidity"][0];
  float turb_max = th["turbidity"][1];
  float temp_min = th["temp"][0];
  float temp_max = th["temp"][1];

  // Sync with server DB
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://192.168.200.106/water_monitoring/thresholds.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String areaStr = (selectedArea == DRINKING)   ? "drinking" :
                     (selectedArea == AGRICULTURE)? "agriculture" :
                     (selectedArea == INDUSTRIAL) ? "industrial" :
                     (selectedArea == IRRIGATION) ? "irrigation" : "drinking";

    String postData = "area=" + areaStr +
                      "&ph_min=" + String(ph_min) + "&ph_max=" + String(ph_max) +
                      "&tds_min=" + String(tds_min) + "&tds_max=" + String(tds_max) +
                      "&turbidity_min=" + String(turb_min) + "&turbidity_max=" + String(turb_max) +
                      "&temp_min=" + String(temp_min) + "&temp_max=" + String(temp_max);

    int code = http.POST(postData);
    if (code == 200) {
      Serial.println("✅ Thresholds updated on server");
      setManualThresholds(ph_min, ph_max, tds_min, tds_max, turb_min, turb_max, temp_min, temp_max);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      Serial.printf("❌ Server update failed (code %d)\n", code);
      server.send(500, "application/json", "{\"success\":false}");
    }
    http.end();
  } else {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"WiFi not connected\"}");
  }
}

// Set usage area
void handleSetArea() {
  if (!server.hasArg("area")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing area\"}");
    return;
  }

  String area = server.arg("area");
  setUsageArea(area);   // updates selectedArea
  resetToAutoThresholds(); // load defaults or server thresholds

  server.send(200, "application/json", "{\"success\":true}");
}
void setManualThresholds(float ph_min, float ph_max,
                         float tds_min, float tds_max,
                         float turb_min, float turb_max,
                         float temp_min, float temp_max) {
  userThresholds.ph_min        = ph_min;
  userThresholds.ph_max        = ph_max;
  userThresholds.tds_min       = tds_min;
  userThresholds.tds_max       = tds_max;
  userThresholds.turbidity_min = turb_min;
  userThresholds.turbidity_max = turb_max;
  userThresholds.temp_min      = temp_min;
  userThresholds.temp_max      = temp_max;

  // Activate manual mode
  manualThresholdActive = true;
  currentThresholds     = userThresholds;

  Serial.println("✅ Manual thresholds applied");
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  sensors.begin();

  if(!SD.begin(SD_CS)) Serial.println("SD init failed");
  else Serial.println("SD initialized");

  setUsageArea("drinking");
  loadThresholds();
  setupWiFi();
  setupTime();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  reconnectMQTT();
  
  
  // Web routes
  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/register", handleRegister);
  server.on("/data", handleData);
  server.on("/setMode", HTTP_POST, handleSetMode);
  server.on("/updateThresholds", HTTP_POST, handleUpdateThresholds);
  server.on("/setArea", handleSetArea);

  server.begin();
  loadThresholds();
  Serial.println("HTTP server started");
}

// -------------------- Main Loop --------------------
void loop() {
  server.handleClient();

  if(millis() - lastWifiCheck > wifiReconnectInterval){
    lastWifiCheck = millis();
    if(WiFi.status()!=WL_CONNECTED) setupWiFi();
    if(!mqttClient.connected()) reconnectMQTT();
  }
  mqttClient.loop();

  if(millis()-lastReadTime>=readInterval){
    lastReadTime = millis();
    readAndProcessSensors();
  }
/*
  static bool logUploaded = false;
  if(WiFi.status()==WL_CONNECTED && !logUploaded){ uploadSDLog(); logUploaded=true; }
  if(WiFi.status()!=WL_CONNECTED) logUploaded=false;*/
}

// -------------------- Threshold Functions --------------------
/*void loadThresholds() {
  if (!SD.exists("/thresholds.txt")) { manualThresholdActive = false; return; }
  File f = SD.open("/thresholds.txt", FILE_READ);
  if (!f) { manualThresholdActive = false; return; }

  String line = f.readStringUntil('\n');
  f.close();

  float vals[8]; int idx = 0, start = 0;
  for (int i=0;i<line.length();i++){
    if (line[i]==',' || i==line.length()-1){
      String part=line.substring(start,i==line.length()-1?i+1:i);
      vals[idx++]=part.toFloat(); start=i+1;
      if(idx>=8) break;
    }
  }

  userThresholds.ph_min=vals[0];
  userThresholds.ph_max=vals[1];
  userThresholds.tds_min=vals[2];
  userThresholds.tds_max=vals[3];
  userThresholds.turbidity_min=vals[4];
  userThresholds.turbidity_max=vals[5];
  userThresholds.temp_min=vals[6];
  userThresholds.temp_max=vals[7];

  manualThresholdActive=true;
  currentThresholds=userThresholds;
}

void setManualThresholds(float ph_min, float ph_max, float tds_min, float tds_max,
                         float turbidity_min, float turbidity_max, float temp_min, float temp_max) {
  userThresholds.ph_min=ph_min;
  userThresholds.ph_max=ph_max;
  userThresholds.tds_min=tds_min;
  userThresholds.tds_max=tds_max;
  userThresholds.turbidity_min=turbidity_min;
  userThresholds.turbidity_max=turbidity_max;
  userThresholds.temp_min=temp_min;
  userThresholds.temp_max=temp_max;

  manualThresholdActive=true;
  currentThresholds=userThresholds;

  File f=SD.open("/thresholds.txt",FILE_WRITE);
  if(f){
    f.printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", ph_min,ph_max,tds_min,tds_max,turbidity_min,turbidity_max,temp_min,temp_max);
    f.close();
  }
}

void resetToAutoThresholds() {
  manualThresholdActive=false;
  switch(selectedArea){
    case DRINKING: currentThresholds=drinking; break;
    case AGRICULTURE: currentThresholds=agriculture; break;
    case INDUSTRIAL: currentThresholds=industrial; break;
    case IRRIGATION: currentThresholds=irrigation; break;
  }
  if(SD.exists("/thresholds.txt")) SD.remove("/thresholds.txt");


}*/

void loadThresholds() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ No WiFi, reverting to defaults");
    resetToAutoThresholds();
    return;
  }

  String areaStr = (selectedArea == DRINKING)   ? "drinking" :
                   (selectedArea == AGRICULTURE)? "agriculture" :
                   (selectedArea == INDUSTRIAL) ? "industrial" :
                   (selectedArea == IRRIGATION) ? "irrigation" : "drinking";

  HTTPClient http;
  String url = "http://192.168.200.106/water_monitoring/thresholds.php?area=" + areaStr;
  http.begin(url);

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    Serial.println("🔹 Thresholds payload: " + payload);

    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      userThresholds.ph_min   = doc["ph_min"];
      userThresholds.ph_max   = doc["ph_max"];
      userThresholds.tds_min  = doc["tds_min"];
      userThresholds.tds_max  = doc["tds_max"];
      userThresholds.turbidity_min = doc["turbidity_min"];
      userThresholds.turbidity_max = doc["turbidity_max"];
      userThresholds.temp_min = doc["temp_min"];
      userThresholds.temp_max = doc["temp_max"];

      manualThresholdActive = true;
      currentThresholds = userThresholds;

      Serial.println("✅ Thresholds loaded from server");
      http.end();
      return;
    } else {
      Serial.println("❌ JSON parse error: " + String(error.c_str()));
    }
  } else {
    Serial.printf("❌ Failed HTTP GET (code %d)\n", code);
  }

  resetToAutoThresholds();
  http.end();
}
/*void handleUpdateThresholds() {
  String body = server.arg("plain");
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonObject th = doc["thresholds"];

  float ph_min   = th["ph"][0];
  float ph_max   = th["ph"][1];
  float tds_min  = th["tds"][0];
  float tds_max  = th["tds"][1];
  float turb_min = th["turbidity"][0];
  float turb_max = th["turbidity"][1];
  float temp_min = th["temp"][0];
  float temp_max = th["temp"][1];

  String areaStr = (selectedArea == DRINKING)   ? "drinking" :
                   (selectedArea == AGRICULTURE)? "agriculture" :
                   (selectedArea == INDUSTRIAL) ? "industrial" :
                   (selectedArea == IRRIGATION) ? "irrigation" : "drinking";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("http://192.168.200.106/water_monitoring/thresholds.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "area=" + areaStr +
                      "&ph_min=" + String(ph_min) + "&ph_max=" + String(ph_max) +
                      "&tds_min=" + String(tds_min) + "&tds_max=" + String(tds_max) +
                      "&turbidity_min=" + String(turb_min) + "&turbidity_max=" + String(turb_max) +
                      "&temp_min=" + String(temp_min) + "&temp_max=" + String(temp_max);

    int code = http.POST(postData);
    if (code == 200) {
      Serial.println("✅ Thresholds updated on server");
      setManualThresholds(ph_min, ph_max, tds_min, tds_max, turb_min, turb_max, temp_min, temp_max);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      Serial.printf("❌ Server update failed (code %d)\n", code);
      server.send(500, "application/json", "{\"success\":false}");
    }
    http.end();
  } else {
    server.send(500, "application/json", "{\"success\":false,\"error\":\"WiFi not connected\"}");
  }
}*/

void resetToAutoThresholds() {
  manualThresholdActive = false;
  switch (selectedArea) {
    case DRINKING:    currentThresholds = drinking; break;
    case AGRICULTURE: currentThresholds = agriculture; break;
    case INDUSTRIAL:  currentThresholds = industrial; break;
    case IRRIGATION:  currentThresholds = irrigation; break;
  }
  Serial.println("♻️ Thresholds reset to WHO/area defaults");
}



// -------------------- Alert Function --------------------
String checkAlerts(float ph, float tds, float turbidity, float temp){
  String alerts="";
  if(ph<currentThresholds.ph_min || ph>currentThresholds.ph_max) alerts+="pH out of range; ";
  if(tds<currentThresholds.tds_min || tds>currentThresholds.tds_max) alerts+="TDS out of range; ";
  if(turbidity<currentThresholds.turbidity_min || turbidity>currentThresholds.turbidity_max) alerts+="Turbidity out of range; ";
  if(temp<currentThresholds.temp_min || temp>currentThresholds.temp_max) alerts+="Temperature out of range; ";

  if(alerts.length()>0 && (!alertActive || alerts!=lastAlert)){
    alertActive=true;
    mqttClient.publish(mqtt_alert_topic,alerts.c_str());
    Serial.println("MQTT ALERT 🚨: "+alerts);
  } else if(alerts.length()==0 && alertActive){
    alertActive=false;
    String clearMsg="✅ All parameters back to safe range.";
    mqttClient.publish(mqtt_alert_topic,clearMsg.c_str());
    Serial.println("MQTT ALERT CLEARED: "+clearMsg);
    sendTelegramMessage(clearMsg);
  }
  return alerts;
}

// -------------------- Sensor Read & Process --------------------
void readAndProcessSensors(){
  float ph=readPH();
  float tds=readTDS();
  float turb=readTurbidity();
  float temp=readTemperature();
  /*float ph=6.5;
  float tds=326;
  float turb=2;
  float temp= 22.5;*/
  if(isnan(ph)||isnan(tds)||isnan(turb)||isnan(temp)){
    lastAlert="Sensor error detected!";
    Serial.println(lastAlert);
  } else {
    lastPh=ph; lastTds=tds; lastTurb=turb; lastTemp=temp;
    lastAlert=checkAlerts(ph,tds,turb,temp);
    logToSD(ph,tds,turb,temp);//save locally
    uploadSDLog(ph,tds,turb,temp);//upload php server
    publishSensorData(ph,tds,turb,temp);//publish to MQTT
  }
}

// -------------------- Read Sensors --------------------
float readPH(){
  int raw=analogRead(PH_PIN);
  if(raw<50||raw>4000) return NAN;
  float voltage=raw*(3.3/4095.0);
  float ph=-5.70*voltage+21.34;
  if(ph<0||ph>14) return NAN;
  return ph;
}

float readTDS(){
  int raw=analogRead(TDS_PIN);
  if(raw<50||raw>4000) return NAN;
  float voltage=raw*(3.3/4095.0);
  float ec=(133.42*voltage*voltage*voltage)-(255.86*voltage*voltage)+(857.39*voltage);
  float tds=ec*0.5;
  if(tds<0||tds>3000) return NAN;
  return tds;
}

float readTurbidity(){
  int raw=analogRead(TURBIDITY_PIN);
  if(raw<50||raw>4000) return NAN;
  float voltage=raw*(3.3/4095.0);
  float ntu=-1120.4*voltage*voltage+5742.3*voltage-4352.9;
  if(ntu<0) ntu=0;
  if(ntu>1000) return NAN;
  return ntu;
}

float readTemperature(){
  sensors.requestTemperatures();
  float temp=sensors.getTempCByIndex(0);
  if(temp==DEVICE_DISCONNECTED_C) return NAN;
  return temp;
}

// -------------------- Other Functions --------------------
void publishSensorData(float ph,float tds,float turb,float temp){
  String payload="{";
  payload+="\"ph\":"+String(ph,2)+",";
  payload+="\"tds\":"+String(tds,1)+",";
  payload+="\"turbidity\":"+String(turb,1)+",";
  payload+="\"temperature\":"+String(temp,1)+",";
  payload+="\"alert\":\""+lastAlert+"\"}";
  if(mqttClient.connected()) mqttClient.publish(mqtt_topic,payload.c_str());
}
/*
void logToSD(float ph,float tds,float turb,float temp){
  String ts=getCurrentTimestamp();
  File f=SD.open("/log.csv",FILE_APPEND);
  if(f){ f.printf("%s,%.2f,%.2f,%.2f,%.2f\n",ts.c_str(),ph,tds,turb,temp); f.close(); }
}
*/

void logToSD(float temperature, float tds, float turbidity, float ph) {
  File file = SD.open("/log.csv", FILE_APPEND);
  if (file) {
    file.printf("%lu,%.2f,%.2f,%.2f,%.2f\n", 
                millis(), temperature, tds, turbidity, ph);
    file.close();
    Serial.println("✅ Data logged to SD");
  } else {
    Serial.println("❌ Failed to open log file");
  }
}

// -------------------- Corrected SD Upload --------------------
/*void uploadSDLog() {
  if (!SD.exists("/log.csv")) return;

  File f = SD.open("/log.csv", FILE_READ);
  if (!f) return;

  if (WiFi.status() != WL_CONNECTED) { f.close(); return; }

  /*HTTPClient http;
  http.begin(logUploadURL);
  http.addHeader("Content-Type", "text/plain");

  int code = http.sendRequest("POST", &f, f.size());

  if (code >= 200 && code < 300) {
    Serial.println("Log uploaded successfully, clearing SD log...");
    SD.remove("/log.csv");
  } else {
    Serial.print("Upload failed, HTTP code: ");
    Serial.println(code);
  }

  http.end();
  f.close();*/
/*
// Build JSON array from CSV
  String jsonPayload = "[";
  bool firstLine = true;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    int idx3 = line.indexOf(',', idx2 + 1);
    int idx4 = line.indexOf(',', idx3 + 1);

    if (idx4 == -1) continue; // invalid line

    String ts = line.substring(0, idx1);
    String ph = line.substring(idx1 + 1, idx2);
    String tds = line.substring(idx2 + 1, idx3);
    String turb = line.substring(idx3 + 1, idx4);
    String temp = line.substring(idx4 + 1);

    if (!firstLine) jsonPayload += ",";
    else firstLine = false;

    jsonPayload += "{";
    jsonPayload += "\"timestamp\":\"" + ts + "\",";
    jsonPayload += "\"ph\":" + ph + ",";
    jsonPayload += "\"tds\":" + tds + ",";
    jsonPayload += "\"turbidity\":" + turb + ",";
    jsonPayload += "\"temperature\":" + temp;
    jsonPayload += "}";
  }

  jsonPayload += "]";
  f.close();

  if (jsonPayload.length() <= 2) return; // nothing to send

  HTTPClient http;
  http.begin(logUploadURL);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(jsonPayload);

  if (code >= 200 && code < 300) {
    Serial.println("SD log uploaded successfully!");
    SD.remove("/log.csv"); // clear file after successful upload
    File newf = SD.open("/log.csv", FILE_WRITE);
    if (newf) newf.close();
  } else {
    Serial.print("Upload failed, HTTP code: ");
    Serial.println(code);
  }

  http.end();
}
*/
void  uploadSDLog(float temp, float tds, float turbidity, float ph) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(logUploadURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "temp=" + String(temp) +
                      "&tds=" + String(tds) +
                      "&turb=" + String(turbidity) +
                      "&ph=" + String(ph);

    int httpResponseCode = http.POST(postData);
    //String payload = http.getString();
   // serial.println("Server response: " + payload);
    if (httpResponseCode > 0) {
      Serial.printf("✅ Server Response: %d\n", httpResponseCode);
    } else {
      Serial.printf("❌ Error Code: %d\n", httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("⚠️ WiFi not connected, skipping upload.");
  }
}


void setupWiFi(){
  if(WiFi.status()==WL_CONNECTED) return;
  WiFi.begin(ssid,password);
  int retries=0;
  while(WiFi.status()!=WL_CONNECTED && retries<20){
    delay(500);
    Serial.print(".");
    retries++;
  }
  if(WiFi.status()==WL_CONNECTED){
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP32WaterMQTTClient")) {
      Serial.println("connected");
      mqttClient.subscribe("waterQuality/commands");
      mqttClient.subscribe("waterQuality/setThresholds");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}


void setUsageArea(const String& area){
  if(area=="drinking"){ 
    currentThresholds=drinking; selectedArea=DRINKING;
  } else if(area=="agriculture"){
    currentThresholds=agriculture; selectedArea=AGRICULTURE;
  } else if(area=="industrial"){
    currentThresholds=industrial; selectedArea=INDUSTRIAL;
  } else if(area=="irrigation"){
    currentThresholds=irrigation; selectedArea=IRRIGATION;
  } else {
    currentThresholds=drinking; selectedArea=DRINKING;
  }
}

void setupTime(){
  const long gmtOffset_sec = 23400; // +06:30
  configTime(gmtOffset_sec, 0, "pool.ntp.org", "time.nist.gov");

}

String getCurrentTimestamp(){
  time_t now=time(nullptr);
  struct tm* t=localtime(&now);
  char buf[20];
  snprintf(buf,sizeof(buf),"%04d-%02d-%02d %02d:%02d:%02d",
           t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
  return String(buf);
}

void mqttCallback(char* topic, byte* payload, unsigned int length){
  String msg = "";
  for (unsigned int i=0; i<length; i++) msg += (char)payload[i];
  Serial.print("MQTT received ["); Serial.print(topic); Serial.print("]: "); Serial.println(msg);

  if (String(topic) == mqtt_alert_topic) {
    lastAlert = msg;
  }
  else if (String(topic) == "waterQuality/commands") {
    if (msg == "reset") resetToAutoThresholds();
    else if (msg == "drinking") setUsageArea("drinking");
    else if (msg == "agriculture") setUsageArea("agriculture");
    else if (msg == "industral") setUsageArea("industrial");
    else if (msg == "irrigation") setUsageArea("irrigation");
  }
  else if (String(topic) == "waterQuality/setThresholds") {
    // Payload format: ph_min,ph_max,tds_min,tds_max,turb_min,turb_max,temp_min,temp_max
    float vals[8]; int idx=0, start=0;
    msg += ","; // ensure last value is read
    for(int i=0; i<msg.length(); i++){
      if(msg[i]==','){
        vals[idx++] = msg.substring(start,i).toFloat();
        start = i+1;
        if(idx>=8) break;
      }
    }
    setManualThresholds(vals[0],vals[1],vals[2],vals[3],vals[4],vals[5],vals[6],vals[7]);
    Serial.println("Manual thresholds updated via MQTT.");
  }
}


void sendTelegramMessage(const String& message){
  if(WiFi.status()!=WL_CONNECTED) return;
  String safeMsg;
  for(int i=0;i<message.length();i++){
    if(message[i]==' ') safeMsg += "%20";
    else safeMsg += message[i];
  }
  HTTPClient http;
  String url="https://api.telegram.org/bot"+String(BOT_TOKEN)+"/sendMessage?chat_id="+String(CHAT_ID)+"&text="+safeMsg;
  http.begin(url);
  http.GET();
  http.end();
}

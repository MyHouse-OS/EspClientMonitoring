#include "M5CoreS3.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WebServer.h>

#define COLOR_BG      M5.Display.color565(15, 23, 42)
#define COLOR_CARD    M5.Display.color565(30, 41, 59)
#define COLOR_ACCENT  M5.Display.color565(99, 102, 241)
#define COLOR_TEXT    M5.Display.color565(248, 250, 252)
#define COLOR_SUBTEXT M5.Display.color565(148, 163, 184)
#define COLOR_SUCCESS M5.Display.color565(34, 197, 94)
#define COLOR_WARNING M5.Display.color565(251, 146, 60)
#define COLOR_ERROR   M5.Display.color565(239, 68, 68)

const char* ssid = "MyHouseOS";
const char* password = "12345678";

const char* serverIP = "192.168.4.2";  
const int serverPort = 3000;

const char* authUrl  = "http://192.168.4.1/link";
const char* meteoUrl = "http://192.168.4.1/temp";
const char* lightUrl = "http://192.168.4.2:3000/toggle/light";
const char* heatUrl  = "http://192.168.4.2:3000/toggle/heat";
const char* doorUrl  = "http://192.168.4.2:3000/toggle/door";

const int PIN_LED_HEAT  = 1;  
const int PIN_LED_LIGHT = 3;  
const int PIN_LED_DOOR  = 16;

String Token = "";
String fullToken = ""; 
bool Connection = true; 

WebServer server(80);

bool watcherActive = false; 
unsigned long lastWatcherTime = 0;
const unsigned long watcherInterval = 3000; 

bool isLightOn = false;
bool isHeatOn = false;
bool isDoorOpen = false;
String currentTemp = "--"; 

void drawCard(int x, int y, int w, int h) {
    M5.Display.fillRoundRect(x, y, w, h, 8, COLOR_CARD);
}

void handleRoot() {
    server.send(200, "text/html", 
        "<html><head><title>M5 Screen</title>"
        "<script>setInterval(() => { document.querySelector('img').src = '/capture?t=' + Date.now(); }, 2000);</script>"
        "<style>body{background:#0f172a;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100vh;margin:0;color:white;font-family:sans-serif;}"
        "img{border:4px solid #6366f1;border-radius:10px;box-shadow:0 0 20px rgba(99,102,241,0.5);}"
        "</style></head>"
        "<body><h2>Vue Directe M5Stack</h2><img src='/capture' /></body></html>");
}

void handleCapture() {
    int w = M5.Display.width();
    int h = M5.Display.height();
    
    uint32_t imageSize = w * h * 3;
    uint32_t fileSize = 54 + imageSize;
    
    uint8_t header[54] = {
        0x42, 0x4D, 
        (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
        0, 0, 0, 0, 
        54, 0, 0, 0,
        40, 0, 0, 0,
        (uint8_t)(w), (uint8_t)(w >> 8), (uint8_t)(w >> 16), (uint8_t)(w >> 24),
        (uint8_t)(-h), (uint8_t)(-h >> 8), (uint8_t)(-h >> 16), (uint8_t)(-h >> 24),
        1, 0, 
        24, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    WiFiClient client = server.client();
    client.write("HTTP/1.1 200 OK\r\nContent-Type: image/bmp\r\nConnection: close\r\n\r\n");
    client.write(header, 54);

    uint16_t* lineBuffer = (uint16_t*)malloc(w * 2);
    uint8_t* rgbBuffer = (uint8_t*)malloc(w * 3);

    if (lineBuffer && rgbBuffer) {
        for (int y = 0; y < h; y++) {
            M5.Display.readRect(0, y, w, 1, lineBuffer);
            for (int x = 0; x < w; x++) {
                uint16_t p = lineBuffer[x];
                p = (p >> 8) | (p << 8); 
                
                rgbBuffer[x*3] = (p & 0x001F) << 3;       
                rgbBuffer[x*3+1] = (p & 0x07E0) >> 3;     
                rgbBuffer[x*3+2] = (p & 0xF800) >> 8;     
            }
            client.write(rgbBuffer, w * 3);
        }
    }
    
    if(lineBuffer) free(lineBuffer);
    if(rgbBuffer) free(rgbBuffer);
}

void updateCardUI(int x, int y, int w, int h, String title, String value, uint16_t valueColor) {
    M5.Display.fillRoundRect(x, y, w, h, 8, COLOR_CARD);
    
    M5.Display.setTextDatum(MC_DATUM); 
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(COLOR_SUBTEXT);
    M5.Display.drawString(title, x + w/2, y + 15);

    M5.Display.setTextFont(4); 
    M5.Display.setTextColor(valueColor);
    M5.Display.drawString(value, x + w/2, y + 42);
}

void drawInterface() {
    M5.Display.fillScreen(COLOR_BG);

    drawCard(5, 5, 310, 30);
    M5.Display.setTextDatum(ML_DATUM); 
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(COLOR_ACCENT);
    M5.Display.drawString("MyHouse Client", 20, 20); 
    
    M5.Display.setTextDatum(MR_DATUM); 
    M5.Display.setTextColor(COLOR_SUCCESS);
    M5.Display.drawString("WiFi OK", 300, 20);

    updateCardUI(5, 40, 152, 75, "LUMIERE", isLightOn ? "ON" : "OFF", isLightOn ? COLOR_WARNING : COLOR_SUBTEXT);
    updateCardUI(163, 40, 152, 75, "CHAUFFAGE", isHeatOn ? "ON" : "OFF", isHeatOn ? COLOR_ERROR : COLOR_SUBTEXT);
    updateCardUI(5, 120, 152, 75, "PORTE", isDoorOpen ? "OPEN" : "CLOSE", isDoorOpen ? COLOR_SUCCESS : COLOR_ERROR);
    updateCardUI(163, 120, 152, 75, "Température", currentTemp + " C", COLOR_ACCENT);

    drawCard(5, 200, 310, 35);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextFont(2);
    if(watcherActive) {
        M5.Display.setTextColor(COLOR_SUCCESS);
        M5.Display.drawString("WATCHER ACTIF (Btn C)", 160, 217);
    } else {
        M5.Display.setTextColor(COLOR_SUBTEXT);
        M5.Display.drawString("PAUSE - Appuyez sur C", 160, 217);
    }
}

void setup ()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    pinMode(PIN_LED_LIGHT, OUTPUT);
    pinMode(PIN_LED_HEAT, OUTPUT);
    pinMode(PIN_LED_DOOR, OUTPUT);

    digitalWrite(PIN_LED_LIGHT, LOW);
    digitalWrite(PIN_LED_HEAT, LOW);
    digitalWrite(PIN_LED_DOOR, LOW);

    M5.Display.setTextSize(1);
    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextFont(2);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(COLOR_SUBTEXT);
    M5.Display.drawString("CONNEXION WIFI...", 160, 120);

    WiFi.begin(ssid, password); 

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connecte !");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", handleRoot);
    server.on("/capture", handleCapture);
    server.begin();

    M5.Display.fillScreen(COLOR_BG);
    M5.Display.setTextColor(COLOR_ACCENT);
    M5.Display.drawString("STREAM IP:", 160, 100);
    M5.Display.setTextFont(4);
    M5.Display.drawString(WiFi.localIP().toString(), 160, 140);
    delay(4000); 

    drawInterface();
}


void checkLight() 
{
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    http.begin(lightUrl);
    http.addHeader("Authorization", fullToken); 
    int code = http.GET();
    
    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);

        String lightState = doc["light"].as<String>();
        isLightOn = (lightState == "true");

        digitalWrite(PIN_LED_LIGHT, isLightOn ? HIGH : LOW);

        updateCardUI(5, 40, 152, 75, "LUMIERE", isLightOn ? "ON" : "OFF", isLightOn ? COLOR_WARNING : COLOR_SUBTEXT);
    } 
    http.end();
}

void checkHeat() 
{
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    http.begin(heatUrl);
    http.addHeader("Authorization", fullToken); 
    int code = http.GET();
    
    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);

        String heatState = doc["heat"].as<String>();
        isHeatOn = (heatState == "true");

        digitalWrite(PIN_LED_HEAT, isHeatOn ? HIGH : LOW);

        updateCardUI(163, 40, 152, 75, "CHAUFFAGE", isHeatOn ? "ON" : "OFF", isHeatOn ? COLOR_ERROR : COLOR_SUBTEXT);
    } 
    http.end();
}

void checkDoor() 
{
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    http.begin(doorUrl);
    http.addHeader("Authorization", fullToken); 
    int code = http.GET();
    
    if (code == 200) {
        String payload = http.getString();
        JsonDocument doc;
        deserializeJson(doc, payload);

        String doorState = doc["door"].as<String>();
        isDoorOpen = (doorState == "true");

        digitalWrite(PIN_LED_DOOR, isDoorOpen ? HIGH : LOW);

        updateCardUI(5, 120, 152, 75, "PORTE", isDoorOpen ? "OPEN" : "CLOSE", isDoorOpen ? COLOR_SUCCESS : COLOR_ERROR);
    } 
    http.end();
}

void getMeteoAPI() {
    if(WiFi.status() != WL_CONNECTED) return;
    HTTPClient http;
    String url = "http://" + String(serverIP) + ":" + String(serverPort) + "/temp";
    http.begin(url);
    http.addHeader("Authorization", fullToken); 

    int code = http.GET();

    if(code == 200) {
        JsonDocument doc;
        deserializeJson(doc, http.getString());
        String temp = doc["temp"];
        currentTemp = temp;
        updateCardUI(163, 120, 152, 75, "Température", currentTemp + " C", COLOR_ACCENT);
    }
    else {
        updateCardUI(163, 120, 152, 75, "Température", "ERR", COLOR_ERROR);
    }
    http.end();
}

void runWatcher() {
    if (!watcherActive) return;

    if (millis() - lastWatcherTime < watcherInterval) {
        return; 
    }
    lastWatcherTime = millis();

    M5.Display.fillCircle(160, 20, 4, COLOR_ACCENT); 

    checkLight();
    checkHeat();
    checkDoor();
    getMeteoAPI();

    delay(50); 
    M5.Display.fillCircle(160, 20, 4, COLOR_CARD); 
}

void authenticate() {
    if(WiFi.status() != WL_CONNECTED) return;

    M5.Display.fillRoundRect(40, 70, 240, 100, 12, COLOR_CARD);
    M5.Display.drawRoundRect(40, 70, 240, 100, 12, COLOR_ACCENT);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextFont(2);
    M5.Display.setTextColor(COLOR_TEXT);
    M5.Display.drawString("Appairage en cours...", 160, 110);

    HTTPClient http;
    http.begin(authUrl);
    http.addHeader("Content-Type", "application/json");
    JsonDocument doc;
    doc["id"] = "F8C096E350CC";
    String jsonString;
    serializeJson(doc, jsonString);
    int httpResponseCode = http.POST(jsonString);

    M5.Display.fillRoundRect(40, 70, 240, 100, 12, COLOR_CARD);
    M5.Display.drawRoundRect(40, 70, 240, 100, 12, COLOR_ACCENT);

    if (httpResponseCode == 200) {
        String response = http.getString();
        JsonDocument responseDoc; 
        deserializeJson(responseDoc, response);
        Token = responseDoc["token"].as<String>(); 
        fullToken = "F8C096E350CC:" + Token;
        
        M5.Display.setTextColor(COLOR_SUCCESS);
        M5.Display.drawString("AUTH REUSSIE !", 160, 120);
    } else {
        M5.Display.setTextColor(COLOR_ERROR);
        M5.Display.drawString("ECHEC AUTH", 160, 120);
    }
    
    http.end();
    delay(1500);
    drawInterface();
}

void loop() 
{
  if (Connection){
    M5.update();

    server.handleClient();
    
    runWatcher();

    if (M5.BtnA.wasPressed()) {
      authenticate();
    } 
    else if (M5.BtnC.wasPressed()) {
       watcherActive = !watcherActive; 
       
       drawCard(5, 200, 310, 35);
       M5.Display.setTextDatum(MC_DATUM);
       M5.Display.setTextFont(2);
       
       if(watcherActive) {
           M5.Display.setTextColor(COLOR_SUCCESS);
           M5.Display.drawString("WATCHER ACTIF (Btn C)", 160, 217);
           lastWatcherTime = millis() - 3000; 
       } else {
           M5.Display.setTextColor(COLOR_SUBTEXT);
           M5.Display.drawString("PAUSE - Appuyez sur C", 160, 217);

           digitalWrite(PIN_LED_LIGHT, LOW);
           digitalWrite(PIN_LED_HEAT, LOW);
           digitalWrite(PIN_LED_DOOR, LOW);
       }
    }

    delay(5);
  }
}

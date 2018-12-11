#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid     = "ssid";
const char* password = "password";

const char* apikey = "apikey"; 
const char* host   = "emonpi.home.lan";

unsigned long last_connection;
const unsigned long post_interval = 1 * 1000; 

volatile unsigned long last_blink;
volatile unsigned long counter = 0;
volatile float power = 0;


void setup(void) {
  Serial.begin(115200);
  delay(20);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("esp8266-pnode");
  
  // No authentication by default
  ArduinoOTA.setPassword((const char *)"1234");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();

  Serial.println("\nReady");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  attachInterrupt(digitalPinToInterrupt(D1), irq_blnk, FALLING);
  last_connection = millis();
  last_blink = micros();
}

  
void irq_blnk() {
  unsigned long now = micros();
  unsigned long delta = now - last_blink;
  last_blink = now;
  if (delta > 90000) {
    power = 3600000000.0/delta; // 1000 imp/kWh
    counter++;
  }
}


void loop() {
  ArduinoOTA.handle();
  yield();
  
  if (counter && (millis() - last_connection > post_interval)) {
    last_connection = millis();
    counter--;
    
    String url = "https://emonpi.home.lan/emoncms/input/post?node=2&json={power:";
    url += round(power);
    url += "}&apikey=";
    url += apikey;
    
    WiFiClient client;
    if (!client.connect(host, 80)) {
      Serial.println("\nconnection failed ...");
      return;
    }
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" + 
                 "User-Agent: esp8266 \r\n" + 
                 "Connection: close\r\n\r\n");
                 
    //Serial.println("!");      
    delay(50);
  }
} 

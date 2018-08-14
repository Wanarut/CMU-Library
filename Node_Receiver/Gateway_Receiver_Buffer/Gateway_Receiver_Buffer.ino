//Communication
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
//Process
#include <ArduinoJson.h>

//Wifi
const char* ssid     = "OASYS";
const char* password = "1234567890";
int Wifi_status = 25;
//Server
const char* host = "192.168.1.40";
//Dual Core
TaskHandle_t TaskA;

#define delay_time 20000

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

#define SS      18
#define RST     14
#define DI0     26
#define BAND    915E6

//Buffering
const int Buffering_Size = 10;
String json_buffer[Buffering_Size];
int rssi_buffer[Buffering_Size];

void setup(){
  Serial.begin(115200);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  while (!Serial); //if just the the basic function, must connect to a computer
  
  Serial.println("LoRa Receiver"); 
  
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  
  if (!LoRa.begin(BAND)){
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!");

  xTaskCreatePinnedToCore(Task1, "gw_potto", 1000, NULL, 1, &TaskA, 0);
  
  pinMode(Wifi_status, OUTPUT);
}

void loop(){
  // try to parse packet
  if (LoRa.parsePacket()){
    //read packet
    String json = "";
    while(LoRa.available()){
      json += (char)LoRa.read();
    }

    if(json != ""){
      //received a packet
      Serial.print("Core 1 Received '");
      // print RSSI of packet
      Serial.print(json + "' with RSSI ");
      int rssi = LoRa.packetRssi();
      Serial.println(rssi);

      DynamicJsonBuffer jsonBuffer;
      JsonObject& data = jsonBuffer.parseObject(json);
      
      json_buffer[(int)data["id"]] = json;
      rssi_buffer[(int)data["id"]] = rssi;
    }
  }
}

void Task1(void * parameter){
  //setup
  int send_prev_mil = 0;
  int led_prev_mil = 0;
  //loop
  while(1){
    if(millis() - send_prev_mil >= delay_time){
      send_prev_mil = millis();
      Serial.print("\nCore 0 connecting to ");
      Serial.println(host);
      All_SendData_Quality();
    }
    if(millis() - led_prev_mil >= 500){
      led_prev_mil = millis();
      digitalWrite(Wifi_status, !digitalRead(Wifi_status));
    }
    delay(10);
  }
}

void All_SendData_Quality(){  
  Serial.println("--------------------------------------------ALL SEND DATA QUALITY--------------------------------------------");

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  for(int i = 0; i < Buffering_Size; i++){
    String json = json_buffer[i];
    json_buffer[i] = "";
    if(json != ""){
      DynamicJsonBuffer jsonBuffer;
      JsonObject& data = jsonBuffer.parseObject(json);
      int rssi = rssi_buffer[i];
    
      String URL = "http://" + String(host) + "/potto/get_data/cmu_lib.php?";
      URL += "id=" + data["id"].as<String>();
      URL += "&hu=" + data["hu"].as<String>();
      URL += "&to=" + data["to"].as<String>();
      URL += "&pr=" + data["pr"].as<String>();
      URL += "&dw=" + data["dw"].as<String>();
      URL += "&gs=" + data["gs"].as<String>();
      URL += "&lx=" + data["lx"].as<String>();
      URL += "&ds=" + data["ds"].as<String>();
      URL += "&vc=" + data["vc"].as<String>();
      URL += "&si=" + (String)rssi;
      
      Serial.print("Requesting URL: ");
      Serial.println(URL);
      
      digitalWrite(Wifi_status, HIGH);
      
      client.print(String("GET ") + URL + " HTTP/1.1\r\n" +
                   "Host: " + host + "\r\n\r\n");
      unsigned long timeout = millis();
      while (client.available() == 0) {
        if (millis() - timeout > 5000){
          Serial.println(">>> Client Timeout!");
          client.stop();
          return;
        }
      }
    }
  }
  Serial.println("---------------------------------------------CLOSEING CONNECTION---------------------------------------------\n");
}

//Communication
#include <SPI.h>
#include <LoRa.h>
#include <AIS_NB_BC95.h>
//Process
#include <ArduinoJson.h>
int Wifi_status = 25;

String apnName = "gw_potto.nb";

const char* host = "www.crflood.com";
String serverIP = "128.199.91.90"; // Your Server IP
String serverPort = "41234"; // Your Server Port

AIS_NB_BC95 AISnb;

#define delay_time 60000

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6  //433E6

//Buffering
const int Buffering_Size = 10;
String json_buffer[Buffering_Size];
int rssi_buffer[Buffering_Size];

int led_prev_mil = 0;

void setup(){
  Serial.begin(115200);
  
  AISnb.debug = true;
 
  AISnb.setupDevice(serverPort);

  String ip1 = AISnb.getDeviceIP();  
  delay(1000);
  
  pingRESP pingR = AISnb.pingIP(serverIP);
  
  Serial.println("\nLoRa Gateway");
  
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  
  if(!LoRa.begin(BAND)){
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!");
  
  pinMode(Wifi_status, OUTPUT);
}
void loop(){
  if (LoRa.parsePacket()){
    //read packet
    String json = "";
    while(LoRa.available()){
      json += (char)LoRa.read();
    }

    if(json != ""){
      //received a packet
      Serial.print("Received '");
      // print RSSI of packet
      Serial.print(json + "' with RSSI ");
      int rssi = LoRa.packetRssi();
      Serial.println(rssi);

      DynamicJsonBuffer jsonBuffer;
      JsonObject& data = jsonBuffer.parseObject(json);

      int id = (int)data["id"];
      json_buffer[id] = json;
      rssi_buffer[id] = rssi;
      
      delay(50);
      
      LoRa.beginPacket();
      LoRa.print(String(id));
      LoRa.endPacket();

      Serial.println("Ack ID " + String(id));
      Serial.println();
      All_SendData_Quality();
    }
  }
  if(millis() - led_prev_mil >= 500){
    led_prev_mil = millis();
    digitalWrite(Wifi_status, !digitalRead(Wifi_status));
  }
}

void All_SendData_Quality(){  
  Serial.println("--------------------------------------------ALL SEND DATA QUALITY--------------------------------------------");

  for(int i = 1; i < Buffering_Size; i++){
    String json = json_buffer[i];
    json_buffer[i] = "";
    if(json != ""){
      DynamicJsonBuffer jsonBuffer;
      JsonObject& data = jsonBuffer.parseObject(json);
      int rssi = rssi_buffer[i];
    
      String URL = "http://" + String(host) + "/crflood/boat/potto/get_data/cmu_lib.php?";
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
      Serial.print(URL);
      
      digitalWrite(Wifi_status, HIGH);
      UDPSend udp = AISnb.sendUDPmsgStr(serverIP, serverPort, URL);
    }
  }
  Serial.println("---------------------------------------------CLOSEING CONNECTION---------------------------------------------\n");
}

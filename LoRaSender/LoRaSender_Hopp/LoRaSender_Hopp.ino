#include <SPI.h>
#include <LoRa.h>
#include<Arduino.h>
//Process
#include <ArduinoJson.h>

String Node_ID = "5";

//Dual Core
TaskHandle_t TaskA;

#define delay_time 20000
//JSON
StaticJsonBuffer<300> jsonBuffer;
JsonObject& data = jsonBuffer.createObject();

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

#define SS      18
#define RST     14
#define DI0     26
#define BAND    915E6  //433E6 -- 这里的模式选择中，检查一下是否可在中国实用915这个频段

//Buffering
int item = 0;
const int Buffering_Size = 10;
String json_buffer[Buffering_Size];

int counter = 0;

void setup() {  
  Serial.begin(115200);
  while (!Serial); //If just the the basic function, must connect to a computer
  
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  Serial.println("\nLoRa Sender ID: " + Node_ID);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!");

  xTaskCreatePinnedToCore(Task1, "hopp_potto", 1000, NULL, 1, &TaskA, 0);

  delay(2000);
}

void loop(){
  data["id"] = Node_ID;
  data["hu"] = counter + 1;
  data["to"] = counter + 2;
  data["pr"] = counter + 3;
  data["dw"] = counter + 4;
  data["gs"] = counter + 5;
  data["lx"] = counter + 6;
  data["ds"] = counter + 7;
  data["vc"] = counter + 8;

  String output;
  data.printTo(output);

  // send packet
  LoRa.beginPacket();
  LoRa.print(output);
  LoRa.endPacket();
  
  Serial.print("Core 1 Send: ");
  Serial.println(output);
  counter++;
  
  delay(delay_time);
}

void Task1(void * parameter){
  //setup
  //loop
  while(1){
    if (LoRa.parsePacket()){
      //read packet
      String json = "";
      while(LoRa.available()){
        json += (char)LoRa.read();
      }
  
      if(json != ""){
        DynamicJsonBuffer jsonBuffer;
        JsonObject& data = jsonBuffer.parseObject(json);
        if(data["id"] != Node_ID){
          //received a packet
          Serial.print("Core 0 Received '");
          // print RSSI of packet
          Serial.print(json + "' with RSSI ");
          Serial.println(LoRa.packetRssi());
          String output;
          data.printTo(output);
        
          // send packet
          LoRa.beginPacket();
          LoRa.print(output);
          LoRa.endPacket();

          Serial.print("Core 0 Hopp: ");
          Serial.println(output);
        }
      }
    }
    //delay(10);
  }
}

#include <SPI.h>
#include <LoRa.h>
#include<Arduino.h>
//Process
#include <ArduinoJson.h>

String Node_ID = "5";

#define delay_time 60000
#define delay_repeat 1000 + (Node_ID.toInt() * 100)
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
#define BAND    925E6  //433E6

int counter = 0;

void setup(){  
  Serial.begin(115200);
  while(!Serial); //If just the the basic function, must connect to a computer
  
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  Serial.println("\nLoRa Sender ID: " + Node_ID);

  if(!LoRa.begin(BAND)){
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!\n");

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
  SendData(output);
  counter++;
  
  delay(delay_time);
}

void SendData(String data){
  int prev_mil = 0;
  do
  {
    int cur_mil = millis();
    if(cur_mil - prev_mil >= delay_repeat){
      prev_mil = cur_mil;
      LoRa.beginPacket();
      LoRa.print(data);
      LoRa.endPacket();
      
      Serial.print("Sending packet: ");
      Serial.println(data);
    }
  }while(!GateWayReceived());
}

bool GateWayReceived(){
  if(LoRa.parsePacket()){
    //read packet
    String response = "";
    while(LoRa.available()){
    response += (char)LoRa.read();
    }
    Serial.print("response: ");
    Serial.print(response);
    if(response == Node_ID){
      Serial.println(" Sent Success");
      return true;
    }
  }else{
    //Serial.println("Nothing Coming");    
  }
  return false;
}

#include <SPI.h>
#include <LoRa.h>
//Process
#include <ArduinoJson.h>
#include <WiFi.h>

const char* ssid     = "POTTO";
const char* password = "1234567890";

const char* host = "192.168.1.34";

// GPIO5  -- SX1278's SCK
// GPIO19 -- SX1278's MISO
// GPIO27 -- SX1278's MOSI
// GPIO18 -- SX1278's CS
// GPIO14 -- SX1278's RESET
// GPIO26 -- SX1278's IRQ(Interrupt Request)

#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6

void setup() {
  Serial.begin(115200);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  while (!Serial); //if just the the basic function, must connect to a computer
  //delay(1000);
  
  Serial.println("LoRa Receiver"); 
  
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initial OK!");
}

void loop() {
  String json = "";
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("\nReceived packet '");

    // read packet
    while (LoRa.available()) {
      json += (char)LoRa.read();
    }

    DynamicJsonBuffer jsonBuffer;
    JsonObject& data = jsonBuffer.parseObject(json);
    data.printTo(Serial);

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());

    if(data["vc"].as<String>() != "") SendData_Quality(data);
  }
}

void SendData_Quality(JsonObject& data){
  Serial.println("---------------------------------SEND DATA QUALITY---------------------------------");
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
  URL += "&si=" + (String)LoRa.packetRssi();
  
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
  }
  
  Serial.print("Requesting URL: ");
  Serial.println(URL);
  
  client.print(String("GET ") + URL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
      if (millis() - timeout > 5000) {
          Serial.println(">>> Client Timeout !");
          client.stop();
          return;
      }
  }

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
  }

  Serial.println("\n");
  Serial.println("closing connection\n");
  
  Serial.println("-----------------------------------------------------------------------------------\n");
}

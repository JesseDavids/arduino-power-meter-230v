#include <Arduino.h>
#include <EmonLib.h>
#include <SPI.h>
#include <Wire.h>

#include <Adafruit_SSD1306.h>
#include <Ethernet2.h>
#include <ArduinoJson.h>

//project power on leonardo dev board
byte mac[] = {0x3E, 0xAD, 0xBE, 0xDF, 0xFE, 0xE7};


EnergyMonitor emon1;
EthernetServer server(80);
String readString;
char jsonOutput[512];

#define ACpin A4
#define VoltagePin A5

#define ACTectionRange 20;
#define VREF 5.0
//int ACpin  = PIN_A4;
//int VoltagePin = PIN_A5;

#define ADC_BITS    10
#define ADC_COUNTS  (1<<ADC_BITS)


void setup() {
  w5500.init(11);

  Serial.begin(9600);
  Ethernet.begin(mac);
  server.begin();
  Serial.println(Ethernet.localIP());

  pinMode(ACpin, INPUT);
  analogReference(INTERNAL);  // set ADC positive reference voltage to 1.1V (internal) see link below for your internal voltage
  //It varies with different boards
  //Go to arduino website
  //https://www.arduino.cc/reference/en/language/functions/analog-io/analogreference/
  emon1.current(ACpin, 20);
  

}
 
// get maximum reading value
uint16_t get_max() {
  uint16_t max_v = 0;
  for(uint8_t i = 0; i < 100; i++) {
    uint16_t r = analogRead(A5);  // read from analog channel (A5)
    if(max_v < r) max_v = r;
    delayMicroseconds(1000);
  }
  return max_v;  

}
 
// main loop
void loop() {
  
  //char buf[10];
  // get amplitude (maximum - or peak value)
  uint32_t v = get_max();
  // get actual voltage (ADC voltage reference = 1.1V)
  //v = v * 1100/1023;
  // multiply with the reference voltage here
  v = v *  64/25;
  int v2 = v / 2;

  // calculate the RMS value ( = peak/√2 )
  v2 /= sqrt(2);

  for(int i = 0; v2 < 5; i++){
    if(v2 == 5){
      int avg = v2 + i / 5;
      Serial.print("Avg: ");
      Serial.println(avg);

    }


  }
  
  double amps = emon1.calcIrms(1480) * 10;
  double amps2 = amps - 0.4;


  
  Serial.print("Current (A): ");
  Serial.println(amps2);
  Serial.print("Voltage (V): ");
  Serial.println(v2);

  int power = amps2 * v2;

  Serial.print("Power (Watts): ");
  Serial.println(power);
  Serial.println("--------------");

  
  
  //delay(500);
 //////////////////////////////////////////////////////////////////////


  Ethernet.maintain();
  EthernetClient client = server.available();
  if(client){
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        Serial.print(c);
        if(readString.length()<100){
          readString += c;
        }
        if (c == '\n'){
          // Very important to specify the connection type as json
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Connection-type: application/json"));
          client.println(F("Connection: close"));
          client.println(); 
          /*
          
          
          // create json object and specify the size.  
          */        
          const size_t capacity = JSON_OBJECT_SIZE(5);         
          StaticJsonDocument<capacity> doc;
          JsonObject object = doc.to<JsonObject>(); 
          object["Current"] = amps2;
          object["Voltage"] = v2;
          object["Power"] = power;
          serializeJsonPretty(doc, jsonOutput);
          client.println(jsonOutput);
          
          break;
        }
      }
    }
  }
  delay(1);
  client.stop();
  delay(500);


}
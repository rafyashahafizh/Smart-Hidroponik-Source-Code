#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

String apiKey = "apikey thinkspeak";

const char* ssid = "Name wifi";
const char* pass = "pass wifi";
const char* server = "api.thingspeak.com";

const int trigPin = 12;
const int echoPin = 14;

#define ONE_WIRE_BUS D2
#define SOUND_VELOCITY 0.034
#define TdsSensorPin A0
#define VREF 3.3
#define SCOUNT 30
#define PIN_RELAY_1 D7
#define PIN_RELAY_2 D8

int analogBuffer [SCOUNT];
int analogBufferTemp [SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

long duration;
float distanceCm;
float averageVoltage = 0;
float tdsValue = 0;

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

WiFiClient client;

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j=0; i < iFilterLen - j - 1; i++) {
    for (i = 0; i < iFilterLen - j -1; i++) {
      if (bTab[i] > bTab[i+1]){
        bTemp = bTab[i];
        bTab[i] = bTab[i+1];
        bTab[i+1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab [(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
 
  while (WiFi.status() != WL_CONNECTED)
  {
  delay(500);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(TdsSensorPin,INPUT);
}

void loop() {
  //jarak
   // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY/2;
  
   if ((distanceCm == 0) )
  {
    Serial.println("Failed to read from sensor!");
    delay(1000);
  }
  else
  {
    Serial.print("Jarak permukaan air:");
    Serial.println(distanceCm);
    delay(1000);
  }
  
  //suhu
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if ((tempC == -127.00) )
  {
    Serial.println("Failed to read from sensor!");
    delay(1000);
  }
  else
  {
    Serial.print("Temperature in Celsius:");
    Serial.println(tempC);
    delay(1000);
  }
  
  //Tds Sensor
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){
    analogSampleTimepoint = millis();
    analogBuffer [analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){
      analogBufferIndex = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
        analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
        // read the analog value more stable by the median filtering algorithm, and convert to voltage value
        averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
        float compensationCoefficient = 1.0+0.02*(tempC-25.0);
        //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
    }
        if ((tdsValue == 0) )
        {
          Serial.println("Failed to read from sensor!");
          delay(1000);
        }
        else
        {
          Serial.print("Tds Value in ppm:");
          Serial.println(tdsValue);
          delay(1000);
        }
  }
  if (client.connect(server,80))
  {
    String postStr = apiKey;
    postStr +="&field2=";
    postStr += String(distanceCm);
    postStr +="&field3=";
    postStr += String(tempC);
    postStr +="&field1=";
    postStr += String(tdsValue);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
client.print("Host: api.thingspeak.com\n");
client.print("Connection: close\n");
client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
client.print("Content-Type: application/x-www-form-urlencoded\n");
client.print("Content-Length: ");
client.print(postStr.length());
client.print("\n\n");
client.print(postStr);
Serial.println("Sent data to Thingspeak");
}
client.stop();
Serial.println("Delay of 15 Sec");
// thingspeak needs minimum 15 sec delay between updates
delay(5000);

if (tdsValue < 1000) {

  digitalWrite(PIN_RELAY_1, HIGH);
  Serial.println("Relay 4 off");
  delay(1000);
}

if (distanceCm < 4) {

  digitalWrite(PIN_RELAY_4, HIGH);
  Serial.println("relay 4 on");
}
 delay(1000); 
}

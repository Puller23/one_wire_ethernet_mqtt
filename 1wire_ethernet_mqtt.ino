#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG_LEVEL 1

#define VERSION 0.1

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 130);

// MQTT Broker
// IP FHEM Server
const char* mqtt_server = "192.168.0.11";
const int mqttPort = 1883;
const String mqttNode = "Nano";
const String mqttDiscoveryPrefix = "fhem/keller/";
// Pin Config
//#define ONE_WIRE_BUS D3



// Time Limits
#define TIME_MAX_REPLAY_TEMP 600000 // 10min
#define TIME_MIN_REPLAY_TEMP 60000 // 1min

float lastTemperature[9];
float lastTimeTemperature[9];
float MAX_DIFF_TEMP = 0.5;

boolean send_version = true;
#define TEMPERATURE_PRECISION 12  

OneWire oneWire(3);
DallasTemperature sensors(&oneWire);

int numSensors=0;


EthernetClient ethClient;
PubSubClient client(ethClient);


void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  sensors.begin();
  numSensors = sensors.getDeviceCount();

  #if DEBUG_LEVEL > 0
    Serial.println("Starte Programm");
    Serial.print("Anzahl 1 Wire Sensoren:");
    Serial.println(numSensors);
  #endif

  connect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  sendVersion();
  sendTemp();
  client.loop();
}

void connect() {
  // MQTT
  client.setServer(mqtt_server, mqttPort);
  delay(200);
  client.publish("FHEM", "hello NANO");
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic","hello world");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void sendVersion()
{
  if (send_version == true)
  {
   char vers[10];
   String vers_str = String(VERSION);
   vers_str.toCharArray(vers, vers_str.length() +1);
   String mqttMSG = mqttDiscoveryPrefix + mqttNode + "/version";
   client.publish(mqttMSG.c_str(), vers);
    
   #if DEBUG_LEVEL > 0
     Serial.println("VERSION");
     Serial.print("mqttMSG:");
     Serial.println(mqttMSG);
     Serial.print(vers);
   #endif
   }
   send_version = false;
}

void sendTemp()
{
  unsigned long time = millis();

  sensors.requestTemperatures();
  for (int i=0; i<numSensors; i++)
  {
    boolean sendNOW = time - lastTimeTemperature[i] > TIME_MAX_REPLAY_TEMP;
    if(time - lastTimeTemperature[i] < TIME_MIN_REPLAY_TEMP)
    {
      return;
    } 
    float temperature = sensors.getTempCByIndex(i);
    if (abs(temperature - lastTemperature[i]) > MAX_DIFF_TEMP && temperature != -127.00 || sendNOW) 
    {
      char temp[10];
      String temp_str = String(temperature);
      temp_str.toCharArray(temp, temp_str.length() +1);
      String mqttMSG = mqttDiscoveryPrefix + mqttNode + "/temp" + i;
      client.publish(mqttMSG.c_str(), temp);
      
      #if DEBUG_LEVEL > 0
        Serial.print("T");
        Serial.print(i);
        Serial.print(":");
        Serial.println(temperature);
        Serial.print("T_unterschied:");
        Serial.println(abs(temperature - lastTemperature[i]));
        Serial.print("mqttMSG:");
        Serial.println(mqttMSG);
      #endif
      lastTemperature[i] = temperature;
      lastTimeTemperature[i]=millis();
    }
 } 
}

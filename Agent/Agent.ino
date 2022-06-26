#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoMqttClient.h>

#define INTERVAL 5000
const char* ssid = "RADIN-L";
const char* password = "radinradin";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const int BROKER_PORT = 1883;
const char pingTopic[]  = "I1820/main/agent/ping";
const char logTopic[]  = "I1820/main/log/send";
const char notifTopic[]  = "I1820/main/configuration/request";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const uint8_t LDR_PIN = A0; 
int LED_PINS[4] = {16, 5, 4, 0};

unsigned long previousMillis = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  String output;

  for (int i = 0; i < 4; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  pinMode(LDR_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  connectToWifi();
  connectToMQTT();

  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.update();

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  Serial.print("Subscribing to topic: ");
  Serial.println(notifTopic);
  Serial.println();
  
  // subscribe to a topic
  mqttClient.subscribe(notifTopic);
  
}

String pingMsg;
String logMsg;

void loop() {
  // put your main code here, to run repeatedly:
  mqttClient.poll();
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= INTERVAL) {
    previousMillis = currentMillis;
    int lightValue = 100 - (analogRead(LDR_PIN) * 100) / 1024;

    // ping 
    pingMsg = pingJson();
    mqttClient.beginMessage(pingTopic);
    mqttClient.print(pingMsg);
    mqttClient.endMessage();
    
    

    // log
    logMsg = logJson(lightValue);
    Serial.println(logMsg);
    mqttClient.beginMessage(logTopic);
    mqttClient.print(logMsg);
    mqttClient.endMessage();
  }
}


void onMqttMessage(int messageSize) {
  String rcvBuff = "";
  
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  while (mqttClient.available()) {
    rcvBuff += (char)mqttClient.read();
  }
  
  parseNotifJson(rcvBuff);
}

String pingJson() {
  StaticJsonDocument<384> doc;
  String output;
  
  doc["id"] = "a-very-unique-id";
  
  JsonArray things = doc.createNestedArray("things");
  
  JsonObject things_0 = things.createNestedObject();
  things_0["id"] = "l0";
  things_0["type"] = "lamp";
  
  JsonObject things_1 = things.createNestedObject();
  things_1["id"] = "l1";
  things_1["type"] = "lamp";
  
  JsonObject things_2 = things.createNestedObject();
  things_2["id"] = "l2";
  things_2["type"] = "lamp";
  
  JsonObject things_3 = things.createNestedObject();
  things_3["id"] = "l3";
  things_3["type"] = "lamp";
  
  JsonObject things_4 = things.createNestedObject();
  things_4["id"] = "ldr";
  things_4["type"] = "light";
  
  JsonArray actions = doc.createNestedArray("actions");
  
  serializeJson(doc, output);

  return output;
}

String logJson(short value) {
  StaticJsonDocument<192> doc;
  String output;

  doc["timestamp"] = timeClient.getEpochTime();
  doc["type"] = "light";
  doc["device"] = "ldr";
  
  JsonObject states_0 = doc["states"].createNestedObject();
  states_0["name"] = "light";
  states_0["value"] = value;
  doc["agent"] = "a-very-unique-id";
  
  serializeJson(doc, output);

  return output;
}

void parseNotifJson(String input) {
  StaticJsonDocument<192> doc;
    
  DeserializationError error = deserializeJson(doc, input);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
  const char* device = doc["device"]; // "l1"
  bool settings_0_value = doc["settings"][0]["value"]; // true

  digitalWrite(LED_PINS[device[1] - '0'], settings_0_value ? HIGH : LOW);
}

void connectToWifi() {
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
}

void connectToMQTT() {
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(WiFi.gatewayIP());

  char tmpBuf[15];
  WiFi.gatewayIP().toString().toCharArray(tmpBuf, 15);

  if (!mqttClient.connect(tmpBuf , BROKER_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

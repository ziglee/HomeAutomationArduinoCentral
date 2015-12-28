#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

#define DHTPIN 5 // PINO DO SENSOR DHT22 (TEMPERATURA E UMIDADE)
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int buttonPins[] = {
    1, //buttons/room
    2, //buttons/bedroom
    3  //buttons/kitchen
  };
const int outputPins[] = {
    11, //lights/room
    12, //lights/bedroom 
    13  //lights/kitchen
  };
const char macstr[] = "deadbeeffeed";
const String clientName = String("arduino:") + macstr;
const String topicName = String("status/fmt/json");
const long debounceDelay = 50;
const unsigned long statusIntervalRepeat = 5000UL;//1800000UL;

IPAddress ip(192, 168, 0, 120);
IPAddress server(192, 168, 0, 102);
EthernetClient ethClient;
PubSubClient client(ethClient);

int buttonStates[3];
int lastButtonStates[3] = {LOW, LOW, LOW};
long lastDebounceTimes[] = {0, 0, 0};
long lastStatusSentTime = 0;
float temperature = 0.0;
float humidity = 0.0;

void setup() {
  Serial.begin(57600);

  for (int i=0; i<3; i++) {
    pinMode(buttonPins[i], INPUT);
  }
  
  for (int i=0; i<3; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }
  
  client.setServer(server, 1883);
  client.setCallback(callback);
  
  Ethernet.begin(mac, ip);

  dht.begin();

  delay(1500);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if ((millis() - lastStatusSentTime) > statusIntervalRepeat) {
    publishStatus();
    lastStatusSentTime = millis();
  }

  for (int i=0; i<3; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = millis();
    }
    if ((millis() - lastDebounceTimes[i]) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;
        if (reading == HIGH) {
          if (i == 0) {
            client.publish("buttons/room", "1");
          } else if (i == 1) {
            client.publish("buttons/bedroom", "1");
          } else if (i == 2) {
            client.publish("buttons/kitchen", "1");
          }
        }
      }
    }
    lastButtonStates[i] = reading;
  }
}

String buildJson() {
  String data = "{";
  data+="\n";
  data+="\"temperature\": ";
  data+=temperature;
  data+= ",";
  data+="\n";
  data+="\"humidity\": ";
  data+=humidity;
  data+="\n";
  data+="}";
  return data;
}

void publishStatus() {  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (!isnan(temperature) && !isnan(humidity)) {
    // Once connected, publish the status...
    char topicStr[26];
    topicName.toCharArray(topicStr,26);
  
    String json = buildJson();
    char jsonStr[200];
    json.toCharArray(jsonStr,200);
    
    boolean pubresult = client.publish(topicStr, jsonStr);
    Serial.print("attempt to send ");
    Serial.println(jsonStr);
    Serial.print("to ");
    Serial.println(topicStr);
    if (pubresult)
      Serial.println("successfully sent");
    else
      Serial.println("unsuccessfully sent");
  } else {
    Serial.println("Failed to read from DHT");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  int index = -1;
  int newState = LOW;
  if (payload[0] == '1') {
    newState = HIGH;
  }
  
  if (strcmp(topic, "lights/room") == 0) {
    index = 0;
  } else if (strcmp(topic, "lights/bedroom") == 0) {
    index = 1;
  } else if (strcmp(topic, "lights/kitchen") == 0) {
    index = 2;
  }

  if (index >= 0) {
    digitalWrite(outputPins[index], newState);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    char clientStr[34];
    clientName.toCharArray(clientStr,34);
    if (client.connect(clientStr)) {
      Serial.println("connected");
      client.subscribe("lights/room");
      client.subscribe("lights/bedroom");
      client.subscribe("lights/kitchen");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


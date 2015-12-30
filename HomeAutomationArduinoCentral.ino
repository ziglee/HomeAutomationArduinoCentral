#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

#define DHTPIN 6 // PINO DO SENSOR DHT22 (TEMPERATURA E UMIDADE)
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int buttonPins[] = {
    23, //buttons/room
    24, //buttons/bedroom
    25  //buttons/kitchen
  };
const int outputPins[] = {
    32, //lights/room
    33, //lights/bedroom 
    34  //lights/kitchen
  };
const char clientName[] = "arduino:home";
const char sensorsStatusTopicName[] = "sensors/status";
const char switchesStatusTopicName[] = "switches/status";

const long debounceDelay = 50;
const unsigned long statusIntervalRepeat = 30000UL;//1800000UL;

IPAddress ip(192, 168, 0, 120);
IPAddress server(192, 168, 0, 101);
EthernetClient ethClient;
PubSubClient client(ethClient);

int buttonStates[3] = {LOW, LOW, LOW};
int lastButtonStates[3] = {LOW, LOW, LOW};
long lastDebounceTimes[] = {0, 0, 0};
long lastStatusSentTime = 0;
float temperature = 0.0;
float humidity = 0.0;

int lightRoomState = LOW;
int lightBedroomState = LOW;
int lightKitchenState = LOW;
bool firstLoopExecuted = false;

void setup() {
  Serial.begin(57600);

  for (int i=0; i<3; i++) {
    pinMode(buttonPins[i], INPUT);
    lastDebounceTimes[i] = millis() - 1000;
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

  if (!firstLoopExecuted) {
    publishSensorsStatus();
    publishSwitchesStatus();
    firstLoopExecuted = true;
  }
  
  if ((millis() - lastStatusSentTime) > statusIntervalRepeat) {
    publishSensorsStatus();
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

String buildSensorsJson() {
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

bool publishSwitchesStatus() {
  String data = "{";
  data+="\n";
  data+="\"room\": ";
  data+=lightRoomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"bedroom\": ";
  data+=lightBedroomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"kitchen\": ";
  data+=lightKitchenState == LOW ? "0" : "1";
  data+="\n";
  data+="}";
  
  char jsonStr[200];
  data.toCharArray(jsonStr,200);
  return client.publish("switches/status", jsonStr, true);
}

void publishSensorsStatus() {  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  if (!isnan(temperature) && !isnan(humidity)) {
    String json = buildSensorsJson();
    char jsonStr[200];
    json.toCharArray(jsonStr,200);
    
    boolean pubresult = client.publish(sensorsStatusTopicName, jsonStr, true);
    Serial.print("sending ");
    Serial.println(jsonStr);
    if (!pubresult)
      Serial.println("unsuccessfully sent");
  } else {
    Serial.println("Failed to read from DHT");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(":");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  int index = -1;
  int newState = payload[0] == '1' ? HIGH : LOW;
  
  if (strcmp(topic, "lights/room/set") == 0) {
    index = 0;
    lightRoomState = newState;
  } else if (strcmp(topic, "lights/bedroom/set") == 0) {
    index = 1;
    lightBedroomState = newState;
  } else if (strcmp(topic, "lights/kitchen/set") == 0) {
    index = 2;
    lightKitchenState = newState;
  }

  if (index >= 0) {
    digitalWrite(outputPins[index], newState);
    publishSwitchesStatus();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientName)) {
      Serial.println("connected");
      client.subscribe("lights/room/set");
      client.subscribe("lights/bedroom/set");
      client.subscribe("lights/kitchen/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


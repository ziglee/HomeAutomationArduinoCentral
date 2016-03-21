#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

//#define MQTT_MAX_PACKET_SIZE 512
#define DHTPIN 6 // PINO DO SENSOR DHT22 (TEMPERATURA E UMIDADE)
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int buttonPins[] = {
    22, //buttons/lighs_room_balcony
    23, //buttons/lighs_room
    24, //buttons/lighs_room_kitchen
    25, //buttons/lighs_kitchen
    26, //buttons/lighs_bathroom
    27, //buttons/lighs_bathroom_mirror
    28, //buttons/lighs_entry_balcony
    29, //buttons/lighs_bedroom
    30, //buttons/lighs_bedroom_balcony
    31, //buttons/lighs_upper_bedroom
    32, //buttons/lighs_service_area
    33, //buttons/lighs_green_roof
    34, //buttons/sockets_bedroom_left
    35  //buttons/sockets_bedroom_right
  };
const int outputPins[] = {
    36, //relays/lighs_room_balcony
    37, //relays/lighs_room
    38, //relays/lighs_room_kitchen
    39, //relays/lighs_kitchen
    40, //relays/lighs_bathroom
    41, //relays/lighs_bathroom_mirror
    42, //relays/lighs_entry_balcony
    43, //relays/lighs_bedroom
    44, //relays/lighs_bedroom_balcony
    45, //relays/lighs_upper_bedroom
    46, //relays/lighs_service_area
    47, //relays/lighs_green_roof
    48, //relays/sockets_bedroom_left
    49  //relays/sockets_bedroom_right
  };
const char clientName[] = "arduino:home";
const char sensorsStatusTopicName[] = "sensors/status";
const char switchesStatusTopicName[] = "switches/status";

const long debounceDelay = 50;
const unsigned long statusIntervalRepeat = 1800000UL;

IPAddress ip(192, 168, 0, 120);
IPAddress server(192, 168, 0, 105);
EthernetClient ethClient;
PubSubClient client(ethClient);

int buttonStates[14] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
int lastButtonStates[14] = {LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
long lastDebounceTimes[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long lastStatusSentTime = 0;
float temperature = 0.0;
float humidity = 0.0;
bool firstLoopExecuted = false;

int relayLightsRoomBalconyState = LOW;
int relayLightsRoomState = LOW;
int relayLightsRoomKitchenState = LOW;
int relayLightsKitchenState = LOW;
int relayLightsBathroomState = LOW;
int relayLightsBathroomMirrorState = LOW;
int relayLightsEntryBalconyState = LOW;
int relayLightsBedroomState = LOW;
int relayLightsBedroomBalconyState = LOW;
int relayLightsUpperBedroomState = LOW;
int relayLightsServiceAreaState = LOW;
int relayLightsGreenRoofState = LOW;
int relaySocketsBedroomLeftState = LOW;
int relaySocketsBedroomRightState = LOW;

void setup() {
  Serial.begin(57600);

  for (int i=0; i<14; i++) {
    pinMode(buttonPins[i], INPUT);
    lastDebounceTimes[i] = millis() - 1000;
  }
  
  for (int i=0; i<14; i++) {
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

  for (int i=0; i<14; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = millis();
    }
    if ((millis() - lastDebounceTimes[i]) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;
        if (reading == HIGH) {
          if (i == 0) {
            client.publish("buttons/lights_room_balcony", "1");
          } else if (i == 1) {
            client.publish("buttons/lights_room", "1");
          } else if (i == 2) {
            client.publish("buttons/lights_room_kitchen", "1");
          } else if (i == 3) {
            client.publish("buttons/lights_kitchen", "1");
          } else if (i == 4) {
            client.publish("buttons/lights_bathroom", "1");
          } else if (i == 5) {
            client.publish("buttons/lights_bathroom_mirror", "1");
          } else if (i == 6) {
            client.publish("buttons/lights_entry_balcony", "1");
          } else if (i == 7) {
            client.publish("buttons/lights_bedroom", "1");
          } else if (i == 8) {
            client.publish("buttons/lights_bedroom_balcony", "1");
          } else if (i == 9) {
            client.publish("buttons/lights_upper_bedroom", "1");
          } else if (i == 10) {
            client.publish("buttons/lights_service_area", "1");
          } else if (i == 11) {
            client.publish("buttons/lights_green_roof", "1");
          } else if (i == 12) {
            client.publish("buttons/sockets_bedroom_left", "1");
          } else if (i == 13) {
            client.publish("buttons/sockets_bedroom_right", "1");
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

void publishSwitchesStatus() {
  String data = "{";
  data+="\n";
  data+="\"lights_room_balcony\": ";
  data+=relayLightsRoomBalconyState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_room\": ";
  data+=relayLightsRoomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_room_kitchen\": ";
  data+=relayLightsRoomKitchenState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_kitchen\": ";
  data+=relayLightsKitchenState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_bathroom\": ";
  data+=relayLightsBathroomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_bathroom_mirror\": ";
  data+=relayLightsBathroomMirrorState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_entry_balcony\": ";
  data+=relayLightsEntryBalconyState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_bedroom\": ";
  data+=relayLightsBedroomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_bedroom_balcony\": ";
  data+=relayLightsBedroomBalconyState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_upper_bedroom\": ";
  data+=relayLightsUpperBedroomState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_service_area\": ";
  data+=relayLightsServiceAreaState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"lights_green_roof\": ";
  data+=relayLightsGreenRoofState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"sockets_bedroom_left\": ";
  data+=relaySocketsBedroomLeftState == LOW ? "0" : "1";
  data+= ",";
  data+="\n";
  data+="\"sockets_bedroom_right\": ";
  data+=relaySocketsBedroomRightState == LOW ? "0" : "1";
  data+="\n";
  data+="}";
  
  char jsonStr[400];
  data.toCharArray(jsonStr,400);
  Serial.println(jsonStr);
  boolean pubresult = client.publish("switches/status", jsonStr, true);
  if (!pubresult)
      Serial.println("unsuccessfully sent switches/status");
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
      Serial.println("unsuccessfully sent sensors/status");
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
  
  if (strcmp(topic, "relays/lights_room_balcony/set") == 0) {
    index = 0;
    relayLightsRoomBalconyState = newState;
  } else if (strcmp(topic, "relays/lights_room/set") == 0) {
    index = 1;
    relayLightsRoomState = newState;
  } else if (strcmp(topic, "relays/lights_room_kitchen/set") == 0) {
    index = 2;
    relayLightsRoomKitchenState = newState;
  } else if (strcmp(topic, "relays/lights_kitchen/set") == 0) {
    index = 3;
    relayLightsKitchenState = newState;
  } else if (strcmp(topic, "relays/lights_bathroom/set") == 0) {
    index = 4;
    relayLightsBathroomState = newState;
  } else if (strcmp(topic, "relays/lights_bathroom_mirror/set") == 0) {
    index = 5;
    relayLightsBathroomMirrorState = newState;
  } else if (strcmp(topic, "relays/lights_entry_balcony/set") == 0) {
    index = 6;
    relayLightsEntryBalconyState = newState;
  } else if (strcmp(topic, "relays/lights_bedroom/set") == 0) {
    index = 7;
    relayLightsBedroomState = newState;
  } else if (strcmp(topic, "relays/lights_bedroom_balcony/set") == 0) {
    index = 8;
    relayLightsBedroomBalconyState = newState;
  } else if (strcmp(topic, "relays/lights_upper_bedroom/set") == 0) {
    index = 9;
    relayLightsUpperBedroomState = newState;
  } else if (strcmp(topic, "relays/lights_service_area/set") == 0) {
    index = 10;
    relayLightsServiceAreaState = newState;
  } else if (strcmp(topic, "relays/lights_green_roof/set") == 0) {
    index = 11;
    relayLightsGreenRoofState = newState;
  } else if (strcmp(topic, "relays/sockets_bedroom_left/set") == 0) {
    index = 12;
    relaySocketsBedroomLeftState = newState;
  } else if (strcmp(topic, "relays/sockets_bedroom_right/set") == 0) {
    index = 13;
    relaySocketsBedroomRightState = newState;
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
      client.subscribe("relays/lights_room_balcony/set");
      client.subscribe("relays/lights_room/set");
      client.subscribe("relays/lights_room_kitchen/set");
      client.subscribe("relays/lights_kitchen/set");
      client.subscribe("relays/lights_bathroom/set");
      client.subscribe("relays/lights_bathroom_mirror/set");
      client.subscribe("relays/lights_entry_balcony/set");
      client.subscribe("relays/lights_bedroom/set");
      client.subscribe("relays/lights_bedroom_balcony/set");
      client.subscribe("relays/lights_upper_bedroom/set");
      client.subscribe("relays/lights_service_area/set");
      client.subscribe("relays/lights_green_roof/set");
      client.subscribe("relays/sockets_bedroom_left/set");
      client.subscribe("relays/sockets_bedroom_right/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


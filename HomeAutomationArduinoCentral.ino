#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <stdlib.h>

#define DHTPIN 6 // PINO DO SENSOR DHT22 (TEMPERATURA E UMIDADE)
#define DHTTYPE DHT22   // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int buttonPins[] = {
    22, //buttons/room_out_1
    23, //buttons/room_out_2
    24, //buttons/room_1
    25, //buttons/room_2
    26, //buttons/corridor_1
    27, //buttons/corridor_2
    28, //buttons/bathroom
    29, //buttons/bedroom
    30, //buttons/bedroom_out_1
    31, //buttons/bedroom_out_2
    32, //buttons/bedroom_porch
    33, //buttons/bed_left
    34, //buttons/bed_right
    35, //buttons/upper_1
    36, //buttons/upper_2
    37, //buttons/entry
    38  //buttons/laundry
  };
const int outputPins[] = {
    39, //relays/room_porch
    40, //relays/room
    41, //relays/counter
    42, //relays/kitchen
    43, //relays/bathroom
    44, //relays/corridor
    45, //relays/entry
    46, //relays/bedroom
    47, //relays/bedroom_porch
    48, //relays/laundry
    49, //relays/upper
    50  //relays/recreation
  };
const char clientName[] = "arduino:home";
const char sensorsStatusTopicName[] = "sensors/status";
const char switchesStatusTopicName[] = "switches/status";

const long debounceDelay = 50;
const unsigned long statusIntervalRepeat = 1800000UL;

IPAddress ip(192, 168, 100, 120);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress server(192, 168, 100, 125);
EthernetClient ethClient;
PubSubClient client;

int buttonStates[17] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
int lastButtonStates[17] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };
long lastDebounceTimes[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
long lastStatusSentTime = 0;
bool firstLoopExecuted = false;

int relayRoomPorchState = HIGH;
int relayRoomState = HIGH;
int relayCounterState = HIGH;
int relayKitchenState = HIGH;
int relayBathroomState = HIGH;
int relayCorridorState = HIGH;
int relayEntryState = HIGH;
int relayBedroomState = HIGH;
int relayBedroomPorchState = HIGH;
int relayLaundryState = HIGH;
int relayUpperState = HIGH;
int relayRecreationState = HIGH;

void setup() {
  delay(1000);
  
  Serial.begin(57600);

  Serial.println("Setup in progress...");

  Serial.println("Setting input pins...");
  for (int i=0; i<17; i++) {
    pinMode(buttonPins[i], INPUT);
    lastDebounceTimes[i] = millis() - 1000;
  }

  Serial.println("Setting output pins...");
  for (int i=0; i<12; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], HIGH);
  }

  Serial.println("Setting DHT sensor...");
  dht.begin();

  Serial.println("Setting Ethernet...");
  Ethernet.begin(mac, ip);
  
  delay(1000);

  Serial.println("MQTT client...");
  client.setClient(ethClient);
  client.setServer(server, 1883);
  client.setCallback(callback);

  delay(1000);
}

void loop() {
  if (!client.loop()) {
    client.disconnect();
    reconnect();
  }

  if (!firstLoopExecuted) {
    publishSensorsStatus();
    publishSwitchesStatus();
    firstLoopExecuted = true;
  }
  
  if ((millis() - lastStatusSentTime) > statusIntervalRepeat) {
    publishSensorsStatus();
    lastStatusSentTime = millis();
  }

  for (int i=0; i<16; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (reading != lastButtonStates[i]) {
      lastDebounceTimes[i] = millis();
    }
    if ((millis() - lastDebounceTimes[i]) > debounceDelay) {
      if (reading != buttonStates[i]) {
        buttonStates[i] = reading;
        if (reading == HIGH) {
          if (i == 0) {
            client.publish("buttons/room_out_1", "1");
          } else if (i == 1) {
            client.publish("buttons/room_out_2", "1");
          } else if (i == 2) {
            client.publish("buttons/room_1", "1");
          } else if (i == 3) {
            client.publish("buttons/room_2", "1");
          } else if (i == 4) {
            client.publish("buttons/corridor_1", "1");
          } else if (i == 5) {
            client.publish("buttons/corridor_2", "1");
          } else if (i == 6) {
            client.publish("buttons/bathroom", "1");
          } else if (i == 7) {
            client.publish("buttons/bedroom", "1");
          } else if (i == 8) {
            client.publish("buttons/bedroom_out_1", "1");
          } else if (i == 9) {
            client.publish("buttons/bedroom_out_2", "1");
          } else if (i == 10) {
            client.publish("buttons/bedroom_porch", "1");
          } else if (i == 11) {
            client.publish("buttons/bed_left", "1");
          } else if (i == 12) {
            client.publish("buttons/bed_right", "1");
          } else if (i == 13) {
            client.publish("buttons/upper_1", "1");
          } else if (i == 14) {
            client.publish("buttons/upper_2", "1");
          } else if (i == 15) {
            client.publish("buttons/entry", "1");
          } else if (i == 16) {
            client.publish("buttons/laundry", "1");
          }
        }
      }
    }
    lastButtonStates[i] = reading;
  }
}

void publishSwitchesStatus() {
  byte message[12] = { byte(relayRoomPorchState == LOW), 
    byte(relayRoomState == LOW),
    byte(relayCounterState == LOW),
    byte(relayKitchenState == LOW),
    byte(relayBathroomState == LOW),
    byte(relayCorridorState == LOW),
    byte(relayEntryState == LOW),
    byte(relayBedroomState == LOW),
    byte(relayBedroomPorchState == LOW),
    byte(relayLaundryState == LOW),
    byte(relayUpperState == LOW),
    byte(relayRecreationState == LOW)};
  boolean pubresult = client.publish("switches/status", message, 12);
  if (!pubresult)
      Serial.println("unsuccessfully sent switches/status");
}

void publishSensorsStatus() {  
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (!isnan(temperature) && !isnan(humidity)) {
    char outstr[15];
    dtostrf(temperature, 5, 1, outstr);
    boolean pubresult = client.publish("sensors/temperature", outstr);
    dtostrf(humidity, 5, 1, outstr);
    client.publish("sensors/humidity", outstr);
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
  int newState = payload[0] == '1' ? LOW : HIGH;
  
  if (strcmp(topic, "relays/room_porch/set") == 0) {
    index = 0;
    relayRoomPorchState = newState;
  } else if (strcmp(topic, "relays/room/set") == 0) {
    index = 1;
    relayRoomState = newState;
  } else if (strcmp(topic, "relays/counter/set") == 0) {
    index = 2;
    relayCounterState = newState;
  } else if (strcmp(topic, "relays/kitchen/set") == 0) {
    index = 3;
    relayKitchenState = newState;
  } else if (strcmp(topic, "relays/bathroom/set") == 0) {
    index = 4;
    relayBathroomState = newState;
  } else if (strcmp(topic, "relays/corridor/set") == 0) {
    index = 5;
    relayCorridorState = newState;
  } else if (strcmp(topic, "relays/entry/set") == 0) {
    index = 6;
    relayEntryState = newState;
  } else if (strcmp(topic, "relays/bedroom/set") == 0) {
    index = 7;
    relayBedroomState = newState;
  } else if (strcmp(topic, "relays/bedroom_porch/set") == 0) {
    index = 8;
    relayBedroomPorchState = newState;
  } else if (strcmp(topic, "relays/laundry/set") == 0) {
    index = 9;
    relayLaundryState = newState;
  } else if (strcmp(topic, "relays/upper/set") == 0) {
    index = 10;
    relayUpperState = newState;
  } else if (strcmp(topic, "relays/recreation/set") == 0) {
    index = 11;
    relayRecreationState = newState;
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
      client.subscribe("relays/room_porch/set");
      client.subscribe("relays/room/set");
      client.subscribe("relays/counter/set");
      client.subscribe("relays/kitchen/set");
      client.subscribe("relays/bathroom/set");
      client.subscribe("relays/corridor/set");
      client.subscribe("relays/entry/set");
      client.subscribe("relays/bedroom/set");
      client.subscribe("relays/bedroom_porch/set");
      client.subscribe("relays/laundry/set");
      client.subscribe("relays/upper/set");
      client.subscribe("relays/recreation/set");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


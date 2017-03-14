#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <stdlib.h>

#define DHTPIN 6 // PINO DO SENSOR DHT22 (TEMPERATURA E UMIDADE)
#define DHTTYPE DHT22   // DHT 22 (AM2302)

#define RELAY_ROOM_PIN 39
#define RELAY_BEDROOM_PORCH_PIN 40
#define RELAY_BEDROOM_PIN 41
#define RELAY_COUNTER_PIN 42
#define RELAY_ENTRY_PIN 43
#define RELAY_BATHROOM_PIN 44
#define RELAY_UPPER_PIN 45
#define RELAY_LAUNDRY_PIN 46
#define RELAY_ROOM_PORCH_PIN 47
#define RELAY_RECREATION_PIN 48
#define RELAY_KITCHEN_PIN 49
#define RELAY_CORRIDOR_PIN 7

#define BUTTON_ROOM_OUT_1_PIN 31
#define BUTTON_ROOM_OUT_2_PIN 28
#define BUTTON_ROOM_1_PIN 22
#define BUTTON_ROOM_2_PIN 29
#define BUTTON_CORRIDOR_1_PIN 32
#define BUTTON_CORRIDOR_2_PIN 26
#define BUTTON_BATHROOM_PIN 27
#define BUTTON_BEDROOM_PIN 36
#define BUTTON_BEDROOM_OUT_1_PIN 30
#define BUTTON_BEDROOM_OUT_2_PIN 35
#define BUTTON_BEDROOM_PORCH_PIN 33
#define BUTTON_BED_LEFT_PIN 37
#define BUTTON_BED_RIGHT_PIN 33
#define BUTTON_UPPER_1_PIN 28
#define BUTTON_UPPER_2_PIN 34
#define BUTTON_ENTRY_PIN 24
#define BUTTON_LAUNDRY_PIN 25

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const int buttonPins[] = {
    BUTTON_ROOM_OUT_1_PIN,
    BUTTON_ROOM_OUT_2_PIN,
    BUTTON_ROOM_1_PIN,
    BUTTON_ROOM_2_PIN,
    BUTTON_CORRIDOR_1_PIN,
    BUTTON_CORRIDOR_2_PIN,
    BUTTON_BATHROOM_PIN,
    BUTTON_BEDROOM_PIN,
    BUTTON_BEDROOM_OUT_1_PIN,
    BUTTON_BEDROOM_OUT_2_PIN,
    BUTTON_BEDROOM_PORCH_PIN,
    BUTTON_BED_LEFT_PIN,
    BUTTON_BED_RIGHT_PIN,
    BUTTON_UPPER_1_PIN,
    BUTTON_UPPER_2_PIN,
    BUTTON_ENTRY_PIN,
    BUTTON_LAUNDRY_PIN
  };
  
const int outputPins[] = {
    RELAY_ROOM_PIN,
    RELAY_BEDROOM_PORCH_PIN,
    RELAY_BEDROOM_PIN,
    RELAY_COUNTER_PIN,
    RELAY_ENTRY_PIN,
    RELAY_BATHROOM_PIN,
    RELAY_UPPER_PIN,
    RELAY_LAUNDRY_PIN,
    RELAY_ROOM_PORCH_PIN,
    RELAY_RECREATION_PIN,
    RELAY_KITCHEN_PIN,
    RELAY_CORRIDOR_PIN
  };
  
const char clientName[] = "arduino:home";

const long debounceDelay = 50;
const unsigned long statusIntervalRepeat = 1800000UL;

DHT dht(DHTPIN, DHTTYPE);

IPAddress ip(192, 168, 100, 120);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress server(192, 168, 100, 125);
EthernetClient ethClient;
PubSubClient client;

int buttonStates[17] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };
int lastButtonStates[17] = { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH };
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
  
  Serial.begin(9600);

  Serial.println("Setup in progress...");

  Serial.println("Setting input pins...");
  for (int i=0; i<17; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
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
        if (reading == LOW) {

          Serial.println(buttonPins[i]); // REMOVER
          
          if (buttonPins[i] == BUTTON_ROOM_OUT_1_PIN) {
            client.publish("buttons/room_out_1", "1");
          } else if (buttonPins[i] == BUTTON_ROOM_OUT_2_PIN) {
            client.publish("buttons/room_out_2", "1");
          } else if (buttonPins[i] == BUTTON_ROOM_1_PIN) {
            client.publish("buttons/room_1", "1");
          } else if (buttonPins[i] == BUTTON_ROOM_2_PIN) {
            client.publish("buttons/room_2", "1");
          } else if (buttonPins[i] == BUTTON_CORRIDOR_1_PIN) {
            client.publish("buttons/corridor_1", "1");
          } else if (buttonPins[i] == BUTTON_CORRIDOR_2_PIN) {
            client.publish("buttons/corridor_2", "1");
          } else if (buttonPins[i] == BUTTON_BATHROOM_PIN) {
            client.publish("buttons/bathroom", "1");
          } else if (buttonPins[i] == BUTTON_BEDROOM_PIN) {
            client.publish("buttons/bedroom", "1");
          } else if (buttonPins[i] == BUTTON_BEDROOM_OUT_1_PIN) {
            client.publish("buttons/bedroom_out_1", "1");
          } else if (buttonPins[i] == BUTTON_BEDROOM_OUT_2_PIN) {
            client.publish("buttons/bedroom_out_2", "1");
          } else if (buttonPins[i] == BUTTON_BEDROOM_PORCH_PIN) {
            client.publish("buttons/bedroom_porch", "1");
          } else if (buttonPins[i] == BUTTON_BED_LEFT_PIN) {
            client.publish("buttons/bed_left", "1");
          } else if (buttonPins[i] == BUTTON_BED_RIGHT_PIN) {
            client.publish("buttons/bed_right", "1");
          } else if (buttonPins[i] == BUTTON_UPPER_1_PIN) {
            client.publish("buttons/upper_1", "1");
          } else if (buttonPins[i] == BUTTON_UPPER_2_PIN) {
            client.publish("buttons/upper_2", "1");
          } else if (buttonPins[i] == BUTTON_ENTRY_PIN) {
            client.publish("buttons/entry", "1");
          } else if (buttonPins[i] == BUTTON_LAUNDRY_PIN) {
            client.publish("buttons/laundry", "1");
          }
        }
      }
    }
    lastButtonStates[i] = reading;
  }
}

void publishSwitchesStatus() {
  byte message[12] = { 
    byte(relayRoomPorchState == LOW), 
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
    byte(relayRecreationState == LOW)
  };
  
  boolean pubresult = client.publish("switches/status", message, 12);
  if (!pubresult) {
    Serial.println("unsuccessfully sent switches/status");
  }
}

void publishSensorsStatus() {  
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (!isnan(temperature) && !isnan(humidity)) {
    char outstr[15];
    
    dtostrf(temperature, 5, 1, outstr);
    boolean pubTemperatureResult = client.publish("sensors/temperature", outstr);
    if (!pubTemperatureResult) {
      Serial.println("unsuccessfully sent sensors/temperature");
    }
      
    dtostrf(humidity, 5, 1, outstr);
    boolean pubHumidityResult = client.publish("sensors/humidity", outstr);
    if (!pubHumidityResult) {
      Serial.println("unsuccessfully sent sensors/humidity");
    } 
  } else {
    Serial.println("Failed to read from DHT");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  int outputPin = -1;
  int newState = payload[0] == '1' ? LOW : HIGH;
  
  if (strcmp(topic, "relays/room_porch/set") == 0) {
    outputPin = RELAY_ROOM_PORCH_PIN;
    relayRoomPorchState = newState;
  } else if (strcmp(topic, "relays/room/set") == 0) {
    outputPin = RELAY_ROOM_PIN;
    relayRoomState = newState;
  } else if (strcmp(topic, "relays/counter/set") == 0) {
    outputPin = RELAY_COUNTER_PIN;
    relayCounterState = newState;
  } else if (strcmp(topic, "relays/kitchen/set") == 0) {
    outputPin = RELAY_KITCHEN_PIN;
    relayKitchenState = newState;
  } else if (strcmp(topic, "relays/bathroom/set") == 0) {
    outputPin = RELAY_BATHROOM_PIN;
    relayBathroomState = newState;
  } else if (strcmp(topic, "relays/corridor/set") == 0) {
    outputPin = RELAY_CORRIDOR_PIN;
    relayCorridorState = newState;
  } else if (strcmp(topic, "relays/entry/set") == 0) {
    outputPin = RELAY_ENTRY_PIN;
    relayEntryState = newState;
  } else if (strcmp(topic, "relays/bedroom/set") == 0) {
    outputPin = RELAY_BEDROOM_PIN;
    relayBedroomState = newState;
  } else if (strcmp(topic, "relays/bedroom_porch/set") == 0) {
    outputPin = RELAY_BEDROOM_PORCH_PIN;
    relayBedroomPorchState = newState;
  } else if (strcmp(topic, "relays/laundry/set") == 0) {
    outputPin = RELAY_LAUNDRY_PIN;
    relayLaundryState = newState;
  } else if (strcmp(topic, "relays/upper/set") == 0) {
    outputPin = RELAY_UPPER_PIN;
    relayUpperState = newState;
  } else if (strcmp(topic, "relays/recreation/set") == 0) {
    outputPin = RELAY_RECREATION_PIN;
    relayRecreationState = newState;
  }

  if (outputPin >= 0) {
    digitalWrite(outputPin, newState);
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
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}


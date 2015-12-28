#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

const unsigned long half_hour = 5000UL;//1800000UL;
float tempC = 0.0;
float humidity = 0.0;

const int buttonPin = 2;    // the number of the pushbutton pin
const int ledPin = 13;      // the number of the LED pin
const char macstr[] = "deadbeeffeed";
const String clientName = String("arduino:") + macstr;
const String topicName = String("status/fmt/json");

IPAddress ip(192, 168, 0, 120);
IPAddress server(192, 168, 0, 102);
EthernetClient ethClient;
PubSubClient client(ethClient);

int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  Serial.begin(57600);

  client.setServer(server, 1883);
  client.setCallback(callback);
  
  Ethernet.begin(mac, ip);

  dht.begin();

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // set initial LED state
  digitalWrite(ledPin, ledState);

  delay(1500);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //publishStatus();
  
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      client.publish("switches/room", "touched");
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}

String buildJson() {
  String data = "{";
  data+="\n";
  data+="\"temperature\": ";
  data+=tempC;
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
  tempC = dht.readTemperature();
  if (!isnan(tempC) && !isnan(humidity)) {
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
    
  if (strcmp(topic, "lights/room1") == 0) {
    Serial.println("Mensagem para luz room1");
    if (payload[0] == '1') {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    char clientStr[34];
    clientName.toCharArray(clientStr,34);
    if (client.connect(clientStr)) {
      Serial.println("connected");
      client.subscribe("lights/room1");
      client.subscribe("lights/room2");
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


#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <FastLED.h>

// Defining Pins
#define DHTPIN 12
#define LED 26
#define SERVO_PIN 2
#define LED_PIN 4
#define NUM_LEDS 16

#define DHTTYPE DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

Servo servo;
CRGB leds[NUM_LEDS];

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqttServer = "broker.hivemq.com";
const char* clientID = "ujaisldaaasdfgh;laslksdja1";
const char* topic = "Tempdata";

unsigned long previousMillis = 0;
const long interval = 1000;
String msgStr = "";
float temp, hum;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(clientID)) {
      Serial.println("MQTT connected");
      client.subscribe("lights");
      client.subscribe("servo");
      client.subscribe("lights/neopixel");
      Serial.println("Topic Subscribed");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String data = "";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "lights") {
    digitalWrite(LED, data == "ON" ? HIGH : LOW);
  }
  else if (String(topic) == "servo") {
    int degree = data.toInt();
    servo.write(degree);
  }
  else if (String(topic) == "lights/neopixel") {
    int red, green, blue;
    sscanf(data.c_str(), "%d,%d,%d", &red, &green, &blue);
    fill_solid(leds, NUM_LEDS, CRGB(red, green, blue));
    FastLED.show();
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  servo.attach(SERVO_PIN, 500, 2400);
  servo.write(0);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  setup_wifi();
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (!isnan(event.temperature)) temp = event.temperature;
    dht.humidity().getEvent(&event);
    if (!isnan(event.relative_humidity)) hum = event.relative_humidity;

    msgStr = String(temp) + "," + String(hum) + ",";
    byte arrSize = msgStr.length() + 1;
    char msg[arrSize];
    msgStr.toCharArray(msg, arrSize);
    client.publish(topic, msg);
    msgStr = "";
  }
}
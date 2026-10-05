#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#ifndef LAB_ENABLE_AWS
#define LAB_ENABLE_AWS 1
#endif

#if LAB_ENABLE_AWS
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define WIFI_IDENTITY WIFI_USERNAME "@mymail.sutd.edu.sg"
#endif

constexpr uint8_t ONE_WIRE_PIN = 22;
constexpr uint8_t OLED_SDA_PIN = 4;
constexpr uint8_t OLED_SCL_PIN = 5;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr unsigned long SENSOR_INTERVAL_MS = 2000;
constexpr uint8_t TEMPERATURE_HISTORY_SIZE = 6;
#if LAB_ENABLE_AWS
constexpr uint16_t MQTT_PORT = 8883;
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000;

const char AWS_IOT_PUBLISH_TOPIC[] = "esp32/pub";
const char AWS_IOT_SUBSCRIBE_TOPIC[] = "esp32/sub";
#endif

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#if LAB_ENABLE_AWS
WiFiClientSecure net;
PubSubClient client(net);
#endif

unsigned long lastSensorRead = 0;
#if LAB_ENABLE_AWS
unsigned long lastReconnectAttempt = 0;
unsigned long lastWiFiAttempt = 0;
#endif
float lastTemperature = DEVICE_DISCONNECTED_C;
float temperatureHistory[TEMPERATURE_HISTORY_SIZE];
uint8_t historyCount = 0;
unsigned long sampleCount = 0;
bool displayReady = false;

#if LAB_ENABLE_AWS
void messageHandler(char *topic, byte *payload, unsigned int length);
#endif

void showTemperature() {
  if (!displayReady) {
    return;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(lastTemperature == DEVICE_DISCONNECTED_C ?
                  "Sensor offline" : "Temperature / 2s");
  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
  if (historyCount == 0) {
    display.setCursor(0, 16);
    display.println("Waiting for sensor");
  }
  for (uint8_t index = 0; index < historyCount; ++index) {
    display.setCursor(0, 16 + index * 8);
    display.print(sampleCount - historyCount + index + 1);
    display.print(". ");
    display.print(temperatureHistory[index], 1);
    display.println(" C");
  }
  display.display();
}

#if LAB_ENABLE_AWS
void startWiFi() {
  Serial.print("Connecting to WPA2-Enterprise Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(
    WIFI_SSID,
    WPA2_AUTH_PEAP,
    WIFI_IDENTITY,
    WIFI_USERNAME,
    WIFI_PASSWORD
  );
  lastWiFiAttempt = millis();
  WiFi.setAutoReconnect(true);
}

void configureAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
  client.setCallback(messageHandler);
  client.setBufferSize(512);
  client.setKeepAlive(60);
  client.setSocketTimeout(5);
}

bool connectAWS() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  if (client.connected()) {
    return true;
  }

  Serial.println("Connecting to AWS IoT...");
  if (client.connect(THINGNAME)) {
    if (!client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC)) {
      Serial.println("Connected, but MQTT subscription failed.");
      client.disconnect();
      return false;
    }

    Serial.println("AWS IoT connected.");
    return true;
  }

  Serial.print("AWS IoT connection failed; MQTT state: ");
  Serial.println(client.state());
  return false;
}

void publishTemperature(float temperature) {
  if (!client.connected()) {
    Serial.println("Measurement not published: AWS IoT is disconnected.");
    return;
  }

  StaticJsonDocument<192> document;
  document["device"] = "ESP32-temp";
  document["temperature_celsius"] = temperature;
  document["temperature_fahrenheit"] = temperature * 1.8f + 32.0f;
  document["timestamp"] = millis() / 1000UL;

  char payload[192];
  const size_t length = serializeJson(document, payload, sizeof(payload));

  if (client.publish(AWS_IOT_PUBLISH_TOPIC,
                     reinterpret_cast<const uint8_t *>(payload),
                     length)) {
    Serial.print("Published: ");
    Serial.println(payload);
  } else {
    Serial.println("MQTT publish failed.");
  }
}
#endif

void readTemperature() {
  sensors.requestTemperatures();
  const float temperature = sensors.getTempCByIndex(0);
  lastTemperature = temperature;
  if (temperature != DEVICE_DISCONNECTED_C) {
    if (historyCount < TEMPERATURE_HISTORY_SIZE) {
      temperatureHistory[historyCount++] = temperature;
    } else {
      for (uint8_t index = 1; index < TEMPERATURE_HISTORY_SIZE; ++index) {
        temperatureHistory[index - 1] = temperatureHistory[index];
      }
      temperatureHistory[TEMPERATURE_HISTORY_SIZE - 1] = temperature;
    }
    ++sampleCount;
  }
  showTemperature();

  if (temperature == DEVICE_DISCONNECTED_C) {
    Serial.println("Failed to read from DS18B20 sensor.");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");

#if LAB_ENABLE_AWS
  publishTemperature(temperature);
#endif
}

#if LAB_ENABLE_AWS
void messageHandler(char *topic, byte *payload, unsigned int length) {
  Serial.print("Incoming MQTT message on ");
  Serial.println(topic);

  StaticJsonDocument<200> document;
  const DeserializationError error =
    deserializeJson(document, payload, length);

  if (error) {
    Serial.print("Invalid JSON: ");
    Serial.println(error.c_str());
    return;
  }

  const char *message = document["message"];
  if (message != nullptr) {
    Serial.println(message);
  } else {
    Serial.println("JSON does not contain a 'message' field.");
  }
}

void maintainConnections() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("Wi-Fi connection timed out; retrying.");
      startWiFi();
    }
    return;
  }

  if (client.connected()) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL_MS) {
    return;
  }
  lastReconnectAttempt = now;
  connectAWS();
}
#endif

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  displayReady = display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  if (displayReady) {
    display.setTextColor(SSD1306_WHITE);
  } else {
    Serial.println("OLED initialization failed.");
  }
  sensors.begin();
  Serial.print("OneWire GPIO: ");
  Serial.println(ONE_WIRE_PIN);
  Serial.print("OneWire devices found: ");
  Serial.println(sensors.getDeviceCount());
#if LAB_ENABLE_AWS
  configureAWS();
  startWiFi();
#endif
  lastSensorRead = millis() - SENSOR_INTERVAL_MS;
}

void loop() {
#if LAB_ENABLE_AWS
  maintainConnections();

  if (client.connected()) {
    client.loop();
  }
#endif

  const unsigned long now = millis();
  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    readTemperature();
  }

  delay(10);
}


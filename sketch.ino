#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

const char* MQTT_HOST = "3.135.234.101";
const int MQTT_PORT = 1883;

const char* DEVICE_ID = "device011";

char TOPIC_ATTRS[64];
char TOPIC_CMD[64];
char TOPIC_STATE[64];

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LDR_PIN 34
#define LED_PIN 2

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastPublish = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

const unsigned long PUBLISH_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_MS = 10000;
const unsigned long MQTT_RETRY_MS = 3000;

bool getTimestamp(char* buf, size_t len) {
  time_t now = time(nullptr);

  if (now < 1600000000) {
    return false;
  }

  struct tm t;
  gmtime_r(&now, &t);
  strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &t);

  return true;
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed.");
  }

  lastWiFiAttempt = millis();
}

void onMessage(char* topic, byte* payload,
               unsigned int length) {
  String msg;

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();
  msg.toLowerCase();

  Serial.print("Command received: ");
  Serial.println(msg);

  if (msg == "on") {
    digitalWrite(LED_PIN, HIGH);
  } else if (msg == "off") {
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.println("Use on or off.");
    return;
  }

  mqtt.publish(
    TOPIC_STATE,
    digitalRead(LED_PIN) ? "on" : "off"
  );
}

void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED || mqtt.connected()) {
    return;
  }

  if (millis() - lastMQTTAttempt < MQTT_RETRY_MS) {
    return;
  }

  lastMQTTAttempt = millis();

  Serial.println("Connecting to MQTT broker...");

  String clientId = String("esp32-") + DEVICE_ID +
                    "-" + String((uint32_t)esp_random(), HEX);

  if (mqtt.connect(clientId.c_str())) {
    Serial.println("MQTT connected!");

    mqtt.subscribe(TOPIC_CMD);

    Serial.print("Subscribed to: ");
    Serial.println(TOPIC_CMD);
  } else {
    Serial.print("MQTT connection failed. Code: ");
    Serial.println(mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  analogReadResolution(12);
  dht.begin();

  snprintf(TOPIC_ATTRS, sizeof(TOPIC_ATTRS),
           "/TEF/%s/attrs", DEVICE_ID);

  snprintf(TOPIC_CMD, sizeof(TOPIC_CMD),
           "/TEF/%s/cmd", DEVICE_ID);

  snprintf(TOPIC_STATE, sizeof(TOPIC_STATE),
           "/TEF/%s/state", DEVICE_ID);

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);

  connectWiFi();

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWiFiAttempt >= WIFI_RETRY_MS) {
      connectWiFi();
    }
    return;
  }

  if (!mqtt.connected()) {
    connectMQTT();
  } else {
    mqtt.loop();
  }

  if (mqtt.connected() &&
      now - lastPublish >= PUBLISH_INTERVAL_MS) {

    lastPublish = now;

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int ldrRaw = analogRead(LDR_PIN);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT22 read failed.");
      return;
    }

    char ts[25];

    if (!getTimestamp(ts, sizeof(ts))) {
      strcpy(ts, "sem-hora");
    }

    char payload[200];

    snprintf(payload, sizeof(payload),
      "{\"temperature\":%.1f,\"humidity\":%.1f,"
      "\"luminosity\":%d,\"timestamp\":\"%s\"}",
      temperature, humidity, ldrRaw, ts);

    bool sent = mqtt.publish(TOPIC_ATTRS, payload);

    Serial.print(sent ? "Published to " : "Publish failed: ");
    Serial.print(TOPIC_ATTRS);
    Serial.print(" : ");
    Serial.println(payload);
  }
}
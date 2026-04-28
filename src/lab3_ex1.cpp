// - RED LED - D26
// - Green LED - D27
// - Blue LED - D14
// - Yellow LED - D12

// - Button (Active high) - D25
// - Light sensor (analog) - D33

// - LCD I2C - SDA: D21
// - LCD I2C - SCL: D22

/**************************************
 * LAB 3 - EXERCISE 1: 
 * 
 * I want to publish message to mqtt every 5 second
 **************************************/


#include "Arduino.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "time.h"

// Pin definitions
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27
#define BLUE_LED_PIN 14
#define YELLOW_LED_PIN 12
#define BUTTON_PIN 25
#define LIGHT_PIN 33

// WiFi credentials
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// MQTT credentials
// const char* MQTT_BROKER = "mqtt.iotserver.uz";
const char* MQTT_BROKER = "167.86.76.145";
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "userTTPU";
const char* MQTT_PASS = "mqttpass";
const char* MQTT_CLIENT_ID = "xakimjonov_lab3_ex1";

// MQTT topics
const char* TOPIC_LIGHT  = "ttpu/iot/xakimjonov/sensors/light";
const char* TOPIC_BUTTON = "ttpu/iot/xakimjonov/events/button";

// MQTT objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Timing
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000;   // 5 seconds

// Button state
bool lastButtonState = 0;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;


/*************************
 * Connect to WiFi
 */
void connectWiFi()
{
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
}


/*************************
 * Connect to MQTT broker
 */
void connectMQTT()
{
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT...");

        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
        {
            Serial.println(" connected!");
        }
        else
        {
            Serial.print(" failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 2s");
            delay(2000);
        }
    }
}


/*************************
 * Publish light sensor reading
 */
void publishLight()
{
    int rawLight = analogRead(LIGHT_PIN);
    unsigned long timestamp = time(nullptr);

    // Build JSON
    JsonDocument doc;
    doc["light"] = rawLight;
    doc["timestamp"] = timestamp;

    char buffer[128];
    serializeJson(doc, buffer);

    mqttClient.publish(TOPIC_LIGHT, buffer);

    Serial.print("[PUB] ");
    Serial.print(TOPIC_LIGHT);
    Serial.print(" -> ");
    Serial.println(buffer);
}


/*************************
 * Publish button event (PRESSED / RELEASED)
 */
void publishButton(const char* eventName)
{
    unsigned long timestamp = time(nullptr);

    JsonDocument doc;
    doc["event"] = eventName;
    doc["timestamp"] = timestamp;

    char buffer[128];
    serializeJson(doc, buffer);

    mqttClient.publish(TOPIC_BUTTON, buffer);

    Serial.print("[PUB] ");
    Serial.print(TOPIC_BUTTON);
    Serial.print(" -> ");
    Serial.println(buffer);
}


/*************************
 * Handle button with debounce
 */
void handleButton()
{
    bool reading = digitalRead(BUTTON_PIN);

    // If reading changed, reset debounce timer
    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    // After debounce delay, accept the new state
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        // Detect press (0 -> 1) and release (1 -> 0)
        static bool stableState = 0;

        if (reading != stableState)
        {
            stableState = reading;

            if (stableState == 1)
            {
                publishButton("PRESSED");
            }
            else
            {
                publishButton("RELEASED");
            }
        }
    }

    lastButtonState = reading;
}

/*************************
 * SETUP
 */
void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- LAB 3 EX 1 ---");

    // LED pins (initialize even though not used here)
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    // Button + light sensor
    pinMode(BUTTON_PIN, INPUT);
    pinMode(LIGHT_PIN, INPUT);

    // Connect to WiFi and MQTT
    connectWiFi();
    connectMQTT();

    // Sync time (so timestamps are real Unix time)
    configTime(0, 0, "pool.ntp.org");
}


/*************************
 * LOOP
 */
void loop()
{
    // Keep MQTT connection alive
    if (!mqttClient.connected())
    {
        connectMQTT();
    }
    mqttClient.loop();

    // Periodic publishing every 5s
    unsigned long now = millis();
    if (now - lastPublish >= PUBLISH_INTERVAL)
    {
        lastPublish = now;
        publishLight();
    }

    // Event-based button publishing
    handleButton();
}
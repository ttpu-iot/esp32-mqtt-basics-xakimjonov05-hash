// - RED LED - D26
// - Green LED - D27
// - Blue LED - D14
// - Yellow LED - D12

// - Button (Active high) - D25
// - Light sensor (analog) - D33

// - LCD I2C - SDA: D21
// - LCD I2C - SCL: D22

/**************************************
 * LAB 3 - EXERCISE 2
 * 
 * Subscribe to MQTT and control LEDs
 **************************************/


#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"

// Pin definitions
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27
#define BLUE_LED_PIN 14
#define YELLOW_LED_PIN 12

// WiFi credentials
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// MQTT credentials
const char* MQTT_BROKER = "mqtt.iotserver.uz";
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "userTTPU";
const char* MQTT_PASS = "mqttpass";
const char* MQTT_CLIENT_ID = "xakimjonov_lab3_ex2";

// MQTT topics for LED control
const char* TOPIC_RED    = "ttpu/iot/xakimjonov/led/red";
const char* TOPIC_GREEN  = "ttpu/iot/xakimjonov/led/green";
const char* TOPIC_BLUE   = "ttpu/iot/xakimjonov/led/blue";
const char* TOPIC_YELLOW = "ttpu/iot/xakimjonov/led/yellow";

// MQTT objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


/*************************
 * Apply LED state based on parsed message
 */
void applyLedState(int pin, const char* colorName, const char* state)
{
    if (strcmp(state, "ON") == 0)
    {
        digitalWrite(pin, HIGH);
        Serial.print("[LED] ");
        Serial.print(colorName);
        Serial.println(" LED -> ON");
    }
    else if (strcmp(state, "OFF") == 0)
    {
        digitalWrite(pin, LOW);
        Serial.print("[LED] ");
        Serial.print(colorName);
        Serial.println(" LED -> OFF");
    }
    else
    {
        Serial.print("[LED] Unknown state: ");
        Serial.println(state);
    }
}


/*************************
 * MQTT message callback - runs whenever a subscribed message arrives
 */
void onMqttMessage(char* topic, byte* payload, unsigned int length)
{
    // Convert payload to a null-terminated string for printing
    char message[256];
    if (length >= sizeof(message)) length = sizeof(message) - 1;
    memcpy(message, payload, length);
    message[length] = '\0';

    Serial.print("[MQTT] Received on ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);

    // Parse JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);

    if (err)
    {
        Serial.print("[ERR] JSON parse failed: ");
        Serial.println(err.c_str());
        return;   // Don't crash, just ignore bad message
    }

    // Extract "state" field
    const char* state = doc["state"];
    if (state == nullptr)
    {
        Serial.println("[ERR] No 'state' field in JSON");
        return;
    }

    // Decide which LED to control based on topic
    if (strcmp(topic, TOPIC_RED) == 0)
    {
        applyLedState(RED_LED_PIN, "Red", state);
    }
    else if (strcmp(topic, TOPIC_GREEN) == 0)
    {
        applyLedState(GREEN_LED_PIN, "Green", state);
    }
    else if (strcmp(topic, TOPIC_BLUE) == 0)
    {
        applyLedState(BLUE_LED_PIN, "Blue", state);
    }
    else if (strcmp(topic, TOPIC_YELLOW) == 0)
    {
        applyLedState(YELLOW_LED_PIN, "Yellow", state);
    }
}


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
 * Connect to MQTT broker and subscribe to topics
 */
void connectMQTT()
{
    while (!mqttClient.connected())
    {
        Serial.print("Connecting to MQTT...");

        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS))
        {
            Serial.println(" connected!");

// Subscribe to all 4 LED topics
            mqttClient.subscribe(TOPIC_RED);
            mqttClient.subscribe(TOPIC_GREEN);
            mqttClient.subscribe(TOPIC_BLUE);
            mqttClient.subscribe(TOPIC_YELLOW);

            Serial.println("[MQTT] Subscribed to all LED topics");
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
 * SETUP
 */
void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- LAB 3 EX 2 ---");

    // LED pins as OUTPUT
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);

    // All LEDs OFF initially
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Setup MQTT
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);

    // Connect
    connectWiFi();
    connectMQTT();
}


/*************************
 * LOOP
 */
void loop()
{
    // Keep MQTT alive and process incoming messages
    if (!mqttClient.connected())
    {
        connectMQTT();
    }
    mqttClient.loop();
}
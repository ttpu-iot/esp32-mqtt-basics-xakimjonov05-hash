// - RED LED - D26
// - Green LED - D27
// - Blue LED - D14
// - Yellow LED - D12

// - Button (Active high) - D25
// - Light sensor (analog) - D33

// - LCD I2C - SDA: D21
// - LCD I2C - SCL: D22

/**************************************
 * LAB 3 - EXERCISE 3
 * 
 * Bi-directional MQTT + LCD with Tashkent time
 **************************************/


#include "Arduino.h"
#include "WiFi.h"
#include <ArduinoJson.h>
#include "PubSubClient.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>

// Pin definitions
#define RED_LED_PIN 26
#define GREEN_LED_PIN 27
#define BLUE_LED_PIN 14
#define YELLOW_LED_PIN 12
#define BUTTON_PIN 25
#define LIGHT_PIN 33

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT credentials
const char* mqtt_server = "mqtt.iotserver.uz";
const int   mqtt_port = 1883;
const char* mqtt_user = "userTTPU";
const char* mqtt_pass = "mqttpass";
const char* client_id = "xakimjonov_lab3_ex3";

// Topics
const char* topic_button  = "ttpu/iot/xakimjonov/events/button";
const char* topic_red     = "ttpu/iot/xakimjonov/led/red";
const char* topic_green   = "ttpu/iot/xakimjonov/led/green";
const char* topic_blue    = "ttpu/iot/xakimjonov/led/blue";
const char* topic_yellow  = "ttpu/iot/xakimjonov/led/yellow";
const char* topic_display = "ttpu/iot/xakimjonov/display";

// MQTT objects
WiFiClient espClient;
PubSubClient client(espClient);

// LCD - I2C address 0x27, 16 cols, 2 rows
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Button debounce
bool lastButtonReading = 0;
bool stableButtonState = 0;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;


/*************************
 * Connect to WiFi
 */
void setup_wifi()
{
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}


/*************************
 * Sync time with NTP for Tashkent (UTC+5)
 */
void setup_time()
{
    Serial.print("Syncing time...");
    configTime(18000, 0, "pool.ntp.org");   // 18000 sec = 5 hours

    struct tm timeinfo;
    int retries = 0;
    while (!getLocalTime(&timeinfo) && retries < 20)
    {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (retries < 20)
    {
        Serial.println(" synced!");
        char buf[30];
        strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &timeinfo);
        Serial.print("Tashkent time: ");
        Serial.println(buf);
    }
    else
    {
        Serial.println(" FAILED");
    }
}


/*************************
 * Update LCD with text + current Tashkent timestamp
 */
void updateLCD(const char* text)
{
    // Truncate to 16 chars
    char line1[17];
    strncpy(line1, text, 16);
    line1[16] = '\0';

    // Get current Tashkent time
    struct tm timeinfo;
    char line2[17];
    if (getLocalTime(&timeinfo))
    {
        strftime(line2, sizeof(line2), "%d/%m %H:%M:%S", &timeinfo);
    }
    else
    {
        strcpy(line2, "no time");
    }

    // Clear and write
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);

    Serial.print("[LCD] L1: ");
    Serial.println(line1);
    Serial.print("[LCD] L2: ");
    Serial.println(line2);
}


/*************************
 * Apply LED state
 */
void applyLedState(int pin, const char* colorName, const char* state)
{
    if (strcmp(state, "ON") == 0)
    {
        digitalWrite(pin, HIGH);
        Serial.print("[LED] ");
        Serial.print(colorName);
        Serial.println(" -> ON");
    }
    else if (strcmp(state, "OFF") == 0)
    {
        digitalWrite(pin, LOW);
        Serial.print("[LED] ");
        Serial.print(colorName);
        Serial.println(" -> OFF");
    }
}

/*************************
 * MQTT message callback
 */
void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("[MQTT] Received on ");
    Serial.print(topic);
    Serial.print(": ");
    for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
    Serial.println();

    // Parse JSON directly from payload + length
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);

    if (error)
    {
        Serial.print("[ERROR] Invalid JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // LED topics -> use "state" field
    if (strcmp(topic, topic_red) == 0)
    {
        const char* state = doc["state"];
        if (state) applyLedState(RED_LED_PIN, "Red", state);
    }
    else if (strcmp(topic, topic_green) == 0)
    {
        const char* state = doc["state"];
        if (state) applyLedState(GREEN_LED_PIN, "Green", state);
    }
    else if (strcmp(topic, topic_blue) == 0)
    {
        const char* state = doc["state"];
        if (state) applyLedState(BLUE_LED_PIN, "Blue", state);
    }
    else if (strcmp(topic, topic_yellow) == 0)
    {
        const char* state = doc["state"];
        if (state) applyLedState(YELLOW_LED_PIN, "Yellow", state);
    }
    // Display topic -> use "text" field
    else if (strcmp(topic, topic_display) == 0)
    {
        const char* text = doc["text"];
        if (text)
        {
            updateLCD(text);
        }
        else
        {
            Serial.println("[ERROR] No 'text' field");
        }
    }
}


/*************************
 * Connect / reconnect to MQTT
 */
void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Connecting to MQTT...");

        if (client.connect(client_id, mqtt_user, mqtt_pass))
        {
            Serial.println("connected!");

            // Subscribe to LED topics
            client.subscribe(topic_red);
            client.subscribe(topic_green);
            client.subscribe(topic_blue);
            client.subscribe(topic_yellow);
            // Subscribe to display topic
            client.subscribe(topic_display);

            Serial.println("Subscribed to all topics");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 2s");
            delay(2000);
        }
    }
}


/*************************
 * Publish button event
 */
void publishButton(const char* eventName)
{
    unsigned long timestamp = time(nullptr);

    JsonDocument doc;
    doc["event"] = eventName;
    doc["timestamp"] = timestamp;

    char buffer[128];
    serializeJson(doc, buffer);

    client.publish(topic_button, buffer);

    Serial.print("[PUB] ");
    Serial.print(topic_button);
    Serial.print(" -> ");
    Serial.println(buffer);
}


/*************************
 * Handle button with debounce
 */
void handleButton()
{
    bool reading = digitalRead(BUTTON_PIN);

    if (reading != lastButtonReading)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        if (reading != stableButtonState)
        {
            stableButtonState = reading;

            if (stableButtonState == 1)
                publishButton("PRESSED");
            else
                publishButton("RELEASED");
        }
    }

    lastButtonReading = reading;
}


/*************************
 * SETUP
 */
void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- LAB 3 EX 3 ---");

    // LED pins
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Button + light sensor
    pinMode(BUTTON_PIN, INPUT);
    pinMode(LIGHT_PIN, INPUT);

  // LCD init
    Wire.begin(21, 22);   // SDA=21, SCL=22
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Starting...");

    // WiFi + time + MQTT
    setup_wifi();
    setup_time();

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // Show ready on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready!");
    lcd.setCursor(0, 1);
    lcd.print("Waiting msg...");
}


/*************************
 * LOOP
 */
void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    handleButton();
}
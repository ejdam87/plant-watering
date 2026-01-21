#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "webpage.hpp"
#include "credentials.hpp"

static const char* HOSTNAME = "plant-watering";

// --- Water Pump
// Pins (L293D) - motor driver
#define ENABLE_PIN 25 //45
#define IN_1_PIN 33 //48
#define IN_2_PIN 26 //47

// pump control
int pump_speed = 255; // 0-255
bool pump_turned_on = false;
bool pump_cycle_active = false;
unsigned long pump_cycle_start = 0;
unsigned long pump_cycle_duration = 0; // ms
bool pump_turned_on_auto = false;
bool pump_cycle_active_auto = false;
unsigned long pump_cycle_start_auto = 0;
unsigned long pump_cycle_duration_auto = 0; // ms
unsigned long pump_cycle_last_watering = 0;

void pump_turn_on()
{
  digitalWrite(IN_1_PIN, 1);
  digitalWrite(IN_2_PIN, 0);
  pump_turned_on = true;
}

void pump_turn_off()
{
  digitalWrite(IN_1_PIN, 0);
  digitalWrite(IN_2_PIN, 0);
  pump_turned_on = false;
}

void set_pump_speed(int speed)
{
  pump_speed = constrain(speed, 0, 255);
  analogWrite(ENABLE_PIN, pump_speed);
}
// ---


// Humidity Sensor (analog pin)
#define HUMIDITY_PIN 35 //5
const int raw_dry = 4095; // sensor on air
const int raw_wet = 1400; // sensor in water
float smoothed_humidity = 0;
float alpha = 0.7; // smoothing factor (0.0–1.0)
int humidity_threshold = 20;

// --- Water Level Sensor
#define WATER_LEVEL_PIN 14 //16
// ---

WebServer server(80);

void handle_root()
{
  server.send_P(200, "text/html", index_html);
}

// API endpoint to get sensor readings
void handle_get_sensors()
{
  int humidity_value = analogRead(HUMIDITY_PIN);
  float humidity_percent = (float)(raw_dry - humidity_value) / (raw_dry - raw_wet) * 100.0;
  humidity_percent = constrain(humidity_percent, 0, 100);
  smoothed_humidity = smoothed_humidity * (1 - alpha) + humidity_percent * alpha;
  bool water_level_state = digitalRead(WATER_LEVEL_PIN);

  String json = "{";
  json += "\"humidity\":" + String(smoothed_humidity) + ",";
  json += "\"water_level\":" + String(water_level_state ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// API endpoint to get pump state
void handle_get_pump()
{
  String json = "{";
  json += "\"running\":" + (pump_turned_on ? String("true") : String("false")) + ",";
  json += "\"speed\":" + String(pump_speed);
  json += "}";

  server.send(200, "application/json", json);
}

// API endpoint to start a pump cycle
void handle_pump_cycle()
{
  if (!server.hasArg("duration"))
  {
    server.send(400, "application/json",
                "{\"error\":\"missing duration\"}");
    return;
  }

  int duration_sec = server.arg("duration").toInt();
  if (duration_sec <= 0)
  {
    server.send(400, "application/json",
                "{\"error\":\"invalid duration\"}");
    return;
  }

  if (pump_cycle_active)
  {
    server.send(409, "application/json",
                "{\"error\":\"pump already running\"}");
    return;
  }

  pump_cycle_duration = (unsigned long)duration_sec * 1000UL;
  pump_cycle_start = millis();
  pump_cycle_active = true;

  pump_turn_on();

  String json = "{";
  json += "\"status\":\"started\",";
  json += "\"duration\":" + String(duration_sec);
  json += "}";

  server.send(200, "application/json", json);
}

// API endpoint to set pump speed
void handle_post_pump_speed()
{
  if (server.hasArg("speed"))
  {
    int speed = server.arg("speed").toInt();
    set_pump_speed(speed);
  }

  String json = "{";
  json += "\"speed\":" + String(pump_speed);
  json += "}";

  server.send(200, "application/json", json);
}

void handle_get_humidity_threshold()
{
  String json = "{";
  json += "\"threshold\":" + String(humidity_threshold);
  json += "}";
  server.send(200, "application/json", json);
}

void handle_post_humidity_threshold()
{
  if (!server.hasArg("threshold"))
  {
    server.send(400, "application/json",
                "{\"error\":\"missing threshold\"}");
    return;
  }

  int threshold = server.arg("threshold").toInt();
  humidity_threshold = constrain(threshold, 0, 100);

  String json = "{";
  json += "\"threshold\":" + String(humidity_threshold);
  json += "}";
  server.send(200, "application/json", json);
}
void setup()
{
  Serial.begin(115200);

  // Pump Setup
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(IN_1_PIN, OUTPUT);
  pinMode(IN_2_PIN, OUTPUT);
  analogWrite(ENABLE_PIN, 255);
  pump_turn_off();

  delay(500);

  // Humidity setup
  pinMode(HUMIDITY_PIN, INPUT);

  // Water Level setup
  pinMode(WATER_LEVEL_PIN, INPUT);

  // Web server setup
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(BRNO_SSID, BRNO_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // server accessible on http://plant-watering.local/
  if (!MDNS.begin(HOSTNAME))
  {
    Serial.println("mDNS setup failed");
  }
  else
  {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS name: http://");
    Serial.print(HOSTNAME);
    Serial.println(".local/");
  }

  server.on("/", handle_root);
  server.on("/api/sensors", HTTP_GET, handle_get_sensors);
  server.on("/api/pump", HTTP_GET, handle_get_pump);
  server.on("/api/pump", HTTP_POST, handle_pump_cycle);
  server.on("/api/pump/speed", HTTP_POST, handle_post_pump_speed);
  server.on("/api/pump/threshold", HTTP_GET, handle_get_humidity_threshold);
  server.on("/api/pump/threshold", HTTP_POST, handle_post_humidity_threshold);

  server.begin();
  Serial.println("Server started");
  // ---
}

void loop()
{
  server.handleClient();

  if (pump_cycle_active && millis() - pump_cycle_start >= pump_cycle_duration)
  {
    pump_turn_off();
    pump_cycle_active = false;
    Serial.println("Manual watering cycle completed");
  }

  if (smoothed_humidity < humidity_threshold && millis() - pump_cycle_last_watering > 10000) // 30 minutes
  {
    Serial.println("Automatic watering commenced!");
    pump_cycle_duration_auto = 5000UL;
    pump_cycle_start_auto = millis();
    pump_cycle_active_auto = true;
    pump_turn_on();
    pump_cycle_last_watering = millis();
  }

  if (pump_cycle_active_auto && millis() - pump_cycle_start_auto >= pump_cycle_duration_auto)
  {
    Serial.println("Turning auto watering off");
    pump_turn_off();
    pump_cycle_active_auto = false;
    Serial.println("Automatic watering cycle completed");
  }
  delay(1);
}

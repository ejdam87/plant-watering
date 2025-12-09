#include <WiFi.h>
#include <WebServer.h>

#include "webpage.hpp"

// --- Water Pump
// Pins (L293D) - motor driver
#define ENABLE_PIN 25
#define IN_1_PIN 33
#define IN_2_PIN 26

// pump control
bool pump_turned_on = false;
int pump_speed = 255; // 0-255

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
#define HUMIDITY_PIN 35

// --- Water Level Sensor
#define WATER_LEVEL_PIN 34
unsigned long water_level_trigger_time = 0;
volatile bool water_level_detected = false;

void IRAM_ATTR handle_water_level()
{
  Serial.println("Water Level Detected!");
  water_level_trigger_time = millis();
  water_level_detected = true;
}
// ---

// Web server
const char* HOTSTOP_SSID = "Dzadam";
const char* HOTSPOT_PASSWORD = "mamradpepe";


const char* FACULTY_SSID = "PV284";
const char* FACULTY_PASSWORD = "Che6GoozIeTe";


WebServer server(80);

void handle_root()
{
  server.send_P(200, "text/html", index_html);
}

// API endpoint to get sensor readings
void handle_get_sensors()
{
  int humidity_value = analogRead(HUMIDITY_PIN);
  Serial.println(humidity_value);

  bool water_level_state = digitalRead(WATER_LEVEL_PIN);
  unsigned long time_since_trigger = millis() - water_level_trigger_time;

  String json = "{";
  json += "\"humidity\":" + String(humidity_value) + ",";
  json += "\"water_level\":" + String(water_level_state ? "true" : "false") + ",";
  json += "\"water_level_time\":" + String(time_since_trigger);
  json += "}";

  server.send(200, "application/json", json);
}

// API endpoint to get pump state
void handle_get_pump()
{
  String json = "{";
  json += "\"state\":" + (pump_turned_on ? String("true") : String("false")) + ",";
  json += "\"speed\":" + String(pump_speed);
  json += "}";
  
  server.send(200, "application/json", json);
}

// API endpoint to control pump
void handle_post_pump()
{
  Serial.println("Pump control request received");
  
  if (server.hasArg("state"))
  {
    String stateStr = server.arg("state");
    Serial.print("State argument: ");
    Serial.println(stateStr);
    
    bool state = (stateStr == "true" || stateStr == "1");
    
    Serial.print("Setting motor to: ");
    Serial.println(state ? "ON" : "OFF");
    
    if (state)
    {
      pump_turn_on();
      Serial.println("Pump turned ON");
    }
    else
    {
      pump_turn_off();
      Serial.println("Pump turned OFF");
    }
  }
  else
  {
    Serial.println("No state argument provided");
  }
  
  String json = "{";
  json += "\"state\":" + (pump_turned_on ? String("true") : String("false")) + ",";
  json += "\"speed\":" + String(pump_speed);
  json += "}";
  
  Serial.print("Sending response: ");
  Serial.println(json);
  
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
  attachInterrupt(digitalPinToInterrupt(WATER_LEVEL_PIN), handle_water_level, CHANGE);

  // Web server setup
  WiFi.begin(FACULTY_SSID, FACULTY_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_root);
  server.on("/api/sensors", HTTP_GET, handle_get_sensors);
  server.on("/api/pump", HTTP_GET, handle_get_pump);
  server.on("/api/pump", HTTP_POST, handle_post_pump);
  server.on("/api/pump/speed", HTTP_POST, handle_post_pump_speed);

  server.begin();
  Serial.println("Server started");
  // ---
}

void loop()
{
  server.handleClient();
  delay(1);
}

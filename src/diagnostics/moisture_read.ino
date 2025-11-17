// Humidity sensor
#define HUMIDITY_PIN 14

void setup()
{
    pinMode(HUMIDITY_PIN, INPUT);
    Serial.begin(115200);
}

void loop()
{
    auto humidity = analogRead(HUMIDITY_PIN);
    Serial.println(humidity);
}

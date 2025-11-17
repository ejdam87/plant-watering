// motor driver pins (L293D)
#define ENABLE_PIN 33
#define IN_1_PIN 25
#define IN_2_PIN 26


void motor_turn_on()
{
    digitalWrite(IN_1_PIN, 1);
    digitalWrite(IN_2_PIN, 0);
}

void motor_turn_off()
{
    digitalWrite(IN_1_PIN, 0);
    digitalWrite(IN_2_PIN, 0);
}

void setup()
{
    pinMode(ENABLE_PIN, OUTPUT);
    pinMode(IN_1_PIN, OUTPUT);
    pinMode(IN_2_PIN, OUTPUT);
    analogWrite(ENABLE_PIN, 255);
    motor_turn_off();

    Serial.begin(115200);
}

void loop(){}

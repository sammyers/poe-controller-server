const int WIRE_PIN = 3;
const long BAUD_RATE = 115200;

int state = LOW;

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WIRE_PIN, OUTPUT);
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    Serial.println(command);
    if (command == "ON") {
      state = HIGH;
    } else if (command == "OFF") {
      state = LOW;
    }
  }
  digitalWrite(WIRE_PIN, state);
  digitalWrite(LED_BUILTIN, state);
}

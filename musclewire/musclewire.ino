const int WIRE_PIN = 3;
const int SENSOR_PIN = A0;
const int LED_PIN = 9;
const long BAUD_RATE = 115200;

const int pulseInterval = 6500;
const int pwmPulseInterval = pulseInterval / 100;
const int sensorThreshold = 350;

// lanternMode:  | lightMode:   | pulseSpeed
//   0: off      |   0: off     |   0: 1
//   1: on       |   1: on      |   1: 2
//   2: pulse    |   2: pulse   |   2: 3
//   3: sensory  |   3: mirror  |   3: 4


struct LanternState {
  byte lanternMode;
  byte lightMode;
  byte pulseSpeed;
};

unsigned long previousMillis = 0;
int wireState = LOW;
LanternState lanternState = { 0, 0, 0 };

int pulseState = LOW;
int pwmPulseState = 0;

/*
 * Calls provided function with the provided `data` argument if a given
 * interval (in milliseconds) has elapsed.
 */
void executeIfTimeElapsed(void (*callback)(void *), int interval, void *data) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    callback(data);
  }
}

LanternState parseCommand(byte optBitField) {
  byte pulseSpeed = (optBitField >> 4) & B11;
  byte lightMode = (optBitField >> 2) & B11;
  byte lanternMode = optBitField & B11;
  return (LanternState) { lanternMode, lightMode, pulseSpeed };
}

byte encodeState(LanternState state) {
 return (state.pulseSpeed << 4) | (state.lightMode << 2) | state.lanternMode;
}

void writeStatus() {
  Serial.write(encodeState(lanternState));
}

void toggleLantern() {
  updateLightPWM();
  digitalWrite(WIRE_PIN, pulseState);
}

void pulseLantern() {
  executeIfTimeElapsed(toggleLantern, pwmPulseInterval, NULL);
}

void updateLightPWM() {
  if (pwmPulseState == 99) {
    pulseState = LOW;
  } else if (pwmPulseState == 0) {
    pulseState = HIGH;
  }
  if (pulseState == LOW) {
    pwmPulseState--;
  } else if (pulseState == HIGH) {
    pwmPulseState++;
  }
}

void sensorUpdate() {

}

void pulseLight() {
  executeIfTimeElapsed(updateLightPWM, pwmPulseInterval, NULL);
  analogWrite(LED_PIN, map(pwmPulseState, 0, 99, 0, 255));
}

void updateLight() {
  switch (lanternState.lightMode) {
    case 0:
      analogWrite(LED_PIN, 0);
      break;
    case 1:
      analogWrite(LED_PIN, 255);
      break;
    case 2:
      pulseLight();
      break;
    case 3:
      switch (lanternState.lanternMode) {
        case 0:
          analogWrite(LED_PIN, 0);
          break;
        case 1:
          analogWrite(LED_PIN, 255);
          break;
        case 2:
          pulseLight();
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void updateLantern() {
  switch (lanternState.lanternMode) {
    case 0:
      digitalWrite(WIRE_PIN, LOW);
      break;
    case 1:
      digitalWrite(WIRE_PIN, HIGH);
      break;
    case 2:
      pulseLantern();
      break;
    case 3:
      sensorUpdate();
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(WIRE_PIN, OUTPUT);
}

void loop() {
  if (Serial.available() > 0) {
    int opts = Serial.read();
    // Check if first bit is 1, if so just return current status
    if (!(opts >> 7)) {
      LanternState command = parseCommand(opts);
      bool newLanternMode = lanternState.lanternMode != command.lanternMode;
      bool newLightMode = lanternState.lightMode != command.lightMode;
      bool lanternPulse = command.lanternMode == 2;
      bool lightPulse = command.lightMode == 2;
      bool lightMirror = command.lightMode == 3;
      if (newLanternMode && lanternPulse && !lightPulse) {
        pwmPulseState = 0;
        pulseState = LOW;
      }
      if (newLightMode && lightPulse && !lanternPulse) {
        pwmPulseState = 0;
        pulseState = LOW;
      }
      if (command.lanternMode == 3) {
        command.lightMode = 3;
      }
      lanternState = command;
    }
    writeStatus();
  }
  updateLantern();
  updateLight();
}

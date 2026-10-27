const int BUTTON_PIN = 9; // button connects pin 9 (chip pin 15) to GND
const int LED1 = 8; // chip pin 14
const int LED2 = 7; // chip pin 13
const int LED3 = 6; // chip pin 12

const int ON_TIME = 300; // how long each LED stays on (ms)
const int OFF_TIME = 100; // gap after each LED turns off (ms)

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // HIGH until pressed
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) // button pressed: run one full cycle
  { 
    digitalWrite(LED1, HIGH);
    delay(ON_TIME);
    digitalWrite(LED1, LOW);
    delay(OFF_TIME);

    digitalWrite(LED2, HIGH);
    delay(ON_TIME);
    digitalWrite(LED2, LOW);
    delay(OFF_TIME);

    digitalWrite(LED3, HIGH);
    delay(ON_TIME);
    digitalWrite(LED3, LOW);
    delay(OFF_TIME);
  }
  // the button is only checked between cycles, so a cycle always finishes
}

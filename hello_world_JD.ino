// microcontroller hello world - Mirelle Lozano
//Thomas Mazur

const int BUTTON_PIN = 9;   // button connects pin 2 to GND
const int LED_PIN = 8;      // LED (with resistor) on pin 8

const unsigned long BLINK_INTERVAL = 500;  // time between LED flips (ms)
unsigned long lastToggle = 0; // last time the LED flipped
bool ledState = false; // false = off, true = on

void setup() {
  pinMode(LED_PIN, OUTPUT); // LED pin sends power out
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // button pin reads, held HIGH until pressed
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) // button is pressed
  { 
    unsigned long now = millis(); // check the clock
    if (now - lastToggle >= BLINK_INTERVAL) // has enough time passed
    {  
      lastToggle = now; // save the time of this flip
      ledState = !ledState; // flip on/off
      digitalWrite(LED_PIN, ledState); // update the LED
    }
  } 
  else 
  { // button is released
    ledState = false;
    digitalWrite(LED_PIN, LOW); // LED off
  }
}


// Lucas Bucht

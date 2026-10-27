-----------1 light code -----------

void setup() {
  pinMode(8, OUTPUT);
  pinMode(2, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(2) == LOW) {   // button pressed
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
  }
}


-----------3 light code -----------

void setup() {
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(2, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(2) == LOW) {   // button pressed
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(8, LOW);

    digitalWrite(9, HIGH);
    delay(500);
    digitalWrite(9, LOW);

    digitalWrite(10, HIGH);
    delay(500);
    digitalWrite(10, LOW);
  }
}

// Edited by Thomas Mazur

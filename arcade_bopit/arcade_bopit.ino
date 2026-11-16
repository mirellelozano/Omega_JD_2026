/*
  BOP-IT 
  Follows our pseudo-code 
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>

// Pins + Settings

// Pins
const uint8_t PIN_FIRE  = 2;    // button press
const uint8_t PIN_COIN  = 4;    // placeholder for limit switch
const uint8_t PIN_JOY_X = A1;   // joystick
#define TFT_RST 8               // display reset
#define TFT_DC  9               // display rs
#define TFT_CS  10              // display cs
// TO DO: Audio pins 

// Settings
const uint8_t  MAX_SCORE = 99;      // reaching this = win
const uint16_t START_MS  = 5000;    // time allowed for the first command (5000 ms or 5 s)
const uint16_t MIN_MS    = 1000;    // time allowed for the last command
const uint8_t  DEBOUNCE_MS = 25;    // ignore switch bounce shorter than this (1000 ms or 1 s)

// 1 = game only ever asks for FIRE (so it's playable with just the one button).
// 0 = full game using fire, coin and joystick.
#define TEST_ONE_BUTTON 1 // switch for testing

// Game states + commands 
enum State   { IDLE, PLAYING, GAME_OVER };
enum Command { CMD_NONE, CMD_COIN, CMD_LEFT, CMD_RIGHT, CMD_FIRE };

// Game variables
State    state = IDLE;
Command  currentCmd = CMD_NONE;   // the command the player must do right now
uint8_t  score = 0;
uint16_t windowMs = START_MS;     // current time limit
uint32_t cmdStart = 0;            // millis() when the current command was announced


//  FIRE BUTTON 

// Set up
void fireInit() 
{
  pinMode(PIN_FIRE, INPUT_PULLUP); // pressed = low
}

// Returns true ONCE, at the moment the button goes down (not while it's held).
bool firePressed() 
{
  static bool     wasDown = false; // was it down?
  static uint32_t lastChange = 0; // last ime of change
  bool isDown = (digitalRead(PIN_FIRE) == LOW); // true if button pressed

  // ignore if nothing changed or if it bounces
  if (isDown == wasDown || millis() - lastChange < DEBOUNCE_MS) 
  {
    return false;
  }

  lastChange = millis(); // saves when change happened
  wasDown = isDown; // save new state
  return isDown; // true on press, false on release
}

//  COIN SWITCH  (same idea as the fire button) - assumed wired as normally open

void coinInit() 
{
  pinMode(PIN_COIN, INPUT_PULLUP);
}

bool coinPressed() 
{
  static bool     wasDown = false;
  static uint32_t lastChange = 0;
  bool isDown = (digitalRead(PIN_COIN) == LOW); // == HIGH in case its wired as normally closed

  if (isDown == wasDown || millis() - lastChange < DEBOUNCE_MS)
  {
    return false;
  }

  lastChange = millis();
  wasDown = isDown;
  return isDown;
}

// JOYSTICK

// guessing based off the fact we go from 0V to 3.3V
const int JOY_LEFT_BELOW  = 170;   // below this = pushed left 
const int JOY_RIGHT_ABOVE = 500;   // above this = pushed right

// returns CMD_LEFT or CMD_RIGHT ONCE when the stick leaves center, otherwise CMD_NONE.
// if left/right turn out backwards, swap CMD_LEFT and CMD_RIGHT here btw
Command joystickMove() 
{
  static int8_t lastDir = 0; // -1 = left, 0 = center, 1 = right, similar to wasDown in previous
  int x = analogRead(PIN_JOY_X); // measure stick currently

  int8_t dir; // will hold -1 (left), 0 (center) or 1 (right)

  if (x < JOY_LEFT_BELOW) 
  { // pushed left
    dir = -1;
  } 
  else if (x > JOY_RIGHT_ABOVE) 
  { // pushed right
    dir = 1;
  } 
  else 
  { // stick is centered
    dir = 0;
  }

  Command result = CMD_NONE; // assume nothing happened

  if (dir != lastDir) 
  { // only react when the direction actually changes
    if (dir == -1) // -1 == left
    {
      result = CMD_LEFT;
    }
    if (dir ==  1) // 1 == right
    {
      result = CMD_RIGHT;
    }

    lastDir = dir; // remember new (if there was a change)
  }
  return result; // left, right, center
}

//  SCREEN

Adafruit_ST7796S_kbv tft(TFT_CS, TFT_DC, TFT_RST); // creates screen object

void displayInit() 
{
  tft.begin(); // start
  tft.setRotation(0); // 0 = portrait
  tft.fillScreen(0x0000); // fill with black
}

void drawText(int16_t x, int16_t y, uint8_t size, uint16_t color, const char *s) 
{
  tft.setCursor(x, y); // text at x,y coordinates
  tft.setTextSize(size); // ex: 1 = 6x8, 3 = 18x24
  tft.setTextColor(color); 
  tft.print(s);
}

// title scren
void showIdle() 
{
  tft.fillScreen(0x0000);
  drawText(30, 120, 4, 0x07FF, "ARCADE"); // cyan font
  drawText(30, 170, 4, 0xFFE0, "BOP-IT");  // yellow font
  drawText(30, 300, 2, 0xFFFF, "Press RED button to start! :D"); // white font
}

// after start clears screen for a new game and shows score 0
void showGameStart() 
{
  tft.fillScreen(0x0000); // black fill
  showScore();
}

// current score
void showScore() 
{
  tft.fillRect(0, 0, 320, 40, 0x0000); // wipe the old number first
  drawText(10, 10, 3, 0xFFE0, "SCORE"); // yellow
  tft.print(score);
}

// the command the player must do
void showCommand(Command c) 
{
  const char *names[] = { "INSERT COIN!", "MOVE LEFT!", "MOVE RIGHT!", "FIRE!" };
  tft.fillRect(0, 200, 320, 50, 0x0000); 
  drawText(20, 210, 3, 0xFFFF, names[c]); // white 
}

// show score and YOU WIN / GAME OVER
void showResult(bool won) 
{
  tft.fillScreen(0x0000);
  if (won)
  {
    drawText(20, 120, 5, 0x07E0, "YOU WIN!"); // green font
  }
  else
  {
    drawText(20, 120, 5, 0xF800, "GAME OVER"); // red font
  }
  drawText(20, 220, 3, 0xFFE0, "Score: "); // yellow
  tft.print(score);
  drawText(20, 300, 2, 0xFFFF, "Press FIRE to play again");
}

//insert code

//  AUDIO

// insert code

// GAME LOGIC

// insert code

// setup (probably gonna use case statements)

// insert code
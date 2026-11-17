/*
  BOP-IT 
  Follows our pseudo-code 
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S.h>

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

Adafruit_ST7796S tft(TFT_CS, TFT_DC, TFT_RST); // creates screen object

// 16-bit colors!!!!
const uint16_t BLACK = 0x0000, WHITE = 0xFFFF, RED = 0xF800, GREEN = 0x07E0, YELLOW = 0xFFE0, CYAN = 0x07FF, GRAY = 0x7BEF;

// Screen layout
const int16_t SCORE_H = 40; // reserved for the score
const int16_t STAR_TOP = 60; // stars appear at this y
const int16_t STAR_BOT = 400; // and fall until this y, then wrap back to the top
const int16_t CMD_Y = 200; // command text strip (y position)
const int16_t CMD_H = 50; // command text strip (height)
const int16_t SHIP_Y = 440; // the ship sits near the bottom (allegedly)

// what the animation should be doing right now
enum ScreenMode { SM_OFF, SM_ATTRACT, SM_PLAYING };
ScreenMode screenMode = SM_OFF;  // SM_OFF = static screen (so like game over), no animation

// the stars: each star has an x, y, and speed 
const uint8_t NUM_STARS = 20; // kept small because the chip has little memory but we can try bigger to see what happens
int16_t starX[NUM_STARS];
int16_t starY[NUM_STARS];
uint8_t starSpeed[NUM_STARS]; // pixels moved per frame (faster = looks closer)

// ship + bullet
int16_t shipX = 160; // ship's horizontal position (160 = center i think)
bool bulletOn = false; // is a bullet currently flying? 
int16_t bulletX, bulletY;

uint32_t lastFrame = 0; // when the animation last updated (millis)

void displayInit() 
{
  tft.init(); // was tft.begin()
  tft.setRotation(0); // 0 = portrait
  tft.fillScreen(BLACK);
}

// print text at (x, y). Size 1 = 6x8 pixels per character, size 3 = 18x24, and so on
void drawText(int16_t x, int16_t y, uint8_t size, uint16_t color, const char *s) 
{
  tft.setCursor(x, y);
  tft.setTextSize(size);
  tft.setTextColor(color);
  tft.print(s);
}

// starrssssss
// scatter the stars at random spots with random speeds 
void initStars() 
{
  for (uint8_t i = 0; i < NUM_STARS; i++) 
  {
    starX[i] = random(0, 320);
    starY[i] = random(STAR_TOP, STAR_BOT);
    starSpeed[i] = random(1, 5); // 1 to 4 pixels per frame
  }
}

// true if y is where text is drawn on the current screen. Stars skip these areas (so dont get in the way of words)
bool inTextZone(int16_t y) 
{
  if (screenMode == SM_ATTRACT)
  {
    return (y > 100 && y < 330); // title + prompt
  }
  return (y >= CMD_Y - 2 && y < CMD_Y + CMD_H + 2); // command strip
}

// shiiiip
// a triangle pointing up (supposedly) drawing it in BLACK erases it
void drawShip(uint16_t color) 
{
  tft.fillTriangle(shipX, SHIP_Y - 18, // tip
                   shipX - 14, SHIP_Y + 10, // bottom left
                   shipX + 14, SHIP_Y + 10, // bottom right
                   color);
}

// -screeeen
//  title screen with the starfield moving behind it (we can change it idc)
void showIdle() 
{
  tft.fillScreen(BLACK);
  drawText(30, 120, 4, CYAN, "ARCADE");
  drawText(30, 170, 4, YELLOW, "BOP-IT");
  drawText(30, 300, 2, WHITE, "Press FIRE to start");
  initStars();
  shipX = 160;
  drawShip(WHITE);
  screenMode = SM_ATTRACT; // tells displayTick() to animate the stars
}

// new game screen so stars, ship, and score 0.
void showGameStart() 
{
  tft.fillScreen(BLACK);
  initStars();
  shipX = 160;
  bulletOn = false;
  drawShip(WHITE);
  screenMode = SM_PLAYING;
  showScore();
}

// current score
void showScore() {
  tft.fillRect(0, 0, 320, SCORE_H, BLACK); // wipe old number first
  drawText(10, 10, 3, YELLOW, "SCORE");
  tft.print(score);
}

// the command the player must do.
void showCommand(Command c) 
{
  const char *names[] = { "", "INSERT COIN!", "MOVE LEFT!", "MOVE RIGHT!", "FIRE!" };
  tft.fillRect(0, CMD_Y, 320, CMD_H, BLACK);
  drawText(20, CMD_Y + 10, 3, WHITE, names[c]);
}

// show you win/game over
void showResult(bool won) 
{
  screenMode = SM_OFF;
  tft.fillScreen(BLACK);
  if (won)
  {
    drawText(20, 120, 5, GREEN, "YOU WIN!");
  }
  else
  {
    drawText(20, 120, 5, RED,   "GAME OVER");
  }
  drawText(20, 220, 3, YELLOW, "Score: ");
  tft.print(score);
  drawText(20, 300, 2, WHITE, "Press FIRE to play again");
}

// react to correct action
void displayAction(Command c) 
{
  if (c == CMD_LEFT || c == CMD_RIGHT) 
  {
    tft.fillRect(shipX - 16, SHIP_Y - 20, 32, 32, BLACK); // erase the ship where it is
    if (c == CMD_LEFT)  shipX = shipX - 40;
    if (c == CMD_RIGHT) shipX = shipX + 40;
    shipX = constrain(shipX, 30, 290); // keep the ship on screen
    drawShip(WHITE); // draw it in the new spot
  }
  if (c == CMD_FIRE && !bulletOn) 
  {
    bulletOn = true;
    bulletX = shipX; // bullet leaves from the ship
    bulletY = SHIP_Y - 30;
  }
}

// star animation
void displayTick() 
{
  if (screenMode == SM_OFF)
  {
    return; // nothing to animate
  }
  if (millis() - lastFrame < 40)
  {
    return; // too soon for the next frame
  }
  lastFrame = millis();

  for (uint8_t i = 0; i < NUM_STARS; i++) 
  {
    if (!inTextZone(starY[i]))
    {
      tft.fillRect(starX[i], starY[i], 2, 2, BLACK);   // erase
    }

    starY[i] = starY[i] + starSpeed[i]; // move down
    if (starY[i] >= STAR_BOT) 
    { // fell off the bottom
      starY[i] = STAR_TOP; // restart at the top
      starX[i] = random(0, 320); // at a new random spot
    }

    if (!inTextZone(starY[i])) 
    {
      uint16_t starColor; // the color this star will be drawn in
      if (starSpeed[i] > 2) 
      {
        starColor = WHITE; // fast stars (speed 3 or 4) are bright (idk if we want or not just a random choice i made)
      } 
      else 
      {
        starColor = GRAY; // slow stars (speed 1 or 2) are dim 
      }
      tft.fillRect(starX[i], starY[i], 2, 2, starColor);
    }
  }

  if (screenMode == SM_PLAYING && bulletOn) 
  {
    tft.fillRect(bulletX - 1, bulletY, 3, 12, BLACK); // erase old bullet
    bulletY = bulletY - 16; // move up
    if (bulletY < CMD_Y + CMD_H) 
    { // reached the command text so stop
      bulletOn = false;
    } 
    else 
    {
      tft.fillRect(bulletX - 1, bulletY, 3, 12, RED); // draw new bullet
    }
  }
}

//  AUDIO

// TODO: insert code

// GAME LOGIC

// next_window(score): time allowed for the next command
uint16_t nextWindow(uint8_t s) 
{
  return START_MS - (uint32_t)(START_MS - MIN_MS) * s / MAX_SCORE; // window = start time - (total time to remove x score / 99): 5000 at score 0, 1000 at score 99
}

// which action did the player just do & returns CMD_NONE if nothing
Command readInputs() 
{
  Command joy = joystickMove();
  bool fire = firePressed();
  bool coin = coinPressed();

  if (coin)
  {
    return CMD_COIN;
  }

  if (fire)
  {
    return CMD_FIRE;
  }
  return joy; // CMD_LEFT, CMD_RIGHT, or CMD_NONE
}

// issue_command(): pick a random command, announce it, start its timer.
void issueCommand() 
{
#if TEST_ONE_BUTTON
  currentCmd = CMD_FIRE;
#else
  currentCmd = (Command)random(CMD_COIN, CMD_FIRE + 1);   // random so picks 1-4 (coin/left/right/fire), top number is excluded, hence +1.
#endif
  // add sound?
  showCommand(currentCmd); // show direction on screen
  cmdStart = millis(); // timer starts AFTER the screen is drawn
}

// win: stop and show the result.
void endGame(bool won) 
{
  showResult(won);
  state = GAME_OVER;
}

// win: reached MAX_SCORE
void win() 
{
  //win audio?
  endGame(true);
}

// fail: wrong action or timeout
void fail() 
{
  //lose audio?
  endGame(false);
}

// succeed: player did the right thing
void succeed() 
{
  displayAction(currentCmd); // ship reacts to the command just completed 
  score++;
  showScore();
  windowMs = nextWindow(score); // speeds up!
  if (score >= MAX_SCORE)
  {
    win();
  }
  else
  {
    issueCommand();
  }
}

// Start a new game
void startGame() 
{
  score = 0;
  windowMs = START_MS; // reset time limit
  randomSeed(micros()); // the timing of a human press makes a random seed
  showGameStart(); 
  state = PLAYING;
  issueCommand();
}

// setup (probably gonna use case statements)

// setup() runs ONCE at power-on
void setup() 
{
  fireInit(); // set up the fire button pin
  coinInit(); // set up the coin switch pin
  displayInit(); // start the screen

  state = IDLE; // game begins waiting for the player
  showIdle(); // draw the title screen
}

// loop() 
void loop() 
{
  displayTick(); // keep the stars and bullet animating
  Command action = readInputs(); // what did the player just do? 

  switch (state) 
  {
    case IDLE:
      // waiting for the player, FIRE starts the game
      if (action == CMD_FIRE) 
      {
        startGame();
      }
      break;

    case PLAYING:
      // check the timer FIRST, so a late press can't happen
      if (millis() - cmdStart > windowMs) 
      {
        fail();
        break;
      }

      if (action == CMD_NONE) 
      {
        break; // nothing happened yet, keep waiting
      }

      if (action == currentCmd) 
      {
        succeed(); // right action
      } 
      else 
      {
        fail(); // wrong action
      }
      break;

    case GAME_OVER:
      // showing the result, FIRE plays again
      if (action == CMD_FIRE) 
      {
        startGame();
      }
      break;
  }
}
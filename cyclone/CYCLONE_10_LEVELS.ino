#include <FastLED.h>
#include <TM1637Display.h>

// ==========================================
// CYCLONE - 10 LEVEL CARNIVAL GAME
// Arduino Uno
// Levels 1-5: one target
// Levels 6-10: two targets
// ==========================================

// Hardware
#define NUM_LEDS 20
#define DATA_PIN 2
#define BUTTON_PIN 4

#define DISPLAY_CLK 8
#define DISPLAY_DIO 7

CRGB leds[NUM_LEDS];
TM1637Display display(DISPLAY_CLK, DISPLAY_DIO);

enum GameState {
  ATTRACT,
  PLAYING,
  WIN_ANIMATION,
  LOSE_ANIMATION
};

GameState gameState = ATTRACT;

byte level = 1;
byte Position = 0;

byte targets[2] = {0, 0};
bool targetHit[2] = {false, false};
byte targetCount = 1;

unsigned long time_now = 0;
unsigned long rainbowTime = 0;
byte hue = 0;

// ==========================================
// BUTTON DEBOUNCE
// ==========================================

bool buttonPressed() {
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static unsigned long debounceTime = 0;

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    debounceTime = millis();
    lastReading = reading;
  }

  if (millis() - debounceTime > 25) {
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

// ==========================================
// SPEED CONTROL
// Levels 1-5 accelerate.
// Level 6 resets to the first-mode speed,
// then accelerates through level 10.
// ==========================================

unsigned long getSpeed() {
  if (level <= 5) {
    return map(level - 1, 0, 4, 50, 15);
  }

  return map(level - 6, 0, 4, 50, 10);
}

// ==========================================
// DISPLAY
// ==========================================

void updateDisplay() {
  display.showNumberDec(level, false, 2, 2);
}

// ==========================================
// RANDOM POSITION GENERATOR
// ==========================================

byte randomPosition(byte excluded1, byte excluded2) {
  byte result;

  do {
    result = random(NUM_LEDS);
  } while (result == excluded1 || result == excluded2);

  return result;
}

// ==========================================
// START A ROUND
// ==========================================

void startRound() {
  Position = random(NUM_LEDS);

  targetCount = (level <= 5) ? 1 : 2;

  targets[0] = randomPosition(Position, 255);
  targets[1] = 255;

  if (targetCount == 2) {
    targets[1] = randomPosition(Position, targets[0]);
  }

  targetHit[0] = false;
  targetHit[1] = false;

  time_now = millis();
  updateDisplay();

  FastLED.clear();
  FastLED.show();

  gameState = PLAYING;
}

// ==========================================
// DRAW GAME
// ==========================================

void drawGame() {
  FastLED.clear();

  for (byte i = 0; i < targetCount; i++) {
    if (!targetHit[i]) {
      leds[targets[i]] = CRGB::Green;
    }
  }

  leds[Position] = CRGB::Red;
  FastLED.show();
}

// ==========================================
// MOVE PLAYER
// ==========================================

void movePlayer() {
  Position++;

  if (Position >= NUM_LEDS) {
    Position = 0;
  }

  drawGame();
}

// ==========================================
// CHECK HIT
// ==========================================

void checkHit() {
  bool hit = false;

  for (byte i = 0; i < targetCount; i++) {
    if (!targetHit[i] && Position == targets[i]) {
      targetHit[i] = true;
      hit = true;
    }
  }

  if (!hit) {
    gameState = LOSE_ANIMATION;
    return;
  }

  bool allTargetsHit = true;

  for (byte i = 0; i < targetCount; i++) {
    if (!targetHit[i]) {
      allTargetsHit = false;
    }
  }

  if (allTargetsHit) {
    gameState = WIN_ANIMATION;
  } else {
    drawGame();
  }
}

// ==========================================
// RAINBOW ATTRACT MODE
// ==========================================

void attractMode() {
  if (millis() - rainbowTime >= 20) {
    rainbowTime = millis();
    fill_rainbow(leds, NUM_LEDS, hue, 7);
    hue++;
    FastLED.show();
  }

  if (buttonPressed()) {
    level = 1;
    startRound();
  }
}

// ==========================================
// GAMEPLAY
// ==========================================

void playGame() {
  if (millis() - time_now >= getSpeed()) {
    time_now = millis();
    movePlayer();
  }

  if (buttonPressed()) {
    checkHit();
  }
}

// ==========================================
// WIN ANIMATION
// ==========================================

void winner() {
  for (byte i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(350);

    FastLED.clear();
    FastLED.show();
    delay(250);
  }

  if (level >= 10) {
    level = 1;
    updateDisplay();
    gameState = ATTRACT;
    return;
  }

  level++;
  startRound();
}

// ==========================================
// LOSE ANIMATION
// ==========================================

void loser() {
  for (byte i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(350);

    FastLED.clear();
    FastLED.show();
    delay(250);
  }

  level = 1;
  updateDisplay();
  gameState = ATTRACT;
}

// ==========================================
// SETUP
// ==========================================

void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.setBrightness(5);
  display.clear();

  randomSeed(analogRead(A0));

  Serial.begin(9600);
  Serial.println("CYCLONE 10 LEVEL SYSTEM READY");

  updateDisplay();
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {
  switch (gameState) {
    case ATTRACT:
      attractMode();
      break;

    case PLAYING:
      playGame();
      break;

    case WIN_ANIMATION:
      winner();
      break;

    case LOSE_ANIMATION:
      loser();
      break;
  }
}

#include <MD_MAX72xx.h>
#include <SPI.h>
#include "LEDAnimationFunctions.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   9 // or SCK
#define DATA_PIN  8  // or MOSI
#define CS_PIN    5  // or SS

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);     // SPI hardware interface

static char     scrollBuf[128];
static char*    scrollPtr;
static uint8_t  scrollState = 0;
static uint8_t  scrollCurLen = 0;
static uint8_t  scrollShowLen = 0;
static uint8_t  scrollColBuf[8];
static uint32_t scrollLastTime = 0;

const uint8_t heartFull[]  = {0x1C,0x3E,0x7E,0xFC};
const uint8_t heartEmpty[] = {0x1C,0x22,0x42,0x84};
const uint8_t inv1[] = {0x0E,0x98,0x7D,0x36,0x3C};
const uint8_t inv2[] = {0x70,0x18,0x7D,0xB6,0x3C};

#define SCROLL_DELAY    2
#define HEARTBEAT_DELAY 25
#define INVADER_DELAY   5
#define FADE_DELAY      5

static uint8_t currentLevel = 255;
static bool    needInit = true;

   byte heartOutline[8] = {
    B00000000,
    B01100110,
    B11111111,
    B11111111,
    B11111111,
    B01111110,
    B00111100,
    B00011000
  };


 // Heart filled (8x8)
  byte heartFilled[8] = {
    B00000000,
    B01100110,
    B10011001,
    B10000001,
    B10000001,
    B01000010,
    B00100100,
    B00011000
  };
// ──────────────────────────────────────────────────────────────
void initStressDisplay() {

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 8);
  mx.clear();
}

// ──────────────────────────────────────────────────────────────
void setStressLevel(uint8_t level) {
  Serial.println(level);
  if (level > 4) level = 4;
  if (currentLevel != level) {
    currentLevel = level;
    needInit = true;
    mx.clear();
  }
}

// ──────────────────────────────────────────────────────────────

bool scrollText(bool init, const char* msg) 
{
  static char buf[128];
  static char* p;
  static uint8_t state = 0;
  static uint8_t  curLen, showLen;
  static uint8_t col[8];
  static uint32_t last = 0;

  if (init) { strcpy(buf, msg); p = buf; state = 0; mx.clear(); last = millis(); }
  if (millis() - last < SCROLL_DELAY) return false;
  last = millis();

  mx.transform(MD_MAX72XX::TSL);
  switch (state) {
    case 0: if (!*p) return true;
            showLen = mx.getChar(*p++, sizeof(col), col);
            curLen = 0; state = 1;
    case 1: mx.setColumn(0, col[curLen++]);
            if (curLen >= showLen) { showLen = *p ? 1 : mx.getColumnCount()-1; curLen = 0; state = 2; }
            break;
    case 2: mx.setColumn(0, 0);
            if (++curLen >= showLen) state = 0;
            break;
  }
  return false;
}

void showBootMessage() {
  static bool init = false;
  if (needInit) init = true;
  bool done = scrollText(init, "STRESS INDICATOR  CHECK YOURS");
  init = false;
  if (done) init = true;
}


void showRelaxed() {
  static bool empty = true;
  static uint32_t last = 0;
  if (needInit) { mx.clear(); empty = true; last = millis() - 300; }

  if (millis() - last >= 1000) {
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    mx.control(MD_MAX72XX::INTENSITY, 5);
    const uint8_t* pat = empty ? heartEmpty : heartFull;
    uint8_t offset = mx.getColumnCount() / 4;
    for (uint8_t h = 1; h <= 3; h++) {
      for (uint8_t i = 0; i < 4; i++) {
        mx.setColumn(h*offset - 4 + i,       pat[i]);
        mx.setColumn(h*offset + 4 - i - 1,   pat[i]);
      }
    }
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    empty = !empty;
    last = millis();
  }
}

void showMildStress() {
  static bool filled = false;
  static uint32_t last = 0;
  if (needInit) { mx.clear(); filled = false; last = millis() - HEARTBEAT_DELAY; }

  if (millis() - last >= HEARTBEAT_DELAY) {
    const uint8_t* pat = filled ? heartFilled : heartOutline;
    for (uint8_t d = 0; d < 4; d++) {
      for (uint8_t r = 0; r < 8; r++) mx.setRow(d, r, pat[r]);
    }
    filled = !filled;
    last = millis();
  }
}

void showStressed() {
  static bool showInvader = true;
  static bool initScroll = false;
  static int8_t pos = -5;
  static bool type = false;
  static uint32_t last = 0;

  if (needInit) {
    showInvader = true;
    initScroll = false;
    pos = -5;
    type = false;
    mx.clear();
    last = millis() - INVADER_DELAY;
  }

  if (showInvader) {
    if (millis() - last >= INVADER_DELAY) {
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      mx.clear();
      const uint8_t* inv = type ? inv1 : inv2;
      for (uint8_t i = 0; i < 5; i++) {
        mx.setColumn(pos - 5 + i,     inv[i]);
        mx.setColumn(pos + 5 - i - 1, inv[i]);
      }
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      pos++; type = !type;
      if (pos >= (int)mx.getColumnCount() + 5) {
        showInvader = false;
        initScroll = true;
      }
      last = millis();
    }
  } else {
    bool done = scrollText(initScroll, "Oh No! You are STRESSED  Relax");
    initScroll = false;
    if (done) {
      showInvader = true;
      pos = -5;
      type = false;
      mx.clear();
      last = millis() - INVADER_DELAY;
    }
  }
}

void showHighStress() {
  static bool fading = true;
  static bool initScroll = true;
  static uint8_t intensity = 15;
  static int8_t dir = -1;
  static uint32_t last = 0;
  static uint32_t start = 0;

  if (needInit) {
    fading = true;
    initScroll = true;
    intensity = 15;
    dir = -1;
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for (uint8_t c = 0; c < mx.getColumnCount(); c++) mx.setColumn(c, 0xFF);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
    start = millis();
    last = millis() - FADE_DELAY;
  }

  if (fading) {
    if (millis() - last >= FADE_DELAY) {
      intensity += dir;
      if (intensity == 0 || intensity == 15) dir = -dir;
      mx.control(MD_MAX72XX::INTENSITY, intensity);
      last = millis();
    }
    if (millis() - start > 800) {
      fading = false;
      initScroll = true;
    }
  } else {
    bool done = scrollText(initScroll, "HIGH STRESS!   CALM DOWN");
    initScroll = false;
    if (done) {
      fading = true;
      intensity = 15;
      dir = -1;
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
      for (uint8_t c = 0; c < mx.getColumnCount(); c++) mx.setColumn(c, 0xFF);
      mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
      mx.control(MD_MAX72XX::INTENSITY, intensity);
      start = millis();
      last = millis() - FADE_DELAY;
    }
  }
}

void updateStressDisplay() {
  if (currentLevel == 255) return;

  switch (currentLevel) {
    case 0: showBootMessage();     break;
    case 1: showRelaxed();         break;
    case 2: showMildStress();      break;
    case 3: showStressed();        break;
    case 4: showHighStress();      break;
  }
  needInit = false;
}
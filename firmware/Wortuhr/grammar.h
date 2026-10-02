#pragma once
// German word-clock face (12 x 11) and the "time -> words" rules.
//
//   col 0123456789AB
//   0   ESAISTHZEHNU
//   1   FÜNFDVIERTEL
//   2   EFZWANZIGGHI
//   3   MINUTENJNACH
//   4   OGVORYHALBKM
//   5   ZWÖLFMSIEBEN
//   6   QZNEUNJEINSK
//   7   EDREILFÜNFTM
//   8   ÄZZWEIHVIERZ
//   9   SECHSEACHTHC
//   10  ELFVZEHNOUHR
//
// The LED strip snakes from the bottom-right corner (LED 0 = "R" of UHR) upwards.
#include <Arduino.h>

static const uint8_t GRID_W = 12, GRID_H = 11;
static const uint16_t NUM_LEDS = GRID_W * GRID_H;

inline uint16_t ledIndex(uint8_t row, uint8_t col) {
  return (row % 2 == 0) ? (NUM_LEDS - 1 - row * GRID_W - col) : (NUM_LEDS - (row + 1) * GRID_W + col);
}

enum WordId : uint8_t {
  W_ES, W_IST, W_FUENF_M, W_ZEHN_M, W_VIERTEL, W_ZWANZIG, W_MINUTEN, W_NACH, W_VOR, W_HALB,
  W_H_EIN, W_H_EINS, W_H_ZWEI, W_H_DREI, W_H_VIER, W_H_FUENF, W_H_SECHS, W_H_SIEBEN,
  W_H_ACHT, W_H_NEUN, W_H_ZEHN, W_H_ELF, W_H_ZWOELF, W_UHR, W_COUNT
};

struct WordPos { uint8_t row, col, len; };

static const char *const WORD_NAMES[W_COUNT] = {
  "ES", "IST", "FÜNF", "ZEHN", "VIERTEL", "ZWANZIG", "MINUTEN", "NACH", "VOR", "HALB",
  "EIN", "EINS", "ZWEI", "DREI", "VIER", "FÜNF", "SECHS", "SIEBEN", "ACHT", "NEUN", "ZEHN", "ELF", "ZWÖLF", "UHR"};

static const WordPos WORDS[W_COUNT] = {
  {0, 0, 2},  // ES
  {0, 3, 3},  // IST
  {1, 0, 4},  // FÜNF (minutes)
  {0, 7, 4},  // ZEHN (minutes)
  {1, 5, 7},  // VIERTEL
  {2, 2, 7},  // ZWANZIG
  {3, 0, 7},  // MINUTEN
  {3, 8, 4},  // NACH
  {4, 2, 3},  // VOR
  {4, 6, 4},  // HALB
  {6, 7, 3},  // EIN  (only in "ES IST EIN UHR")
  {6, 7, 4},  // EINS
  {8, 2, 4},  // ZWEI
  {7, 1, 4},  // DREI
  {8, 7, 4},  // VIER
  {7, 6, 4},  // FÜNF
  {9, 0, 5},  // SECHS
  {5, 6, 6},  // SIEBEN
  {9, 6, 4},  // ACHT
  {6, 2, 4},  // NEUN
  {10, 4, 4}, // ZEHN
  {10, 0, 3}, // ELF
  {5, 0, 5},  // ZWÖLF
  {10, 9, 3}, // UHR
};

static const WordId HOUR_WORDS[13] = {
  W_H_ZWOELF, W_H_EINS, W_H_ZWEI, W_H_DREI, W_H_VIER, W_H_FUENF, W_H_SECHS,
  W_H_SIEBEN, W_H_ACHT, W_H_NEUN, W_H_ZEHN, W_H_ELF, W_H_ZWOELF
};

struct GrammarOpts {
  bool minuten = true;      // "fünf MINUTEN nach" (as in the original German version)
  uint8_t v20 = 0;          // :20  0 = "zwanzig nach drei"   1 = "zehn vor halb vier"
  uint8_t v40 = 0;          // :40  0 = "zwanzig vor vier"    1 = "zehn nach halb vier"
  bool esIstAlways = true;  // false: "ES IST" only at full and half hour
};

struct Phrase {
  uint8_t n = 0;
  WordId w[8];
  uint32_t mask = 0;
  void add(WordId id) { w[n++] = id; mask |= (1UL << id); }
};

inline Phrase buildPhrase(int hour24, int minute, const GrammarOpts &o) {
  Phrase p;
  int b = minute / 5;
  bool next = b >= 5 || (b == 4 && o.v20 == 1);
  int h = (hour24 + (next ? 1 : 0)) % 12;
  if (o.esIstAlways || b == 0 || b == 6) { p.add(W_ES); p.add(W_IST); }
  auto mins = [&](WordId num, WordId dir) { p.add(num); if (o.minuten) p.add(W_MINUTEN); p.add(dir); };
  switch (b) {
    case 0: break;
    case 1: mins(W_FUENF_M, W_NACH); break;
    case 2: mins(W_ZEHN_M, W_NACH); break;
    case 3: p.add(W_VIERTEL); p.add(W_NACH); break;
    case 4:
      if (o.v20 == 0) mins(W_ZWANZIG, W_NACH);
      else { p.add(W_ZEHN_M); p.add(W_VOR); p.add(W_HALB); }
      break;
    case 5: p.add(W_FUENF_M); p.add(W_VOR); p.add(W_HALB); break;
    case 6: p.add(W_HALB); break;
    case 7: p.add(W_FUENF_M); p.add(W_NACH); p.add(W_HALB); break;
    case 8:
      if (o.v40 == 0) mins(W_ZWANZIG, W_VOR);
      else { p.add(W_ZEHN_M); p.add(W_NACH); p.add(W_HALB); }
      break;
    case 9: p.add(W_VIERTEL); p.add(W_VOR); break;
    case 10: mins(W_ZEHN_M, W_VOR); break;
    default: mins(W_FUENF_M, W_VOR); break;
  }
  if (b == 0) { p.add(h == 1 ? W_H_EIN : HOUR_WORDS[h]); p.add(W_UHR); }
  else p.add(HOUR_WORDS[h]);
  return p;
}

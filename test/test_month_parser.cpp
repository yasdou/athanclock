// Tests für month_parser.h mit einer echten aladhan-Antwort (test/fixtures).
// Braucht ArduinoJson (nur Header):
//
//   (im Ordner src/)
//   g++ -std=c++11 -Wall -Wextra -I. -isystem <ArduinoJson>/src test/test_month_parser.cpp prayer_logic.cpp -o test_month_parser
//   ./test_month_parser

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "month_parser.h"

using namespace prayer;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                              \
  do {                                                           \
    checks++;                                                    \
    if (!(cond)) {                                               \
      failures++;                                                \
      printf("  FEHLER %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                            \
  } while (0)

#define CHECK_EQ(a, b)                                                                     \
  do {                                                                                     \
    checks++;                                                                              \
    long _a = (long)(a), _b = (long)(b);                                                   \
    if (_a != _b) {                                                                        \
      failures++;                                                                          \
      printf("  FEHLER %s:%d: %s == %ld, erwartet %ld\n", __FILE__, __LINE__, #a, _a, _b); \
    }                                                                                      \
  } while (0)

// Verhält sich für den Parser wie Arduinos Stream (read/readBytes/find/findUntil).
class StringReader {
 public:
  explicit StringReader(const std::string& s) : data_(s), pos_(0) {}

  int read() { return pos_ < data_.size() ? (unsigned char)data_[pos_++] : -1; }

  size_t readBytes(char* buf, size_t n) {
    size_t k = 0;
    while (k < n && pos_ < data_.size()) buf[k++] = data_[pos_++];
    return k;
  }

  bool find(const char* target) {
    size_t at = data_.find(target, pos_);
    if (at == std::string::npos) {
      pos_ = data_.size();
      return false;
    }
    pos_ = at + strlen(target);
    return true;
  }

  // Wie Arduino: liest bis target (true) oder terminator (false) gefunden ist.
  bool findUntil(const char* target, const char* terminator) {
    size_t tl = strlen(target), ml = strlen(terminator);
    while (pos_ < data_.size()) {
      if (data_.compare(pos_, tl, target) == 0) {
        pos_ += tl;
        return true;
      }
      if (data_.compare(pos_, ml, terminator) == 0) {
        pos_ += ml;
        return false;
      }
      pos_++;
    }
    return false;
  }

 private:
  std::string data_;
  size_t pos_;
};

static std::string loadFixture() {
  const char* path = "test/fixtures/aladhan_2026_11_mainz.json";
  FILE* f = fopen(path, "rb");
  if (!f) {
    printf("Fixture nicht gefunden: %s (Tests im Ordner src/ starten)\n", path);
    exit(2);
  }
  std::string s;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) s.append(buf, n);
  fclose(f);
  return s;
}

static std::string json;

static void fillSentinel(MonthTimes& m) {
  clearMonth(m);
  m.year = 1999;
  m.month = 1;
  m.days = 31;
  m.t[0][FAJR] = 123;
}

static bool unchanged(const MonthTimes& m) { return m.year == 1999 && m.days == 31 && m.t[0][FAJR] == 123; }

static int dayCallbacks = 0;
static void countDay() { dayCallbacks++; }

static void testRealResponse() {
  StringReader r(json);
  MonthTimes m;
  clearMonth(m);
  const char* err = NULL;
  dayCallbacks = 0;
  bool ok = parseMonthStream(r, 2026, 11, m, &err, countDay);
  if (!ok) printf("  Fehlertext: %s\n", err);
  CHECK(ok);
  CHECK_EQ(m.year, 2026);
  CHECK_EQ(m.month, 11);
  CHECK_EQ(m.days, 30);
  CHECK_EQ(dayCallbacks, 30);
  CHECK_EQ(m.t[0][FAJR], 5 * 60 + 45);    // "05:45 (CET)"
  CHECK_EQ(m.t[0][SHURUK], 7 * 60 + 16);  // Sunrise "07:16 (CET)"
  CHECK_EQ(m.t[0][ISHA], 18 * 60 + 35);
  CHECK_EQ(m.t[29][FAJR], 6 * 60 + 24);
  CHECK_EQ(m.t[29][ISHA], 18 * 60 + 6);
  CHECK_EQ(m.t[30][FAJR], -1);  // 31. November gibt es nicht
  CHECK(strcmp(m.timezone, "Europe/Berlin") == 0);

  // Plausibilität: an jedem Tag aufsteigende Reihenfolge
  for (int d = 0; d < m.days; d++)
    for (int i = 1; i < kPrayerCount; i++) CHECK(m.t[d][i] > m.t[d][i - 1]);
}

// Falscher Monat angefragt (Dezember hat 31 Tage) -> abgelehnt, alte Daten bleiben.
static void testWrongDayCount() {
  StringReader r(json);
  MonthTimes m;
  fillSentinel(m);
  const char* err = NULL;
  CHECK(!parseMonthStream(r, 2026, 12, m, &err));
  CHECK(unchanged(m));
  CHECK(strlen(err) > 0);
}

// Verbindungsabbruch mitten in der Antwort.
static void testTruncated() {
  StringReader r(json.substr(0, json.size() / 2));
  MonthTimes m;
  fillSentinel(m);
  const char* err = NULL;
  CHECK(!parseMonthStream(r, 2026, 11, m, &err));
  CHECK(unchanged(m));
}

// Fehlender Schlüssel an einem Tag.
static void testMissingPrayer() {
  std::string broken = json;
  // Im letzten "timings"-Block (weiter hinten steht "Isha" auch in meta.method.params)
  size_t at = broken.find("\"Isha\"", broken.rfind("\"timings\""));
  CHECK(at != std::string::npos);
  broken.replace(at, 6, "\"Ishx\"");
  StringReader r(broken);
  MonthTimes m;
  fillSentinel(m);
  const char* err = NULL;
  CHECK(!parseMonthStream(r, 2026, 11, m, &err));
  CHECK(unchanged(m));
}

// Fehlerantwort ohne Daten (z. B. unbekannte Stadt).
static void testNoData() {
  StringReader r("{\"code\":400,\"status\":\"Bad Request\",\"data\":\"Unable to find city\"}");
  MonthTimes m;
  fillSentinel(m);
  const char* err = NULL;
  CHECK(!parseMonthStream(r, 2026, 11, m, &err));
  CHECK(unchanged(m));
}

static void testEmptyArray() {
  StringReader r("{\"code\":200,\"data\":[]}");
  MonthTimes m;
  fillSentinel(m);
  CHECK(!parseMonthStream(r, 2026, 11, m, NULL));
  CHECK(unchanged(m));
}

int main() {
  json = loadFixture();

  struct {
    const char* name;
    void (*fn)();
  } tests[] = {
      {"Echte aladhan-Antwort (Nov 2026, Mainz)", testRealResponse},
      {"Falsche Anzahl Tage", testWrongDayCount},
      {"Abgebrochene Antwort", testTruncated},
      {"Fehlende Gebetszeit", testMissingPrayer},
      {"Fehlerantwort ohne Daten", testNoData},
      {"Leeres Array", testEmptyArray},
  };

  for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
    int before = failures;
    tests[i].fn();
    printf("%s %s\n", failures == before ? "[ OK ]" : "[FAIL]", tests[i].name);
  }
  printf("\n%d Pruefungen, %d Fehler\n", checks, failures);
  return failures == 0 ? 0 : 1;
}

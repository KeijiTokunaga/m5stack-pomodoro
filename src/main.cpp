#include <M5Unified.h>
#include <Preferences.h>
#include "Pomodoro.h"
#include <math.h>

Pomodoro timer;
Preferences prefs;
M5Canvas canvas(&M5.Display);
bool soundOn = true;
bool dirty = true;
bool canvasReady = false;
uint32_t lastPaint = 0, lastSave = 0, alarmAt = 0;
bool alarmActive = false;
int lastPulse = -1;
const uint32_t muted = 0x8B929E;
constexpr int touchLeft = 51, touchTop = 145, touchWidth = 364, touchHeight = 180;

void save() {
  // One NVS blob keeps phase, count and remaining coherent after power loss.
  uint32_t data[] = {1, timer.phase, timer.remaining, timer.completed, soundOn};
  prefs.putBytes("state", data, sizeof(data));
  lastSave = millis();
}
void silence() {
  alarmActive = false;
  M5.Speaker.stop();
  M5.Power.setVibration(0);
}
void action(char key) {
  silence();
  if (key == 'a') timer.toggle(millis());
  if (key == 'r') timer.reset();
  if (key == 'n') timer.skip();
  if (key == 'm') soundOn = !soundOn;
  save();
  dirty = true;
}
void label(const char* text, int y, int size, uint32_t color) {
  canvas.setTextColor(color);
  canvas.setTextSize(size);
  canvas.drawCenterString(text, 233, y);
}
void paint() {
  const uint32_t accent = timer.phase == Pomodoro::Focus ? 0xFF725E : 0x55D9B1;
  canvas.fillScreen(TFT_BLACK);
  const float fraction = static_cast<float>(timer.remaining) / timer.duration();
  // Small segments avoid a full-circle arc special case at 25:00.
  for (int i = 0; i < 120; ++i) {
    float angle = (i * 3 - 90) * 0.01745329252f;
    int x = 233 + lroundf(cosf(angle) * 218);
    int y = 233 + lroundf(sinf(angle) * 218);
    canvas.fillCircle(x, y, 4, i < fraction * 120 ? accent : 0x24282F);
  }
  label("POMODORO", 63, 2, muted);
  label(timer.phase == Pomodoro::Focus ? "FOCUS" :
        timer.phase == Pomodoro::ShortBreak ? "SHORT BREAK" : "LONG BREAK", 111, 3, accent);
  canvas.fillRoundRect(touchLeft, touchTop, touchWidth, touchHeight, 32,
                       timer.phase == Pomodoro::Focus ? 0x301A17 : 0x112E26);
  canvas.drawRoundRect(touchLeft, touchTop, touchWidth, touchHeight, 32, accent);
  char text[48];
  uint32_t seconds = (timer.remaining + 999) / 1000;
  snprintf(text, sizeof(text), "%02lu:%02lu", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
  label(text, 170, 8, TFT_WHITE);
  label(timer.running ? "TAP TO PAUSE" : "TAP TO START", 254, 3, accent);
  label("TOUCH ANYWHERE IN THIS BOX", 295, 2, muted);
  for (int i = 0; i < 4; ++i) {
    int done = timer.completed % 4;
    if (timer.phase == Pomodoro::LongBreak) done = 4;
    canvas.fillCircle(188 + i * 30, 351, 8, i < done ? accent : 0x30353D);
  }
  snprintf(text, sizeof(text), "%lu DONE  /  SOUND %s", (unsigned long)timer.completed, soundOn ? "ON" : "OFF");
  label(text, 395, 2, muted);
  canvas.pushSprite(0, 0);
  dirty = false;
  lastPaint = millis();
}
void status() {
  Serial.printf("POMODORO phase=%u remaining_ms=%lu running=%u completed=%lu display=%dx%d psram=%u\n",
    timer.phase, (unsigned long)timer.remaining, timer.running,
    (unsigned long)timer.completed, M5.Display.width(), M5.Display.height(), ESP.getPsramSize());
}
void setup() {
  auto cfg = M5.config();
  cfg.internal_mic = false;
  cfg.internal_imu = false;
  M5.begin(cfg);
  Serial.begin(115200);
  M5.Display.setBrightness(150);
  M5.Speaker.setVolume(100);
  M5.Power.setVibration(0);
  prefs.begin("pomodoro", false);
  uint32_t data[5] = {};
  if (prefs.getBytesLength("state") == sizeof(data)) {
    prefs.getBytes("state", data, sizeof(data));
    if (data[0] == 1 && timer.restore(data[1], data[2], data[3])) soundOn = data[4] != 0;
  }
  canvas.setColorDepth(16);
  canvasReady = canvas.createSprite(466, 466) != nullptr;
  if (!canvasReady) {
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawCenterString("Display memory error", 233, 220);
    Serial.println("ERROR: canvas allocation failed");
  } else paint();
  status();
}
void loop() {
  M5.update();
  uint32_t now = millis();
  if (timer.tick(now)) {
    save();
    alarmActive = true;
    alarmAt = now;
    lastPulse = -1;
    dirty = true;
    status();
  }
  if (M5.BtnA.wasHold()) action('n');
  else if (M5.BtnA.wasClicked()) action('a');
  if (M5.BtnB.wasHold()) action('r');
  else if (M5.BtnB.wasClicked()) action('m');
  auto touch = M5.Touch.getDetail();
  if (touch.wasClicked()) {
    if (touch.x >= touchLeft && touch.x < touchLeft + touchWidth &&
        touch.y >= touchTop && touch.y < touchTop + touchHeight) action('a');
    else if (touch.y >= 380 && touch.y <= 430) action('m');
  }
  // USB control supports repeatable hardware smoke tests and diagnostics.
  while (Serial.available()) {
    char key = Serial.read();
    if (key == 'a' || key == 'r' || key == 'n' || key == 'm') action(key);
    if (key == '?' || key == 'a' || key == 'r' || key == 'n' || key == 'm') status();
  }
  if (alarmActive) {
    uint32_t elapsed = now - alarmAt;
    if (elapsed >= 3000) silence();
    else {
      int pulse = elapsed / 500;
      if (pulse != lastPulse) {
        lastPulse = pulse;
        M5.Power.setVibration(pulse % 2 == 0 ? 160 : 0);
        if (soundOn && pulse % 2 == 0) M5.Speaker.tone(880, 180);
      }
    }
  }
  if (timer.running && now - lastSave >= 60000) save();
  if (canvasReady && (dirty || (timer.running && now - lastPaint >= 200))) paint();
  delay(5);
}

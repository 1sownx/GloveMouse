#include <Wire.h>
#include <BleMouse.h>

BleMouse mouse("Glove Mouse", "DIY", 100);

const int MPU = 0x68;
const int BTN_L = 3;   // левый клик
const int BTN_R = 4;   // правый клик

float offX = 0, offY = 0;
float sx = 0, sy = 0;
int still = 0;

bool lastL = false, lastR = false;
unsigned long tL = 0, tR = 0;

void readGyro(int16_t &gx, int16_t &gy) {
  Wire.beginTransmission(MPU);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6);
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();
}

void handleButton(int pin, uint8_t button, bool &last, unsigned long &t) {
  bool pressed = (digitalRead(pin) == LOW);
  if (pressed != last && millis() - t > 30) {   // защита от дребезга
    t = millis();
    last = pressed;
    if (mouse.isConnected()) {
      if (pressed) mouse.press(button);
      else mouse.release(button);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_L, INPUT_PULLUP);
  pinMode(BTN_R, INPUT_PULLUP);
  Wire.begin(5, 6);

  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);          // выйти из сна
  Wire.endTransmission();

  delay(3000);            // положите руку неподвижно
  Serial.println("Calibrating, keep hand still...");
  long ax = 0, ay = 0;
  for (int i = 0; i < 200; i++) {
    int16_t gx, gy;
    readGyro(gx, gy);
    ax += gx; ay += gy;
    delay(5);
  }
  offX = ax / 200.0;
  offY = ay / 200.0;

  mouse.begin();
  Serial.println("Ready. Connect via Bluetooth: Glove Mouse");
}

void loop() {
  int16_t gx, gy;
  readGyro(gx, gy);

  float rx = gx - offX;
  float ry = gy - offY;

  // автоподстройка нуля, пока рука почти неподвижна
  if (abs(rx) < 300 && abs(ry) < 300) {
    still++;
    if (still > 50) {          // около 0,5 с покоя
      offX += rx * 0.01;
      offY += ry * 0.01;
    }
  } else {
    still = 0;
  }

  float vx = rx / 131.0;       // градусов/сек
  float vy = ry / 131.0;

  if (abs(vx) < 3) vx = 0;     // мёртвая зона
  if (abs(vy) < 3) vy = 0;

  sx = sx * 0.6 + vx * 0.4;    // сглаживание
  sy = sy * 0.6 + vy * 0.4;

  const float SENS = 0.25;
  int dx = (int)(sx * SENS);
  int dy = (int)(sy * SENS);

  if (mouse.isConnected() && (dx != 0 || dy != 0)) {
    mouse.move(dx, dy);
  }

  handleButton(BTN_L, MOUSE_LEFT, lastL, tL);
  handleButton(BTN_R, MOUSE_RIGHT, lastR, tR);
Serial.println(String(gx) + " " + String(gy));
  delay(10);
}
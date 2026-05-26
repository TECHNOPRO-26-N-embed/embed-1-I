// ミニチュア版歩行者信号灯
// 赤LED: D8 / 緑LED: D9 / パッシブブザー: D3

const byte rLedPin = 8;
const byte gLedPin = 9;
const byte bzPin = 3;

// 0: 赤信号中, 1: 青信号中
int sigState = 0;

unsigned long stateMs = 0;
const unsigned long rDurMs = 5000;
const unsigned long gDurMs = 5000;

const int gToneHz = 2000;
const unsigned long stuckMs = 10000;

// 前回適用したブザー状態（0:停止, 1:鳴動）
int lastBzState = -1;

int switchState(unsigned long nowMs);
void setLeds(int state);
void setBuzzer(int state, int toneHz);
bool checkStuck(unsigned long nowMs);
void fixBuzzer(int state, int toneHz);

// 信号リズム用
const int patternCount = 4;
const int patternTones[patternCount] = { 523, 392, 523, 392 }; // ド・ソ・ド・ソ
const int patternDurations[patternCount] = { 150, 300, 150, 500 };  // ms単位

int patternIndex = 0;
unsigned long patternStartMs = 0;
bool isToneOn = false;

void setup() {
  pinMode(rLedPin, OUTPUT);
  pinMode(gLedPin, OUTPUT);
  pinMode(bzPin, OUTPUT);

  sigState = 0;
  stateMs = millis();
  lastBzState = 0;

  digitalWrite(rLedPin, HIGH);
  digitalWrite(gLedPin, LOW);
  noTone(bzPin);

  patternIndex = 0;
  patternStartMs = millis();
  isToneOn = false;
}

void loop() {
  unsigned long nowMs = millis();

  sigState = switchState(nowMs);

  if (checkStuck(nowMs)) {
    sigState = 0;
  }

  setLeds(sigState);
  setBuzzerRhythm(sigState, nowMs);
  fixBuzzer(sigState, gToneHz);
}

int switchState(unsigned long nowMs) {
  if (sigState != 0 && sigState != 1) {
    sigState = 0;
    stateMs = nowMs;
    return sigState;
  }

  unsigned long elapsed = nowMs - stateMs;

  if (sigState == 0 && elapsed >= rDurMs) {
    sigState = 1;
    stateMs = nowMs;
  } else if (sigState == 1 && elapsed >= gDurMs) {
    sigState = 0;
    stateMs = nowMs;
  }

  return sigState;
}

void setLeds(int state) {
  if (state == 0) {
    digitalWrite(rLedPin, HIGH);
    digitalWrite(gLedPin, LOW);
  } else if (state == 1) {
    digitalWrite(rLedPin, LOW);
    digitalWrite(gLedPin, HIGH);
  } else {
    // 想定外は安全側（赤）
    digitalWrite(rLedPin, HIGH);
    digitalWrite(gLedPin, LOW);
  }
}

void setBuzzer(int state, int toneHz) {
  if ((state != 0 && state != 1) || toneHz <= 0) {
    noTone(bzPin);
    lastBzState = 0;
    return;
  }

  // 状態が同じなら再設定しない
  if (state == lastBzState) {
    return;
  }

  if (state == 1) {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else {
    noTone(bzPin);
    lastBzState = 0;
  }
}

bool checkStuck(unsigned long nowMs) {
  unsigned long limit = stuckMs;

  if (sigState == 0) {
    limit = rDurMs * 2;
  } else if (sigState == 1) {
    limit = gDurMs * 2;
  }

  if (limit == 0) {
    limit = stuckMs;
  }

  if ((nowMs - stateMs) > limit) {
    sigState = 0;
    stateMs = nowMs;
    return true;
  }

  return false;
}

void fixBuzzer(int state, int toneHz) {
  if (state != 0 && state != 1) {
    noTone(bzPin);
    lastBzState = 0;
    return;
  }

  // 設計上の期待値と前回値が一致していれば補正不要
  if (lastBzState == state) {
    return;
  }

  if (state == 1 && toneHz > 0) {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else {
    noTone(bzPin);
    lastBzState = 0;
  }
}

void setBuzzerRhythm(int state, unsigned long nowMs) {
  if (state != 1) {
    noTone(bzPin);
    patternIndex = 0;
    isToneOn = false;
    return;
  }

  int toneHz = patternTones[patternIndex];
  int duration = patternDurations[patternIndex];

  if (nowMs - patternStartMs >= duration) {
    // 次のパターンへ
    patternIndex = (patternIndex + 1) % patternCount;
    patternStartMs = nowMs;
    isToneOn = !isToneOn;
  }

  if (isToneOn) {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else {
    noTone(bzPin);
    lastBzState = 0;
  }
}

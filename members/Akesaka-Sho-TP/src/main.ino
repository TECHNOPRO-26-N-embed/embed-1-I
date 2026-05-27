// =====================
// ピン定義
// =====================
#define PIN_BUTTON    2
#define PIN_LED_RED   9
#define PIN_LED_BLUE  10
#define PIN_LED_GREEN 11
#define PIN_LED_YELLOW 12
#define PIN_BUZZER    3

// =====================
// 状態管理
// =====================
int currentState = 0;  // 0:待機 1:再生 2:完了

// =====================
// タイマー
// =====================
unsigned long lastMillis_Note = 0;
unsigned long lastDebounceTime = 0;
const int DEBOUNCE_DELAY = 50;

unsigned int noteInterval = 300;

// =====================
// ボタン
// =====================
bool lastButton = false;

// =====================
// 曲① きらきら星
// =====================
int song1_notes[] = {
  262,262,392,392,440,440,392,
  349,349,330,330,294,294,262
};
int song1_length = 14;

// =====================
// 曲② かえるの歌
// =====================
int song2_notes[] = {
  262,294,330,262,
  262,294,330,262,
  330,349,392,
  330,349,392
};
int song2_length = 14;

// =====================
// 再生管理
// =====================
int noteIndex = 0;
int songSelect = 0;

// =====================
// setup
// =====================
void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_YELLOW, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);

  // 起動確認
  digitalWrite(PIN_LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(PIN_LED_GREEN, LOW);
}

// =====================
// loop
// =====================
void loop() {
  unsigned long now = millis();
  bool pressed = readButton();

  // 待機
  if (currentState == 0) {
    if (pressed) {
      noteIndex = 0;
      currentState = 1;
      Serial.println("START");
    }
  }

  // 再生
  else if (currentState == 1) {

    // 再生中に押したら停止
    if (pressed) {
      stopAll();
      return;
    }

    if (now - lastMillis_Note >= noteInterval) {
      lastMillis_Note = now;

      int note;
      int length;

      if (songSelect == 0) {
        note = song1_notes[noteIndex];
        length = song1_length;
      } else {
        note = song2_notes[noteIndex];
        length = song2_length;
      }

      if (noteIndex < length) {
        playNote(note);
        noteIndex++;
      } else {
        currentState = 2;
      }
    }
  }

  // 完了
  else if (currentState == 2) {
    stopAll();

    // 次の曲へ
    if (pressed) {
      songSelect = (songSelect + 1) % 2;
      currentState = 0;
      Serial.println("NEXT SONG");
    }
  }
}

// =====================
// ボタン（デバウンス）
// =====================
bool readButton() {
  bool reading = digitalRead(PIN_BUTTON) == LOW;

  if (reading && !lastButton) {
    unsigned long now = millis();
    if (now - lastDebounceTime > DEBOUNCE_DELAY) {
      lastDebounceTime = now;
      lastButton = true;
      return true;
    }
  }

  if (!reading) {
    lastButton = false;
  }

  return false;
}

// =====================
// 音＋LED（←ここが重要修正）
// =====================
void playNote(int note) {

  // 全LED消灯
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);

  // 音を鳴らす
  tone(PIN_BUZZER, note);

  // 音階ごとにLED制御
  switch (note) {

    // ド・ソ → 赤
    case 262:
    case 392:
      digitalWrite(PIN_LED_RED, HIGH);
      break;

    // レ・ラ → 青
    case 294:
    case 440:
      digitalWrite(PIN_LED_BLUE, HIGH);
      break;

    // ミ・シ → 緑
    case 330:
    case 494:
      digitalWrite(PIN_LED_GREEN, HIGH);
      break;

    // ファ → 黄
    case 349:
      digitalWrite(PIN_LED_YELLOW, HIGH);
      break;

    default:
      // それ以外は何もしない
      break;
  }
}

// =====================
// 全停止
// =====================
void stopAll() {
  noTone(PIN_BUZZER);

  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_YELLOW, LOW);

  currentState = 0;
  Serial.println("STOP");
}
<<<<<<< HEAD
#include <math.h>  // pow関数（指数計算）を使うためのライブラリ

//==============================
// ピン設定
//==============================
// ブザーとLEDを接続しているArduinoのピン番号
const int BUZZ_PIN = 3;     // ブザー
const int LED_RED   = 9;    // 赤LED
const int LED_BLUE  = 10;   // 青LED
const int LED_GREEN = 11;   // 緑LED
const int LED_YELLOW = 12;  // 黄LED

//==============================
// 音程（ドレミ）定義
//==============================
// 音符を分かりやすく扱うための列挙型
enum Pitch {
  NOTE_REST, // 休符
  NOTE_C,    // ド
  NOTE_D,    // レ
  NOTE_E,    // ミ
  NOTE_F,    // ファ
  NOTE_G,    // ソ
  NOTE_A,    // ラ
  NOTE_B     // シ
};

//==============================
// ノート構造体
//==============================
// 1つの音の情報をまとめたデータ構造
struct Note {
  Pitch pitch;   // 音名（ドレミ）
  int octave;    // オクターブ（高さ）
  int duration;  // 長さ（ミリ秒）
};

//==============================
// ハッピーバースデーのメロディ
//==============================
// 「ハッピーバースデー」の音符データ配列
Note melody[] = {
  // ミレドレミラソ
  { NOTE_E, 4, 300 },
  { NOTE_D, 4, 300 },
  { NOTE_C, 4, 600 },
  { NOTE_D, 4, 600 },
  { NOTE_E, 4, 600 },
  { NOTE_A, 4, 600 },
  { NOTE_G, 4, 600 },

  // ミミミレドレ
  { NOTE_E, 4, 300 },
  { NOTE_E, 4, 300 },
  { NOTE_E, 4, 600 },
  { NOTE_D, 4, 300 },
  { NOTE_C, 4, 300 },
  { NOTE_D, 4, 600 }
};

// 配列の長さ（音符の数）
int melodyLength = sizeof(melody) / sizeof(melody[0]);

//==============================
// 音程 → 周波数変換
//==============================
// 音名とオクターブから実際の周波数(Hz)を計算する
int pitchToFrequency(Pitch pitch, int octave) {
  if (pitch == NOTE_REST) return 0;  // 休符は音を出さない

  int baseFreq = 0;

  // オクターブ4の基準周波数
  switch (pitch) {
    case NOTE_C: baseFreq = 262; break;
    case NOTE_D: baseFreq = 294; break;
    case NOTE_E: baseFreq = 330; break;
    case NOTE_F: baseFreq = 349; break;
    case NOTE_G: baseFreq = 392; break;
    case NOTE_A: baseFreq = 440; break;
    case NOTE_B: baseFreq = 494; break;
    default: return 0;
  }

  // オクターブを計算（倍音関係：1オクターブ上がると2倍）
  return baseFreq * pow(2, octave - 4);
}

//==============================
// 初期設定
//==============================
void setup() {
  pinMode(BUZZ_PIN, OUTPUT);     // ブザー出力設定
  pinMode(LED_RED, OUTPUT);      // 各LEDを出力に設定
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
}

//==============================
// メインループ
//==============================
void loop() {

  // メロディを最初から最後まで再生
  for (int i = 0; i < melodyLength; i++) {

    // 周波数を取得
    int freq = pitchToFrequency(melody[i].pitch, melody[i].octave);

    // 音とLEDの制御
    playNote(melody[i].pitch, freq);

    // 音の長さだけ待つ
    delay(melody[i].duration);

    // 音停止 & LED消灯
    noTone(BUZZ_PIN);
    clearLED();

    // 次の音との間隔
    delay(50);
  }

  delay(3000); // 曲終了後3秒待って繰り返し
}

//==============================
// 音＋LED制御
//==============================
void playNote(Pitch pitch, int freq) {

  clearLED(); // 一度すべて消灯

  if (freq > 0) {
    tone(BUZZ_PIN, freq); // 音を鳴らす
  }

  // 音ごとにLEDの色を変える
  switch (pitch) {

    case NOTE_C:
    case NOTE_G:
      digitalWrite(LED_RED, HIGH);   // 赤
      break;

    case NOTE_D:
    case NOTE_A:
      digitalWrite(LED_BLUE, HIGH);  // 青
      break;

    case NOTE_E:
    case NOTE_B:
      digitalWrite(LED_GREEN, HIGH); // 緑
      break;

    case NOTE_F:
      digitalWrite(LED_YELLOW, HIGH); // 黄
=======
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
>>>>>>> 8eca6b24a66232c574a20e0e4706a1ce9cb13e8e
      break;
  }
}

<<<<<<< HEAD
//==============================
// LED消灯処理
//==============================
void clearLED() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
}
=======
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
>>>>>>> 8eca6b24a66232c574a20e0e4706a1ce9cb13e8e

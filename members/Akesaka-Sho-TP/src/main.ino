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
      break;
  }
}

//==============================
// LED消灯処理
//==============================
void clearLED() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
}

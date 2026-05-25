#include <Arduino.h>

//==============================
// ピン設定
//==============================
const int BUZZ_PIN = 3;

//==============================
// 音程・音符・メロディー定義
//==============================
enum Pitch {
  NOTE_REST,
  NOTE_Des,
  NOTE_Es,
  NOTE_E,
  NOTE_F,
  NOTE_As,
  NOTE_B
};

// beatDiv: 1=全音符, 2=2分, 4=4分, 8=8分...
struct NoteEvent {
  Pitch pitch;
  int octave;
  int beatDiv;
};

struct Melody {
  const NoteEvent* notes;
  int noteCount;
  int bpm;
};

//==============================
// 変換・再生共通関数
//==============================
int pitchToFrequency(Pitch pitch, int octave) {
  if (pitch == NOTE_REST) {
    return 0;
  }

  int baseFreq = 0;
  switch (pitch) {
    case NOTE_Des: baseFreq = 278; break;
    case NOTE_Es: baseFreq = 310; break;
    case NOTE_E: baseFreq = 322; break;
    case NOTE_F: baseFreq = 354; break;
    case NOTE_As: baseFreq = 398; break;
    case NOTE_B: baseFreq = 430; break;
    default: return 0;
  }

  int shift = octave - 4;
  if (shift > 0) return baseFreq << shift;
  if (shift < 0) return baseFreq >> (-shift);
  return baseFreq;
}

unsigned long beatDivToDurationMs(int bpm, int beatDiv) {
  if (bpm <= 0 || beatDiv <= 0) {
    return 0;
  }
  // 4分音符 = 60000 / bpm
  // 任意音価 = (4分音符) * (4 / beatDiv)
  return (60000UL * 4UL) / ((unsigned long)bpm * (unsigned long)beatDiv);
}

void playMelody(const Melody& melody) {
  for (int i = 0; i < melody.noteCount; i++) {
    const NoteEvent& n = melody.notes[i];

    unsigned long durationMs = beatDivToDurationMs(melody.bpm, n.beatDiv);
    int freq = pitchToFrequency(n.pitch, n.octave);

    if (freq > 0) {
      unsigned long playMs = (durationMs * 9UL) / 10UL;  // 90%
      tone(BUZZ_PIN, freq, playMs);
    } else {
      noTone(BUZZ_PIN);
    }

    delay(durationMs);
    noTone(BUZZ_PIN);
  }
}

//==============================
// メロディーデータ（関数呼び出しだけで再生）
//==============================
void playColorYourNight() {
  static const NoteEvent notes[] = {
    {NOTE_Des, 4, 4}, {NOTE_B, 4, 4}, {NOTE_As, 4, 16}, {NOTE_As, 4, 8},
    {NOTE_Es, 4, 4}, {NOTE_Es, 4, 4}, {NOTE_Des, 5, 2}
  };

  const Melody m = {
    notes,
    (int)(sizeof(notes) / sizeof(notes[0])),
    380
  };

  playMelody(m);
}

//==============================
// 初期化
//==============================
void setup() {
  pinMode(BUZZ_PIN, OUTPUT);
}

//==============================
// メインループ
//==============================
void loop() {
  // ここは関数呼び出しだけで再生
  playColorYourNight();

  // 1回だけ再生して停止
  while (1) {
    // ここは関数呼び出しだけで再生
    playColorYourNight();

    delay(10000);  // 10秒待ってから再度再生
  }
}
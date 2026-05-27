#include <LiquidCrystal.h>

//==============================
// ピン設定
//==============================
const int BUZZ_PIN = 3;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//==============================
// グローバル変数
//==============================

// メロディーの長さ（melody 定義後に計算するのが重要）
const int NUM_NOTES = 3;

//==============================
// 音程を列挙体で定義
//==============================
enum Pitch
{
    NOTE_REST, // 休符
    NOTE_C,
    NOTE_D,
    NOTE_E,
    NOTE_F,
    NOTE_G,
    NOTE_A,
    NOTE_B
};

//==============================
// 1音を表す構造体
//==============================
struct Note
{
    enum Pitch pitch; // 音程
    int octave;       // オクターブ（4, 5 など）
    int duration;     // 音の長さ（ms）
};

//==============================
// メロディーデータ
//==============================

struct Note melody[3][64] = {
    {// カエルの歌
     {NOTE_C, 4, 600},
     {NOTE_D, 4, 600},
     {NOTE_E, 4, 600},
     {NOTE_F, 4, 600},
     {NOTE_E, 4, 600},
     {NOTE_D, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_E, 4, 600},
     {NOTE_F, 4, 600},
     {NOTE_G, 4, 600},
     {NOTE_A, 4, 600}, //
     {NOTE_G, 4, 600},
     {NOTE_F, 4, 600},
     {NOTE_E, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_C, 4, 300},
     {NOTE_C, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 600},
     {NOTE_D, 4, 600},
     {NOTE_C, 4, 600},
     {NOTE_REST, 4, 600},
     {NOTE_REST, 5, 600}},
    {// きらきら星
     {NOTE_C, 4, 300},
     {NOTE_C, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_G, 4, 600},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_C, 4, 600},
     {NOTE_C, 4, 300},
     {NOTE_C, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_G, 4, 600},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_C, 4, 600},
     {NOTE_G, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_D, 4, 600},
     {NOTE_G, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_D, 4, 600},
     {NOTE_C, 4, 300},
     {NOTE_C, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_G, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_A, 4, 300},
     {NOTE_G, 4, 600},
     {NOTE_F, 4, 300},
     {NOTE_F, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_E, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_D, 4, 300},
     {NOTE_C, 4, 600},
     {NOTE_REST, 5, 600}},
    {// ハッピーバースデー
     {NOTE_G, 4, 450},
     {NOTE_G, 4, 150},
     {NOTE_A, 4, 600},
     {NOTE_G, 4, 600},
     {NOTE_C, 5, 600},
     {NOTE_B, 4, 1200},
     {NOTE_G, 4, 450},
     {NOTE_G, 4, 150},
     {NOTE_A, 4, 600},
     {NOTE_G, 4, 600},
     {NOTE_D, 5, 600},
     {NOTE_C, 5, 1200},
     {NOTE_G, 4, 450},
     {NOTE_G, 4, 150},
     {NOTE_G, 5, 600},
     {NOTE_E, 5, 600},
     {NOTE_C, 5, 600},
     {NOTE_B, 4, 600},
     {NOTE_A, 4, 1200},
     {NOTE_F, 5, 450},
     {NOTE_F, 5, 150},
     {NOTE_E, 5, 600},
     {NOTE_C, 5, 600},
     {NOTE_D, 5, 600},
     {NOTE_C, 5, 1200},
     {NOTE_REST, 5, 600}}};

//==============================
// enum → 周波数変換
//==============================
int pitchToFrequency(int pitch, int octave)
{
    if (pitch == NOTE_REST)
        return 0;

    int baseFreq = 0;

    switch (pitch)
    {
    case NOTE_C:
        baseFreq = 262;
        break;
    case NOTE_D:
        baseFreq = 294;
        break;
    case NOTE_E:
        baseFreq = 330;
        break;
    case NOTE_F:
        baseFreq = 349;
        break;
    case NOTE_G:
        baseFreq = 392;
        break;
    case NOTE_A:
        baseFreq = 440;
        break;
    case NOTE_B:
        baseFreq = 494;
        break;
    default:
        return 0;
    }

    // octave = 4 を基準に上下させる（負のシフト対策済み）
    int shift = octave - 4;
    if (shift > 0)
    {
        return baseFreq << shift;
    }
    else if (shift < 0)
    {
        return baseFreq >> (-shift);
    }
    else
    {
        return baseFreq;
    }
}

//==============================
// 初期化
//==============================
void setup()
{
    // pinMode(BUZZ_PIN, OUTPUT);
    lcd.begin(16, 2);
}

//==============================
// メインループ
//==============================
void loop()
{
    while (1)
    {
        for (int j = 0; j < NUM_NOTES; j++)
        {
            for (int i = 0; i < 64; i++)
            {
                switch (j)
                {
                case 0:
                    lcd.setCursor(0, 0);
                    lcd.print("The Flog Song   ");
                    break;
                case 1:
                    lcd.setCursor(0, 0);
                    lcd.print("Kirakira Boshi");
                    break;
                case 2:
                    lcd.setCursor(0, 0);
                    lcd.print("Happy Birthday!");
                    break;
                }

                int freq = pitchToFrequency(melody[j][i].pitch, melody[j][i].octave);

                if (freq > 0)
                {
                    // 音の長さの90%だけ鳴らす（安全設計）
                    int playTime = melody[j][i].duration * 0.9;
                    tone(BUZZ_PIN, freq, playTime);
                }

                delay(melody[j][i].duration);
            }
        }
    }
}
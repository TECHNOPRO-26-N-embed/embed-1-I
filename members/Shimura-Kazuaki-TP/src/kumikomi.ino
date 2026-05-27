// ミニチュア版歩行者信号灯
// 赤LED: D8 / 緑LED: D9 / パッシブブザー: D3

// const: 値を変更しない定数
// byte : 0〜255 を扱う小さな整数型（Arduinoでピン番号によく使う）

// rLedPin: 赤LEDを接続したデジタルピン番号（D8）
const byte rLedPin = 8;

// gLedPin: 緑LEDを接続したデジタルピン番号（D9）
const byte gLedPin = 9;

// bzPin: パッシブブザーを接続したデジタルピン番号（D3）
const byte bzPin = 3;

// sigState: 現在の信号状態を表す変数（0: 赤信号中, 1: 青信号中）
// int を使うのは、状態値の比較（== 0 / == 1）を分かりやすくするため
int sigState = 0;

// stateMs: 現在の状態に切り替わった時刻（millisの値）
// switchState() で「どれだけ時間が経ったか」を計算する基準になる
unsigned long stateMs = 0;

// rDurMs: 赤信号を維持する時間（ミリ秒）
// 5000ms = 5秒
const unsigned long rDurMs = 5000;

// gDurMs: 青信号を維持する時間（ミリ秒）
// 5000ms = 5秒
const unsigned long gDurMs = 5000;

// gToneHz: 青信号中に鳴らすブザーの周波数（Hz）
const int gToneHz = 2000;

// stuckMs: 異常監視の基準時間（ミリ秒）
// checkStuck() で遷移停止を判定する際のデフォルト閾値
const unsigned long stuckMs = 10000;

// lastBzState: 直前にブザーへ適用した状態を覚えておく変数
// 0: 停止, 1: 鳴動, -1: まだ未初期化（起動直後の目印）
// 同じ状態を連続で指示したとき、tone/noTone の無駄な再実行を防ぐために使う
int lastBzState = -1;

// ここから下は「関数の宣言（プロトタイプ）」
// 先に名前・引数・戻り値だけを示して、loop() などから先に呼べるようにしている

// switchState: 経過時間を見て赤/青の状態を切り替え、現在状態を返す
int switchState(unsigned long nowMs);

// setLeds: 引数 state に応じて赤LED/緑LEDの点灯状態を反映する
void setLeds(int state);

// setBuzzer: 引数 state と toneHz に応じてブザーの鳴動/停止を制御する
void setBuzzer(int state, int toneHz);

// checkStuck: 状態遷移が止まっていないか監視し、異常時は true を返す
bool checkStuck(unsigned long nowMs);

// fixBuzzer: 記録状態と実際の期待状態の不一致を補正する
void fixBuzzer(int state, int toneHz);

// 信号リズム用の設定値

// patternCount: リズムを構成する要素数
// ここを変える場合は、下の2つの配列要素数も同じにする
const int patternCount = 4;

// patternTones: 各ステップで使う音の周波数（Hz）
// 例: 523Hz=ド, 392Hz=ソ
const int patternTones[patternCount] = { 523, 392, 523, 392 }; // ド・ソ・ド・ソ

// patternDurations: 各ステップを維持する時間（ms）
// patternTones と同じ添字を使い、音と時間を1対1で対応させる
const int patternDurations[patternCount] = { 150, 300, 150, 500 };  // ms単位

// patternIndex: 現在再生中のリズム位置（配列の添字）
// 0 〜 patternCount-1 の範囲で進み、最後まで行くと先頭に戻る
int patternIndex = 0;

// patternStartMs: 現在ステップを開始した時刻（millisの値）
// nowMs との差を使って、次のステップへ進むタイミングを判定する
unsigned long patternStartMs = 0;

// isToneOn: 現在ステップでブザーを鳴らすかどうかのフラグ
// true=鳴らす, false=止める（tone/noTone の切り替えに使用）
bool isToneOn = false;

// 初期設定を行い、起動時の表示と内部状態をそろえる関数
void setup() 
{
  // 使用するピンを出力モードに設定する
  pinMode(rLedPin, OUTPUT);
  pinMode(gLedPin, OUTPUT);
  pinMode(bzPin, OUTPUT);

  // 起動直後の状態を「赤信号中」に初期化する
  sigState = 0;

  // 現在時刻を記録して、状態遷移タイマーの基準にする
  stateMs = millis();

  // ブザー状態の記録を「停止」にそろえる
  lastBzState = 0;

  // 初期表示を赤点灯・緑消灯にする
  digitalWrite(rLedPin, HIGH);
  digitalWrite(gLedPin, LOW);

  // 起動時はブザーを停止しておく
  noTone(bzPin);

  // リズム再生の内部状態を先頭ステップに初期化する
  patternIndex = 0;
  patternStartMs = millis();
  isToneOn = false;
}

// 現在時刻に基づいて状態更新・異常監視・出力反映を繰り返す関数
void loop() 
{
  // 現在時刻（ミリ秒）を取得する
  unsigned long nowMs = millis();

  // 経過時間に応じて信号状態を更新する（赤↔青）
  sigState = switchState(nowMs);

  // 状態遷移が止まっている異常を検知したら、安全側の赤信号に戻す
  if (checkStuck(nowMs)) 
  {
    sigState = 0;
  }

  // 更新後の状態を出力デバイスへ反映する
  setLeds(sigState);

  // 青信号中のみ、設定したリズムでブザーを鳴動/停止する
  setBuzzerRhythm(sigState, nowMs);

  // ブザー状態の不一致があれば補正する（安全対策）
  fixBuzzer(sigState, gToneHz);
}

// 経過時間から赤信号と青信号を切り替えて現在状態を返す関数
int switchState(unsigned long nowMs) 
{
  // 想定外の状態値は安全側（赤信号）へ矯正してタイマーを再始動する
  if (sigState != 0 && sigState != 1) 
  {
    sigState = 0;
    stateMs = nowMs;
    return sigState;
  }

  // 現在の状態に入ってからの経過時間を計算する
  unsigned long elapsed = nowMs - stateMs;

  // 赤信号中に所定時間が経過したら青信号へ切り替える
  if (sigState == 0 && elapsed >= rDurMs) 
  {
    sigState = 1;
    stateMs = nowMs;
  // 青信号中に所定時間が経過したら赤信号へ切り替える
  } else if (sigState == 1 && elapsed >= gDurMs) 
  {
    sigState = 0;
    stateMs = nowMs;
  }

  // 更新後（または維持中）の現在状態を返す
  return sigState;
}

// 渡された状態値に応じて赤LEDと緑LEDの点灯状態を設定する関数
void setLeds(int state) 
{
  // 引数 state に応じて、赤LED/緑LEDの表示状態を反映する
  // state=0（赤信号中）: 赤LEDを点灯し、緑LEDを消灯する
  if (state == 0) 
  {
    digitalWrite(rLedPin, HIGH);
    digitalWrite(gLedPin, LOW);
  // state=1（青信号中）: 緑LEDを点灯し、赤LEDを消灯する
  } else if (state == 1) 
  {
    digitalWrite(rLedPin, LOW);
    digitalWrite(gLedPin, HIGH);
  } else 
  {
    // 想定外の値は安全側（赤信号表示）に固定する
    digitalWrite(rLedPin, HIGH);
    digitalWrite(gLedPin, LOW);
  }
}

// 渡された状態値と周波数に応じてブザーを鳴動または停止する関数
void setBuzzer(int state, int toneHz) 
{
  // 状態値が不正、または周波数が0以下なら安全のため無音にする
  if ((state != 0 && state != 1) || toneHz <= 0) 
  {
    noTone(bzPin);
    lastBzState = 0;
    return;
  }

  // 前回と同じ状態なら、tone/noTone の再発行を避ける
  if (state == lastBzState) 
  {
    return;
  }

  // state=1（青信号中）なら指定周波数で鳴らす
  if (state == 1) 
  {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else 
  {
    // state=0（赤信号中）なら停止する
    noTone(bzPin);
    lastBzState = 0;
  }
}

// 状態遷移が止まっていないかを監視し、異常時に復旧する関数
bool checkStuck(unsigned long nowMs) 
{
  // まずはデフォルトの監視閾値を設定する
  unsigned long limit = stuckMs;

  // 現在状態に応じて監視閾値を決める（通常は各状態時間の2倍）
  if (sigState == 0) 
  {
    limit = rDurMs * 2;
  } else if (sigState == 1) 
  {
    limit = gDurMs * 2;
  }

  // 設定値異常で0になった場合は、デフォルト値で監視を継続する
  if (limit == 0) 
  {
    limit = stuckMs;
  }

  // 現在状態の継続時間が閾値を超えたら「遷移停止」と判定する
  if ((nowMs - stateMs) > limit) 
  {
    // 安全側として赤信号に戻し、監視タイマーを再初期化する
    sigState = 0;
    stateMs = nowMs;
    return true;
  }

  // 異常なし
  return false;
}

// ブザーの記録状態と期待状態の不一致を補正する関数
void fixBuzzer(int state, int toneHz) 
{
  // 状態値が不正な場合は、安全のため無音に戻して記録も停止にそろえる
  if (state != 0 && state != 1) 
  {
    noTone(bzPin);
    lastBzState = 0;
    return;
  }

  // 期待する状態と記録済み状態が一致していれば補正は不要
  if (lastBzState == state) 
  {
    return;
  }

  // 不一致時のみ補正を行う: 青信号かつ有効周波数なら鳴動へ補正
  if (state == 1 && toneHz > 0) 
  {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else 
  {
    // それ以外は停止側へ補正する（赤信号時/無効周波数時）
    noTone(bzPin);
    lastBzState = 0;
  }
}

// 青信号中にリズムパターンでブザーを鳴動・停止させる関数
void setBuzzerRhythm(int state, unsigned long nowMs) 
{
  // 青信号中（state=1）以外はリズム再生を停止し、内部状態を初期化する
  if (state != 1) 
  {
    noTone(bzPin);
    patternIndex = 0;
    isToneOn = false;
    return;
  }

  // 現在のステップに対応する音程と継続時間を取得する
  int toneHz = patternTones[patternIndex];
  int duration = patternDurations[patternIndex];

  // 現在ステップの時間が過ぎたら、次のステップへ進める
  if (nowMs - patternStartMs >= duration) 
  {
    // 末尾まで進んだら先頭へ戻る（循環再生）
    patternIndex = (patternIndex + 1) % patternCount;
    patternStartMs = nowMs;
    // 鳴らす区間と止める区間を交互に切り替える
    isToneOn = !isToneOn;
  }

  // フラグに応じて鳴動/停止を反映し、適用状態を記録する
  if (isToneOn) 
  {
    tone(bzPin, toneHz);
    lastBzState = 1;
  } else 
  {
    noTone(bzPin);
    lastBzState = 0;
  }
}

// ===== ピン定義 =====
// 各部品をArduinoのどのピンに接続するかを決める
const int PIN_TRIG        = 9;    // 超音波センサー：信号を送る
const int PIN_ECHO        = 10;   // 超音波センサー：反射を受ける
const int PIN_LED         = 3;    // LED
const int PIN_BUZZER      = 5;    // ブザー
const int PIN_BTN_CONFIRM = 2;    // 確認ボタン（押すとLOW）
const int PIN_BTN_STOP    = 4;    // 停止ボタン（押すとLOW）

// ===== 状態定義 =====
// システムの動作状態を数字で表現する（状態機械）
#define STATE_STANDBY    0   // 人がいない状態
#define STATE_WORKING    1   // 人がいる状態
#define STATE_CHECK_WAIT 2   // 確認待ち（10秒）
#define STATE_ALERT      3   // 警告中
#define STATE_SAFE       4   // 安全状態

int  currentState = STATE_STANDBY; // 現在の状態
bool alertFlag    = false;         // ブザーが鳴ったか（重複防止）

// ===== 変数 =====
int distance     = 0;   // 現在の距離
int prevDistance = 0;   // 前回の正しい距離（エラー対策）
int stableCount  = 0;   // 同じ状態が続いた回数（ノイズ防止）

bool btnConfirm = false; // 確認ボタン
bool btnStop    = false; // 停止ボタン

int ledState = LOW; // LEDの状態（ON/OFF）

// ===== タイマー変数（millis用）=====
unsigned long lastMillis_Sensor   = 0; // センサー更新時間
unsigned long lastMillis_LED      = 0; // LED点滅時間
unsigned long lastMillis_Check    = 0; // 確認待ち開始時間
unsigned long lastMillis_Debounce = 0; // ボタン誤動作防止時間

// ===== 定数 =====
const unsigned long SENSOR_INTERVAL = 100;   // センサーは100msごとに読む
const unsigned long LED_INTERVAL    = 500;   // LEDは500msで点滅
const unsigned long CHECK_TIMEOUT   = 10000; // 10秒後に警告
const unsigned long DEBOUNCE_DELAY  = 50;    // ボタンの誤動作防止

const int DIST_PRESENT = 50;   // 50cm以内 → 人がいる
const int DIST_ABSENT  = 100;  // 100cm以上 → 人がいない
const int DIST_MIN     = 2;    // 最小距離
const int DIST_MAX     = 400;  // 最大距離
const int STABLE_COUNT = 3;    // 3回連続で確定

// =================================================
// setup()
// =================================================
void setup() {

  // ピンの設定（入力 / 出力）
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_CONFIRM, INPUT_PULLUP);
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);

  // シリアル通信（デバッグ用）
  Serial.begin(9600);

  // 起動確認（LEDとブザー）
  digitalWrite(PIN_LED, HIGH);
  delay(500);
  digitalWrite(PIN_LED, LOW);

  tone(PIN_BUZZER, 1000);
  delay(100);
  noTone(PIN_BUZZER);

  Serial.println("System ready. State: STANDBY");
}

// =================================================
// loop()
// =================================================
void loop() {

  unsigned long now = millis(); // 現在の時間を取得

  measureDistance(now); // 距離を更新
  btnConfirm = readButton(PIN_BTN_CONFIRM, now); // ボタン読み取り
  btnStop    = readButton(PIN_BTN_STOP, now);

  // ===== 状態ごとの処理 =====
  switch (currentState) {

    case STATE_STANDBY:
      // 人が近づいたら
      if (distance < DIST_PRESENT) {
        stableCount++;
        if (stableCount >= STABLE_COUNT) {
          currentState = STATE_WORKING; // 作業状態へ
          stableCount = 0;
          Serial.println("State: WORKING");
        }
      } else stableCount = 0;
      break;

    case STATE_WORKING:
      // 人が離れたかチェック
      if (detectLeaving(distance)) {
        currentState = STATE_CHECK_WAIT;
        lastMillis_Check = now; // タイマー開始
        Serial.println("State: CHECK_WAIT");
      }
      break;

    case STATE_CHECK_WAIT: {
      int result = waitForConfirm(now);

      if (result == 1) {
        currentState = STATE_SAFE; // ボタン押下
        Serial.println("State: SAFE");
      }
      else if (result == 2) {
        currentState = STATE_ALERT; // 10秒後
        alertFlag = false;
        Serial.println("State: ALERT");
      }
      break;
    }

    case STATE_ALERT:
      // 警告動作
      activateAlert(now);

      if (btnConfirm || btnStop) {
        stopAlert(); // 停止
      }
      break;

    case STATE_SAFE:
      // 人が戻ったら作業に戻る
      if (distance < DIST_PRESENT) {
        stableCount++;
        if (stableCount >= STABLE_COUNT) {
          currentState = STATE_WORKING;
          stableCount = 0;
          Serial.println("State: WORKING (returned)");
        }
      } else stableCount = 0;
      break;
  }

  updateOutput(now); // 出力更新（LEDなど）
}

// =================================================
// readDistance()
// =================================================
int readDistance() {

  // 超音波を送る
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // 反射時間を取得
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duration == 0) return -1; // タイムアウト

  int dist = duration / 58; // cmに変換

  // 異常値チェック
  if (dist < DIST_MIN || dist > DIST_MAX) return -1;

  return dist;
}

// =================================================
// measureDistance()
// =================================================
void measureDistance(unsigned long now) {

  // 100msごとに実行
  if (now - lastMillis_Sensor < SENSOR_INTERVAL) return;

  lastMillis_Sensor = now;

  int raw = readDistance();

  if (raw == -1) {
    distance = prevDistance; // エラー時は前回値
  } else {
    distance = raw;
    prevDistance = raw;
  }
}

// =================================================
// readButton()
// =================================================
bool readButton(int pin, unsigned long now) {

  // 押されていない
  if (digitalRead(pin) == HIGH) return false;

  // チャタリング防止（早すぎる入力は無視）
  if (now - lastMillis_Debounce < DEBOUNCE_DELAY) return false;

  lastMillis_Debounce = now;
  return true;
}

// =================================================
// detectLeaving()
// =================================================
bool detectLeaving(int dist) {

  if (dist <= DIST_ABSENT) {
    stableCount = 0; // 条件を満たさない
    return false;
  }

  stableCount++;

  if (stableCount >= STABLE_COUNT) {
    stableCount = 0;
    return true; // 離席確定
  }

  return false;
}

// =================================================
// waitForConfirm()
// =================================================
int waitForConfirm(unsigned long now) {

  if (btnConfirm) return 1; // ボタン押された

  if (now - lastMillis_Check >= CHECK_TIMEOUT) return 2; // タイムアウト

  return 0;
}

// =================================================
// activateAlert()
// =================================================
void activateAlert(unsigned long now) {

  // LED高速点滅
  if (now - lastMillis_LED >= 200) {
    lastMillis_LED = now;
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState);
  }

  // ブザー鳴動（1回だけ）
  if (!alertFlag) {
    tone(PIN_BUZZER, 1000);
    alertFlag = true;
  }
}

// =================================================
// stopAlert()
// =================================================
void stopAlert() {

  noTone(PIN_BUZZER); // ブザー停止
  digitalWrite(PIN_LED, LOW); // LED消灯

  alertFlag = false;
  currentState = STATE_SAFE;

  Serial.println("Alert stopped");
}

// =================================================
// updateOutput()
// =================================================
void updateOutput(unsigned long now) {

  // 確認待ち → ゆっくり点滅
  if (currentState == STATE_CHECK_WAIT) {
    if (now - lastMillis_LED >= LED_INTERVAL) {
      lastMillis_LED = now;
      ledState = !ledState;
      digitalWrite(PIN_LED, ledState);
    }
  }

  // 作業中 → 点灯
  if (currentState == STATE_WORKING) {
    digitalWrite(PIN_LED, HIGH);
  }

  // その他 → 消灯
  if (currentState == STATE_STANDBY || currentState == STATE_SAFE) {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
  }
}
const int trigPin = 9;
const int echoPin = 10;
const int buzzer = 3;

// 距離関連
long duration;
int distance;

// 状態管理
int state = 0; 
// 0:無音 1:弱 2:中 3:強 4:エラー（強警告と同じ動作）

// millis制御
unsigned long prevMeasureTime = 0;
unsigned long prevBeepTime = 0;
bool beepState = false;

// エラー管理
int errorCount = 0;
const int errorThreshold = 3;

// 設定値
const int measureInterval = 100; // 測定周期(ms)

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 距離測定（周期制御） ---
  if (currentMillis - prevMeasureTime >= measureInterval) {
    prevMeasureTime = currentMillis;
    int measured = measureDistance();

    if (measured < 2 || measured > 200) { // 有効範囲外・異常値
      errorCount++;
      if (errorCount >= errorThreshold) {
        state = 4; // エラー状態（強警告）
      }
    } else {
      errorCount = 0;
      distance = measured;
      state = judgeState(distance);
    }

    Serial.print("Distance: ");
    Serial.println(measured);
    Serial.print("State: ");
    Serial.println(state);
  }

  // --- ブザー制御 ---
  controlBuzzer(currentMillis);
}

// ==========================
// 距離測定
// ==========================
int measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  if (duration == 0) return 0; // タイムアウトも異常扱い

  int dist = duration * 0.034 / 2;
  return dist;
}

// ==========================
// 状態判定
// ==========================
int judgeState(int d) {
  if (d > 100) return 0;
  else if (d > 50) return 1;
  else if (d > 20) return 2;
  else return 3;
}

// ==========================
// ブザー制御（millis版）
// ==========================
void controlBuzzer(unsigned long currentMillis) {

  int interval;
  int freq;

  if (state == 0) { // 無音
    noTone(buzzer);
    return;
  }
  if (state == 4 || state == 3) { // 強警告またはエラー
    tone(buzzer, 2000);
    return;
  }
  if (state == 1) { // 弱警告
    interval = 1000;
    freq = 1000;
  } else if (state == 2) { // 中警告
    interval = 400;
    freq = 1500;
  } else {
    noTone(buzzer);
    return;
  }

  // 断続音制御（non-blocking）
  if (currentMillis - prevBeepTime >= interval / 2) {
    prevBeepTime = currentMillis;

    if (beepState) {
      noTone(buzzer);
      beepState = false;
    } else {
      tone(buzzer, freq);
      beepState = true;
    }
  }
}
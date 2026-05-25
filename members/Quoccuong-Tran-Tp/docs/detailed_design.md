# 詳細設計書 — 組込み開発実習

<!-- 作成者: トラン　クオククオン / 日付: 2026-05-25 / グループ: 1-i -->

> **このドキュメントの目的**
> 基本設計書（basic_design.md）で「**どのような構造で作るか**」を決めました。
> この詳細設計書では「**各処理を具体的にどう実装するか**」を決めます。
> 書き終わったとき、**コードの骨格がほぼ完成している**状態を目指してください。

> [!NOTE]
> **V字モデルにおける位置づけ**
> 詳細設計書 ←→ **単体テスト**（関数・部品ごとのテスト）が対応します。
> 「この関数が正しく動くか」の確認は Section 5 の単体テスト仕様書で計画します。
> ※ 必須機能全体が動くかの「結合テスト」は基本設計書（Section 6）に記載します。

---

## 0. 基本設計書との接続確認

| 項目 | basic_design.md から転記 |
|:--|:--|
| 作品タイトル | 忘れ物を防止するデスク用警告装置 |
| 状態の種類（1-2 状態遷移から） | 待機中 / 作業中 / 確認待ち / 警告中 / 安全（合計5種類） |
| 実装する関数の数（2-2 関数一覧から） | 　13個（setup・loop含む）|
| グローバル変数の合計バイト数（2-1 SRAM確認から） | 　10B |

---

## 1. グローバル変数・定数の設計

> ※ 基本設計書（2-1 データ設計）をもとに、**型と初期値まで**決めます。
> ここで設計した変数は、この後の関数設計でそのまま使います。

```
【ピン定義】（basic_design.md 3-1 から転記）
  const int PIN_TRIG        = 9;   // 超音波センサー Trig（距離測定のトリガ送信）
const int PIN_ECHO        = 10;  // 超音波センサー Echo（反射波の受信）
const int PIN_LED         = 3;   // 赤LED（警告表示用）※ 220Ω抵抗を直列接続
const int PIN_BUZZER      = 5;   // パッシブブザー（警告音）※ PWM対応ピン
const int PIN_BTN_CONFIRM = 2;   // 確認ボタン（INPUT_PULLUP → 押すと LOW）
const int PIN_BTN_STOP    = 4;   // 停止ボタン（INPUT_PULLUP → 押すと LOW）

【状態管理】（basic_design.md 1-2 の状態名から転記）
#define STATE_STANDBY    0   // 待機中：人がいない、センサーで監視中
#define STATE_WORKING    1   // 作業中：人が机の前にいる
#define STATE_CHECK_WAIT 2   // 確認待ち：離席を検知、10秒間ボタン待ち
#define STATE_ALERT      3   // 警告中：忘れ物の可能性、LED点滅＋Buzzer
#define STATE_SAFE       4   // 安全：ユーザーが確認済み

// ── 状態管理変数 ────────────────────────────────────────────────────
int currentState = STATE_STANDBY;  // 現在の状態（最初は「待機中」から始まる）

【タイマー（millis()用）】（basic_design.md 2-3 から転記）
// ── タイマー変数（millis()用）─────────────────────────────────────
unsigned long lastMillis_Sensor  = 0;  // センサー前回読み取り時刻
unsigned long lastMillis_LED     = 0;  // LED前回切り替え時刻
unsigned long lastMillis_Check   = 0;  // 確認待ち開始時刻
unsigned long lastMillis_Debounce = 0; // ボタンデバウンス前回時刻

// ── 周期・閾値の定数 ────────────────────────────────────────────────
const unsigned long SENSOR_INTERVAL  = 100;   // センサー読み取り周期（ms）
const unsigned long LED_INTERVAL     = 500;   // LED点滅周期（ms）
const unsigned long CHECK_TIMEOUT    = 10000; // 確認待ちタイムアウト（10秒）
const unsigned long DEBOUNCE_DELAY   = 50;    // チャタリング防止時間（ms）

// ── 距離判定の閾値 ──────────────────────────────────────────────────
const int DIST_PRESENT  = 50;   // これより近い → 人が机にいると判断（cm）
const int DIST_ABSENT   = 100;  // これより遠い → 席を離れたと判断（cm）
const int DIST_MIN      = 2;    // これより近い → センサー異常値（cm）
const int DIST_MAX      = 400;  // これより遠い → センサー異常値（cm）
const int STABLE_COUNT  = 3;    // 何回連続で同じ判定が続いたら確定とするか


【センサー・入力値】（basic_design.md 2-1 から転記）
int  distance    = 0;     // 超音波センサーで測定した距離（単位：cm）
int  prevDistance = 0;    // 前回の正常な距離値（異常値フィルタリング用）
bool btnConfirm  = false; // 確認ボタンの状態（true = 押されている）
bool btnStop     = false; // 停止ボタンの状態（true = 押されている）
bool alertFlag   = false; // 警告フラグ（true = 警告中）
int  ledState    = LOW;   // LED の現在の点灯状態（点滅制御用）
int  stableCount = 0;     // 連続して同じ状態が続いた回数（ノイズ除去用）


【その他のフラグ・カウンター】
  （自分のものを追加）
```

---

## 2. 各関数の詳細設計

> ※ 基本設計書（2-2 関数一覧）で定義した各関数の「中身」を設計します。
> **疑似コード**（日本語＋処理の流れ）で書いてください。実際のC++コードは書かなくてOKです。

---

### `setup()` — 初期化処理

```

### 2.1`setup()` — 初期化処理
```
【処理の流れ】

1. ピンモードを設定する
   - pinMode(PIN_TRIG,        OUTPUT)       // センサーへ信号を送る側
   - pinMode(PIN_ECHO,        INPUT)        // センサーから信号を受け取る側
   - pinMode(PIN_LED,         OUTPUT)       // LEDを光らせる側
   - pinMode(PIN_BUZZER,      OUTPUT)       // ブザーを鳴らす側
   - pinMode(PIN_BTN_CONFIRM, INPUT_PULLUP) // ボタン（押すとLOW）
   - pinMode(PIN_BTN_STOP,    INPUT_PULLUP) // ボタン（押すとLOW）

2. シリアル通信を開始する（デバッグ確認用）
   - Serial.begin(9600)

3. 起動確認（ユーザーへのフィードバック）
   - LEDを500ms点灯 → 消灯
   - ブザーを100ms鳴らす → 止める
   → これで「ちゃんと起動した」ことが確認できる

4. 初期状態をシリアルに出力する
   - Serial.println("System ready. State: STANDBY")


---

### 2.2`loop()` — メインループ

> ※ loop() は「状態ごとに何をするか」だけ書く。細かい処理は各関数に任せる。

```
【処理の流れ】

＜毎ループ実行すること＞
  - 入力を読む（readButton(), readSensor() などを呼ぶ）
  - 現在時刻を取得: now = millis()

＜currentState が 0（待機中）のとき＞
  - センサー値を監視する
  - 検知条件を満たしたら → currentState = 1

＜currentState が 1（動作中）のとき＞
  - メイン処理を行う
  - 終了条件を満たしたら → currentState = 2

＜currentState が 2（完了）のとき＞
  - 完了表示をする
  - リセットボタンが押されたら → currentState = 0

＜currentState が 3（エラー）のとき＞
  - エラー表示をする / リセットを待つ
```
```
### 2.2 loop()-メインループ
【処理の流れ】

＜毎ループ実行すること＞

  1. 現在時刻を取得: now = millis()
  2. センサーで距離を読む: measureDistance(now)
  3. ボタン状態を読む: btnConfirm = readButtonConfirm(now)
                       btnStop    = readButtonStop(now)
  4. 状態に応じた処理を実行（以下の switch 分岐）
  5. 出力を更新: updateOutput(now)

＜currentState が STATE_STANDBY（待機中）のとき＞
  - センサーで人を検知しているか確認
  - distance < DIST_PRESENT が STABLE_COUNT 回連続 → STATE_WORKING へ遷移
  - LED消灯、ブザー停止を維持

＜currentState が STATE_WORKING（作業中）のとき＞
  - 離席していないか監視
  - distance > DIST_ABSENT が STABLE_COUNT 回連続 → STATE_CHECK_WAIT へ遷移
  - 遷移時に lastMillis_Check = millis()（タイマー開始）
  - 緑LED点灯を維持

＜currentState が STATE_CHECK_WAIT（確認待ち）のとき＞
  - waitForConfirm(now) を呼び出す
  - ボタンが押された     → STATE_SAFE へ遷移
  - 10秒タイムアウト    → STATE_ALERT へ遷移
  - LEDをゆっくり点滅（500ms周期）

＜currentState が STATE_ALERT（警告中）のとき＞
  - activateAlert(now) を呼び出す（LED高速点滅＋ブザー）
  - 確認ボタンまたは停止ボタンが押された → stopAlert() → STATE_SAFE へ遷移

＜currentState が STATE_SAFE（安全）のとき＞
  - 全出力を停止（LED消灯・ブザー停止）
  - distance < DIST_PRESENT が STABLE_COUNT 回連続 → STATE_WORKING へ遷移
    ※ 人が戻ってきたので作業中に戻る

## 2-3. readDistance() — 超音波センサーで距離を測定する


|対応機能ID|	F01（距離測定機能）|
|引数|	なし|
|戻り値|	int（距離 cm）、異常値の場合は -1|
|呼び出し元|	measureDistance() → loop()から呼ばれる|

【処理の流れ】

1. Trigピンを LOW にする（2μs待機）
   → センサーをリセットして安定させるため

2. Trigピンを HIGH にする（10μs待機）
   → この間に超音波パルスを発射する

3. Trigピンを LOW に戻す

4. pulseIn(PIN_ECHO, HIGH) で反射波の時間（μs）を取得する
   → 超音波が往復した時間を測定する

5. 距離を計算する
   distance = duration / 58
   ※ 音速340m/s → 1cm往復 ≒ 58μs という関係から

6. 異常値チェック
   - distance < DIST_MIN（2cm）または distance > DIST_MAX（400cm）の場合
   - Serial.println("センサー異常値: スキップ") してから -1 を返す

7. 正常な場合は distance を返す


### 2-4. measureDistance() — 距離データを取得・更新する

【処理の流れ】

引数: now（unsigned long）← millis()の現在値

1. now - lastMillis_Sensor >= SENSOR_INTERVAL（100ms）かチェック
   - 条件を満たさない場合: 何もせず return（まだ読み取り時間ではない）

2. lastMillis_Sensor = now（次の計測のために時刻を更新）

3. int raw = readDistance() を呼ぶ

4. raw が -1（異常値）の場合:
   - distance = prevDistance（前回の正常値をそのまま使う）
   - return

5. raw が正常値の場合:
   - distance = raw
   - prevDistance = raw（次回の「前回値」として保存）

### 2-5. readButtonConfirm() — 確認ボタンの状態を取得（デバウンス付き）

|対応機能ID|	共通（入力系）|
|引数	|now（unsigned long）← millis()の現在値|
|戻り値	|bool（true = ボタンが確定して押された）|
|呼び出し元|	loop()|

【処理の流れ】

1. digitalRead(PIN_BTN_CONFIRM) でボタンの生の値を読む
   → INPUT_PULLUP なので、押されていると LOW（=0）が返る

2. LOW（押されている）かどうか確認
   - HIGH（押されていない）→ false を返す（早期 return）

3. チャタリング判定:
   - now - lastMillis_Debounce < DEBOUNCE_DELAY（50ms）→ false を返す
   ※ 50ms以内の連続入力は「ノイズ・チャタリング」として無視

4. DEBOUNCE_DELAY 以上経過している場合:
   - lastMillis_Debounce = now（タイマー更新）
   - true を返す（本当に押されたと確定）

【エラー・異常ケース】
- ボタンが押しっぱなしの場合: 最初の1回だけ true になり、以後は
  DEBOUNCE_DELAY でフィルタリングされるため、重複カウントしない

### 2-6. readButtonStop() — 停止ボタンの状態を取得
【処理の流れ】
  readButtonConfirm() とほぼ同じ。対象ピンが PIN_BTN_STOP に変わるだけ。
  同一の lastMillis_Debounce を共有するか、別変数 lastMillis_DebounceStop を用意する。
  → 推奨: 別変数を用意して、2つのボタンが独立してデバウンスできるようにする。

### 2-7. detectLeaving() — 離席を検知する

|対応機能ID|	F02（離席検知機能）|
|引数|	distance（int）← 現在の距離（cm）|
|戻り値|	bool（true = 離席を確定検知）|
|呼び出し元|	loop() の STATE_WORKING 処理内|

【処理の流れ】

1. distance > DIST_ABSENT（100cm）かどうかチェック
   - 超えていない場合: stableCount = 0 にリセットして false を返す

2. 超えている場合: stableCount を 1 増やす

3. stableCount >= STABLE_COUNT（3回）かどうかチェック
   - まだ3回未満 → false を返す（確定していない）
   - 3回以上   → stableCount = 0 にリセットして true を返す
     ※ 「3回連続で100cm超え = 本当に離席した」と確定する

【エラー・異常ケース】
- センサーが一瞬誤読して距離が大きくなっても、3回連続でなければ無視される

### 2-8. waitForConfirm() — 確認待ち処理（10秒タイムアウト）
【処理の流れ】

引数: now（unsigned long）← millis()の現在値
戻り値: int  0=まだ待機中、1=確認済み（安全）、2=タイムアウト（警告へ）

1. 確認ボタンが押されたか確認: readButtonConfirm(now) が true の場合
   → return 1（安全確認済み）

2. タイムアウト確認: now - lastMillis_Check >= CHECK_TIMEOUT（10000ms）
   → return 2（タイムアウト → 警告へ）

3. どちらでもない場合
   → return 0（まだ待機中）

【補足】
lastMillis_Check は STATE_WORKING → STATE_CHECK_WAIT 遷移時に
loop() 内で millis() を代入しておく（タイマーのスタート時刻）。

### 2-9. activateAlert() — 警告を出力する（LED高速点滅＋ブザー）

【処理の流れ】

引数: now（unsigned long）← millis()の現在値

1. LED 高速点滅（200ms周期）:
   - now - lastMillis_LED >= 200 かチェック
   - 条件を満たした場合: lastMillis_LED = now
   - ledState を反転: ledState = (ledState == LOW) ? HIGH : LOW
   - digitalWrite(PIN_LED, ledState)

2. ブザーを鳴らす:
   - tone(PIN_BUZZER, 1000)  ← 1000Hz の音を出す
     ※ tone() はバックグラウンドで音を出し続けるため、毎ループ呼ぶ必要なし
     ※ 最初に STATE_ALERT に入ったとき1回だけ呼べばよい

【エラー・異常ケース】
- ブザーの音が鳴り続けることはユーザーにとって不快になる場合がある
- ボタン入力があれば stopAlert() を呼んで必ず停止できる

### 2-10. stopAlert() — 警告を停止して安全状態にする

【処理の流れ】

1. ブザーを止める: noTone(PIN_BUZZER)

2. LEDを消灯する: digitalWrite(PIN_LED, LOW)

3. フラグをリセットする:
   - alertFlag   = false
   - ledState    = LOW
   - stableCount = 0

4. currentState = STATE_SAFE に変更する
   （loop()の次のサイクルから STATE_SAFE の処理が走る）

5. デバッグ出力:
   Serial.println("Alert stopped. State → SAFE")

### 2-11. updateOutput() — 状態に応じてLED・ブザーを制御する
|状態|LED動作|ブザー動作|
|STATE_STANDBY（待機中）|	消灯|	停止|
|STATE_WORKING（作業中|緑色点灯（常時）|	停止|
|STATE_CHECK_WAIT（確認待ち）|	ゆっくり点滅|（500ms周期）	停止|
|STATE_ALERT（警告中）|	高速点滅（200ms周期）|	1000Hz で鳴動|
|STATE_SAFE（安全）|	消灯|	停止|

### 2-12. setThreshold() — 距離閾値の変更（追加機能 A01）
【処理の流れ】

1. Serial.available() > 0 かチェック（シリアルにデータが来ているか）
2. データがある場合: int newVal = Serial.parseInt() で数値を読む
3. 50 <= newVal <= 300 の範囲チェック
   - 範囲内: DIST_ABSENT の値を newVal に更新、Serial.println("閾値変更: " + newVal)
   - 範囲外: Serial.println("エラー: 50〜300の範囲で入力してください")

### 2-13. changeLEDMode() — LED点灯パターン変更（追加機能 A02）
【処理の流れ】

1. ボタンが押され続けている時間を計測
2. 2秒（2000ms）以上押された場合: LEDモードを次のパターンに切り替える
   モード0: 500ms点滅（通常） → モード1: 200ms点滅（高速） → モード0 に戻る
3. 変更後は Serial.println で現在のモードを出力する

```

---

## 3. 重要ロジックの詳細設計

### 3-1. チャタリング防止（デバウンス処理）

> ※ ボタンを使う場合は必ず設計してください。

```
【【考え方】
  ボタンが押されたとき、50ms 以内の連続入力は「同じ1回の押下」として無視する。

【処理の流れ（readButtonConfirm の中に実装）】
  1. ボタンのデジタル値を読む（digitalRead）
     → INPUT_PULLUP のため「押した = LOW、離した = HIGH」
  2. HIGH（離した）なら false を返す
  3. LOW（押した）の場合:
     - now - lastMillis_Debounce < DEBOUNCE_DELAY（50ms）→ false（無視）
     - now - lastMillis_Debounce >= DEBOUNCE_DELAY  → true（確定）
       かつ lastMillis_Debounce = now を更新

【必要な変数（Section 1 で定義済み）】
  lastMillis_Debounce : unsigned long  // 前回確定した時刻
  DEBOUNCE_DELAY      : const 50       // チャタリング判定時間（ms）

```

---

### 3-2. millis() を使ったタイマー管理

```
【考え方】
  「前回実行した時刻」を記録しておき、
  「今の時刻 − 前回時刻 ≥ 周期」のときだけ処理を実行する。

【処理の流れ（LED点滅の例）】
  1. now = millis()  ← 現在のミリ秒を取得
  2. now - lastMillis_LED >= LED_INTERVAL（500ms）かどうか確認
  3. 条件を満たした場合:
     - LEDのON/OFFを切り替える（ledState を反転）
     - lastMillis_LED = now  ← 「実行した時刻」を更新
  4. 条件を満たさない場合: 何もしない（次のループで再チェック）

【このシステムで millis() を使う処理】
  処理名              周期         変数名
  センサー読み取り    100ms        lastMillis_Sensor
  LED点滅             500ms/200ms  lastMillis_LED
  確認待ちタイムアウト 10000ms     lastMillis_Check
  ボタンデバウンス    50ms         lastMillis_Debounce

  （basic_design.md 2-3 のタイミング設計から転記して具体化する）
```

---

### 3-3. 超音波センサーの距離計算ロジック（任意）

> **【任意】** 複雑なロジックがある場合のみ記入してください。
> 例：「距離に応じたLED点灯パターン」「ゲームの衝突判定」「温度の閾値判定」

```
【動作原理】
  1. Trig ピンに 10μs の HIGH 信号を送る
  2. センサーが 40kHz の超音波を発射する
  3. 超音波が物体に当たって跳ね返ってくる
  4. Echo ピンが HIGH になる時間を計測する（pulseIn）

【距離計算の公式】
  duration = pulseIn(PIN_ECHO, HIGH)  ← 往復時間（μs）
  distance = duration / 58            ← 距離（cm）に変換

  ※ なぜ 58 で割るのか？
    音速 = 340 m/s = 0.034 cm/μs
    往復するので 2 で割る → 0.034 / 2 = 0.017 cm/μs
    逆数 → 1 / 0.017 ≒ 58.8 ≒ 58

【有効測定範囲】
  最小: 2cm  最大: 400cm
  → この範囲外の値は異常値として -1 を返す（readDistance() で処理）


```

---

## 4. デバッグ出力計画（任意）

> **【任意】** 関数設計（Section 2）と並行して記入すると効果的です。
> 「動かない」ときに何を確認すればいいかを事前に計画しておきます。
> 実装後は不要な Serial.println() を削除すること。

| No | 確認したい内容 | 挿入する関数 | Serial.println の内容例 |
|:---|:---|:---|:---|
| 1 | センサーの距離値が正しく取れているか | `readDistance()` | `Serial.print("dist: "); Serial.println(distance);` |
| 2 | 状態遷移が正しく起きているか | `loop()` | `Serial.print("state: "); Serial.println(currentState);` |
| 3 | ボタンのデバウンスが効いているか | `readButtonConfirm()`|`Serial.println("btn_confirm: confirmed");`|
| 4 |停止ボタンが正しく読めているか  | readButtonStop() | Serial.println("btn_stop: confirmed"); |
| 5 | 確認待ちタイムアウトの時刻確認 | waitForConfirm() | Serial.print("elapsed: "); Serial.println(now - lastMillis_Check); |
| 6 | 異常値フィルタリングが機能しているか | measureDistance() | Serial.println("sensor err: using prev val"); |

---

## 5. 単体テスト仕様書（V字モデル：詳細設計 ↔ 単体テスト）

> ※ 各関数・部品が「単体で正しく動くか」を確認するテスト項目を設計します。
> 「実際の結果」欄は実装後に記入します。

### 5-1. 入力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | readButtonConfirm() | 確認ボタンを1回しっかり押す | true が返る | | [ ] |
| 2 |readButtonConfirm()| ボタンを素早く連打する（10回/秒） |最初の1回だけ true、以降は false（デバウンス動作） | | [ ] |
| 3 | readButtonStop() | 停止ボタンを1回押す| 仕様範囲内の値が返る |true が返る | [ ] |
| 4 | readDistance() | センサー前面 30cm に障害物を置く |25〜35cm の値が返る（±20%以内） | | [ ] |
| 5 | readDistance() |センサーの前を手で完全に遮蔽する（2cm未満） |-1 が返る（異常値として除外） | | [ ] |
| 6 | readDistance() | センサーを空（障害物なし・400cm超）に向ける|-1 が返る（異常値として除外） | | [ ] |
| 7 | detectLeaving() |distance = 120cm を 3回連続で渡す | 3回目に true が返る| | [ ] |
| 8 |detectLeaving()|distance = 120cmを1回、次に 40cmを渡す | false が返る（stableCount がリセットされる）| | [ ] |

### 5-2. 出力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 |updateOutput() / STATE_STANDBY|currentState = 0（待機中）を設定| LED消灯、ブザー停止| | [ ] |
| 2 |updateOutput() / STATE_WORKING|currentState = 1（作業中）を設定 | LED点灯（常時） | | [ ] |
| 3 | activateAlert() |currentState = 3（警告中）を設定して数秒待つ |LED 200ms周期で点滅、ブザーが鳴る | | [ ] |
| 4 |stopAlert()|警告中にボタンを押す | LED消灯・ブザー停止・currentState = 4 | | [ ] |
### 5-3. タイミング・並行動作テスト

| No | テスト内容 | テスト手順 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | delay()による処理停止がないか | LED点滅中に確認ボタンを押す | ボタン入力が無視されない（即座に反応） | | [ ] |
| 2 | millis()タイマーの周期精度 | LED点滅をストップウォッチで10秒間計測 | 設計値（500ms）±10%以内で点滅する | | [ ] |
| 3 | 確認待ち10秒タイムアウト | STATE_CHECK_WAIT に入り、ボタンを押さずに待つ | 約10秒後に STATE_ALERT に遷移する | | [ ] |
| 4 |センサー読み取り周期 |Serial.print で distance 出力を確認 | 約100ms（10回/秒）で更新される| | [ ] |
---

## 6. AIレビュー記録

> グループレビューの前に必ず実施してください。

### Q1: 実装上の問題確認

> 「この詳細設計書に書いた関数と処理フローをもとに Arduino でコードを書きます。バグになりやすい箇所・処理の抜け・型の問題はありますか？」

**AIの回答（要約）：**
・pulseIn() はタイムアウト設定なしだと最大1秒ブロックする可能性がある
  → pulseIn(PIN_ECHO, HIGH, 30000) のように第3引数でタイムアウト（μs）を指定すること
・unsigned long のオーバーフロー（約49日後に0に戻る）は、
  now - lastMillis >= interval の計算式なら自動的に正しく処理される（問題なし）
・tone() と noTone() はピンを間違えると音が止まらなくなる → 定数 PIN_BUZZER を必ず使うこと
・STATE_SAFE 後に人が戻ったとき STATE_WORKING に戻る処理を忘れずに実装すること

**対応した内容：**
・readDistance() に pulseIn のタイムアウト引数（30000μs = 30ms）を追加する設計に修正
・STATE_SAFE の loop() 処理に「distance < DIST_PRESENT → STATE_WORKING へ」を明記した
・全ピン制御に定数名（PIN_BUZZER等）を使用するよう設計書内に注記を追加

---

### Q2: 単体テスト仕様の確認

> 「Section 5 の単体テスト仕様書で、各関数の動作が正しく検証できていますか？テストが不足している項目や、境界値テストが必要な箇所を教えてください。」

**AIの回答（要約）：**
・detectLeaving() の境界値テスト（distance = 100cm ちょうど）が不足している
・waitForConfirm() のタイムアウト境界（9999ms vs 10000ms）のテストを追加するとよい
・両方のボタンを同時に押したときの動作テストが未定義

**対応した内容：**

・単体テスト 5-1 に detectLeaving() の境界値テスト（No.7, No.8）を追加した
・5-3 にタイムアウトのタイミングテスト（No.3）を追加した
・両ボタン同時押しは「どちらか一方が先に検知されたほうが優先される」と仕様を明確化

---

## 7. グループレビュー記録

### 7-1. 指摘一覧

| No | 指摘内容 | 指摘者 | 対応 |
|:---|:---|:---|:---|
| 1 |  |  |  |
| 2 |  |  |  |
| 3 |  |  |  |

### 7-2. レビューを受けて変更した点

-
-

---

*初版: YYYY-MM-DD / AIレビュー: YYYY-MM-DD / グループレビュー後更新: YYYY-MM-DD*

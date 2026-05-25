# 詳細設計書 — 組込み開発実習

<!-- 作成者: 松田 眞幸/ 日付: 2026-05-22 / グループ: 1-i -->

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
| 作品タイトル | アプローチアラート |
| 状態の種類（1-2 状態遷移から） | 9種類 |
| 実装する関数の数（2-2 関数一覧から） | 11個 |
| グローバル変数の合計バイト数（2-1 SRAM確認から） | 20B |

---

## 1. グローバル変数・定数の設計

> ※ 基本設計書（2-1 データ設計）をもとに、**型と初期値まで**決めます。
> ここで設計した変数は、この後の関数設計でそのまま使います。

```
const int trigPin = 9;
const int echoPin = 10;
const int buzzer = 3;

// 距離関連
long duration;
int distance;

// 状態管理
int state = 0; 
// 0:無音 1:弱 2:中 3:強

// millis制御
unsigned long prevMeasureTime = 0;
unsigned long prevBeepTime = 0;
bool beepState = false;

// 設定値
const int measureInterval = 100; // 測定周期(ms)
```

---

## 2. 各関数の詳細設計

> ※ 基本設計書（2-2 関数一覧）で定義した各関数の「中身」を設計します。
> **疑似コード**（日本語＋処理の流れ）で書いてください。実際のC++コードは書かなくてOKです。

---

### `setup()` — 初期化処理

```
【処理の流れ】
1. ピンモードを設定する
   - PIN_BUTTON  → INPUT_PULLUP
   - PIN_LED_*   → OUTPUT
   - PIN_BUZZER  → OUTPUT

2. ライブラリの初期化（使うものだけ）
   - 例: lcd.begin(16, 2)
   - 例: servo.attach(PIN_SERVO)

3. Serial.begin(9600)（デバッグ用）

4. 起動確認（任意）: 緑LEDを1秒点灯して消灯
```

**↓ 自分の setup() を設計してください**
```
【処理の流れ】
1.ピンモードを設定する
  trigPin > OUTPUT
  echoPin > INPUT
  buzzer  > OUTPUT

2. Serial.begin(9600) (デバック用)
```

---

### `loop()` — メインループ

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

**↓ 自分の loop() を設計してください**
```
【処理の流れ】

＜毎ループ実行すること＞
- 現在時刻を取得する: currentMillis = millis()
- 距離測定の周期条件を確認する
  - 条件: currentMillis - prevMeasureTime >= measureInterval
  - 条件を満たしたら以下を実行
    1. prevMeasureTime を currentMillis で更新
    2. measureDistance() を呼び、distance を更新
    3. judgeState(distance) を呼び、state を更新
       - state=0: 無音
       - state=1: 弱警告
       - state=2: 中警告
       - state=3: 強警告
    4. シリアルに距離を出力する
  - 条件を満たさない場合は距離測定をスキップし、前回の state を維持
- controlBuzzer(currentMillis) を毎ループ呼び出して音を更新する

＜state が 0（無音）のとき＞
- noTone() でブザー停止

＜state が 1（弱警告）のとき＞
- 周波数1000Hz、周期1000msの断続音
- currentMillis - prevBeepTime >= 500ms でON/OFFを切替

＜state が 2（中警告）のとき＞
- 周波数1500Hz、周期400msの断続音
- currentMillis - prevBeepTime >= 200ms でON/OFFを切替

＜state が 3（強警告）のとき＞
- tone(2000Hz) を連続出力

```

---

### （関数ごとに以下のブロックをコピーして追加してください）

> ※ 基本設計書 2-2 の関数一覧に記載した関数を1つずつ設計します。

---

### `setup()` — ピン設定とシリアル初期化を行う

**basic_design.md 2-2 との対応：** 初期化（setup()）

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. trigPin を OUTPUT、echoPin を INPUT、buzzer を OUTPUT に設定する。
2. Serial.begin(9600) でシリアル通信を初期化する。

【エラー・異常ケース】
- ピン番号設定ミス: 配線と設計値を照合して修正する。
```

---

### `loop()` — 周期測定と状態別ブザー制御を繰り返す

**basic_design.md 2-2 との対応：** （共通）メイン制御（loop()）

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. currentMillis = millis() を取得する。
2. measureInterval(100ms) 経過時のみ measureDistance() と judgeState() を実行し、distance/state を更新する。
3. Serial.print/println で距離を出力する。
4. controlBuzzer(currentMillis) を毎ループ呼び、state に応じた音を出す。

【エラー・異常ケース】
- 測定値が不正（0や過大）: judgeState 前に異常判定を入れ、警告状態へ遷移する。
```

---

### `measureDistance()` — 超音波センサで距離(cm)を取得する

**basic_design.md 2-2 との対応：** 距離測定（measureDistance()）/ 距離計算（calculateDistance()）

**引数：** なし

**戻り値：** int: 測定距離（cm）

```
【処理の流れ】
1. Trig を LOW(2us)→HIGH(10us)→LOW の順で出力し、超音波を発信する。
2. pulseIn(echoPin, HIGH) でエコー継続時間 duration を取得する。
3. dist = duration * 0.034 / 2 で距離(cm)へ変換して返す。

【エラー・異常ケース】
- pulseIn が0を返す: タイムアウト/未接続の可能性として異常扱いにする。
- 距離が有効範囲外: 異常値として警告処理へ回す。
```

---

### `judgeState(int d)` — 距離に応じて警告状態を判定する

**basic_design.md 2-2 との対応：** 状態判定（judgeState()）

**引数：** `d`（int）: 判定対象の距離(cm)

**戻り値：** int: 状態値（0:無音 / 1:弱 / 2:中 / 3:強）

```
【処理の流れ】
1. d > 100 なら state=0（無音）を返す。
2. 50 < d <= 100 なら state=1（弱警告）を返す。
3. 20 < d <= 50 なら state=2（中警告）を返す。
4. d <= 20 なら state=3（強警告）を返す。

【エラー・異常ケース】
- d が0や負値: 異常値として扱う判定分岐を別途追加する。
```

---

### `controlBuzzer(unsigned long currentMillis)` — 状態に応じてブザー音を制御する

**basic_design.md 2-2 との対応：** ブザー制御（controlBuzzer()）/ 音出力（playTone()相当）

**引数：** `currentMillis`（unsigned long）: 現在時刻(ms)

**戻り値：** なし（void）

```
【処理の流れ】
1. state=0 のとき noTone() で停止して終了する。
2. state=1 は interval=1000ms/freq=1000Hz、state=2 は interval=400ms/freq=1500Hz を設定する。
3. state=3 は tone(2000) を連続出力して終了する。
4. state=1/2 では currentMillis - prevBeepTime >= interval/2 ごとに beepState を反転し、tone/noTone を切替える。

【エラー・異常ケース】
- state が想定外の値: 安全のため 強警告 を実行して終了する。
```

---

## 3. 重要ロジックの詳細設計

### 3-1. チャタリング防止（デバウンス処理）

> ※ ボタンを使う場合は必ず設計してください。

```
【考え方】
  ボタンが押されたとき、50ms 以内の連続入力は「同じ1回の押下」として無視する。

【処理の流れ】
  1. ボタンのデジタル値を読む（digitalRead）
  2. 前回確定した時刻（lastDebounceTime）からの経過時間を計算する
  3. 経過時間 < DEBOUNCE_DELAY（例: 50ms）→ 無視する
  4. 経過時間 ≥ DEBOUNCE_DELAY → ボタンの状態として確定する
  5. lastDebounceTime を更新する

【必要な変数（Section 1 に追加済みか確認）】
  lastDebounceTime : unsigned long   // 前回確定した時刻
  DEBOUNCE_DELAY   : const int = 50  // チャタリング判定時間（ms）
```

---

### 3-2. millis() を使ったタイマー管理

```
【考え方】
  「前回実行した時刻」を記録しておき、「今の時刻 − 前回時刻 ≥ 周期」なら実行する。

【処理の流れ（例: LED点滅）】
  1. now = millis()
  2. now - lastMillis_LED >= LED_INTERVAL かどうか確認
  3. 条件を満たした場合: LEDのON/OFFを切り替え、lastMillis_LED = now
  4. 条件を満たさない場合: 何もしない（次のループで再チェック）

【自分のシステムで millis() を使う処理】
  1. 距離測定周期の管理（100ms）
    - currentMillis = millis() を取得する。
    - currentMillis - prevMeasureTime >= measureInterval(100) を満たした場合のみ測定する。
    - 測定実行後、prevMeasureTime = currentMillis に更新する。
    - これにより、距離測定は100msごとに実行され、他処理を停止させない。

  2. 弱/中警告の断続音管理（non-blocking）
    - state=1（弱）: interval=1000ms, freq=1000Hz
    - state=2（中）: interval=400ms, freq=1500Hz
    - currentMillis - prevBeepTime >= interval/2 を満たしたら beepState を反転する。
    - beepState=true のとき tone(buzzer, freq)、false のとき noTone(buzzer) を実行する。
    - 切替実行後、prevBeepTime = currentMillis に更新する。

  3. 強警告/無音の管理
    - state=3（強）: tone(buzzer, 2000) を連続出力する。
    - state=0（無音）: noTone(buzzer) で停止する。
    - いずれも delay() を使わず、次ループへ即時移行する。
```

---

### 3-3. その他の重要ロジック（任意）

> **【任意】** 複雑なロジックがある場合のみ記入してください。
> 例：「距離に応じたLED点灯パターン」「ゲームの衝突判定」「温度の閾値判定」

```
【処理の流れ】
1.
2.
3.

【入力値と出力値の関係】

```

---

## 4. デバッグ出力計画（任意）

> **【任意】** 関数設計（Section 2）と並行して記入すると効果的です。
> 「動かない」ときに何を確認すればいいかを事前に計画しておきます。
> 実装後は不要な Serial.println() を削除すること。

| No | 確認したい内容 | 挿入する関数 | Serial.println の内容例 |
|:---|:---|:---|:---|
| 1 | 距離測定値が正しく取得できるか（T2/T3） | `loop()` | `Serial.println(distance);` |
| 2 | 状態判定がしきい値どおりか（T4/T15） | `loop()` | `Serial.println(state);` |
| 3 | 断続音の切替周期が正しいか（T6） | `controlBuzzer()` | `Serial.println(beepState);` |
| 4 | 測定周期が約100msか（T8） | `loop()` | `Serial.println(currentMillis - prevMeasureTime);` |
| 5 | 異常値・タイムアウト時に警告遷移するか（T11/T12/T13） | `measureDistance()` / `loop()` | `Serial.println("WARN: sensor abnormal");` |
| 6 | 応答時間0.5秒以内を満たすか（T14） | `loop()` | `Serial.println(currentMillis);` |

---

## 5. 単体テスト仕様書（V字モデル：詳細設計 ↔ 単体テスト）

> ※ 各関数・部品が「単体で正しく動くか」を確認するテスト項目を設計します。
> 「実際の結果」欄は実装後に記入します。

### 5-1. 入力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | measureDistance() | 既知距離（30cm）に物体を置く | 近い値（±数cm）が返る | | [ ] |
| 2 | measureDistance() | pulseInが0を返す場合 | 異常値として警告状態へ遷移 | | [ ] |
| 3 | measureDistance() | 距離が負値や極端な値の場合 | 異常値として警告状態へ遷移 | | [ ] |
| 4 | judgeState() | d=120 を入力 | state=0（無音）が返る | | [ ] |
| 5 | judgeState() | d=100 を入力 | state=1（弱警告）が返る（境界値） | | [ ] |
| 6 | judgeState() | d=70 / d=30 / d=10 を入力 | state=1 / 2 / 3 が返る | | [ ] |
| 7 | judgeState() | d=50 を入力 | state=2（中警告）が返る（境界値） | | [ ] |
| 8 | judgeState() | d=20 を入力 | state=3（強警告）が返る（境界値） | | [ ] |
| 9 | loop()（currentMillis判定） | currentMillis - prevMeasureTime < 100 の状態で1ループ実行 | 距離測定を実行せず、stateを維持 | | [ ] |
|10 | loop()（currentMillis判定） | currentMillis - prevMeasureTime >= 100 の状態で1ループ実行 | measureDistance() が1回実行される | | [ ] |
|11 | デバウンス処理 | 50ms未満の連続ボタン入力 | 状態が切り替わらない | | [ ] |
|12 | デバウンス処理 | 50ms以上の間隔でボタン入力 | 状態が切り替わる | | [ ] |

### 5-2. 出力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | controlBuzzer() | state=0, 任意のcurrentMillis | noTone() が呼ばれ無音になる | | [ ] |
| 2 | controlBuzzer() | state=1, currentMillis - prevBeepTime < 500 | 音状態が切り替わらない | | [ ] |
| 3 | controlBuzzer() | state=1, currentMillis - prevBeepTime = 499ms | 切替なし（境界値直前） | | [ ] |
| 4 | controlBuzzer() | state=1, currentMillis - prevBeepTime = 500ms | beepState が反転し tone/noTone が切替（境界値） | | [ ] |
| 5 | controlBuzzer() | state=2, currentMillis - prevBeepTime = 199ms | 切替なし（境界値直前） | | [ ] |
| 6 | controlBuzzer() | state=2, currentMillis - prevBeepTime = 200ms | 1500Hz の断続音に切替（境界値） | | [ ] |
| 7 | controlBuzzer() | state=3, 任意のcurrentMillis | 2000Hz の連続音が出る | | [ ] |
| 8 | controlBuzzer() | state=-1, 任意のcurrentMillis | 強警告（2000Hz連続音）となる（想定外stateの安全動作） | | [ ] |
| 9 | controlBuzzer() | state=4, 任意のcurrentMillis | 強警告（2000Hz連続音）となる（想定外stateの安全動作） | | [ ] |
|10 | Serial出力 | 距離測定時 | Serial.println(distance) で距離が出力される | | [ ] |
|11 | Serial出力 | 状態判定時 | Serial.println(state) でstateが出力される | | [ ] |
|12 | Serial出力 | 異常値・警告時 | Serial.println("WARN: sensor abnormal") など警告メッセージが出力される | | [ ] |

### 5-3. タイミング・並行動作テスト

| No | テスト内容 | テスト手順 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | 測定周期100msの境界確認（入力系No.9/10） | currentMillis - prevMeasureTime を 99ms と 100ms に設定して loop() を実行 | 99msでは測定しない、100msで1回だけ測定する（境界値） | | [ ] |
| 2 | 弱警告の切替境界確認（出力系No.3/4） | state=1 で currentMillis - prevBeepTime を 499ms と 500ms に設定して controlBuzzer() を実行 | 499msでは切替なし、500msでbeepStateが反転する（境界値） | | [ ] |
| 3 | 中警告の切替境界確認（出力系No.5/6） | state=2 で currentMillis - prevBeepTime を 199ms と 200ms に設定して controlBuzzer() を実行 | 199msでは切替なし、200msで1500Hzの断続音へ切替（境界値） | | [ ] |
| 4 | 並行動作確認（測定とブザー制御の独立性） | 連続ループ中に測定周期到達タイミングと断続音切替タイミングを重ねて実行 | 距離測定と音切替の両方が遅延なく実行される | | [ ] |
| 5 | delay未使用確認（非ブロッキング動作） | 5秒連続実行し、シリアル出力の更新間隔と音切替間隔を観察 | 測定周期と音切替周期が継続し、処理停止が発生しない | | [ ] |

---

## 6. AIレビュー記録

> グループレビューの前に必ず実施してください。

### Q1: 実装上の問題確認

> 「この詳細設計書に書いた関数と処理フローをもとに Arduino でコードを書きます。バグになりやすい箇所・処理の抜け・型の問題はありますか？」

**AIの回答（要約）：**
Section 5 の単体テスト仕様書は主要な関数の基本動作や状態遷移、タイミング制御を広くカバーしていますが、以下の点でテストが不足しています。

【不足・追加が必要なテスト項目】
1. measureDistance() の異常値・エラー処理（pulseInが0や負値、極端な値の場合の警告遷移）
2. judgeState() のしきい値（d=100, 50, 20など）での境界値テスト
3. controlBuzzer() の異常系（stateが想定外の値の場合の安全動作）
4. チャタリング防止（デバウンス）のテスト（50ms未満/以上の連続入力）
5. シリアル出力内容の検証（距離、state、警告メッセージ等）

【境界値テストが必要な箇所】
・ judgeState() のしきい値（d=100, 50, 20）
・ controlBuzzer() の切替タイミング（500ms, 200ms など）
・ measureDistance() の有効範囲の上下限

**対応した内容：**
・Section 5-1, 5-2, 5-3 に以下のテスト項目を追加・修正した：
  - measureDistance() の異常値（0, 負値, 極端な値）入力時の警告遷移テスト
  - judgeState() で d=100, 50, 20 の境界値入力時のstate判定テスト
  - controlBuzzer() で stateが想定外の値（例: -1, 4）時の安全動作テスト
  - デバウンス処理の50ms未満/以上の連続入力テスト（ボタン入力がある場合）
  - Serial出力内容（距離、state、警告メッセージ等）の検証テスト
・既存テスト項目の期待結果欄に、境界値や異常系の具体的な期待動作を明記した。

---

### Q2: 単体テスト仕様の確認

> 「Section 5 の単体テスト仕様書で、各関数の動作が正しく検証できていますか？テストが不足している項目や、境界値テストが必要な箇所を教えてください。」


**AIの回答（要約）：**
Section 5 の単体テスト仕様書は主要な関数の基本動作や状態遷移、タイミング制御を広くカバーしていますが、以下の点でテストが不足しています。

【不足・追加が必要なテスト項目】
1. measureDistance() の異常値・エラー処理（pulseInが0や負値、極端な値の場合の警告遷移）
2. judgeState() のしきい値（d=100, 50, 20など）での境界値テスト
3. controlBuzzer() の異常系（stateが想定外の値の場合の安全動作）
4. チャタリング防止（デバウンス）のテスト（50ms未満/以上の連続入力）
5. シリアル出力内容の検証（距離、state、警告メッセージ等）

【境界値テストが必要な箇所】
- judgeState() のしきい値（d=100, 50, 20）
- controlBuzzer() の切替タイミング（500ms, 200ms など）
- measureDistance() の有効範囲の上下限

**対応した内容：**
・Section 5-1, 5-2, 5-3 に不足していたテスト項目（異常値・境界値・デバウンス・想定外state・シリアル出力）を追加・修正した。
・既存テストの期待結果も、境界値や異常系の動作を具体的に記載した。

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

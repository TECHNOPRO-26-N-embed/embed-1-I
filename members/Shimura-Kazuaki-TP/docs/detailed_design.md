# 詳細設計書 — 組込み開発実習

<!-- 作成者: 志村一亮 / 日付: 2026-05-25 / グループ: 1-I -->

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
| 作品タイトル | ミニチュア版歩行者信号灯 |
| 状態の種類（1-2 状態遷移から） | 2種類（赤信号中、青信号中） |
| 実装する関数の数（2-2 関数一覧から） | 7　個 |
| グローバル変数の合計バイト数（2-1 SRAM確認から） | 19 B（設計値） |

---

## 1. グローバル変数・定数の設計

> ※ 基本設計書（2-1 データ設計）をもとに、**型と初期値まで**決めます。
> ここで設計した変数は、この後の関数設計でそのまま使います。

```
【ピン定義】（basic_design.md 3-1 から転記）
  redLedPin           : const byte = 8   // 赤LED（D8）
  greenLedPin         : const byte = 9   // 緑LED（D9）
  buzzerPin           : const byte = 3   // パッシブブザー（D3）

【状態管理】（basic_design.md 1-2 の状態名から転記）
  currentSignalState : int = 0                 // 0:赤信号中 1:青信号中

【タイマー（millis()用）】（basic_design.md 2-3 から転記）
  stateStartMillis   : unsigned long = 0       // 現在状態に入った時刻
  redDurationMs      : const unsigned long = 5000
  greenDurationMs    : const unsigned long = 5000

【音制御】（basic_design.md 2-1 から転記）
  greenToneHz        : const int = 2000

【異常監視用（basic_design.md 4 の対応）】
  stateStuckLimitMs       : const unsigned long = 10000  // 2倍時間で停止判定
  lastBuzzerAppliedState  : int = -1                     // 前回適用したブザー状態
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
1. redLedPin（D8）, greenLedPin（D9）, buzzerPin（D3）をすべて OUTPUT に設定する。
2. 状態変数を初期化する。
  - currentSignalState = 0（赤信号中）
  - stateStartMillis = millis()
  - lastBuzzerAppliedState = 0（初期の無音状態と同期）
3. 初期状態の出力を確定する。
  - redLedPin を HIGH（赤点灯）
  - greenLedPin を LOW（緑消灯）
  - noTone(buzzerPin) でブザー停止
4. （任意）デバッグ時のみ Serial.begin(9600) を実行し、起動ログを1回出力する。
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
  - nowMillis = millis() を取得する
  - currentSignalState = updateSignalState(nowMillis) を呼ぶ（時間経過で赤/青を切り替える）
  - checkStateStuck(nowMillis) を呼ぶ
    - true が返った場合は currentSignalState を 0（赤信号中）として扱い、以降の出力を安全側で再適用する
  - applySignalLeds(currentSignalState) を呼ぶ（状態に応じてLEDを反映する）
  - applySignalBuzzer(currentSignalState, greenToneHz) を呼ぶ（状態が変化したときだけ tone/noTone を更新する）
  - enforceBuzzerConsistency(currentSignalState, greenToneHz) を呼ぶ（想定外の不一致を補正する）

＜currentSignalState が 0（赤信号中）のとき＞
  - 赤の時間（redDurationMs）に達するまで赤LEDを点灯し続ける
  - ブザーは停止状態を維持する

＜currentSignalState が 1（青信号中）のとき＞
  - 青の時間（greenDurationMs）に達するまで緑LEDを点灯し続ける
  - ブザーは greenToneHz を維持する（毎ループで tone を再発行しない）

＜異常時（監視ロジックで検知）のとき＞
  - 想定外の状態値になった場合は currentSignalState を 0（赤信号中）に戻す
  - stateStartMillis = nowMillis でタイマーを再初期化する
  - 安全側として赤LED点灯・緑LED消灯・ブザー停止に戻す
```

---

### `updateSignalState(nowMillis)` — 信号状態を時間経過で切り替える

**basic_design.md 2-2 との対応：** millis()を使って赤信号中/青信号中を時間で切り替える

**引数：** `nowMillis`（unsigned long）: 現在時刻（millis）

**戻り値：** int（0:赤信号中、1:青信号中）

```
【処理の流れ】
1. currentSignalState が 0/1 以外なら、以下を実行して終了する。
  - currentSignalState = 0 に矯正
  - stateStartMillis = nowMillis に再初期化
  - currentSignalState を返す
2. elapsed = nowMillis - stateStartMillis を計算する。
3. currentSignalState が 0 かつ elapsed >= redDurationMs の場合:
  - currentSignalState = 1 に更新
  - stateStartMillis = nowMillis に更新
4. currentSignalState が 1 かつ elapsed >= greenDurationMs の場合:
  - currentSignalState = 0 に更新
  - stateStartMillis = nowMillis に更新
5. currentSignalState を返す。

【エラー・異常ケース】
- currentSignalState の異常値は最優先で補正し、以降の遷移判定は行わない。
```

---

### `applySignalLeds(signalState)` — 信号状態に応じてLEDを点灯する

**basic_design.md 2-2 との対応：** signalState に応じて赤LEDまたは緑LEDを点灯する

**引数：** `signalState`（int）: 現在の信号状態

**戻り値：** void

```
【処理の流れ】
1. signalState == 0 の場合:
  - redLedPin を HIGH
  - greenLedPin を LOW
2. signalState == 1 の場合:
  - redLedPin を LOW
  - greenLedPin を HIGH
3. それ以外の場合:
  - 安全側として redLedPin を HIGH、greenLedPin を LOW

【エラー・異常ケース】
- 想定外の状態値は赤信号表示に固定し、状態更新処理で正規状態（0/1）へ復帰させる。
```

---

### `applySignalBuzzer(signalState, toneHz)` — 青信号中のみブザーを鳴らす

**basic_design.md 2-2 との対応：** 青信号中のみブザーを鳴らし、赤信号中は停止する

**引数：** `signalState`（int）: 現在の信号状態
**引数：** `toneHz`（int）: ブザー周波数

**戻り値：** void

```
【処理の流れ】
1. signalState が 0/1 以外、または toneHz <= 0 の場合は noTone(buzzerPin) を実行し、lastBuzzerAppliedState = 0 を記録して終了する。
2. signalState と lastBuzzerAppliedState を比較し、同じなら何もしない（再設定しない）。
3. signalState == 1 の場合のみ tone(buzzerPin, toneHz) を実行する。
4. signalState == 0 の場合は noTone(buzzerPin) を実行する。
5. 実際に反映した状態を lastBuzzerAppliedState に記録する。

【エラー・異常ケース】
- 異常値（signalState 不正、toneHz <= 0）は必ず無音側に倒して安全優先とする。
```

---

### `checkStateStuck(nowMillis)` — 状態遷移停止を検知して復旧する

**basic_design.md 2-2 との対応：** 異常系（状態遷移が止まる）の監視と復帰

**引数：** `nowMillis`（unsigned long）: 現在時刻（millis）

**戻り値：** bool（復旧を実行したら true）

```
【処理の流れ】
1. expectedLimit を stateStuckLimitMs で初期化する。
2. currentSignalState に応じて expectedLimit を上書きする。
  - 赤信号中: redDurationMs * 2
  - 青信号中: greenDurationMs * 2
3. expectedLimit が 0 の場合は stateStuckLimitMs を代替閾値として使う。
4. nowMillis - stateStartMillis > expectedLimit の場合は異常と判定する。
5. 異常時は currentSignalState = 0、stateStartMillis = nowMillis に再初期化し、true を返す。
6. 異常なしなら false を返す。

【エラー・異常ケース】
- 設定値異常で expectedLimit が 0 になっても、stateStuckLimitMs を使って監視を継続する。
```

---

### `enforceBuzzerConsistency(signalState, toneHz)` — ブザー出力不一致を補正する

**basic_design.md 2-2 との対応：** 異常系（ブザー出力不一致）の補正

**引数：** `signalState`（int）: 現在の信号状態
**引数：** `toneHz`（int）: 青信号時の目標周波数

**戻り値：** void

```
【処理の流れ】
1. signalState が 0/1 以外の場合は noTone(buzzerPin) を実行し、lastBuzzerAppliedState = 0 を記録して終了する。
2. 期待状態を判定する（赤: noTone、青: tone）。
3. lastBuzzerAppliedState と signalState が一致していれば何もしない。
4. 不一致時のみ補正処理を実行する。
  - 赤信号中は noTone(buzzerPin) を再適用
  - 青信号中かつ toneHz > 0 の場合は tone(buzzerPin, toneHz) を再適用
5. lastBuzzerAppliedState を signalState に更新する。

【エラー・異常ケース】
- toneHz が異常値（toneHz <= 0）のときは noTone(buzzerPin) を実行し、lastBuzzerAppliedState = 0 に更新する。
```

---

## 3. 重要ロジックの詳細設計

### 3-1. チャタリング防止（デバウンス処理）

> ※ ボタンを使う場合は必ず設計してください。

```
【考え方】
  現時点の実装範囲ではボタン入力を使用しないため、デバウンス処理は未適用とする。

【処理の流れ】
  1. 今回は入力部品なし（タイマーのみ）で実装する。
  2. 第2段階で音量調整ボタンを追加する場合は、50msデバウンスを実装する。

【必要な変数（第2段階で追加）】
  lastDebounceTime : unsigned long = 0
  debounceDelayMs  : const unsigned long = 50
```

---

### 3-2. millis() を使ったタイマー管理

```
【考え方】
  状態に入った時刻 stateStartMillis を保持し、現在時刻との差分で遷移判定する。

【処理の流れ（本システム）】
  1. nowMillis = millis() を取得する。
  2. 赤信号中なら elapsed = nowMillis - stateStartMillis を計算し、elapsed >= redDurationMs を判定する。
  3. 青信号中なら elapsed = nowMillis - stateStartMillis を計算し、elapsed >= greenDurationMs を判定する。
  4. 遷移したら stateStartMillis = nowMillis に更新する。
  5. ループごとに LED出力を反映し、ブザーは状態変化時のみ更新する（毎ループでtoneを再発行しない）。
  6. 異常監視（状態停止・ブザー不一致）を毎ループ実行し、必要時は安全側へ補正する。

【自分のシステムで millis() を使う処理】
  - 信号状態の切り替え判定（赤→青 / 青→赤）
  - 出力反映（LED点灯、青信号時ブザー維持）
  - 異常監視（状態停止の検知）
  - ブザー不一致補正のタイミング管理
```

---

### 3-3. その他の重要ロジック（任意）

> **【任意】** 複雑なロジックがある場合のみ記入してください。
> 例：「距離に応じたLED点灯パターン」「ゲームの衝突判定」「温度の閾値判定」

```
【処理の流れ】
1. 状態停止監視:
  - nowMillis - stateStartMillis が状態時間の2倍を超えたら異常と判定。
  - currentSignalState を赤信号中へ戻し、stateStartMillis を再初期化。
2. ブザー不一致監視:
  - currentSignalState と lastBuzzerAppliedState を比較。
  - 不一致時のみ tone/noTone を再適用して補正。
  - toneHz <= 0 の場合は無音側（noTone）へ強制補正。
3. 安全側出力:
  - 想定外の状態値が来たら赤LED点灯 + noTone へ強制遷移。

【入力値と出力値の関係】
  - 入力: currentSignalState, nowMillis
  - 出力: LED出力状態、ブザー出力状態、必要に応じた状態変数の再初期化
```

---

## 4. デバッグ出力計画（任意）

> **【任意】** 関数設計（Section 2）と並行して記入すると効果的です。
> 「動かない」ときに何を確認すればいいかを事前に計画しておきます。
> 実装後は不要な Serial.println() を削除すること。

| No | 確認したい内容 | 挿入する関数 | Serial.println の内容例 |
|:---|:---|:---|:---|
| 1 | 状態遷移が正しく起きているか | `updateSignalState()` | `Serial.println(currentSignalState);` |
| 2 | 経過時間判定が設計通りか | `updateSignalState()` | `Serial.println(nowMillis - stateStartMillis);` |
| 3 | ブザー制御が状態と一致しているか | `applySignalBuzzer()` | `Serial.println(lastBuzzerAppliedState);` |
| 4 | 異常復旧が動作したか | `checkStateStuck()` | `Serial.println("stuck recovered");` |

---

## 5. 単体テスト仕様書（V字モデル：詳細設計 ↔ 単体テスト）

> ※ 各関数・部品が「単体で正しく動くか」を確認するテスト項目を設計します。
> 「実際の結果」欄は実装後に記入します。

### 5-1. 入力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | updateSignalState(nowMillis) | 赤信号中で 5000ms 経過させる | 戻り値が青信号中（1）になる | | [ ] |
| 2 | updateSignalState(nowMillis) | 青信号中で 5000ms 経過させる | 戻り値が赤信号中（0）になる | | [ ] |
| 3 | updateSignalState(nowMillis) | stateStartMillis から 4999ms 時点で確認 | 状態が切り替わらない | | [ ] |
| 4 | updateSignalState(nowMillis) | currentSignalState=2（異常値）で実行 | 0に矯正され、stateStartMillis が nowMillis で再初期化される | | [ ] |
| 5 | checkStateStuck(nowMillis) | 同一状態で 10000ms ちょうどを作る | 異常判定されず false を返す | | [ ] |
| 6 | checkStateStuck(nowMillis) | 同一状態で 10001ms を作る | 異常判定され true を返し赤信号中に復帰する | | [ ] |
| 7 | enforceBuzzerConsistency(signalState, toneHz) | lastBuzzerAppliedState を不一致にして実行 | 状態に応じた tone/noTone に補正される | | [ ] |
| 8 | enforceBuzzerConsistency(signalState, toneHz) | lastBuzzerAppliedState と signalState を一致させて実行 | 余計な再適用をせず状態を維持する | | [ ] |

### 5-2. 出力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | applySignalLeds(0) | signalState=0 を渡す | 赤LED点灯、緑LED消灯になる | | [ ] |
| 2 | applySignalLeds(1) | signalState=1 を渡す | 緑LED点灯、赤LED消灯になる | | [ ] |
| 3 | applySignalLeds(2) | signalState=2（異常値）を渡す | 安全側として赤LED点灯、緑LED消灯になる | | [ ] |
| 4 | applySignalBuzzer(0, 2000) / applySignalBuzzer(1, 2000) | 赤・青それぞれの状態で呼び分ける | 赤は無音、青のみ2000Hzで鳴る | | [ ] |
| 5 | applySignalBuzzer(1, 2000) を連続2回 | 1回目で鳴動開始後、同じ状態で再実行する | 2回目は再設定せず鳴動状態を維持する | | [ ] |
| 6 | applySignalBuzzer(1, 0) | toneHz=0 を渡す | noTone が適用され無音になる | | [ ] |

### 5-3. タイミング・並行動作テスト

| No | テスト内容 | テスト手順 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | delay()による処理停止がないか | 1周期（赤→青→赤）を観察しながら経過時間を測る | 切替が止まらず、常に5000msごとに遷移する | | [ ] |
| 2 | millis()タイマーの周期精度 | 赤/青の各点灯時間をストップウォッチで3回計測 | 各状態が5000ms±許容誤差で維持される | | [ ] |
| 3 | 異常復旧の並行動作 | 状態更新を停止させる条件を作り通常ループを継続 | 出力を維持しつつ異常検知後に赤信号へ復帰する | | [ ] |

---

## 6. AIレビュー記録

> グループレビューの前に必ず実施してください。

### Q1: 実装上の問題確認

> 「この詳細設計書に書いた関数と処理フローをもとに Arduino でコードを書きます。バグになりやすい箇所・処理の抜け・型の問題はありますか？」

**AIの回答（要約）：**

loop の処理フローと異常系関数の接続が弱い
loop の記述は「異常時に戻す」と書かれていますが、現在の毎ループ処理に checkStateStuck / enforceBuzzerConsistency を呼ぶ流れが明示されていない。
結果として、異常系が設計だけで終わり、コードに載らないリスクがある。

ブザー制御を毎ループで tone し続ける設計は不安定化しやすい
applySignalBuzzer を毎ループ呼ぶ前提だと、ボードや実装次第で音が途切れる・不安定になる可能性がある。
状態変化時だけ tone/noTone を更新する形（前回状態との差分適用）に寄せると安全。

**対応した内容：**
対処した。

---

### Q2: 単体テスト仕様の確認

> 「Section 5 の単体テスト仕様書で、各関数の動作が正しく検証できていますか？テストが不足している項目や、境界値テストが必要な箇所を教えてください。」

**AIの回答（要約）：**

異常値テスト不足（状態値異常、toneHz異常）
理由: 安全側へ倒す設計を入れているのに、そこが未検証だと実機で誤動作時に止めきれない可能性がある。

applySignalBuzzer の「同状態なら再設定しない」仕様の未検証
理由: 今回の安定化の要点そのもの。ここを試験しないと、毎ループ再発行に戻っても気づきにくい。

**対応した内容：**
- updateSignalState の異常値入力テスト（currentSignalState=2）を追加。
- checkStateStuck の境界値テスト（10000msちょうど / 10001ms）を追加。
- applySignalLeds の異常値入力テスト（signalState=2）を追加。
- applySignalBuzzer の異常値テスト（toneHz=0）を追加。
- applySignalBuzzer の「同状態なら再設定しない」確認テスト（連続2回呼び出し）を追加。
- enforceBuzzerConsistency の一致時/不一致時の両ケースを確認するテストを追加。


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

# 詳細設計書 — 組込み開発実習

<!-- 作成者: あなたの名前 / 日付: YYYY-MM-DD / グループ: 〇-〇 -->

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
| 作品タイトル | 現在の曲名を表示！8bit音楽プレイヤー  |
| 状態の種類（1-2 状態遷移から） | 2個 |
| 実装する関数の数（2-2 関数一覧から） | 　9個 |
| グローバル変数の合計バイト数（2-1 SRAM確認から） | 　500B |

---

## 1. グローバル変数・定数の設計

> ※ 基本設計書（2-1 データ設計）をもとに、**型と初期値まで**決めます。
> ここで設計した変数は、この後の関数設計でそのまま使います。

```
【ピン定義】（basic_design.md 3-1 から転記）
  PIN_BUTTON    = 4    // タクトスイッチ（再生/停止, INPUT_PULLUP）
  PIN_POT       = A0   // ポテンショメータ（曲切替）
  PIN_LCD_SDA   = A4   // LCD1602 I2C SDA
  PIN_LCD_SCL   = A5   // LCD1602 I2C SCL
  PIN_BUZZER    = 3    // パッシブブザー

【状態管理】（basic_design.md 1-2 の状態名から転記）
  currentState  : byte = 0  // 0:待機中 1:計測中 2:操作実行（停止/再生・前曲/次曲）
  isPlaying     : bool = true // true:再生中 false:停止中

【タイマー（millis()用）】（basic_design.md 2-3 から転記）
  POT_INTERVAL_MS     : const unsigned long = 100  // ポテンショメータ監視周期（100ms）
  BUZZER_INTERVAL_MS  : const unsigned long = 20   // ブザー音符更新周期（20ms）
  BUTTON_INTERVAL_MS  : const unsigned long = 10   // ボタン監視周期（10ms）
  LCD_INTERVAL_MS     : const unsigned long = 250  // LCD表示更新周期（250ms）
  DEBOUNCE_DELAY      : const unsigned long = 50   // ボタン確定遅延（50ms）
  lastCheckMillis_Pot    : unsigned long = 0       // ポテンショメータ前回実行時刻
  lastCheckMillis_Buzzer : unsigned long = 0       // ブザー前回実行時刻
  lastCheckMillis_Button : unsigned long = 0       // ボタン前回実行時刻
  lastCheckMillis_LCD    : unsigned long = 0       // LCD前回実行時刻

【センサー・入力値】（basic_design.md 2-1 から転記）
  lastDebounceMillis_PlayPause : unsigned long = 0  // 再生/停止ボタン最終確定時刻
  lastDebounceMillis_Prev       : unsigned long = 0  // 前曲ボタン最終確定時刻
  lastDebounceMillis_Next       : unsigned long = 0  // 次曲ボタン最終確定時刻
  currentSongIndex : int = 0            // 現在再生中の曲番号

【その他のフラグ・カウンター】
  songs              : Song[] = （曲データで初期化） // 曲名・音階列・長さ・BPMなど
  playbackStartMillis: unsigned long = 0            // 現在曲の再生開始時刻
  elapsedSec         : unsigned int = 0             // 現在曲の経過再生時間（秒）
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
1. Arduinoのピンモードを設定する
   - ボタンピンをINPUT_PULLUPに設定
   - ブザーピンをOUTPUTに設定
   - LCDピンをI2C通信に設定
2. 曲データを初期化する（songs配列を設定）
3. LCDを初期化して「準備完了」と表示する
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
- ボタン入力を読み取る（readButtons() を呼ぶ）
- ポテンショメータ値を読み取る（readPotentiometer() を呼ぶ）
- 再生時間表示を更新する（displayPlaybackTime() を呼ぶ）
- 曲名表示を更新する（displayCurrentSongName() を呼ぶ）

＜currentState が 0（待機中）のとき＞
- ボタン入力を監視し、再生ボタンが押されたら → currentState = 1
- ポテンショメータ入力を監視し、閾値に達したら → changeSongByPot() を実行

＜currentState が  1（再生中）のとき＞
- 再生/停止ボタン押下時に togglePlayPause() を実行する
- isPlaying が false になったら → currentState = 2
- ポテンショメータ入力を監視し、閾値に達したら → changeSongByPot() を実行

＜currentState が 2（停止中）のとき＞
- 再生/停止ボタン押下時に togglePlayPause() を実行する
- isPlaying が true になったら → currentState = 1
- ポテンショメータ入力を監視し、閾値に達したら → changeSongByPot() を実行
```

---

### （関数ごとに以下のブロックをコピーして追加してください）

> ※ 基本設計書 2-2 の関数一覧に記載した関数を1つずつ設計します。

---

### `readButtons()` — 1個のボタン入力をデバウンス付きで取得する

**basic_design.md 2-2 との対応：** （共通）ボタン読出 | `readButtons()` | 1個のボタン入力をデバウンス付きで取得する | なし |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、3個のボタンの生値（digitalRead）を読む
2. 各ボタンについて、前回確定時刻からの経過時間を確認する
  - 経過時間 < DEBOUNCE_DELAY の間は変化を無視する
  - 経過時間 >= DEBOUNCE_DELAY で状態を確定し、対象ボタンの最終確定時刻を更新する
3. 確定した押下イベントを操作フラグに反映する
  - 再生/停止ボタン: isPlaying 切替または状態遷移要求を立てる

【エラー・異常ケース】
- 異常な値が来た場合: そのボタン入力は無効として扱い、前回の確定状態を維持する
- 同時押しが発生した場合: 競合を避けるため、再生/停止を優先して曲変更要求は次回ループに回す
```
### `readPotentiometer()` — ポテンショメータ値を取得し曲選択に反映する

**basic_design.md 2-2 との対応：** （共通）ポテンショメータ読出 | `readPotentiometer()` | ポテンショメータ値を取得し曲選択に反映する | なし |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、前回実行時刻 lastCheckMillis_Pot からの経過時間を確認する
2. 経過時間 < POT_INTERVAL_MS の場合は何もしない（周期管理）
3. 経過時間 >= POT_INTERVAL_MS の場合、analogRead でポテンショメータ値を取得する
4. 取得した値が曲切替の閾値を超えているか判定する
5. 閾値を超えていれば currentSongIndex を増減し、lastCheckMillis_Pot を更新する

【エラー・異常ケース】
- アナログ値が異常（範囲外）の場合は無視し、currentSongIndex を変更しない
```

### `updateLcdDisplay()` — 現在の曲名と再生時間をLCD1602に表示する

**basic_design.md 2-2 との対応：** F01 | 必須機能①: 曲名表示 | `updateLcdDisplay()` | 現在の曲名と再生時間をLCD1602に表示する | songName, elapsedSec | なし | loop()内 |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、前回表示更新時刻からの経過時間を確認する
2. 経過時間 < LCD_INTERVAL_MS の場合は何もしない（周期管理）
3. 経過時間 >= LCD_INTERVAL_MS の場合、LCD表示を更新する
4. 現在選択中の曲名（songs[currentSongIndex].name）を取得する
5. 経過再生時間（elapsedSec）を取得し、分:秒形式に変換する
6. LCD1602の1行目に曲名、2行目に再生時間を表示する
7. 表示更新後、lastCheckMillis_LCD を now で更新する

【エラー・異常ケース】
- 曲名や再生時間が異常値の場合は「---」や「00:00」として表示する
```

### `displayCurrentSongName()` — 再生中の曲名を1行目に表示する

**basic_design.md 2-2 との対応：** | F01 | 必須機能①: 曲名表示 | `displayCurrentSongName()` | 再生中の曲名を1行目に表示する | currentSongIndex | なし | loop()内 |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在選択中の曲番号 currentSongIndex から曲名を取得する（songs[currentSongIndex].name）
2. LCD1602の1行目に曲名を表示する
4. 曲名が長い場合は適宜スクロールや省略表示を行う

【エラー・異常ケース】
- currentSongIndex が範囲外の場合は「---」を表示する
```

### `displayPlaybackTime()` — 経過時間を2行目にmm:ss形式で表示する

**basic_design.md 2-2 との対応：** | F02 | 必須機能②: 再生時間表示 | `displayPlaybackTime()` | 経過時間を2行目にmm:ss形式で表示する | playbackStartMillis | なし | loop()内 |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、unsigned long 差分（now - playbackStartMillis）で経過時間を計算する
2. 差分ミリ秒を秒に変換して elapsedSec を求める
3. elapsedSec から「分」と「秒」を算出する（min = elapsedSec / 60, sec = elapsedSec % 60）
4. 1桁の値は 0 埋めし、mm:ss 形式の表示文字列を作成する
5. LCD1602 の2行目にカーソルを移動し、作成した再生時間を表示する

【エラー・異常ケース】
- playbackStartMillis が未初期化と判定される場合は 00:00 を表示する
- millis() オーバーフロー発生時も unsigned long 差分計算で継続動作させる
```

### `changeSongByPot()` — ポテンショメータ操作で前曲/次曲へ切替える

**basic_design.md 2-2 との対応：** | F03 | 必須機能③: 曲の前後切替 | `changeSongByPot()` | ポテンショメータ操作で前曲/次曲へ切替える | potValue | int（更新後index） | loop()内 |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、ポテンショメータ監視周期（POT_INTERVAL_MS）を満たすか確認する
2. 周期を満たしたら analogRead(PIN_POT) で potValue を取得する
3. potValue を前回値または中央基準値と比較し、右方向なら「次曲」、左方向なら「前曲」と判定する
4. 判定結果に応じて currentSongIndex を +1 または -1 する
5. currentSongIndex が 0 未満または曲数以上にならないように範囲補正する（先頭/末尾で折り返し可）
6. 曲が切り替わった場合は playbackStartMillis を現在時刻でリセットし、再生時間表示を初期化する
7. 更新後の index を確定し、次回判定用にポテンショメータ関連の時刻/値を更新する

【エラー・異常ケース】
- potValue が異常値（0未満または1023超相当）の場合は入力を無効として index を変更しない
- 閾値付近で値が揺れる場合は不感帯を設け、連続切替を防止する
```

### `togglePlayPause()` — 再生/停止ボタン押下で状態を切替える

**basic_design.md 2-2 との対応：** | A01 | 追加機能①: 再生/停止 | `togglePlayPause()` | 再生/停止ボタン押下で状態を切替える | isPlaying | bool（更新後状態） | loop()内 |

**引数：** なし

**戻り値：** なし（void）

```
【処理の流れ】
1. 現在時刻 now = millis() を取得し、再生/停止ボタンの入力値を読む
2. now - lastDebounceMillis_PlayPause が DEBOUNCE_DELAY 以上か確認する
3. 条件を満たし、押下が確定した場合は isPlaying を反転する
4. isPlaying が true の場合は currentState = 1 に更新し、playbackStartMillis を再設定する
5. isPlaying が false の場合は currentState = 2 に更新し、ブザー出力を停止する
6. 最後に lastDebounceMillis_PlayPause = now を更新する

【エラー・異常ケース】
- ボタン入力値が不正な場合は状態を変更せず、isPlaying を維持する
- チャタリングが連続した場合でも 1 回の押下として扱う
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
  2. 前回確定した時刻（lastDebounceMillis_*）からの経過時間を計算する
  3. 経過時間 < DEBOUNCE_DELAY（例: 50ms）→ 無視する
  4. 経過時間 ≥ DEBOUNCE_DELAY → ボタンの状態として確定する
  5. ボタンごとの lastDebounceMillis_* を更新する

【必要な変数（Section 1 に追加済みか確認）】
  lastDebounceMillis_PlayPause : unsigned long
  lastDebounceMillis_Prev      : unsigned long
  lastDebounceMillis_Next      : unsigned long
  DEBOUNCE_DELAY               : const unsigned long = 50
```

---

### 3-2. millis() を使ったタイマー管理

```
【考え方】
  「前回実行した時刻」を記録しておき、「今の時刻 − 前回時刻 ≥ 周期」なら実行する。

【処理の流れ（例: LED点滅）】
  1. now = millis()
  2. (unsigned long)(now - lastMillis_LED) >= LED_INTERVAL かどうか確認
  3. 条件を満たした場合: LEDのON/OFFを切り替え、lastMillis_LED = now
  4. 条件を満たさない場合: 何もしない（次のループで再チェック）

【自分のシステムで millis() を使う処理】
  - ボタン監視: (unsigned long)(now - lastCheckMillis_Button) >= BUTTON_INTERVAL_MS
  - ポテンショメータ監視: (unsigned long)(now - lastCheckMillis_Pot) >= POT_INTERVAL_MS
  - LCD更新: (unsigned long)(now - lastCheckMillis_LCD) >= LCD_INTERVAL_MS
  ※ いずれも unsigned long 差分で判定し、オーバーフロー時も成立するようにする
```

---

### 3-3. その他の重要ロジック（任意）

> **【任意】** 複雑なロジックがある場合のみ記入してください。
> 例：「距離に応じたLED点灯パターン」「ゲームの衝突判定」「温度の閾値判定」

```
【処理の流れ】
1. ポテンショメータの中央値（例: 512）を基準に、不感帯（例: ±80）を設定する
2. 基準値 + 不感帯より大きい場合を「次曲」、基準値 - 不感帯より小さい場合を「前曲」と判定する
3. しきい値をまたいだ瞬間だけ切替えるため、前回判定状態（左/中立/右）を保持する
4. 曲切替後は一定時間の再入力抑制（例: 150ms）を入れて連続誤動作を防ぐ
5. currentSongIndex は 0〜(曲数-1) に必ず補正する

【エラー・異常ケース】
- analogRead が異常値の場合は中立として扱い、曲切替を行わない
- 曲数が 0 の場合は index 操作を行わず処理を終了する

【入力値と出力値の関係】
- 入力: potValue（0〜1023）
- 出力: currentSongIndex（0〜曲数-1）

```

---

## 4. デバッグ出力計画（任意）

> **【任意】** 関数設計（Section 2）と並行して記入すると効果的です。
> 「動かない」ときに何を確認すればいいかを事前に計画しておきます。
> 実装後は不要な Serial.println() を削除すること。

| No | 確認したい内容 | 挿入する関数 | Serial.println の内容例 |
|:---|:---|:---|:---|
| 1 | ポテンショメータ値が正しく取れているか | `readPotentiometer()` | `Serial.println(potValue);` |
| 2 | 状態遷移が正しく起きているか | `loop()` | `Serial.println(currentState);` |
| 3 | チャタリング処理が効いているか | `readButtons()` | `Serial.println("btn confirmed");` |
| 4 |  |  |  |

---

## 5. 単体テスト仕様書（V字モデル：詳細設計 ↔ 単体テスト）

> ※ 各関数・部品が「単体で正しく動くか」を確認するテスト項目を設計します。
> 「実際の結果」欄は実装後に記入します。

### 5-1. 入力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | readButtons() | 再生/停止ボタンを1回押す | 押下イベントが1回だけ確定される | | [ ] |
| 2 | readButtons() | ボタンを素早く2回押す（チャタリング相当） | デバウンスにより1回分のみ有効になる | | [ ] |
| 3 | readPotentiometer() | ポテンショメータを中立付近で保持する | currentSongIndex が変化しない | | [ ] |
| 4 | changeSongByPot() | ポテンショメータを左・右へ操作する | currentSongIndex が -1、 +1 される（範囲内で補正） | | [ ] |
| 5 | togglePlayPause() | 再生/停止ボタンを押す | isPlaying が true/false で切り替わる | | [ ] |
| 6 | changeSongByPot() | potValue を閾値ちょうど（下限/上限）に設定する | 不感帯判定が正しく働き、意図しない曲切替が起きない | | [ ] |
| 7 | changeSongByPot() | currentSongIndex を先頭(0)と末尾(曲数-1)で曲切替する | 先頭/末尾の境界で index が破綻せず仕様通りに補正される | | [ ] |
| 8 | readPotentiometer() | 異常値入力（analogRead 範囲外相当）を与える | 入力を無効化し currentSongIndex を変更しない | | [ ] |

### 5-2. 出力系テスト

| No | テスト対象の関数 | 入力・操作 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | displayCurrentSongName() | currentSongIndex を有効な曲番号に設定して実行する | LCD1行目に対応する曲名が表示される | | [ ] |
| 2 | displayPlaybackTime() | playbackStartMillis を基準時刻に設定し数秒後に実行する | LCD2行目に mm:ss 形式の経過時間が表示される | | [ ] |
| 3 | updateLcdDisplay() | currentSongIndex と elapsedSec を更新して実行する | LCD1行目に曲名、2行目に再生時間が同時に正しく更新される | | [ ] |
| 4 | displayPlaybackTime() | playbackStartMillis を未初期化値（0想定）で実行する | 00:00 など安全な初期表示になり、異常表示しない | | [ ] |
| 5 | displayPlaybackTime() | millis() オーバーフロー直前/直後を模擬して実行する | 経過時間計算が破綻せず、表示が連続性を保つ | | [ ] |

### 5-3. タイミング・並行動作テスト

| No | テスト内容 | テスト手順 | 期待する結果 | 実際の結果 | 合否 |
|:---|:---|:---|:---|:---|:---|
| 1 | 非ブロッキング動作の確認（入力と表示の並行処理） | 再生中に再生/停止ボタンを押しながらポテンショメータを操作する | ボタン操作と曲切替が遅延なく反映され、LCD表示も停止せず更新される | | [ ] |
| 2 | millis()周期処理の精度確認 | ボタン監視（10ms）・ポテンショメータ監視（100ms）・LCD更新（250ms）を一定時間観測する | 各処理が設定周期付近で実行され、極端な周期ずれや取りこぼしが発生しない | | [ ] |
| 3 | 同時入力時の競合確認（ボタン＋ポテンショメータ） | 曲切替操作中に再生/停止ボタンを連続操作する | 優先順位ルール通りに処理され、状態と曲番号が不整合にならない | | [ ] |

---

## 6. AIレビュー記録

> グループレビューの前に必ず実施してください。

### Q1: 実装上の問題確認

> 「この詳細設計書に書いた関数と処理フローをもとに Arduino でコードを書きます。バグになりやすい箇所・処理の抜け・型の問題はありますか？」

**AIの回答（要約）：**
主な注意点は、周期値と前回時刻の変数混同、未記入関数や未定義関数名の残存、配列インデックス境界処理不足、デバウンス時刻の管理粒度不足、そして millis のオーバーフローを考慮しない差分計算です。

**対応した内容：**
- 周期定数（`*_INTERVAL_MS`）と前回時刻（`lastCheckMillis_*`）を分離し、差分判定を統一
- `togglePlayPause()` の処理フローを具体化し、未定義関数名を既存関数へ統一
- `currentSongIndex` 境界、ボタン別デバウンス時刻、`millis()` オーバーフロー差分計算の方針を明記

---

### Q2: 単体テスト仕様の確認

> 「Section 5 の単体テスト仕様書で、各関数の動作が正しく検証できていますか？テストが不足している項目や、境界値テストが必要な箇所を教えてください。」

**AIの回答（要約）：**
概ね主要機能は網羅できていますが、potValue閾値ちょうど・currentSongIndexの先頭/末尾・playbackStartMillis未初期化/オーバーフロー・同時入力（ボタン＋ポテンショメータ）・異常値入力（analogRead範囲外）の境界/異常系テストが不足しています。

**対応した内容：**
単体テストにpotValue閾値ちょうど・currentSongIndexの先頭/末尾・playbackStartMillis未初期化/オーバーフロー・同時入力（ボタン＋ポテンショメータ）・異常値入力（analogRead範囲外）の境界/異常系テストを追加

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

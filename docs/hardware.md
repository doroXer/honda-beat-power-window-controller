# Hardware / ハードウェア

## Controller / コントローラー

- Arduino Pro Mini 5V / 16MHz
- Internal EEPROM is used to retain selected window state information. / 状態情報の一部をArduino内蔵EEPROMへ保存します。

## Motor drive / モーター駆動

The sketch assumes a MOSFET H-bridge for each window motor. Two Arduino outputs per motor select UP, DOWN, or STOP. STOP is `LOW / LOW`.

本コードは左右各ウインドウモーターにMOSFET Hブリッジを使用する構成を前提としています。各モーターを2本のArduino出力でUP、DOWN、STOPに切り替えます。STOPは`LOW / LOW`です。

## Current sensing / 電流検出

- ACS712 20A sensor for the right motor / 右モーター用ACS712 20A
- ACS712 20A sensor for the left motor / 左モーター用ACS712 20A
- Right current input: A0 / 右電流入力: A0
- Left current input: A1 / 左電流入力: A1

Current sensing is used for endpoint detection by combining a threshold check with short-term current flatness detection.

電流検出は、閾値判定と短時間の電流平坦化判定を組み合わせた終点検出に使用します。

## Inputs and outputs / 入出力

| Pin | Use / 用途 |
|---|---|
| D2 | Right UP output / 右UP出力 |
| D3 | Right DOWN output / 右DOWN出力 |
| D4 | Left UP output / 左UP出力 |
| D5 | Left DOWN output / 左DOWN出力 |
| D6 | Right door input / 右ドア入力 |
| D7 | Left door input / 左ドア入力 |
| D8 | Right UP switch / 右UPスイッチ |
| D9 | Right DOWN switch / 右DOWNスイッチ |
| D10 | Left UP switch / 左UPスイッチ |
| D11 | Left DOWN switch / 左DOWNスイッチ |
| D12 | IG/ACC input / IG/ACC入力 |
| D13 | Power-hold output / 電源保持出力 |
| A0 | Right current / 右電流 |
| A1 | Left current / 左電流 |

Door and window-switch inputs use `INPUT_PULLUP` in the current software. The active switch state is LOW.

ドア入力とウインドウスイッチ入力は現行ソフトでは`INPUT_PULLUP`を使用し、スイッチON状態はLOWです。

## Power hold / 電源保持

The controller drives a power-hold output so that selected behavior and EEPROM storage can continue for a limited time after IG/ACC is removed. The current software uses a 180-second hold setting, while door activity can refresh the hold timer.

コントローラーは電源保持出力を制御し、IG/ACCがOFFになった後も一定時間、動作とEEPROM保存を継続できる構成です。現行ソフトの保持時間設定は180秒で、ドア操作によって保持タイマーが更新されます。

## Protection requirements / 保護要件

The actual vehicle installation must include appropriate fusing, wire sizing, transient protection, reverse-polarity protection where required, heat management, and safe H-bridge behavior. The Arduino I/O must not be used directly to switch window-motor current.

実車搭載時は、適切なヒューズ、配線容量、サージ対策、必要に応じた逆極性保護、熱対策、安全なHブリッジ動作を確保してください。Arduino I/Oでウインドウモーター電流を直接開閉してはいけません。

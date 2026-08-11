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

## Power architecture / 電源構成

This controller is not powered only from the vehicle IG/ACC circuit. It also requires a connection to the vehicle's always-on battery supply (BATT). The always-on supply is switched by an external MOSFET-based power-hold circuit, allowing the Arduino and the required control circuitry to remain powered after IG/ACC is switched OFF.

本コントローラーは、車両のIG/ACC電源だけで動作する構成ではありません。車両の常時電源（BATT）にも接続し、その常時電源から制御回路への給電を外部のMOSFETを使用した電源保持回路で開閉します。これにより、IG/ACCがOFFになった後もArduinoおよび必要な制御回路への給電を継続できます。

IG/ACC is therefore used primarily as a vehicle-key-state input rather than as the only controller power source. The software reads IG/ACC on D12 and controls the external power-hold circuit from D13.

したがってIG/ACCは、コントローラーを直接給電する唯一の電源というより、車両のキー状態を判定するための入力として使用します。ソフトウェアはD12でIG/ACC状態を読み取り、D13から外部の電源保持回路を制御します。

A conceptual power arrangement is shown below. The exact MOSFET, regulator, protection devices, and high-side/low-side implementation depend on the actual hardware design.

概念的な電源構成を以下に示します。MOSFET、レギュレータ、保護素子、およびハイサイド／ローサイド等の具体的な回路方式は、実際のハードウェア設計に合わせて構成してください。

```text
Vehicle battery (BATT, always on)
        |
   Fuse / protection
        |
 MOSFET power switch
        |
 Regulator / power circuit
        |
 Arduino + control circuit
        |
       D13 --------> MOSFET power-hold control

Vehicle IG/ACC
        |
        +----------> D12 (IG/ACC sense)
```

```text
車両常時電源（BATT）
        |
   ヒューズ／保護回路
        |
 MOSFET電源スイッチ
        |
 レギュレータ／電源回路
        |
 Arduino＋制御回路
        |
       D13 --------> MOSFET電源保持制御

車両IG/ACC
        |
        +----------> D12（IG/ACC検出）
```

The diagram is a functional block diagram, not a complete vehicle wiring schematic. In particular, the controller must have a valid startup path so that power is initially supplied when IG/ACC is turned ON before the Arduino can assert D13.

上図は機能を示すブロック図であり、車両用の完全な配線図ではありません。特に、Arduinoが起動してD13を出力できるようになる前に、IG/ACC ONを契機として制御回路へ初期給電できる起動経路が必要です。

## Power-on and self-hold operation / 電源投入と自己保持動作

When the vehicle key turns IG/ACC ON, the external power circuit must first supply the controller from the vehicle electrical system. After the Arduino starts, the software detects HIGH on D12 and drives D13 HIGH. The external MOSFET power circuit then keeps the BATT-derived controller supply enabled.

車両キーを操作してIG/ACCがONになると、まず外部電源回路によって車両電源からコントローラーへ給電する必要があります。Arduino起動後、ソフトウェアはD12のHIGHを検出してD13をHIGHにし、外部MOSFET電源回路によってBATT由来の制御電源を保持します。

In this sense, the controller uses a self-holding power architecture: once started, the Arduino can keep its own external power path active through D13 until the software decides that post-key-off operation is complete.

この意味で、本コントローラーは自己保持型の電源構成です。一度起動すると、ArduinoはD13を介して自分自身の外部給電経路を保持し、キーOFF後に必要な動作が完了した時点でソフトウェアから電源保持を解除できます。

## Key-off operation / キーOFF後の動作

When IG/ACC turns OFF, D12 becomes LOW, but the controller does not immediately lose power because D13 continues to hold the external MOSFET power circuit ON. This allows the controller to continue monitoring the door switches, performing the partial-down / return-up behavior, and retaining the required state information.

IG/ACCがOFFになるとD12はLOWになりますが、D13によって外部MOSFET電源回路のON状態を保持するため、コントローラーの電源は直ちには切れません。この間もドアスイッチを監視し、パーシャルダウン／復帰UP等の処理や必要な状態保持処理を継続できます。

The current v0.0 software uses `POWER_HOLD_MS = 180000`, corresponding to a nominal 180-second hold period. Door-open activity refreshes `powerHoldTime`, so the hold period is extended from the latest relevant activity rather than being an unconditional fixed 180 seconds after key-off.

現行v0.0ソフトウェアでは `POWER_HOLD_MS = 180000`、すなわち公称180秒の保持時間を設定しています。ドア開を検出すると `powerHoldTime` が更新されるため、単純にキーOFFから必ず180秒で切れるのではなく、関連するドア操作を起点として保持時間が延長されます。

When the hold period expires, the software saves the window state to EEPROM, stops both motor outputs, waits for the configured settle time, and then drives D13 LOW. The external MOSFET circuit can then disconnect the controller from the always-on BATT supply.

保持時間が終了すると、ソフトウェアはウインドウ状態をEEPROMへ保存し、左右のモーター出力を停止し、設定された待ち時間の後にD13をLOWにします。これによって外部MOSFET回路が常時電源（BATT）からコントローラーを切り離せる構成とします。

## Why an always-on battery supply is used / 常時電源を使用する理由

The purpose of the BATT connection is not simply to delay shutdown. The power-window controller needs to remain functional after the ignition supply disappears because door operation and partial-down behavior can occur after key-off. A controller powered only from IG/ACC would stop immediately and could not perform these functions.

BATT接続の目的は、単に電源OFFを遅らせることではありません。パワーウインドウコントローラーでは、キーOFF後にもドア操作やパーシャルダウン動作が発生するため、IG電源が消失した後も制御機能を維持する必要があります。IG/ACCだけから給電する構成では、キーOFFと同時に停止してこれらの処理を実行できません。

Unlike a short capacitor-only hold-up intended merely to finish a shutdown operation, this architecture deliberately uses the vehicle battery as the post-key-off energy source and disconnects it with the MOSFET circuit when continued operation is no longer required.

単に終了処理を完了するための短時間のコンデンサ保持とは異なり、本構成ではキーOFF後の動作用エネルギー源として車両バッテリーを使用し、継続動作が不要になった時点でMOSFET回路によって切り離します。

## Protection and fail-safe considerations / 保護・フェイルセーフ上の注意

Because this circuit is connected to an always-on vehicle battery supply, appropriate protection is particularly important. Provide a correctly rated fuse close to the BATT source, suitable wire sizing, automotive transient protection, reverse-polarity protection where required, and adequate thermal design for the regulator and MOSFET power path.

本回路は車両の常時電源へ接続するため、保護設計が特に重要です。BATT取り出し部近傍への適切な容量のヒューズ、十分な配線容量、車載サージ対策、必要に応じた逆極性保護、レギュレータおよびMOSFET電源経路の熱設計を行ってください。

The external power-hold circuit should also define a safe MOSFET state while the Arduino is reset, unpowered, or its D13 pin is not yet actively driven. Appropriate gate pull-up or pull-down components and a startup path should be selected for the actual MOSFET topology. A failure that leaves the hold circuit permanently ON can cause continuous standby current and eventually discharge the vehicle battery.

また、Arduinoがリセット中、無給電、またはD13をまだ能動的に駆動していない状態でも、外部電源保持回路のMOSFET状態が不定にならないようにしてください。実際のMOSFET構成に応じて適切なゲートのプルアップ／プルダウンおよび起動経路を設ける必要があります。電源保持回路が異常により常時ONになると、暗電流が継続して車両バッテリーを放電させる可能性があります。

The Arduino I/O must not directly switch window-motor current or vehicle battery current. D13 is a control signal for an appropriately designed external power-switch circuit.

Arduino I/Oでウインドウモーター電流や車両バッテリー電流を直接開閉してはいけません。D13は、適切に設計された外部電源スイッチ回路を制御するための信号です。

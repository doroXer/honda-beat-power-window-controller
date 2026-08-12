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

## Door-switch sensing / ドアスイッチ入力の読み取り

The door-switch signals have two separate functions in this system:

- hardware startup of the controller power supply
- door-state sensing by the Arduino

本システムでは、ドアスイッチ信号を次の2つの目的に使用します。

- 制御系電源を起動するためのハードウェアトリガー
- Arduinoによるドア開閉状態の読み取り

The Arduino reads the right and left door states independently on D6 and D7. The current software configures both pins as `INPUT_PULLUP` and treats LOW as the active door-open state.

Arduinoは右ドアをD6、左ドアをD7で個別に読み取ります。現行ソフトでは両入力を`INPUT_PULLUP`とし、LOWをドア開状態として扱います。

The implemented hardware includes diodes in the two Arduino door-sensing paths to prevent unwanted reverse current toward the vehicle-side door-switch circuits. The exact diode type and vehicle-side electrical polarity are not specified here.

実装したハードウェアでは、Arduino側の左右ドア入力経路にダイオードを設け、制御回路から車両ドアスイッチ回路側への不要な逆流を防止します。ダイオードの具体的な型番および車両側ドアスイッチの電気的極性については、ここでは規定しません。

```text
Right door switch ----> diode / sensing path ----> D6
Left door switch -----> diode / sensing path ----> D7
```

The door-startup path described below and the Arduino sensing path serve different purposes. A door operation can first turn the controller power ON through the hardware startup circuit; after the Arduino starts, the software reads D6/D7 and performs the required door-linked control.

後述するドアによる電源起動経路とArduinoによるドア状態読み取り経路は、役割が異なります。まずドア操作によってハードウェア起動回路から制御系電源をONにし、その後Arduinoが起動してD6/D7を読み取り、必要なドア連動制御を実行します。

## Power architecture / 電源構成

This controller is not powered only from the vehicle IG/ACC circuit. It also requires a connection to the vehicle's always-on battery supply (BATT). The controller supply is switched by an external high-side P-channel MOSFET so that the controller can be started by IG/ACC or door operation and can subsequently keep itself powered through Arduino D13.

本コントローラーは、車両のIG/ACC電源だけで動作する構成ではありません。車両の常時電源（BATT）にも接続し、外部のハイサイドPチャネルMOSFETで制御系電源を開閉します。これにより、IG/ACCまたはドア操作から制御系を起動し、その後Arduino D13によって自己保持できます。

The P-channel MOSFET gate is normally pulled toward the OFF state by a pull-up resistor. Four independent optocoupler paths act on the common gate-control node:

1. IG/ACC startup
2. Right-door startup
3. Left-door startup
4. Arduino D13 self-hold

PチャネルMOSFETのゲートはプルアップ抵抗によって通常はOFF側に保持されます。共通のゲート制御点に対して、次の4系統の独立したフォトカプラ経路を設けます。

1. IG/ACCによる起動
2. 右ドアによる起動
3. 左ドアによる起動
4. Arduino D13による自己保持

When any one of these paths becomes active, the common MOSFET gate is pulled toward GND and the P-channel MOSFET turns ON, supplying the controller from the always-on battery source. All four paths therefore control the same controller-power MOSFET.

これらのいずれか1系統が有効になると、共通のMOSFETゲートがGND側へ引かれてPチャネルMOSFETがONとなり、常時電源から制御系へ給電します。4系統はいずれも同じ制御系電源MOSFETを駆動します。

```text
Vehicle BATT (always on)
        |
   Fuse / protection
        |
 High-side P-ch MOSFET
        |
 Regulator / power circuit
        |
 Arduino + current sensors + motor-control circuitry

P-ch MOSFET gate control:

 pull-up toward OFF
        |
        +---- IG/ACC optocoupler ----+
        +---- Right-door optocoupler-+----> common gate pull-down
        +---- Left-door optocoupler--+
        +---- D13 optocoupler -------+
```

```text
車両常時電源（BATT）
        |
   ヒューズ／保護回路
        |
 ハイサイドPch MOSFET
        |
 レギュレータ／電源回路
        |
 Arduino＋電流センサー＋モーター制御部

Pch MOSFETゲート制御:

 OFF側へのプルアップ
        |
        +---- IG/ACCフォトカプラ ----+
        +---- 右ドアフォトカプラ ----+----> 共通ゲートをGND側へ
        +---- 左ドアフォトカプラ ----+
        +---- D13フォトカプラ -------+
```

The optocoupler part numbers, LED current-limiting resistor values, MOSFET part number, gate-resistor values, and vehicle-side door-switch electrical polarity are hardware-specific and are not defined here.

フォトカプラの型番、LED電流制限抵抗値、MOSFET型番、ゲート周辺の抵抗値、および車両側ドアスイッチの電気的極性については、ここでは規定しません。

## Door-triggered power startup / ドア操作による電源起動

The door switches are not used only as Arduino inputs. The right and left door signals also have independent optocoupler paths that can turn the controller-power MOSFET ON at hardware level.

ドアスイッチはArduinoへの状態入力だけに使用しているわけではありません。左右のドア信号にはそれぞれ独立したフォトカプラ経路があり、ハードウェアレベルで制御系電源MOSFETをONにできます。

This is required because the Arduino cannot read D6/D7 after its own power has already been removed. If a door is operated while the controller is off, the door-triggered hardware path first turns the controller supply ON. After the Arduino starts, the software can then read the corresponding door input and determine whether partial-down operation should be performed.

これは、Arduino自身の電源が切れた状態ではD6/D7を読み取れないためです。制御系電源OFF中にドアが操作された場合、まずドア起動用ハードウェア経路によって制御系電源をONにします。その後Arduinoが起動し、対応するドア入力を読み取ってパーシャルダウンを実行すべきかを判定します。

Thus IG/ACC, either door, or the Arduino self-hold output can keep the same controller-power MOSFET active:

したがって、IG/ACC、左右いずれかのドア、またはArduinoの自己保持出力のいずれからでも、同じ制御系電源MOSFETをON状態にできます。

```text
Power ON = IG/ACC OR right door OR left door OR D13 self-hold
```

## Power-on and self-hold operation / 電源投入と自己保持動作

When IG/ACC or either door starts the controller, the external hardware first supplies the controller from the vehicle's always-on electrical system. After the Arduino starts, the software can assert D13. D13 drives its own optocoupler path and keeps the same P-channel MOSFET ON under software control.

IG/ACCまたは左右いずれかのドア操作によって制御系が起動すると、まず外部ハードウェアによって車両常時電源から制御系へ給電されます。Arduino起動後はD13を出力でき、D13専用のフォトカプラ経路を介して同じPチャネルMOSFETをソフトウェア制御でON状態に保持します。

In this sense, the controller uses a self-holding power architecture: an external event starts it, and the Arduino then keeps its own external power path active until the software decides that continued operation is no longer required.

この意味で、本コントローラーは自己保持型の電源構成です。外部イベントで起動し、その後Arduino自身が、継続動作が不要と判断するまで外部給電経路を保持します。

## Key-off operation / キーOFF後の動作

When IG/ACC turns OFF, the controller does not necessarily lose power immediately. If D13 self-hold is active, the controller remains powered and can continue monitoring the door switches, performing partial-down / return-up behavior, and retaining the required state information.

IG/ACCがOFFになっても、制御系電源が直ちに切れるとは限りません。D13による自己保持が有効であれば制御系は動作を継続し、ドアスイッチ監視、パーシャルダウン／復帰UP、必要な状態保持処理を継続できます。

The current v0.0 software uses `POWER_HOLD_MS = 180000`, corresponding to a nominal 180-second hold period. While IG/ACC is ON the timer is continuously refreshed. A detected door-open condition also refreshes `powerHoldTime`, so the hold period can be extended from door activity.

現行v0.0ソフトウェアでは`POWER_HOLD_MS = 180000`、すなわち公称180秒の保持時間を設定しています。IG/ACCがONの間は保持タイマーを継続的に更新し、ドア開を検出した場合にも`powerHoldTime`を更新するため、ドア操作を起点として保持時間を延長できます。

When the hold period expires, the software saves the window state to EEPROM, stops both motor outputs, waits for the configured settle time, and then drives D13 LOW. With no other startup path active, the external MOSFET circuit can then disconnect the controller from the always-on BATT supply.

保持時間が終了すると、ソフトウェアはウインドウ状態をEEPROMへ保存し、左右のモーター出力を停止し、設定された待ち時間の後にD13をLOWにします。このとき他の起動経路が有効でなければ、外部MOSFET回路によって常時電源（BATT）から制御系を切り離すことができます。

## Why an always-on battery supply is used / 常時電源を使用する理由

The purpose of the BATT connection is not simply to delay shutdown. The power-window controller needs to respond to door operation before key-on and after key-off. A controller powered only from IG/ACC could neither remain operational after key-off nor restart from a door event after its supply had been removed.

BATT接続の目的は、単に電源OFFを遅らせることではありません。本パワーウインドウコントローラーはキーON前およびキーOFF後のドア操作にも応答する必要があります。IG/ACCだけから給電する構成では、キーOFF後の動作を継続できず、電源停止後にドア操作から再起動することもできません。

Unlike a short capacitor-only hold-up intended merely to finish a shutdown operation, this architecture deliberately uses the vehicle battery as the energy source and disconnects it with the MOSFET circuit when continued operation is no longer required.

単に終了処理を完了するための短時間のコンデンサ保持とは異なり、本構成では車両バッテリーを動作用エネルギー源として使用し、継続動作が不要になった時点でMOSFET回路によって切り離します。

## Protection and fail-safe considerations / 保護・フェイルセーフ上の注意

Because this circuit is connected to an always-on vehicle battery supply, appropriate protection is particularly important. Provide a correctly rated fuse close to the BATT source, suitable wire sizing, automotive transient protection, reverse-polarity protection where required, and adequate thermal design for the regulator and MOSFET power path.

本回路は車両の常時電源へ接続するため、保護設計が特に重要です。BATT取り出し部近傍への適切な容量のヒューズ、十分な配線容量、車載サージ対策、必要に応じた逆極性保護、レギュレータおよびMOSFET電源経路の熱設計を行ってください。

The external power-hold circuit should also define a safe MOSFET state while the Arduino is reset, unpowered, or its D13 pin is not yet actively driven. The gate pull-up shown conceptually above provides the normal OFF direction; the detailed component values must be selected for the actual hardware. A failure that leaves the hold circuit permanently ON can cause continuous standby current and eventually discharge the vehicle battery.

また、Arduinoがリセット中、無給電、またはD13をまだ能動的に駆動していない状態でも、外部電源保持回路のMOSFET状態が不定にならないようにしてください。上記の概念図ではゲートプルアップが通常のOFF方向を与えますが、具体的な部品定数は実際のハードウェアに合わせて選定する必要があります。電源保持回路が異常により常時ONになると、暗電流が継続して車両バッテリーを放電させる可能性があります。

The Arduino I/O must not directly switch window-motor current or vehicle battery current. D13 is a control signal for the optocoupler / external power-switch circuit.

Arduino I
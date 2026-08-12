# Control Logic / 制御ロジック

## Manual operation / 手動操作

Each window has independent UP and DOWN switch inputs. While a switch is held, the corresponding H-bridge direction output is driven. Releasing the switch normally stops the motor unless the timing state has entered auto operation.

左右各ウインドウには独立したUP/DOWNスイッチ入力があります。スイッチ操作中は対応するHブリッジ方向出力を駆動します。通常はスイッチを離すと停止しますが、タイマー状態がオート動作へ入った場合は継続動作します。

## Dual-window auto operation / 左右同時オート動作

Simultaneous operation of both UP switches starts auto UP for both windows. Simultaneous operation of both DOWN switches starts auto DOWN for both windows.

左右のUPスイッチを同時操作すると両窓オートUP、左右のDOWNスイッチを同時操作すると両窓オートDOWNを開始します。

A timeout provides a secondary upper bound on continuous auto drive.

オート動作にはタイムアウトを設け、連続駆動時間の上限も設定しています。

## Current-based endpoint detection / 電流ベース終点検出

The software does not stop merely because a single current sample exceeds a threshold. The sequence is:

1. Ignore the motor-start inrush period.
2. Sample motor current at a fixed interval.
3. Require current to exceed the configured threshold.
4. Require several consecutive samples to stay within a configured flatness range.
5. Apply a short additional push time.
6. Stop the motor and record endpoint state.

ソフトウェアは、電流が1回閾値を超えただけでは停止しません。判定の流れは次の通りです。

1. モーター起動直後の突入電流期間を無視。
2. 一定周期でモーター電流を取得。
3. 電流が設定閾値を超えていることを確認。
4. 複数サンプルが設定した平坦化範囲内に収まることを確認。
5. 短時間の追加押し込みを実施。
6. モーターを停止し、終点状態を記録。

This approach is intended to distinguish sustained endpoint/stall current from short transients. It is an implementation technique, not a safety-certified obstacle-detection system.

この方式は、短い過渡電流と継続する終点・ストール電流を区別するための実装です。安全認証された障害物検知システムではありません。

## Door-linked partial down / ドア連動パーシャルダウン

Door operation can start the controller even while IG/ACC is OFF through the hardware startup paths described in `hardware.md`. After startup, the software reads the corresponding door input and determines whether partial-down operation is appropriate.

`hardware.md`に記載したハードウェア起動経路により、IG/ACCがOFFの状態でもドア操作から制御系を起動できます。起動後、ソフトウェアは対応するドア入力を読み取り、パーシャルダウンを実行すべき状態かどうかを判定します。

The right and left door inputs are independent. In the current software D6 is the right-door input and D7 is the left-door input. Both use `INPUT_PULLUP`, and LOW is treated as the active door-open state. A 100 ms door debounce delay is used before the door-open action is processed.

左右のドア入力は独立しています。現行ソフトではD6が右ドア入力、D7が左ドア入力です。両方とも`INPUT_PULLUP`を使用し、LOWをドア開状態として扱います。ドア開時の処理前には100 msのドアデバウンス時間を設けています。

Partial down is not performed on every door opening. It is executed only when all of the following conditions are satisfied for the corresponding side:

- partial down has not already been performed for the current door-open event
- endpoint detection has previously completed (`END_REACHED`)
- the last recorded window operation direction was UP (`SW_UP`)

ドアを開けるたびに無条件でパーシャルダウンするわけではありません。対象側について次の条件がすべて成立した場合だけ実行します。

- 今回のドア開に対して、まだパーシャルダウンを実行していない
- 終点検出が成立済みである（`END_REACHED`）
- 最後に記録されたウインドウ操作方向がUPである（`SW_UP`）

`END_REACHED` by itself does not identify which end of travel was reached. Combining it with the last UP direction is therefore used to infer that the recorded endpoint is the fully closed side.

`END_REACHED`だけでは、どちら側の終点へ到達したかを識別できません。そのため、最後の操作方向がUPであることと組み合わせて、記録された終点が全閉側であると推定します。

When these conditions are satisfied and the corresponding door is opened, the window is driven DOWN for `PARTIAL_DOWN_MS = 300` ms and then stopped. After this movement the endpoint state is changed to `END_UNKNOWN`, because the window is no longer at the previously detected endpoint. The per-door partial state is also set so that the same open-door event cannot repeatedly trigger additional downward movement.

条件成立時に対応するドアを開くと、ウインドウを`PARTIAL_DOWN_MS = 300` msだけDOWN駆動し、その後停止します。この移動後は既知の終点位置から離れているため、終点状態を`END_UNKNOWN`へ変更します。また、ドアごとのパーシャル状態をセットし、同じドア開状態の間に繰り返しDOWN動作しないようにします。

When the door is subsequently closed, return-UP operation starts only if that door previously triggered partial down. The return movement is not another fixed 300 ms movement. Instead, the motor is commanded UP and the normal current-based endpoint detection is used to stop the motor at the closed-side endpoint.

その後ドアを閉じると、そのドアでパーシャルダウンを実行済みの場合だけ復帰UPを開始します。復帰動作は単純に300 msだけ逆方向へ動かす方式ではありません。モーターをUP方向へ駆動し、通常の電流ベース終点検出によって全閉側終点で停止させます。

The resulting sequence is:

```text
Door operation while controller is off
        |
        +--> hardware door-start path turns controller power ON
        |
        +--> Arduino starts and reads D6 / D7
        |
        +--> closed-side history conditions satisfied?
                | no  -> no partial-down movement
                |
                | yes
                v
             DOWN 300 ms
                |
             stop
                |
        endpoint state -> unknown
                |
          wait for door close
                |
             return UP
                |
     current-based endpoint detection
                |
             stop at endpoint
```

処理の流れは次のようになります。

```text
制御系電源OFF中にドア操作
        |
        +--> ドア起動ハード経路で制御系電源ON
        |
        +--> Arduino起動、D6 / D7を読み取り
        |
        +--> 全閉側履歴条件が成立？
                | No  -> パーシャルダウンしない
                |
                | Yes
                v
             300 ms DOWN
                |
              停止
                |
        終点状態 -> UNKNOWN
                |
            ドア閉待ち
                |
              復帰UP
                |
       電流ベース終点検出
                |
             終点で停止
```

## Key-off power hold and EEPROM / キーOFF後電源保持とEEPROM

Door operation also participates in power control. At hardware level, either door can start the controller through its optocoupler startup path. After the Arduino starts, D13 provides software-controlled self-hold. While the controller is running, a detected door-open condition also refreshes the software power-hold timer.

ドア操作は電源制御にも関係します。ハードウェア上は、左右いずれかのドアからフォトカプラ起動経路を介して制御系を起動できます。Arduino起動後はD13によってソフトウェア制御の自己保持を行います。また、制御中にドア開を検出すると、ソフトウェア上の電源保持タイマーも更新します。

While IG/ACC is ON, the power-hold output is kept active and the hold timer is continuously refreshed. When IG/ACC is OFF, the controller can remain powered through D13 self-hold. The current software uses a nominal 180-second hold period, and door-open activity can restart that timing period.

IG/ACCがONの間は電源保持出力を有効にし、保持タイマーを継続的に更新します。IG/ACCがOFFの場合でも、D13の自己保持によって制御系の動作を継続できます。現行ソフトでは公称180秒の保持時間を使用し、ドア開操作によってその計時を更新できます。

When the hold period expires, the software saves selected window state to EEPROM, stops both motors, waits briefly, and then releases D13. If IG/ACC and both door-start paths are inactive at that point, the external P-channel MOSFET can turn the controller power OFF.

保持時間が終了すると、必要なウインドウ状態をEEPROMへ保存し、左右モーターを停止し、短時間待った後にD13を解除します。その時点でIG/ACCおよび左右のドア起動経路も非アクティブであれば、外部PチャネルMOSFETによって制御系電源をOFFにできます。

## Main configurable values / 主な設定値

| Setting / 設定 | Current value / 現在値 |
|---|---:|
| Auto hold / オート判定保持 | 700 ms |
| Auto timeout / オートタイムアウト | 7000 ms |
| Power hold / 電源保持 | 180000 ms |
| Door debounce / ドアデバウンス | 100 ms |
| Partial down / パーシャルダウン | 300 ms |
| Current threshold / 電流閾値 | ADC 700 |
| Start-current ignore / 起動電流無視 | 300 ms |
| Current sample interval / 電流サンプル周期 | 20 ms |
| Flat sample count / 平坦化サンプル数 | 5 |
| Flat delta / 平坦化差分 | ADC 15 |
| Additional endpoint push / 終点追加押し込み | 50 ms per side / 左右各50 ms |

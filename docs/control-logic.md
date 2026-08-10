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

## Partial down / パーシャルダウン

When the stored state indicates that a window was fully up, opening the corresponding door can drive the window downward for the configured partial-down time. Closing the door then commands return-up behavior.

保存状態からウインドウが全閉位置にあると判断できる場合、対応するドアを開くと設定時間だけDOWN駆動します。ドアを閉じると復帰UPを指令します。

## Key-off power hold and EEPROM / キーOFF後電源保持とEEPROM

While IG/ACC is ON, the power-hold output is kept active and the hold timer is refreshed. Door activity can also refresh the timer. When the hold period expires, the software saves selected window state to EEPROM, stops both motors, waits briefly, and then releases the power-hold output.

IG/ACCがONの間は電源保持出力を有効にし、保持タイマーを更新します。ドア操作でもタイマーを更新します。保持時間が終了すると、必要なウインドウ状態をEEPROMへ保存し、左右モーターを停止し、短時間待った後に電源保持出力を解除します。

## Main configurable values / 主な設定値

| Setting / 設定 | Current value / 現在値 |
|---|---:|
| Auto hold / オート判定保持 | 700 ms |
| Auto timeout / オートタイムアウト | 7000 ms |
| Power hold / 電源保持 | 180000 ms |
| Partial down / パーシャルダウン | 300 ms |
| Current threshold / 電流閾値 | ADC 700 |
| Start-current ignore / 起動電流無視 | 300 ms |
| Current sample interval / 電流サンプル周期 | 20 ms |
| Flat sample count / 平坦化サンプル数 | 5 |
| Flat delta / 平坦化差分 | ADC 15 |
| Additional endpoint push / 終点追加押し込み | 50 ms per side / 左右各50 ms |

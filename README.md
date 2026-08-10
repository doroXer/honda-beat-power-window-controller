# doroXer dark — Honda Beat Power Window Controller

Arduino-based power-window controller for the Honda Beat (PP1), including manual control, dual-window auto operation, current-based endpoint detection, partial-down behavior, and key-off power hold.

ホンダ ビート（PP1）向けのArduinoベース・パワーウインドウコントローラーです。手動操作、左右同時オート動作、電流ベースの終点検出、パーシャルダウン、キーOFF後の電源保持を実装します。

> **Repository status / リポジトリ状態**  
> This repository is currently private and is being prepared for a future public release.  
> 現在はPrivateで公開準備中です。内容確認後にPublic化する予定です。

## doroXer dark

`doroXer dark` is the automotive / embedded-electronics side of doroXer. This repository contains the release-oriented version of the Honda Beat power-window controller project.

`doroXer dark` は、doroXerの自動車・組込み電子工作系の活動区分です。本リポジトリでは、Honda Beat用パワーウインドウコントローラーを利用者向けに整理して収録します。

## Included version / 収録バージョン

- **v0.0** — current formal version / 現在の正式版

Only the latest formal version is included here. Intermediate experiments and development history remain in the private `honda-beat-arduino-projects` development repository.

本リポジトリには正式版の最新版のみ収録します。途中の実験版や開発履歴はPrivateの `honda-beat-arduino-projects` 開発リポジトリに残します。

## Target hardware / 対象ハードウェア

- Arduino Pro Mini 5V / 16MHz
- MOSFET H-bridge motor driver for each window motor / 左右各ウインドウモーター用MOSFET Hブリッジモータードライバ
- ACS712 20A current sensor for each motor / 左右各モーター用ACS712 20A電流センサー
- Right/left door-switch inputs / 左右ドアスイッチ入力
- IG/ACC detection input / IG/ACC検出入力
- Power-hold MOSFET circuit / キーOFF後動作用の電源保持MOSFET回路
- Arduino internal EEPROM / Arduino内蔵EEPROM

## Main functions / 主な機能

- Manual UP/DOWN control for right and left windows / 左右ウインドウの手動UP/DOWN制御
- Auto UP/DOWN by simultaneous right/left switch operation / 左右スイッチ同時操作によるオートUP/DOWN
- Auto stop using current threshold plus current flatness detection / 電流閾値＋電流平坦化判定によるオートストップ
- Door-open partial down / ドア開時パーシャルダウン
- Door-close return up / ドア閉時の復帰UP
- Key-off power hold / キーOFF後の電源保持
- EEPROM state retention / EEPROMによる状態保持

## Pin assignment / ピン割り当て

| Arduino pin | Function / 機能 |
|---|---|
| D2 | Right motor driver UP / 右モータードライバUP |
| D3 | Right motor driver DOWN / 右モータードライバDOWN |
| D4 | Left motor driver UP / 左モータードライバUP |
| D5 | Left motor driver DOWN / 左モータードライバDOWN |
| D6 | Right door switch, `LOW = open` / 右ドアスイッチ、`LOW = ドア開` |
| D7 | Left door switch, `LOW = open` / 左ドアスイッチ、`LOW = ドア開` |
| D8 | Right UP switch, `LOW = on` / 右UPスイッチ、`LOW = ON` |
| D9 | Right DOWN switch, `LOW = on` / 右DOWNスイッチ、`LOW = ON` |
| D10 | Left UP switch, `LOW = on` / 左UPスイッチ、`LOW = ON` |
| D11 | Left DOWN switch, `LOW = on` / 左DOWNスイッチ、`LOW = ON` |
| D12 | IG/ACC detection / IG/ACC検出 |
| D13 | Power-hold output / 電源保持出力 |
| A0 | Right current sensor / 右電流センサー |
| A1 | Left current sensor / 左電流センサー |

## Current-based auto stop / 電流ベースのオートストップ

The endpoint detector ignores motor-start inrush current, then checks whether current exceeds the configured threshold and remains sufficiently flat over a short sample window. After endpoint detection, the motor is driven for a short additional push time and then stopped.

終点検出では、モーター起動直後の突入電流を無視した後、電流が設定閾値を超え、短時間のサンプル窓で十分に平坦化しているかを判定します。終点検出後は短時間だけ追加で押し込み、その後モーターを停止します。

Default parameters / 初期設定値:

| Parameter / パラメータ | Value / 値 |
|---|---:|
| Current threshold / 電流閾値 | ADC 700 |
| Motor-start ignore time / 起動直後無視時間 | 300 ms |
| Current sample interval / 電流サンプリング周期 | 20 ms |
| Flatness window / 平坦化判定窓 | 5 samples / 5サンプル |
| Flatness delta / 平坦化許容差 | ADC 15 |
| Auto-stop push time / オートストップ押し込み時間 | 50 ms |

## Repository structure / 構成

```text
software/
  v0.0/
    pws_v0.0.ino
docs/
```

The software version number is kept identical to the private development repository. Public-release preparation does not renumber the software.

ソフトウェアのバージョン番号はPrivate開発リポジトリと共通です。公開用に整理する際も、ソフトウェアの番号を付け直しません。

## Safety and disclaimer / 安全上の注意・免責

This project is a personal experimental and research result for automotive electronics. Use appropriate fusing, wire sizing, insulation, motor-driver protection, and mechanical safeguards. Verify all applicable laws and vehicle requirements before installation.

本プロジェクトは個人による自動車電子工作の実験・研究成果です。適切なヒューズ、配線容量、絶縁、モータードライバ保護、機械的安全対策を行い、車両へ適用する前に法令・車両要件への適合を利用者自身で確認してください。

**The current-based auto-stop function is not equivalent to certified anti-pinch / obstacle-detection protection.** Do not rely on it as a safety-certified pinch-protection system.

**本コードの電流ベース・オートストップは、認証された挟み込み防止・障害物検知機能と同等ではありません。** 安全認証された挟み込み防止機能として依存しないでください。

The author provides no warranty and accepts no responsibility for vehicle damage, accidents, injury, legal non-compliance, or other loss arising from use of this project.

本プロジェクトの使用によって生じた車両故障、事故、負傷、法令不適合、その他の損害について、作者は保証・責任を負いません。

## License / ライセンス

Software in this repository is released under the MIT License unless otherwise stated.

本リポジトリのソフトウェアは、特記のない限りMIT Licenseで公開します。

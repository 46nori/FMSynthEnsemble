# midism

GM準拠のMIDI音源モジュール

## 開発環境

VSCodeでの開発を前提としている。

1. [このドキュメント](https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html)に従いRaspberry Pi PicoにDebug Probeを接続し、OpenOCD、GDBをインストールする
2. VSCodeをインストールする。
3. VSCodeの拡張機能で`Raspberry Pi Pico`をインストールする。[Getting Start Guide](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)のChapter3 参照。

デフォルトではRasopberry Pi Pico2(RP2350)用の設定になっている。  
Pico(RP2040)を使用する場合は[.vscode/launch.json](./.vscode/launch.json)の以下の部分で、`target/rp2040.cfg`が有効になるようにする。

```json
            "configFiles": [
                "interface/cmsis-dap.cfg",
                //"target/rp2040.cfg"
                "target/rp2350.cfg"
            ],
```

### ビルド方法

本ディレクトリでVSCodeを起動する。
`Raspberry Pi Pico Project`のペインを開き、`Compile Project`を選択する。  

### Flashメモリへの焼き込み

Flashメモリにプログラムを焼き込むことで、スタンドアロン動作が可能になる。

`Raspberry Pi Pico Project`のペインを開き、`Flash Project`を選択する。

すでにデバッグ実行中の場合は、デバッグ停止ボタンで現在のデバッグセッションを中止すること。

### 実機デバッグ

`Raspberry Pi Pico Project`のペインを開き、`Debug Project`を選択する。
ターゲットにオブジェクトが書き込まれ、実行が始まる。ブレークポイントやステップ実行などの設定ができる。

すでにデバッグ実行中の場合は、デバッグ停止ボタンで現在のデバッグセッションを中止すること。

### Doxygenによるドキュメント生成

1. Doxygenのインストール (1度だけ実行すれば良い)

    ```bash
    brew install doxygen
    ```

2. ドキュメントの生成  
   `build/docs/`以下にHTMLが生成される。

    ```bash
    cd build/
    cmake ..
    make doc
    ```

### clang-formatによるソースコード整形

1. clang-formatのインストール (1度だけ実行すれば良い)

    ```bash
    brew install clang-format
    ```

2. ソースコード整形

    ```bash
    cd build/
    cmake --build . --target format
    ```

## 仕様

- [MIDIインプリメンテーションチャート](./docs/MIDI_ImplementationChart.md)
- [ソフトウェア設計仕様](./docs/README.md)
- ソースファイル

    ```bash
    .
    ├── CMakeLists.txt                      CMakeの設定
    ├── main.cpp                            メインプログラム
    ├── Debugger.cpp
    ├── Debugger.h                          簡易デバッグ機能
    ├── config.h                            機能のConfiguration
    ├── docs
    ├── hal                                 ハードウェア抽象化レイヤ
    │   ├── HAL.h
    │   ├── OpnBase.cpp
    │   ├── OpnBase.h                       OPNのインターフェース(基底クラス)
    │   ├── RP2040.cpp
    │   ├── RP2040.h                        RP2040の制御クラス
    │   ├── YM2203.h                        YM2203のインターフェース(OpnBaseの派生クラス)
    │   ├── YM2608.cpp
    │   ├── YM2608.h                        YM2608のインターフェース(OpnBaseの派生クラス)
    │   └── tone
    │       └── tone_table.inc              FM音源パラメータ(音色データ)
    ├── midi
    │   ├── MidiFactory.cpp
    │   ├── MidiFactory.h                   MIDI関連クラスのインスタンス生成と紐付け
    │   ├── MidiPanel.h                     MIDIパネルの制御
    │   ├── MidiProcessor.cpp
    │   ├── MidiProcessor.h                 MIDIメッセージの処理クラス
    │   ├── channel
    │   │   ├── MidiChannel.cpp
    │   │   ├── MidiChannel.h               MidiChannelインターフェース(基底クラス)
    │   │   ├── MidiChannelObserver.h       Voice再割り当てのオブザーバー
    │   │   ├── NoteChannel.cpp
    │   │   ├── NoteChannel.h               楽曲用MIDI Channel(MidiChannelの派生クラス)
    │   │   ├── RhythmChannel.cpp
    │   │   └── RhythmChannel.h             リズム用MIDI Channel(MidiChannelの派生クラス)
    │   └── voice
    │       ├── CsmVoice.cpp
    │       ├── CsmVoice.h                  CSM音声合成のボイス(Voiceの派生クラス)
    │       ├── NoteVoice.cpp
    │       ├── NoteVoice.h                 楽曲用ボイス(Voiceの派生クラス)
    │       ├── Voice.cpp
    │       ├── Voice.h                     Voiceインターフェース(基底クラス)
    │       ├── VoiceAllocator.cpp
    │       ├── VoiceAllocator.h            MIDI ChannelへのVoiceの割り当て
    │       └── csm
    │           └── VOICE.dat               CSM音声データ
    ├── pico_sdk_import.cmake
    └── usb                                 TinyUSBの設定ファイル
        ├── tusb_config.h
        └── usb_descriptors.cpp
    ```

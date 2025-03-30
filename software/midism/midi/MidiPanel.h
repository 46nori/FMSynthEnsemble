//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <cstdint>

#include "OpnBase.h"

class MidiPanel {
private:
    OpnBase& module;                                               // 使用するOPN
    uint8_t row;                                                   // 現在の列番号
    uint8_t p_row;                                                 // 直前の列番号
    uint8_t PA7_4[4]                  = {0x00, 0x00, 0x00, 0x00};  // LEDの列状態(上位4bit)
    static constexpr uint8_t PA3_0[4] = {0b00001110, 0b00001101, 0b00001011,
                                         0b00000111};  // Row制御データ(下位4bit)
    static constexpr uint16_t mask[4] = {0xfff0, 0xff0f, 0xf0ff, 0x0fff};

    uint16_t midi_sw_state;     // MIDI ChannelのON/OFF状態(正論理16bit分)
    uint16_t led_status;        // LEDの状態(正論理16bit分)
    uint16_t midi_reset_count;  // MIDIリセットボタン連続押下回数
    bool bReset;                // MIDIリセットボタン押下状態
    bool bShowKeyOn;            // LED表示モード

public:
    /**
     * @brief コンストラクタ
     * @param module    使用するOPN
     */
    MidiPanel(OpnBase& module);
    MidiPanel() = delete;
    ~MidiPanel();

    /**
     * @brief 1列分のスイッチの状態とLEDの点灯状態の更新
     * @details 1回の呼び出しで、1列分のスイッチの読み出しと、LEDの点灯を行う。
     * CPUの空き時間に本APIを繰り返し呼び出すこと。
     * bShowKeyOn=falseの場合、4サイクル前の当該列のMIDIスイッチの状態をLEDに反映する。
     * midiswは4サイクル分の遅延あり。buttonはリアルタイムの値を返す。
     * LEDのダイナミック点灯をかねているので、点灯光量やちらつきは呼び出し頻度に依存する。
     */
    void Update();

    /**
     * @brief LEDの表示データをセットする
     * @param led 16bitのビットパターン(正論理)
     * @details 次回のUpdate()の呼び出しでLEDに状態が反映される。
     * Update()ごとに4bit分ずつ更新されるので、全データの表示には
     * 4回のUpdate()の呼び出しが必要。
     */
    void SetLed(uint16_t led);

    /**
     * @brief LED表示
     * @param row       列番号 0 - 3
     * @param col       列データ(4bit分)
     */
    void DisplayRow(uint8_t row, uint8_t col);

    /**
     * @brief MIDIリセットの検出
     * @return true: MIDIリセットボタン押下を検知
     */
    bool IsMidiReset();

    /**
     * @brief MIDI Channel ON/OFFスイッチ状態の取得
     * @return 16bitのビットパターン(正論理) MSB:Ch16, LSB:Ch1
     */
    uint16_t GetMidiSwState();
};

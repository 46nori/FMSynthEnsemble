//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <array>

#include "MidiFactory.h"

/**
 * @brief MidiProcessor class
 */
class MidiProcessor {
private:
    std::array<MidiChannel*, MIDI_CHANNELS>& channels;

    enum MIDI_MESSAGE {
        NOTE_OFF         = 0x8,
        NOTE_ON          = 0x9,
        KEY_AFTER_TOUCH  = 0xa,
        CONTROL_CHANGE   = 0xb,
        PROGRAM_CHANGE   = 0xc,
        CH_AFTER_TOUCH   = 0xd,
        PITCH_BEND       = 0xe,
        SYSTEM_EXCLUSIVE = 0xf
    };

    uint16_t enabled_channels;  // MIDI ChannelのON/OFF状態
    uint16_t note_on_status;    // MIDI ChannelのNoteOn状態
    uint8_t status_byte;        // ステータスバイトの保持用

    // System Exclusive message
    bool isSysEx;
    int q_index;            // index of msg_queue
    uint8_t msg_queue[10];  // data buffer between 0xf0 and 0xf7.

public:
    /**
     * @brief コンストラクタ
     * @param channels MIDIチャンネルの配列
     */
    MidiProcessor(std::array<MidiChannel*, MIDI_CHANNELS>& channels);
    MidiProcessor() = delete;
    ~MidiProcessor();

    /**
     * @brief MIDI 16チャンネル分のON/OFF設定
     * @param states    ビットマップ 1:ON, 0:OFF
     */
    void EnableChannels(uint16_t states);

    /**
     * @brief MIDIメッセージ処理
     * @return MIDI ChannelのNoteOn状態のビットマップ
     */
    uint16_t Exec(uint8_t msg[3], int num);

    /**
     * @brief MIDIチャンネルのリセット
     */
    void Reset();

private:
    void process_event(const uint8_t msg[3]);
    void dump_message(const uint8_t msg[3], int num);
    void process_sysex_msg(int length);
    bool push(uint8_t msg);

    //
    //  System Exclusive message handling
    //
    static constexpr uint8_t GM_SYSTEM_ON[] = {0x7e, 0x7f, 0x09, 0x01};
    static constexpr uint8_t XG_RESET[]     = {0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00};
    static constexpr uint8_t GS_RESET[] = {0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41};
    static constexpr uint8_t DEBUG_1[]  = {0x00, 0x00};  // デバック用独自定義
};

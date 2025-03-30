//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include "MidiChannel.h"
#include "MidiChannelObserver.h"
#include "YM2608.h"

/**
 * @brief Rhythm Channel class (CH=10)
 */
class RhythmChannel : public MidiChannel, public MidiChannelObserver {
private:
    OpnBase* module;

public:
    static constexpr int MIDI_RHYTHM_CHANNEL = 9;  // MIDI CH=10

    /**
     * @brief コンストラクタ
     * @param module Rhythmを割り当てるFM音源モジュール
     */
    RhythmChannel(OpnBase* module);
    RhythmChannel() = delete;

    /**
     * @brief デストラクタ
     */
    virtual ~RhythmChannel();

    /**
     * @brief MIDI Volumeをセット
     * @param vol MIDI Volume (0 - 127)
     */
    void SetVolume(int vol) override;

    /**
     * @brief Note On
     * @param key MIDI Note No.
     * @param velocity MIDI Velocity
     * @return -1:Fail, 0:NoteOff, 1:NoteOn
     */
    int NoteOn(int key, int velocity) override;

    /**
     * @brief Note Off
     * @param key MIDI Note No.
     * @return -1:Fail, 0:NoteOff, 1:Keep NoteOn
     */
    int NoteOff(int key) override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceのうち未使用のものを解放する
     * @return FM音源のVoiceは使用しないので常にnullptrを返す
     */
    Voice* Release(int mid, bool type) override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceをすべて解放する
     * @details FM音源のVoiceは使用しないので何もしない
     */
    void ReleaseAll() override;

    /**
     * @brief MIDIチャンネルのリセット
     */
    void Reset() override;

    // Debug
    void dump() override;

private:
    /**
     * @brief RTL/ILの初期化
     * @param rtl RTL Volume (0 - 127)
     * @param il  IL  Volume (0 - 127)
     */
    void init_volume(uint8_t rtl, uint8_t il);
};

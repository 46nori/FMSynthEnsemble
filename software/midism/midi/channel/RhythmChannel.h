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
#include <array>
#include <vector>

/**
 * @brief Rhythm Channel class (CH=10)
 */
class RhythmChannel : public MidiChannel, public MidiChannelObserver {
private:
    std::vector<OpnBase*> modules;          // 使用可能なモジュール(YM2608)
    uint8_t last_module            = 0;     // 直前に発音したモジュールのindex
    uint8_t cur_module             = 0;     // 現在のモジュールのindex
    int16_t last_exclusive_note[6] = {-1};  // 各排他グループの直前のノート番号を記憶

public:
    static constexpr int MIDI_RHYTHM_CHANNEL = 9;  // MIDI CH=10

    /**
     * @brief コンストラクタ
     * @param modules FM音源モジュールの配列
     */
    RhythmChannel(std::array<OpnBase*, 4>& input_modules);
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
     * @details 排他ノートを除き消音処理は行わない
     */
    int NoteOn(int key, int velocity) override;

    /**
     * @brief Note Off
     * @param key MIDI Note No.
     * @return 0:NoteOff
     * @details 消音処理は行わない
     */
    int NoteOff(int key) override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceのうち未使用のものを解放する
     * @return FM音源のVoiceは使用しないので常にnullptrを返す
     */
    Voice* Release(int mid, bool type) override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceをすべて解放する
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

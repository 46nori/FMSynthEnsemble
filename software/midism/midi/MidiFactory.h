//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <array>

#include "MidiChannel.h"
#include "OpnBase.h"
#include "config.h"

/**
 * @brief MidiFactory class
 */
class MidiFactory {
    std::array<OpnBase*, 4>& modules;
    std::array<MidiChannel*, MIDI_CHANNELS> channels;

public:
    /**
     * @brief コンストラクタ
     * @param modules FM音源モジュールの配列
     */
    MidiFactory(std::array<OpnBase*, 4>& modules);
    MidiFactory() = delete;

    /**
     * @brief デストラクタ
     */
    virtual ~MidiFactory();

    /**
     * @brief MIDIチャンネルとMIDI Voiceの生成
     * @param rhythm_module 割り当てるリズム音源モジュール
     * @return 生成したMIDIチャンネルの配列
     */
    std::array<MidiChannel*, MIDI_CHANNELS>& Create(OpnBase* rhythm_module = nullptr);
};

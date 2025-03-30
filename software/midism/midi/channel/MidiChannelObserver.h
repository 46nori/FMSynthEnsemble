//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once

class Voice;

/**
 * @brief Channel Observer Interface
 */
class MidiChannelObserver {
public:
    /**
     * @brief チャンネルに割り当てられたVoiceのうち未使用のものを解放する
     * @return 割り当てできない場合はnullptrを返す
     */
    virtual Voice* Release(int mid, bool type) = 0;

    /**
     * @brief 割り当てられたVoiceをすべて解放する
     */
    virtual void ReleaseAll() = 0;
};

struct ObserverInfo {
    int channel;
    MidiChannelObserver* observer;
};

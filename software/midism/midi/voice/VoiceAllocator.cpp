//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "VoiceAllocator.h"

#include "Debugger.h"

VoiceAllocator& VoiceAllocator::GetInstance() {
    static VoiceAllocator instance;
    return instance;
}

void VoiceAllocator::AddVoice(Voice* voice) {
    voice_pool.push_back(voice);
}

void VoiceAllocator::DeleteAllVoices() {
    for (auto& voice : voice_pool) {
        delete voice;
    }
    voice_pool.clear();
}

void VoiceAllocator::AddObserver(int channel, MidiChannelObserver* observer) {
    observers.push_back({channel, observer});
}

void VoiceAllocator::DeleteAllObserver() {
    observers.clear();
}

Voice* VoiceAllocator::AllocateVoice(int channel, int mid, bool type) {
    Voice* candidate = nullptr;

    // note_voice_poolから未割り当てのVoiceを探す
    for (auto* voice : voice_pool) {
        if (voice->IsFree() && voice->GetType() == type) {
            // 未割り当てのVoiceがあった
            candidate = voice;
            if (mid == -1 || candidate->GetModuleId() == mid) {
                voice->SetChannel(channel);
                return voice;
            }
        }
    }
    // 他のChannelに割り当てた中から未使用Voiceを探す
    // 優先度の低いMIDI Channelから依頼する
    for (auto it = observers.rbegin(); it != observers.rend(); ++it) {
        if (it->channel != channel) {
            auto voice = it->observer->Release(mid, type);
            if (voice) {
                // 未使用Voiceがあった
                voice->SetChannel(channel);
                return voice;
            }
        }
    }
    if (candidate) {
        // note_voice_poolで見つかった候補を返す
        candidate->SetChannel(channel);
        return candidate;
    }
    // 未使用Voiceがなかった
    ++failed_count;
    return nullptr;
}

/**
 * @brief Voiceを解放する
 * @details MIDI Channelに割り当て済みのVoiceを解放し、全Voiceをリセットする
 */
void VoiceAllocator::Reset() {
    failed_count = 0;
    // Channelに割り当て済みのVoiceを強制解放
    for (auto& info : observers) {
        info.observer->ReleaseAll();
    }
    // 全Voiceをリセット
    for (auto& voice : voice_pool) {
        voice->Reset();
    }
}

//
// For debug
//
int VoiceAllocator::GetFailedCount() {
    return failed_count;
}

void VoiceAllocator::dump() {
    printf("\n=== Voice List ===\n");
    for (auto& voice : voice_pool) {
        voice->dump();
    }
}

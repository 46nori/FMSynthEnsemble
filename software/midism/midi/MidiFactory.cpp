//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "MidiFactory.h"

#include "CsmVoice.h"
#include "NoteChannel.h"
#include "NoteVoice.h"
#include "RhythmChannel.h"
#include "config.h"

//#define ENABLE_CSM

MidiFactory::MidiFactory(std::array<OpnBase*, 4>& modules) : modules(modules) {
}

MidiFactory::~MidiFactory() {
    VoiceAllocator::GetInstance().DeleteAllVoices();
    VoiceAllocator::GetInstance().DeleteAllObserver();
    for (auto* channel : channels) {
        delete channel;
    }
}

std::array<MidiChannel*, MIDI_CHANNELS>& MidiFactory::Create(OpnBase* rhythm_module) {
    VoiceAllocator& allocator = VoiceAllocator::GetInstance();

    // VoiceAllocatorにFM音源モジュールのチャンネルを登録
    // 音楽用
    int vid = 0;  // voice id
    for (auto* module : modules) {
        if (module) {
            module->init();
            for (int ch = 0; ch < module->fm_get_channels(); ++ch) {
#if ENABLE_CSM != 0
                if (ch != 2) {
                    allocator.AddVoice(new NoteVoice(*module, ch, vid++));
                }
#else
                allocator.AddVoice(new NoteVoice(*module, ch, vid++));
#endif
            }
        }
    }

#if ENABLE_CSM != 0
    // CSM音声合成用
    CsmVoice* csm = new CsmVoice(modules, vid++);
    csm->Init(true);  // CSMを割り込み駆動で動作させる
    allocator.AddVoice(csm);
#endif

    // MIDIチャンネルのインスタンスを生成
    for (int i = 0; i < channels.size(); i++) {
        if (i == RhythmChannel::MIDI_RHYTHM_CHANNEL) {
            // リズムチャンネル
            RhythmChannel* rc = new RhythmChannel(rhythm_module);
            channels[i]       = rc;
            // VoiceAllocatorにオブザーバーを登録
            allocator.AddObserver(i, rc);
        } else {
            // ノートチャンネル
            NoteChannel* nc = new NoteChannel(i);
            channels[i]     = nc;
            // VoiceAllocatorにオブザーバーを登録
            allocator.AddObserver(i, nc);
        }
    }
    return channels;
}

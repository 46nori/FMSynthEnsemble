//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "NoteChannel.h"

#include "Debugger.h"
#include "config.h"

NoteChannel::NoteChannel(int no) : MidiChannel(no), bCsmVoiceMode(false) {
    allocator = &VoiceAllocator::GetInstance();
}

NoteChannel::~NoteChannel() {
}

void NoteChannel::Reset() {
    Hold1(0);
    for (auto& voice : activeQueue) {
        voice->NoteOff();
    }
    for (auto& voice : holdQueue) {
        voice->NoteOff();
    }
    MidiChannel::Reset();
    bCsmVoiceMode = false;
}

void NoteChannel::moveVoice(std::list<Voice*>& src, std::list<Voice*>::iterator it,
                            std::list<Voice*>& dst) {
    dst.splice(dst.end(), src, it);
    // この時点で呼び出し元のitは無効になる
}

void NoteChannel::moveAllVoices(std::list<Voice*>& src, std::list<Voice*>& dst) {
    dst.splice(dst.end(), src);
}

Voice* NoteChannel::getFreeVoice(int mid, bool type, bool fromFirst) {
    // 最近使ったmoduleに属するVoiceを優先的に探す。
    // NoteVoiceをチャンネル内で再利用する場合は末尾から、解放する場合は先頭から探す。
    // CsmVoiceは頻度が少なく先頭に滞留する傾向がある前提で先頭から探す。

    if (freeQueue.empty()) {
        return nullptr;
    }

    Voice* voice = nullptr;
    if (type == true) {
        // CsmVoiceを先頭から探す
        for (auto it = freeQueue.begin(); it != freeQueue.end(); ++it) {
            //if ((*it)->GetType() == type &&
            //    (mid == -1 || (*it)->GetModuleId() == mid)) {
            if ((*it)->GetType() == type) {
                voice = *it;
                freeQueue.erase(it);
                return voice;
            }
        }
    } else if (fromFirst) {
        // 最近使ったmoduleに属するNoteVoiceを先頭から探す
        auto it        = freeQueue.begin();
        auto end       = freeQueue.end();
        auto candidate = it;
        for (; it != end; ++it) {
            if ((*it)->GetType() == type) {
                if (voice == nullptr) {
                    candidate = it;  // 先頭に最も近いVoice候補
                }
                voice = *it;  // イテレータが失効する前にVoiceを保存
                if (mid == -1 || voice->GetModuleId() == mid) {
                    freeQueue.erase(it);
                    return voice;
                }
            }
        }
        // なければ先頭に最も近い候補から取り出す
        if (voice) {
            voice = *candidate;
            freeQueue.erase(candidate);
            return voice;
        }
    } else {
        // 最近使ったmoduleに属するNoteVoiceを末尾から探す
        auto it        = freeQueue.rbegin();
        auto end       = freeQueue.rend();
        auto candidate = it;
        for (; it != end; ++it) {
            if ((*it)->GetType() == type) {
                if (voice == nullptr) {
                    candidate = it;  // 末尾に最も近いVoice候補
                }
                voice = *it;  // イテレータが失効する前にVoiceを保存
                if (mid == -1 || voice->GetModuleId() == mid) {
                    freeQueue.erase(std::next(it).base());
                    return voice;
                }
            }
        }
        // なければ末尾に最も近い候補から取り出す
        if (voice) {
            voice = *candidate;
            freeQueue.erase(std::next(candidate).base());
            return voice;
        }
    }
    // 再利用可能なVoiceが存在しない
    return nullptr;
}

Voice* NoteChannel::Release(int mid, bool type) {
    // freeQueueの先頭から未使用のVoiceを探す
    auto voice = getFreeVoice(mid, type, true);
    if (voice) {
        ++rel_success_count;
    } else {
        ++rel_fail_count;
    }
    return voice;
}

void NoteChannel::ReleaseAll() {
    activeQueue.clear();
    holdQueue.clear();
    freeQueue.clear();
}

void NoteChannel::BankSelect_LSB(uint8_t val) {
    MidiChannel::BankSelect_LSB(val);
#if ENABLE_CSM != 0
    if (bk_program >> 24 == 0x03) {
        bCsmVoiceMode = true;
    } else {
        bCsmVoiceMode = false;
    }
#endif
}

void NoteChannel::SetProgram(uint8_t no) {
    MidiChannel::SetProgram(no);
}

void NoteChannel::SetVolume(int vol) {
    for (auto& voice : activeQueue) {
        voice->SetVolume(vol);
    }
    for (auto& voice : holdQueue) {
        voice->SetVolume(vol);
    }
    volume = vol;  // SetVolume()後に更新する
}

int NoteChannel::NoteOn(int key, int velocity) {
    // Velocity=0なのでNoteOff処理
    if (velocity == 0) {
        return NoteOff(key);
    }

    int mid = -1;  // 最近使ったmoduleが不明

    // holdQueue内の同一keyのVoiceを探して再利用
    for (auto it = holdQueue.begin(); it != holdQueue.end(); ++it) {
        if ((*it)->GetKey() == key) {
            // 強制Damp & 再利用
            (*it)->NoteOff();
            (*it)->NoteOn(key, bk_program, volume, effect, outputLR);
            DPRINTF(1, " H%02d ", (*it)->id);
            moveVoice(holdQueue, it, activeQueue);  // activeQueueに移動
            return 1;
        }
        mid = (*it)->GetModuleId();  // 最近使ったmodule
    }
    // activeQueue内の同一keyのVoiceを探して再利用
    for (auto& voice : activeQueue) {
        if (voice->GetKey() == key) {
            // 強制Damp & 再利用
            voice->NoteOff();
            voice->NoteOn(key, bk_program, volume, effect, outputLR);
            DPRINTF(1, " A%02d ", voice->id);
            return 1;
        }
        mid = (voice)->GetModuleId();  // 最近使ったmodule
    }

    // freeQueue内のVoiceを再利用
    //   なるべく最近使ったものから探す
    Voice* voice = getFreeVoice(mid, bCsmVoiceMode, false);
    if (voice == nullptr) {
        // 使用可能なVoiceがないので新規にAllocate
        voice = allocator->AllocateVoice(channel, mid, bCsmVoiceMode);
        if (voice == nullptr) {
            // AllocateできなかったのでNoteOn失敗
            ++rel_fail_count;
            DPRINTF(1, "!!!!!!!!!!");
            return -1;
        }
        // 新規にAllocateできた
        DPRINTF(1, " N%02d ", voice->id);
    } else {
        // 再利用
        DPRINTF(1, " F%02d ", voice->id);
    }
    // 新規にAllocateしたVoiceをActiveキューに追加
    voice->NoteOn(key, bk_program, volume, effect, outputLR);
    activeQueue.push_back(voice);

    return 1;
}

int NoteChannel::NoteOff(int key) {
    for (auto it = activeQueue.begin(); it != activeQueue.end();) {
        if ((*it)->GetKey() == key) {
            if ((*it)->DecrementNoteOnCount() > 0) {
                // オーバラップノートのため、まだNoteOffしない
                DPRINTF(1, " K%02d ", (*it)->id);
                return 1;
            }
            if (hold1 == false) {
                (*it)->NoteOff();
                DPRINTF(1, " -%02d ", (*it)->id);       // itが無効になるのでここで出力
                moveVoice(activeQueue, it, freeQueue);  // freeQueueに移動
            } else {
                // Hold状態なのでNoteOffを保留する
                DPRINTF(1, " H%02d ", (*it)->id);       // itが無効になるのでここで出力
                moveVoice(activeQueue, it, holdQueue);  // holdQueueに移動
            }
            return 0;
        } else {
            ++it;
        }
    }
    DPRINTF(1, " -?? ");
    return -1;
}

void NoteChannel::Hold1(int val) {
    if (val >= 64) {
        // Hold1 on
        hold1 = true;
    } else {
        // Hold1 off
        hold1 = false;
        for (auto& voice : holdQueue) {
            voice->NoteOff();
        }
        moveAllVoices(holdQueue, freeQueue);
    }
}

void NoteChannel::DataEntry_MSB(uint8_t val) {
    if (rpn_msb == 0) {
        if (rpn_lsb == 0) {
            // PitchBend Sensitivity (LSBは使用しない)
            effect.pbs = val;
        }
        if (rpn_lsb == 2) {
#if ENABLE_COARSE_TUNE == 1
            // Master Coarse Tuning (LSBは使用しない)
            effect.coarse_tune = (int)val - 64;
#endif
        }
    } else if (nrpn_msb == 1) {
        if (nrpn_lsb == 8) {
            // Bibrate rate
            effect.vbrate = val;
        } else if (nrpn_lsb == 9) {
            // Vibrato depth
            effect.vbdepth = val;
        }
    }
}

void NoteChannel::PitchBend(int16_t val) {
    if (effect.pbv != val) {  // 変化があった時のみ適用
        effect.pbv = val;
        for (auto& voice : activeQueue) {
            voice->SetPitch(effect);
        }
        // holdQueueのVoiceには効果が薄いため省略
    }
}

void NoteChannel::SetModulation(uint8_t val) {
    if (effect.vbdepth != val) {  // 変化があった時のみ適用
        effect.vbdepth = val;
        for (auto& voice : activeQueue) {
            voice->SetModulation(effect, outputLR);
        }
    }
}

void NoteChannel::SetPan(uint8_t val) {
    if (pan != val) {
        if (val < 42) {
            outputLR = MidiChannel::Output::L;
        } else if (val < 84) {
            outputLR = MidiChannel::Output::LR;
        } else {
            outputLR = MidiChannel::Output::R;
        }
        for (auto& voice : activeQueue) {
            voice->SetModulation(effect, outputLR);
        }
        pan = val;
    }
}

// Debug
void NoteChannel::dump() {
    MidiChannel::dump();
    printf("  TYPE=%s\n", bCsmVoiceMode ? "CSM" : "Note");
    printf("  activeQ=%2zu :", activeQueue.size());
    for (auto& voice : activeQueue) {
        printf(" %2d", voice->id);
    }
    printf("\n    holdQ=%2zu :", holdQueue.size());
    for (auto& voice : holdQueue) {
        printf(" %2d", voice->id);
    }
    printf("\n    freeQ=%2zu :", freeQueue.size());
    for (auto& voice : freeQueue) {
        printf(" %2d", voice->id);
    }
    printf("\n");
}

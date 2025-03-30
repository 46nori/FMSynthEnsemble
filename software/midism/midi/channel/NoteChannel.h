//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <list>

#include "MidiChannel.h"
#include "MidiChannelObserver.h"
#include "VoiceAllocator.h"

/**
 * @brief NoteChannel class
 */
class NoteChannel : public MidiChannel, public MidiChannelObserver {
private:
    VoiceAllocator* allocator;
    std::list<Voice*> activeQueue;  // NoteON状態のVoiceキュー
    std::list<Voice*> holdQueue;    // NoteOFF待ち状態のVoiceキュー
    std::list<Voice*> freeQueue;    // 未使用状態のVoiceキュー
    bool bCsmVoiceMode;             // true:CsmVoice, false:NoteType

    /**
     * @brief freeQueueから未使用のVoiceを取得する
     * @param mid       最近使ったmodule id
     * @param type      true:CsmVoice, false:NoteVoice
     * @param fromFirst true:先頭から検索する / false:末尾から検索する
     * @details 最近使ったmoduleに属するVoiceを優先的に探す。
     */
    Voice* getFreeVoice(int mid, bool type, bool fromFirst);

    /**
     * @brief Voiceをキュー間で移動する
     * @param src 移動元キュー
     * @param it  移動元キューのイテレータ
     * @param dst 移動先キュー
     * @details srcキューのitが指し示すVoice 1つ分を、dstキューに移動する
     */
    void moveVoice(std::list<Voice*>& src, std::list<Voice*>::iterator it, std::list<Voice*>& dst);

    /**
     * @brief Voiceをすべて移動する
     * @param src 移動元キュー
     * @param dst 移動先キュー
     * @details srcキューの内容をすべてdstキューに移動する
     */
    void moveAllVoices(std::list<Voice*>& src, std::list<Voice*>& dst);

public:
    /**
     * @brief コンストラクタ
     * @param no MIDI Channel No.
     */
    NoteChannel(int no);
    NoteChannel() = delete;

    virtual ~NoteChannel();

    /**
     * @brief MIDIチャンネルのリセット
     */
    void Reset() override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceのうち未使用のものを解放する
     * @return 解放したVoiceへのポインタ
     * @details 未使用のVoiceがない場合はnullptrを返す
     */
    Voice* Release(int mid, bool type) override;

    /**
     * @brief 当該チャンネルに割り当てられたVoiceをすべて解放する
     */
    void ReleaseAll() override;

    /**
     * @brief CC#32 Bank select LSB
     * @details LSBをセットすると同時にProgramに反映する。
     *          発音に反映されるのは次回のNoteOnから。
     *          MSB=3でCSM Voice modeに切り替える。
     */
    void BankSelect_LSB(uint8_t val) override;

    /**
     * @brief MIDI Program No.をセット
     * @param no MIDI Program No.
     * @details MIDI channelからbk_programに変換する
     *          Bank MSB bit 31-24
     *          Bank LSB bit 23-16
     *          Program  bit 15- 0
     */
    void SetProgram(uint8_t no) override;

    /**
     * @brief MIDI Volumeをセット
     * @param vol MIDI Volume
     * @details 現在発音中のVoiceのVolumeをリアルタイムに変更する
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
     * @brief Hold1 (damper pedal)
     * @param val ダンパー値
     */
    virtual void Hold1(int val) override;

    /**
     * @brief CC#6 Data entry MSB
     * @param   val Data Entry MSB
     */
    virtual void DataEntry_MSB(uint8_t val) override;

    /**
     * @brief PitchBend
     * @param val PitchBend value (-8192 - 8191)
     * @details pbv/pbsに応じたピッチ変更をNote ONのVoiceに即座に適用する
     */
    virtual void PitchBend(int16_t val) override;

    /**
     * @brief CC#1 Modulation
     * @param val Modulation depth
     * @details モジュレーションをNote ONのVoiceに即座に適用する
     */
    virtual void SetModulation(uint8_t val) override;

    /**
     * @brief CC#10 Pan
     * @param val Pan value
     * @details パンを設定をNote ONのVoiceに即座に適用する。
     * ハード的な制約で連続表現できないため、valを以下のように分割する。
     *    0 - 41 : L
     *   43 - 83 : LR
     *   85 -127 : R
     */
    virtual void SetPan(uint8_t val) override;

    // Debug
    void dump() override;
};

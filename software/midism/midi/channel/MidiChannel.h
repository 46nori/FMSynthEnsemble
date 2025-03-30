//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <cstdint>

#include "Voice.h"

/**
 * @brief MidiChannel class
 */
class MidiChannel {
public:
    enum Output {
        L  = 0x80,  // Left
        R  = 0x40,  // Right
        LR = 0xc0,  // Left + Right
    };

protected:
    // Channel parameters
    const int channel;   // MIDI Channel No.
    int32_t bk_program;  // Bank/Program No.
    int volume;          // Volume
    int pan;             // Pan
    uint8_t outputLR;    // Output L/R
    VoiceEffect effect;

    // CC parameters
    bool hold1;  // CC#64 Hold1 (dumper pedal)
    // PRN control
    uint8_t rpn_msb;  // CC#101 RPN MSB
    uint8_t rpn_lsb;  // CC#100 RPN LSB
    // PRN control
    uint8_t nrpn_msb;  // CC#99 NRPN MSB
    uint8_t nrpn_lsb;  // CC#98 NRPN LSB
    // Bank select
    uint8_t bk_msb;  // CC#0 Bank select MSB

    // Debug
    int rel_success_count;  // DEBUG
    int rel_fail_count;     // DEBUG

public:
    /**
     * @brief コンストラクタ
     * @param no MIDI Channel No.
     * @details 
     * bk_program: Voiceのデフォルト音色(0)に合わせる
     * volume    : 0にするとMIDI Volume設定されるまで音が出ない。
     * 　　　　　   -1にして音色のデフォルトTotal Levelを維持させる。
     * pbs       : デフォルトは2
     */
    MidiChannel(int no);
    virtual ~MidiChannel();

    /**
     * @brief MIDIチャンネルのリセット
     */
    virtual void Reset();

    /**
     * @brief MIDI Channel No.を返す
     * @return MIDI Channel No.
     */
    int GetNumber();

    /**
     * @brief Voiceの出力先(L/R/LR)をセット
     * @return MIDI Channel No.
     */
    void SetOutputLR(uint8_t lr);

    /**
     * @brief CC#0 Bank select MSB
     */
    void BankSelect_MSB(uint8_t val);

    /**
     * @brief CC#32 Bank select LSB
     */
    virtual void BankSelect_LSB(uint8_t val);

    /**
     * @brief MIDI Program No.をセット
     * @param no MIDI Program No. (0-127)
     * @details MIDI channelからbk_programに変換する
     * Bank MSB bit 31-24
     * Bank LSB bit 23-16
     * Program  bit 15- 0
     */
    virtual void SetProgram(uint8_t no);

    /**
     * @brief MIDI Program No.を返す
     * @return MIDI Program No.
     */
    virtual uint32_t GetProgram();

    /**
     * @brief MIDI Volumeをセット
     * @param vol MIDI Volume
     */
    virtual void SetVolume(int vol);

    /**
     * @brief MIDI Note On
     * @param key MIDI Note No.
     * @param velocity MIDI Velocity
     * @return -1:Fail, 0:NoteOff, 1:NoteOn
     */
    virtual int NoteOn(int key, int velocity) = 0;

    /**
     * @brief MIDI Note Off
     * @param key MIDI Note No.
     * @return -1:Fail, 0:NoteOff, 1:Keep NoteOn
     */
    virtual int NoteOff(int key) = 0;

    /**
     * @brief CC#64 Hold1 (damper pedal)処理を行う
     * @param val ダンパー値 ON(val>=64)/OFF(val<64)
     * @details ダンパーペダルのON/OFFを受け取り、チャンネル内の状態を更新する。
     */
    virtual void Hold1(int val);

    /**
     * @brief PitchBend
     * @param val -8192 to 8191
     */
    virtual void PitchBend(int16_t val);

    /**
     * @brief CC#99 NRPN MSB
     */
    virtual void NRPN_MSB(uint8_t val);

    /**
     * @brief CC#98 NRPN LSB
     */
    virtual void NRPN_LSB(uint8_t val);

    /**
     * @brief CC#101 RPN MSB
     */
    virtual void RPN_MSB(uint8_t val);

    /**
     * @brief CC#100 RPN LSB
     */
    virtual void RPN_LSB(uint8_t val);

    /**
     * @brief CC#6 Data entry MSB
     */
    virtual void DataEntry_MSB(uint8_t val);

    /**
     * @brief CC#38 Data entry LSB
     */
    virtual void DataEntry_LSB(uint8_t val);

    /**
     * @brief CC#1 Modulation
     */
    virtual void SetModulation(uint8_t val);

    /**
     * @brief CC#10 Pan
     */
    virtual void SetPan(uint8_t val);

    // Debug
    virtual void dump();
    virtual void stats();
};

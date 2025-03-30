//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include "OpnBase.h"
#include "Voice.h"

/**
 * @brief Voice class
 */
class NoteVoice : public Voice {
private:
    OpnBase& module;      // FM module
    const uint8_t fm_ch;  // FM moduleのChannel No.
    int16_t pbv;          // PitchBend値

public:
    /**
     * @brief コンストラクタ
     * @param module FM音源モジュール
     * @param ch     FM音源モジュールのチャンネル番号
     * @param id     Voice ID (for debug)
     */
    NoteVoice(OpnBase& module, uint8_t ch, int id);
    NoteVoice() = delete;

    /**
     * @brief デストラクタ
     */
    virtual ~NoteVoice();

    /**
     * @brief Voice内部状態をリセットする
     */
    void Reset() override;

    /**
     * @brief Module IDを返す
     * @return Module ID
     */
    int GetModuleId() override;

    /**
     * @brief MIDI Program(音色)のセット
     * @param no MIDI Program No. (0-127)
     * @details 現在のProgram値から更新された場合に限り、音色パラメータをセットする
     */
    void SetProgram(int32_t no) override;

    /**
     * @brief MIDI Volumeのセット
     * @param vol MIDI Volume (0-127)
     * @details 現在のVolume値から更新された場合に限り、音量をセットする
     */
    void SetVolume(int vol) override;

    /**
     * @brief Note On
     * @param note       MIDI Note No.
     * @param bk_program MIDI Bank/Program No.
     * @param volume     MIDI Volume (0-127)
     * @param effect     Voice effect
     * @param lr         Output Both(0xc0), Left(0x80), Right(0x40)
     * @details FM音源の発音を開始する。
     *          effectの設定値により、PitchBendやModulationを設定する
     */
    void NoteOn(int note, int32_t bk_program, int volume, VoiceEffect& effect, uint8_t lr) override;

    /**
     * @brief Note Off
     * @details Note FM音源の発音を停止する
     */
    void NoteOff() override;

    /**
     * @brief 現在のkeyを基準にPitchを設定する
     * @param effect Voice effect
     *               effect.pbv PitchBend値 (-8192-8191)
     *               effect.pbs PitchBend Sensitivity (0-127)
     * @details PitchBendを指定しない場合はeffect.pbv=0とする
     */
    void SetPitch(VoiceEffect& e) override;

    /**
     * @brief Modulation
     * @param effect Voice effect
     * @param lr     Output Both(0xc0), Left(0x80), Right(0x40)
     * @details effect.vdepth  PMS(Phase modulation Sensitivity) (0-127)
     *          effect.vbrate  LFO frequency (0-127)
     */
    void SetModulation(VoiceEffect& effect, uint8_t lr) override;

    // Debug
    void dump() override;
};

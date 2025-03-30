//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <cstdint>

/**
 * @brief RPN/NRPN設定管理
 */
class VoiceEffect {
public:
    // PitchBend
    int16_t pbv;  // CC#14 PitchBend (-8192-8191)
    uint8_t pbs;  // PitchBend Sensitivity (0-127)
    // Vibrato
    uint8_t vbrate;   // Vibrato rate  (LFO Frequency)
    uint8_t vbdepth;  // Vibrato depth (LFO AMS)
    // Tuning
    int8_t coarse_tune;  // Coarse tuning

    VoiceEffect() : pbv(0), pbs(2), vbrate(0), vbdepth(0), coarse_tune(0) {}

    void Init() {
        pbv         = 0;
        pbs         = 2;  // デフォルト +/-2半音
        vbrate      = 0;
        vbdepth     = 0;
        coarse_tune = 0;
    }
};

/**
 * @brief Voice class
 */
class Voice {
private:
    int note_on_count;  // NoteOn回数 (Keyオーバーラップ時のカウント用)
    const bool type;    // true:CsmVoice, false:NoteVoice
    int midi_ch;        // 属しているMIDI Channel No.

protected:
    int32_t bk_program;  // Bank/Program No.
                         //  Bank MSB: bit 31-24 (0-127)
                         //  Bank LSB: bit 23-16 (0-127)
                         //  Program : bit 15- 0 (0-127)
    int volume;          // MIDI Volume (0:min - 127:max)
    int key;             // Note No. (0-127)

public:
    const int id;  // デバッグ用

public:
    /**
     * @brief コンストラクタ
     * @param type   Voice種別 true:CsmVoice, false:NoteVoice
     * @param id     Voice ID (for debug)
     */
    Voice(bool type, int id);
    Voice() = delete;

    /**
     * @brief デストラクタ
     */
    virtual ~Voice();

    /**
     * @brief Voice内部状態をリセットする
     */
    virtual void Reset();

    /**
     * @brief Voice種別を返す
     * @return true:CsmVoice, false:NoteVoice
     */
    bool GetType();

    /**
     * @brief Voiceが未割り当てかどうかを返す
     * @return true:未割り当て, false:割り当て済み
     */
    bool IsFree();

    /**
     * @brief Voiceの割り当て先MIDIチャンネルを返す
     * @return MIDI Channel No.
     */
    int GetChannel();

    /**
     * @brief Voiceを指定したMIDIチャンネルに割り当てる
     * @param channel MIDI Channel No.
     */
    void SetChannel(int channel);

    /**
     * @brief 現在のNote番号を返す
     * @return MIDI Note No.
     */

    int GetKey();
    /**
     * @brief Note On のリファレンスカウンタのセット
     * @details Note Offされずに同一NoteのNote Onが発生した場合の管理用カウンタ
     */
    void SetNoteOnCount(int val);

    /**
     * @brief Note On のリファレンスカウンタの取得
     * @return リファレンスカウンタ
     * @details Note Offされずに同一NoteのNote Onが発生した場合の管理用カウンタ
     */
    int GetNoteOnCount();

    /**
     * @brief Note On のリファレンスカウンタのインクリメント
     * @return リファレンスカウンタ
     * @details Note Offされずに同一NoteのNote Onが発生した場合の管理用カウンタ
     */
    int IncrementNoteOnCount();

    /**
     * @brief Note On のリファレンスカウンタのデクリメント
     * @return リファレンスカウンタ
     * @details Note Offされずに同一NoteのNote Onが発生した場合の管理用カウンタ
     */
    int DecrementNoteOnCount();

    /**
     * @brief Module IDを返す
     * @return Module ID
     */
    virtual int GetModuleId() = 0;

    /**
     * @brief MIDI Program(音色)のセット
     * @param no MIDI Program No. (0-127)
     * @details 現在のProgram値から更新された場合に限り、音色パラメータをセットする
     */
    virtual void SetProgram(int32_t no) = 0;

    /**
     * @brief MIDI Volumeのセット
     * @param vol MIDI Volume (0-127)
     * @details 現在のVolume値から更新された場合に限り、音量をセットする
     */
    virtual void SetVolume(int vol) = 0;

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
    virtual void NoteOn(int note, int32_t bk_program, int volume, VoiceEffect& effect,
                        uint8_t lr) = 0;
    /**
     * @brief Note Off
     * @details Note FM音源の発音を停止する
     */
    virtual void NoteOff() = 0;

    /**
     * @brief 現在のkeyを基準にPitchを設定する
     * @param effect Voice effect
     *               effect.pbv PitchBend値 (-8192-8191)
     *               effect.pbs PitchBend Sensitivity (0-127)
     * @details PitchBendを指定しない場合はeffect.pbv=0とする
     */
    virtual void SetPitch(VoiceEffect& e) = 0;

    /**
     * @brief Modulation
     * @param effect Voice effect
     * @param lr     Output Both(0xc0), Left(0x80), Right(0x40)
     * @details effect.vdepth  PMS(Phase modulation Sensitivity) (0-127)
     *          effect.vbrate  LFO frequency (0-127)
     */
    virtual void SetModulation(VoiceEffect& effect, uint8_t lr) = 0;

    virtual void dump();
};

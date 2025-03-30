//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <array>
#include <cstdint>

#include "OpnBase.h"
#include "Voice.h"

// 音声データ
#include "csm/VOICE.dat"

class CsmVoice : public Voice {
private:
    std::array<OpnBase*, 4>& modules;
    int operators;  // 使用するオペレータ数
    int docks;      // 使用するFM音源モジュール数
    int modTB;      // Timer Bを使用するFM音源モジュール

    int frame;         // 現在再生中のフレーム
    int interp_count;  // 現在の補間回数
    int lastFrame;     // 最終フレーム
    bool isLastFrame;  // 最終フレームフラグ

    struct frame_format data;  // CSM音声データ
    struct diff_format {
        int16_t Pitch;
        int16_t TL[CSM_N];
        int16_t Freq[CSM_N];
    };
    struct diff_format diff;  // フレーム補完用データ

public:
    /**
     * @brief コンストラクタ
     * @param modules   FM音源モジュールのリスト
     * @param id        Voice ID (for debug)
     */
    CsmVoice(std::array<OpnBase*, 4>& modules, int id);
    CsmVoice() = delete;

    /**
     * @brief デストラクタ
     */
    virtual ~CsmVoice();

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
     * @param no MIDI Bank/Program No.
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
     * @param note    MIDI Note No.
     * @param program MIDI Bank/Program No.
     * @param volume  MIDI Volume (0-127)
     * @param effect  Voice effect
     * @param lr      Output Both(0xc0), Left(0x80), Right(0x40)
     * @details FM音源の発音を開始する。
     *          effectの設定値により、PitchBendやModulationを設定する
     */
    void NoteOn(int note, int32_t program, int volume, VoiceEffect& effect, uint8_t lr) override;

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

    /**
     * @brief CSMモードの動作設定を行う
     * @param bInterrupt  true:割り込み駆動, false:ポーリング
     */
    void Init(bool bInterrupt);

    /**
     * @brief 再生開始とフレームの更新処理
     * @param isFirst true:再生開始, false:フレーム更新(2回目以降)
     * @details 2回目以降はTimer Bのオーバーフローごとに呼び出す。
     */
    void UpdateFrame(bool isFirst);

    /** 
     * @brief フレームオーバーの検出
     * @return true:フレームオーバー
     * @details Timer Bのオーバーフローをチェックする
     */
    bool IsFrameOver();

    /**
     * @brief 全てのTimer A/BとCH3の発音を停止
     */
    void Stop();

    // Debug
    void dump() override;

private:
    /**
     *  @brief CH3の初期化
     */
    void init_ch3(OpnBase& opn);

    /** 
     * @brief フレームパラメータの更新
     * @param isFirst true:最初のフレームから再生する
     * @return true:最終フレームを検出した
     * @details 最終フレームなら、次回のTimer Bのオーバーフローで再生を終了する
     */
    bool update(bool isFirst);
};

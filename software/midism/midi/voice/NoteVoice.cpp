//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "NoteVoice.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "Debugger.h"

/**
 * @brief  OPN TL(Total Level)音量テーブル
 * @details MIDI volume(x:0-127)をTL(y:127-0)に変換する。
 *          y = 127 - 60.27*log(x+1)
 */
static constexpr uint8_t opn_volume[128] = {
    127, 109, 98, 91, 85, 80, 76, 73, 69, 67, 64, 62, 60, 58, 56, 54, 53, 51, 50, 49, 47, 46,
    45,  44,  43, 42, 41, 40, 39, 38, 37, 36, 35, 35, 34, 33, 32, 32, 31, 30, 30, 29, 29, 28,
    27,  27,  26, 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19, 19, 18, 18, 17,
    17,  17,  16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10,
    10,  9,   9,  9,  8,  8,  8,  8,  7,  7,  7,  6,  6,  6,  6,  5,  5,  5,  5,  4,  4,  4,
    4,   3,   3,  3,  3,  3,  2,  2,  2,  2,  1,  1,  1,  1,  1,  0,  0,  0};

/**
 * @brief 現在のkeyを基準にPitchを設定する
 * @param pbv PitchBend値 (-8192-8191)
 * @param pbs PitchBend Sensitivity (0-127)
 * @details PitchBendを指定しない場合はpbv=0とする
 *  以下の理由でkey == -1のチェックは不要
 *  - NoteOn()からは、keyがセットされた後で呼ばれる。
 *  - NoteChannelからは、KeyOnのVoiceとして呼ばれる。
 */
static constexpr int PBS_MARGIN    = 2;
static constexpr uint16_t fnum[16] = {
    // pbs <= 2の場合の参照用
    0x0226,  // A# (-2)
    0x0247,  // B  (-1)
    // 通常使用する1オクターブ分のF-Number
    // OpnBase.hのfm_pitch_table[]と同じ
    0x0269,  // C
    0x028e,  // C#
    0x02b4,  // D
    0x02de,  // D#
    0x0309,  // E
    0x0338,  // F
    0x0369,  // F#
    0x039c,  // G
    0x03d3,  // G#
    0x040e,  // A
    0x044b,  // A#
    0x048d,  // B
    // pbs <= 2の場合の参照用
    0x04d3,  // C  (+1)
    0x051c,  // C# (+2)
};

NoteVoice::NoteVoice(OpnBase& module, uint8_t ch, int id)
    : Voice(false, id),  // NoteType
      module(module),
      fm_ch(ch) {
    SetProgram(0);   // デフォルト音色
    SetVolume(100);  // デフォルト音量
}

NoteVoice::~NoteVoice() {
}

void NoteVoice::Reset() {
    // コンストラクタと同じ設定にする
    // 外部キーボードから使用するときなどのために音色をデフォルトに戻しておく
    Voice::Reset();
    SetProgram(0);
    SetVolume(100);
    pbv = 0;
}

int NoteVoice::GetModuleId() {
    return module.id;
}

void NoteVoice::SetProgram(int32_t no) {
    if (bk_program != no) {
        module.fm_set_tone(fm_ch, no & 0xff);
        bk_program = no;
        DPRINTF(2, " P%04x:%d ", no >> 16, no & 0xff);
    }
}

void NoteVoice::SetVolume(int vol) {
    if (volume != vol) {
        module.fm_set_volume(fm_ch, bk_program, opn_volume[vol]);
        volume = vol;
    }
}

void NoteVoice::NoteOn(int note, int32_t bk_program, int volume, VoiceEffect& effect, uint8_t lr) {
    SetProgram(bk_program);
    SetVolume(volume);  // must be after SetProgram()
    if (note != key || effect.pbv != pbv) {
        key = note;
        SetPitch(effect);
    }
    module.fm_turnon_key(fm_ch);
    SetModulation(effect, lr);
    IncrementNoteOnCount();
}

void NoteVoice::NoteOff() {
    module.fm_turnoff_key(fm_ch);
    SetNoteOnCount(0);
}

void NoteVoice::SetPitch(VoiceEffect& effect) {
    //  以下の理由でkey == -1のチェックは不要
    //  - NoteOn()からは、keyがセットされた後で呼ばれる。
    //  - NoteChannelからは、KeyOnのVoiceとして呼ばれる。

#if ENABLE_COARSE_TUNE == 1
    int16_t key = Voice::key - effect.coarse_tune;
#endif
    uint8_t pbs = effect.pbs;
    pbv         = effect.pbv;

    //
    // PitchBendなし
    //
    if (pbs == 0 || pbv == 0) {
        if (key < 12) {
            module.fm_set_pitch(fm_ch, 0, 0, 0);
        } else if (key > 107) {
            // PitchBend計算用のマージンを使ってkey=108までサポート
            module.fm_set_pitch(fm_ch, 11, 7, fnum[14] - fnum[13]);
        } else {
#if 1
            module.fm_set_pitch(fm_ch, key % 12, key / 12 - 1, 0);
#else
            div_t k = div(key, 12);
            module.fm_set_pitch(fm_ch, k.rem, k.quot - 1, 0);
#endif
        }
        return;
    }

    //
    // PitchBendあり
    //
    int8_t oct, k;
    int16_t pbkey, diff;

    if (pbs <= PBS_MARGIN) {
        // 現在のNote番号基準
        pbkey = key;
    } else {
        // PitchBend位置に最も近いNote番号基準
        pbkey = (int32_t)pbv * pbs / 8191 + key;
    }

    //   k : 基準NoteのPitch Tableのインデックス
    // oct : 基準Noteのオクターブ(Block Number)
    if (pbkey < 12) {
        k   = 2;
        oct = 0;
    } else if (pbkey > 107) {
        k   = 11 + PBS_MARGIN;
        oct = 7;
    } else {
#if 1
        k   = pbkey % 12 + PBS_MARGIN;
        oct = pbkey / 12 - 1;
#else
        div_t pbk = div(pbkey, 12);
        k         = pbk.rem + PBS_MARGIN;
        oct       = pbk.quot - 1;
#endif
    }

    // diff : PitchBend位置での、基準NoteからのF-Numberの偏差
    if (pbs <= PBS_MARGIN) {
        // pbs=2(デフォルト)以下の場合(特別処理)
        //   現在のNote基準に+/-2半音までのF-Numberを線形補完
        if (pbv > 0) {
            diff = (int32_t)(fnum[k + pbs] - fnum[k]) * pbv / 8191;
        } else {
            diff = (int32_t)(fnum[k] - fnum[k - pbs]) * pbv / 8192;
        }
    } else {
        // pbs>2の場合(汎用処理)
        //   pbkeyを基準Noteに+/-1半音内のF-Numberを線形補完
        // key   pbkey        pbkey+1 : 0-127
        //  v      v            v
        //  |--..--+------------+-------->| 8191
        //  |--..--------->| pbv      : pb値: -8192 - +8191
        //         |------>| a        : pbkeyからの偏差
        //         |----------->| b   : 1半音あたりのpb値
        //  |--..--|-------o----|--
        //        fnum[k]    fnum[k+1]: F-Number
        //         |------>| diff     : fnum[k]からの偏差(求める値)
        //
        //      b = 8191/pbs
        //      a = pbv % b
        // diff = (fnum[k+1] - fnum[k]) * a/b
        int m     = 8191 / pbs;
        float a_b = (float)(pbv % m) / m;
        if (pbv > 0) {
            diff = (fnum[k + 1] - fnum[k]) * a_b;
        } else {
            diff = (fnum[k] - fnum[k - 1]) * a_b;
        }
    }

    module.fm_set_pitch(fm_ch, k - PBS_MARGIN, oct, diff);
    DPRINTF(1, " PB k=%d, diff=%d ", pbkey, diff);
}

void NoteVoice::SetModulation(VoiceEffect& effect, uint8_t lr) {
    if (effect.vbdepth == 0) {
        module.fm_set_output_lr(fm_ch, lr);
        module.fm_turnoff_LFO();
    } else {
        module.fm_set_LFO_PMS(fm_ch, effect.vbdepth >> 4, lr);
        module.fm_turnon_LFO(effect.vbrate >> 4);
    }
}

// Debug
void NoteVoice::dump() {
    printf("ID=%02d CH=%02d PG=%04x %04x VOL=%3d KEY=%3d TYPE=%s OPN=%d-%d\n", id, GetChannel(),
           bk_program >> 16, bk_program & 0xffff, volume, GetKey(), GetType() ? "CSM " : "Note",
           module.id, fm_ch);
}

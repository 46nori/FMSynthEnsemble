//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "CsmVoice.h"

#include <cstring>

#include "Debugger.h"
#include "RP2040.h"

//#define ENABLE_INTERPOLATION
#ifdef ENABLE_INTERPOLATION
constexpr int INTERPOLATE = 4;
#endif

struct VoiceElement {
    int start;
    int length;
} ve[]{
    {  3, 27}, // T
    { 27, 27}, // E
    { 50, 27}, // C
    { 78, 27}, // H
    { 95, 27}, // N
    {130, 27}, // O
    {158, 25}, // P
    {182, 27}, // O
    {210, 24}, // L
    {229, 27}, // I
    {256, 27}, // S
    {340, 48}, // TOKIO
};
static constexpr int num_of_voice = sizeof(ve) / sizeof(struct VoiceElement);

CsmVoice::CsmVoice(std::array<OpnBase*, 4>& modules, int id)
    : Voice(true, id),  // CSM type
      modules(modules),
      docks(0),
      operators(0),
      modTB(0),
      frame(0),
      interp_count(0),
      lastFrame(0),
      isLastFrame(false) {
    SetProgram(0);   // デフォルト音色
    SetVolume(100);  // デフォルト音量
}

CsmVoice::~CsmVoice() {
}

void CsmVoice::Reset() {
    // コンストラクタと同じ設定にする
    Voice::Reset();
    SetProgram(0);
    SetVolume(100);
    frame        = 0;
    interp_count = 0;
    lastFrame    = 0;
    isLastFrame  = false;
}

int CsmVoice::GetModuleId() {
    // NoteVoiceと動作を合わせるために、便宜的にModule IDを返す
    return modules[0]->id;
}

void CsmVoice::SetProgram(int32_t no) {
    bk_program = no;
}

void CsmVoice::SetVolume(int vol) {
    volume = vol;
}

void CsmVoice::NoteOn(int note, int32_t bk_program, int volume, VoiceEffect& effect, uint8_t lr) {
    key = note;
    SetProgram(bk_program);
    SetVolume(volume);  // must be after SetProgram()
    UpdateFrame(true);
    SetModulation(effect, lr);
    IncrementNoteOnCount();
}

void CsmVoice::NoteOff() {
    SetNoteOnCount(0);
}

void CsmVoice::SetPitch(VoiceEffect& e) {
    // ピッチベンドは非対応
}

void CsmVoice::SetModulation(VoiceEffect& effect, uint8_t lr) {
    // パン設定だけ行う
    // CH3のみにLFOをかけられないのでModulationには対応しない
    for (OpnBase* opn : modules) {
        if (opn) {
            opn->fm_set_output_lr(2, lr);  // LR出力設定
        }
    }
}

void CsmVoice::Init(bool bInterrupt) {
    // 使用するFM音源モジュール数
    docks = ((CSM_N % 4 == 0) ? CSM_N / 4 : CSM_N / 4 + 1);

    // 実際に使用するオペレータ数 (<= CSM_N)
    operators = CSM_N;
    if (operators > CSM_N) operators = CSM_N;
    if (operators < 4) operators = 4;

    // 使用するFM音源モジュールのCH3を初期化
    for (OpnBase* opn : modules) {
        if (opn) {
            init_ch3(*opn);
        }
    }

    // フレーム周期をTimer Bに設定
#ifdef ENABLE_INTERPOLATION
    constexpr uint8_t nb = (uint8_t)(256 - 125 * (float)FRAME_PERIOD / (INTERPOLATE * 36));
#else
    constexpr uint8_t nb = (uint8_t)(256 - 125 * (float)FRAME_PERIOD / 36);
#endif
    modules[modTB]->set_timer_b(nb);

    // 割り込み駆動の場合
    if (bInterrupt) {
        // 割り込み信号の接続先GPIOを選択
        uint8_t gpio = FM_IRQ0;
        switch (modTB) {
        case 1:
            gpio = FM_IRQ1;
            break;
        case 2:
            gpio = FM_IRQ2;
            break;
        case 3:
            gpio = FM_IRQ3;
            break;
        default:
            break;
        }
        // コールバック処理をlambda式で登録
        attach_isr_callback(gpio, [this]() { UpdateFrame(false); });
    }
}

void CsmVoice::UpdateFrame(bool isFirst) {
    if (isFirst) {
        update(true);
    } else if (isLastFrame) {
        // 最終フレームなのでTimerB割り込み信号の発生を停止
        modules[modTB]->set_timer_mode(0x30);
        isLastFrame = false;
        frame       = 0;
    } else {
        // 2回目以降の更新処理
        update(false);
    }
}

bool CsmVoice::IsFrameOver() {
    return modules[modTB]->read_status() & 0x02;
}

void CsmVoice::Stop() {
    for (OpnBase* opn : modules) {
        if (opn) {
            opn->fm_turnoff_key(2);
            opn->set_timer_mode(0x30);
        }
    }
}

void CsmVoice::init_ch3(OpnBase& opn) {
    constexpr int ch3 = 2;  // CH3

    // CH3のKey ON時のエンベロープパターン
    constexpr OpnBase::fm_env env = {
        0x00,  // KS
        0x1f,  // AR max
        0x1f,  // DR max
        0x1f,  // SR max
        0x00,  // SL = 0 makes no DR effect
        0x0a,  // RR
    };

    opn.set_timer_mode(0x30);  // Timer A/B をリセット

    opn.set_fmch3_mode(2);                          // CH3をCSMモードにセット
    opn.fm_turnoff_key(ch3);                        // CH3の全オペレータをOFF
    opn.fm_set_algorithm(ch3, 0, 7);                // Feedback=0, Algorithm=7
    for (int op = 0; op < 4; op++) {                // Init Operaters
        opn.fm_set_detune_multiple(ch3, op, 0, 1);  // Detune=0, Multiple=1
        opn.fm_set_total_level(ch3, op, 0x7f);      // Total Level (-96dB)
        opn.fm_set_envelope(ch3, op, env);          // Envelope
        opn.fm_set_fnumber_ch3(op, 0, 0);           // Block/F-Number
    }
    opn.fm_set_output_lr(ch3, 0xc0);  // LR両方に出力(YM2608の場合)
}

bool CsmVoice::update(bool isFirst) {
    // 初回呼び出し処理
    if (isFirst) {
        interp_count = 0;
        frame        = ve[key % num_of_voice].start;
        lastFrame    = frame + ve[key % num_of_voice].length;
        isLastFrame  = false;
        data         = (struct frame_format){0};
    }

    uint16_t pitch = frame_data[frame].Pitch;
#if 1
    if (frame == lastFrame) {
        pitch &= 0x7fff;
        isLastFrame = true;
    }
#else
    // Pitchの最上位ビットが立っている場合は最終フレーム
    if (pitch & 0x8000) {
        pitch &= 0x7fff;
        isLastFrame = true;
    }
#endif

#ifdef ENABLE_INTERPOLATION
    if (interp_count == 0) {
        // 差分の計算
        diff.Pitch = (pitch - data.Pitch) / INTERPOLATE;
        for (int j = 0; j < operators; j++) {
            diff.TL[j]   = (frame_data[frame].TL[j] - data.TL[j]) / INTERPOLATE;
            diff.Freq[j] = (frame_data[frame].Freq[j] - data.Freq[j]) / INTERPOLATE;
        }
    }

    // 線形補間
    data.Pitch += diff.Pitch;
    for (int j = 0; j < operators; j++) {
        data.TL[j] += diff.TL[j];
        data.Freq[j] += diff.Freq[j];
    }
#endif

    // フレームパラメータの更新
    for (int d = 0; d < docks; d++) {
#ifdef ENABLE_INTERPOLATION
        modules[d]->set_timer_a(data.Pitch);  // ピッチ (Timer A)
#else
        modules[d]->set_timer_a(pitch);  // ピッチ (Timer A)
#endif
        for (uint8_t op = 0; op < 4; op++) {
            int n = d * 4 + op;
            if (n < operators) {
#ifdef ENABLE_INTERPOLATION
                uint8_t tl = data.TL[n];
#else
                uint8_t tl = frame_data[frame].TL[n];
#endif
                modules[d]->fm_set_total_level(2, op, tl);  // 振幅

                uint16_t fnum = 0;
                for (int blk = 5; blk >= 0; blk--) {
                    // f = 72 * csm_f * (2**20) / 4000000 / (2 ** (blk - 1))
#ifdef ENABLE_INTERPOLATION
                    uint32_t f = ((uint32_t)38 * data.Freq[n]) >> blk;
#else
                    uint32_t f = ((uint32_t)38 * frame_data[frame].Freq[n]) >> blk;
#endif
                    if (f < 2048) {
                        fnum = ((blk << 11) | f) & 0x3fff;
                        break;
                    }
                }
                modules[d]->fm_set_fnumber_ch3(op, fnum >> 8, fnum & 0xff);  // 周波数
            }
        }
    }

    // Timer B(フレーム)とTimer A(ピッチ)をスタート
    for (int d = 0; d < docks; d++) {
        if (modules[d] != nullptr) {
            if (d == modTB) {
                modules[d]->set_timer_mode(0x2b);  // Timer A & B
            } else {
                modules[d]->set_timer_mode(0x01);  // Timer A
            }
        }
    }

#ifdef ENABLE_INTERPOLATION
    if (interp_count++ == INTERPOLATE) {
        interp_count = 0;
        frame++;
    }
    return isLastFrame;
#else
    frame++;
    return isLastFrame;
#endif
}

// Debug
void CsmVoice::dump() {
    printf("ID=%02d CH=%02d PG=%04x %04x VOL=%3d KEY=%3d TYPE=%s\n", id, GetChannel(),
           bk_program >> 16, bk_program & 0xffff, volume, GetKey(), GetType() ? "CSM " : "Note");
}

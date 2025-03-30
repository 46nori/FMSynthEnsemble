//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "RhythmChannel.h"

#include "Debugger.h"

//
// GM Percussion map
//
static constexpr YM2608::RtmInst percussion_map[54] = {
    // GM1
    YM2608::RtmInst::BD,    // #35(B0)   アコースティック・バスドラム(Acoustic Bass Drum)
    YM2608::RtmInst::BD,    // #36(C1)   バスドラム1(Bass Drum 1)
    YM2608::RtmInst::RIM,   // #37(C#1)  サイドスティック(Side Stick)
    YM2608::RtmInst::SD,    // #38(D1)   アコースティック・スネア(Acoustic Snare)
    YM2608::RtmInst::RIM,   // N/A #39(D#1)  手拍子(Hand Clap)
    YM2608::RtmInst::SD,    // #40(E1)   エレクトリック・スネア(Electric Snare)
    YM2608::RtmInst::TOM,   // #41(F1)   ロー・フロア・タム(Low Floor Tom)
    YM2608::RtmInst::HH,    // #42(F#1)  クローズド・ハイハット(Closed Hi-hat)
    YM2608::RtmInst::TOM,   // #43(G1)   ハイ・フロア・タム(High Floor Tom)
    YM2608::RtmInst::HH,    // #44(G#1)  ペダル・ハイハット(Pedal Hi-hat)
    YM2608::RtmInst::TOM,   // #45(A1)   ロー・タム(Low Tom)
    YM2608::RtmInst::HH,    // #46(A#1)  オープン・ハイハット(Open Hi-hat)
    YM2608::RtmInst::TOM,   // #47(B1)   ロー・ミッド・タム(Low-Mid Tom)
    YM2608::RtmInst::TOM,   // #48(C2)   ハイ・ミッド・タム(High Mid Tom)
    YM2608::RtmInst::TOP,   // #49(C#2)  クラッシュ・シンバル1(Crash Cymbal 1)
    YM2608::RtmInst::TOM,   // #50(D2)   ハイ・タム(High Tom)
    YM2608::RtmInst::TOP,   // #51(D#2)  ライド・シンバル1(Ride Cymbal 1)
    YM2608::RtmInst::TOP,   // N/A #52(E2)   チャイニーズ・シンバル(Chinese Cymbal)
    YM2608::RtmInst::TOP,   // N/A #53(F2)   ライド・ベル(Ride Bell)
    YM2608::RtmInst::NONE,  // N/A #54(F#2)  タンバリン(Tambourine)
    YM2608::RtmInst::TOP,   // #55(G2)   スプラッシュ・シンバル(Splash Cymbal)
    YM2608::RtmInst::NONE,  // N/A #56(G#2)  カウベル(Cowbell)
    YM2608::RtmInst::TOP,   // #57(A2)   クラッシュ・シンバル2(Crash Cymbal 2)
    YM2608::RtmInst::NONE,  // N/A #58(A#2)  ヴィブラ・スラップ(Vibra-slap)
    YM2608::RtmInst::TOP,   // #59(B2)   ライドシンバル2(Ride Cymbal 2)
    YM2608::RtmInst::TOM,   // #60(C3)   ハイ・ボンゴ(High Bongo)
    YM2608::RtmInst::TOM,   // #61(C#3)  ロー・ボンゴ(Low Bongo)
    YM2608::RtmInst::TOM,   // #62(D3)   ミュート・ハイ・コンガ(Mute Hi Conga)
    YM2608::RtmInst::TOM,   // #63(D#3)  オープン・ハイ・コンガ(Open Hi Conga)
    YM2608::RtmInst::TOM,   // #64(E3)   ロー・コンガ(Low Conga)
    YM2608::RtmInst::TOM,   // #65(F3)   ハイ・ティンバレ(High Timbale)
    YM2608::RtmInst::TOM,   // #66(F#3)  ロー・ティンバレ(Low Timbale)
    YM2608::RtmInst::NONE,  // #67(G3)   ハイ・アゴゴ(High Agogo)
    YM2608::RtmInst::NONE,  // #68(G#3)  ロー・アゴゴ(Low Agogo)
    YM2608::RtmInst::NONE,  // #69(A3)   カバサ(Cabasa)
    YM2608::RtmInst::NONE,  // #70(A#3)  マラカス(Maracas)
    YM2608::RtmInst::NONE,  // #71(B3)   ショート・ホイッスル(Short Whistle)
    YM2608::RtmInst::NONE,  // #72(C4)   ロング・ホイッスル(Long Whistle)
    YM2608::RtmInst::NONE,  // #73(C#4)  ショート・ギロ(Short Guiro)
    YM2608::RtmInst::NONE,  // #74(D4)   ロング・ギロ(Long Guiro)
    YM2608::RtmInst::NONE,  // #75(D#4)  クラベス(Claves)
    YM2608::RtmInst::NONE,  // #76(E4)   ハイ・ウッドブロック(Hi Wood Block)
    YM2608::RtmInst::NONE,  // #77(F4)   ロー・ウッドブロック(Low Wood Block)
    YM2608::RtmInst::NONE,  // #78(F#4)  ミュート・クイーカ(Mute Cuica)
    YM2608::RtmInst::NONE,  // #79(G4)   オープン・クイーカ(Open Cuica)
    YM2608::RtmInst::NONE,  // #80(G#4)  ミュート・トライアングル(Mute Triangle)
    YM2608::RtmInst::NONE,  // #81(A4)   オープン・トライアングル(Open Triangle)
    // GM2
    YM2608::RtmInst::HH,    // #82(A#4)  シェイカー(Shaker)
    YM2608::RtmInst::NONE,  // #83(B4)   ジングルベル(Jingle Bell)
    YM2608::RtmInst::NONE,  // #84(C5)   ベルツリー(Bell Tree)
    YM2608::RtmInst::NONE,  // #85(C#5)  カスタネット(Castanets)
    YM2608::RtmInst::NONE,  // #86(D5)   ミュート・スルド(Mute Surdo)
    YM2608::RtmInst::NONE,  // #87(D#5)  オープン・スルド(Open Surdo)
    YM2608::RtmInst::NONE,  // #88(E5)
};

//  RTL(Rhythm Total Level)音量テーブル
//  MIDI volume(x:0-127)を RTL(y:0-63)に変換する。
//          ｙ = 29.90*log(x+1)
static constexpr uint8_t RTLvolume[128] = {
    0,  9,  14, 18, 21, 23, 25, 27, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 38, 39, 40, 40,
    41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 45, 46, 46, 47, 47, 47, 48, 48, 48, 49, 49, 49,
    49, 50, 50, 50, 51, 51, 51, 51, 52, 52, 52, 52, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54,
    55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58,
    58, 58, 59, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61,
    61, 61, 61, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63,
};

//  IL(Instrument Level)音量テーブル
//  MIDI NoteOn velocity(x:0-127)を IL(y:0-31)に変換する。
//          ｙ = 14.71*log(x+1)
static constexpr uint8_t ILvolume[128] = {
    0,  4,  7,  9,  10, 11, 12, 13, 14, 15, 15, 16, 16, 17, 17, 18, 18, 18, 19, 19, 19, 20,
    20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24,
    24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29,
    29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
};

RhythmChannel::RhythmChannel(OpnBase* module) : MidiChannel(MIDI_RHYTHM_CHANNEL), module(module) {
    SetOutputLR(LR);        // L/R両チャンネル出力に設定
    init_volume(100, 127);  // デフォルト音量
}

RhythmChannel::~RhythmChannel() {
}

void RhythmChannel::Reset() {
    // コンストラクタと同じ設定にする
    MidiChannel::Reset();
    init_volume(100, 127);
}

void RhythmChannel::init_volume(uint8_t rtl, uint8_t il) {
    SetVolume(rtl);
    il = ILvolume[il];
    module->rtm_set_inst_level(YM2608::RtmInst::BD, il);
    module->rtm_set_inst_level(YM2608::RtmInst::SD, il);
    module->rtm_set_inst_level(YM2608::RtmInst::TOP, il);
    module->rtm_set_inst_level(YM2608::RtmInst::HH, il);
    module->rtm_set_inst_level(YM2608::RtmInst::TOM, il);
    module->rtm_set_inst_level(YM2608::RtmInst::RIM, il);
}

void RhythmChannel::SetVolume(int vol) {
    if (volume != vol) {
        volume = vol;
        module->rtm_set_total_level(RTLvolume[vol]);
    }
}

int RhythmChannel::NoteOn(int key, int velocity) {
    int st = 1;
    if (key >= 35 && key < sizeof(percussion_map) / sizeof(YM2608::RtmInst) + 35) {
        YM2608::RtmInst note = percussion_map[key - 35];
        if (note != YM2608::RtmInst::NONE) {
            if (velocity == 0) {
                module->rtm_damp_key(note);
                st = 0;
            } else {
                // velocityに応じた音量変化
                module->rtm_set_inst_level(note, ILvolume[volume]);
                module->rtm_turnon_key(note);
            }
            DPRINTF(1, " *%02d ", key);
            return st;
        }
    }
    DPRINTF(1, " ?%02d ", key);
    return -1;
}

int RhythmChannel::NoteOff(int key) {
    return NoteOn(key, 0);
}

Voice* RhythmChannel::Release(int mid, bool type) {
    return nullptr;
}

void RhythmChannel::ReleaseAll() {
}

// Debug
void RhythmChannel::dump() {
    MidiChannel::dump();
    printf("  TYPE=RTM\n");
}

//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "YM2608.h"

YM2608::YM2608(HAL& hal, float clock, int id) : OpnBase(hal, clock, id) {
}

YM2608::~YM2608() {
}

void YM2608::rtm_turnon_key(int rtm) {
    hal.write(0x10, rtm & 0x3f, 0, WAIT_576);
}

void YM2608::rtm_damp_key(int rtm) {
    hal.write(0x10, rtm | 0x80, 0, WAIT_576);
}

void YM2608::rtm_set_total_level(uint8_t tl) {
    hal.write(0x11, tl, 0, WAIT_83);
}

void YM2608::rtm_set_inst_level(int rtm, uint8_t tl, uint8_t lr) {
    tl = lr | (tl & 0x1f);
    switch (rtm) {
    case BD:
        hal.write(0x18, tl, 0, WAIT_83);
        break;
    case SD:
        hal.write(0x19, tl, 0, WAIT_83);
        break;
    case TOP:
        hal.write(0x1a, tl, 0, WAIT_83);
        break;
    case HH:
        hal.write(0x1b, tl, 0, WAIT_83);
        break;
    case TOM:
        hal.write(0x1c, tl, 0, WAIT_83);
        break;
    case RIM:
        hal.write(0x1d, tl, 0, WAIT_83);
        break;
    }
}

void YM2608::init() {
    // Init CH0-2
    OpnBase::init();

    // OPNA mode, Enable TB IRQ
    hal.write(0x29, 0x82, 0, WAIT_83);

    // Init CH3-5
    for (int ch = 3; ch < 6; ch++) {
        fm_turnoff_key(ch);        // Turn Off Key
        fm_set_fnumber(ch, 0, 0);  // Block/F-Number
        for (int op = 0; op < 4; op++) {
            fm_set_total_level(ch, op, 0x7f);  // Total Level(mute)
        }
    }

    // Mute Rythm volume
    rtm_set_total_level(0x3f);              // RTL(mute)
    rtm_set_inst_level(RtmInst::BD, 0x00);  // IL(mute)
    rtm_set_inst_level(RtmInst::SD, 0x00);
    rtm_set_inst_level(RtmInst::TOP, 0x00);
    rtm_set_inst_level(RtmInst::HH, 0x00);
    rtm_set_inst_level(RtmInst::TOM, 0x00);
    rtm_set_inst_level(RtmInst::RIM, 0x00);
}

void YM2608::fm_turnon_LFO(uint8_t freq) {
    hal.write(0x22, 0x08 | freq & 0x7, 0, WAIT_83);
}

void YM2608::fm_turnoff_LFO() {
    hal.write(0x22, 0x00, 0, WAIT_83);
}

void YM2608::fm_set_LFO_PMS(uint8_t ch, uint8_t pms, uint8_t lr) {
    LFO_pms    = pms & 7;
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    hal.write(0xb4 + ch, lr | LFO_ams | LFO_pms, a1, WAIT_47);
}

void YM2608::fm_set_LFO_AMS(uint8_t ch, uint8_t op, uint8_t ams, uint8_t lr) {
    LFO_ams    = ams & 3;
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    hal.write(0xb4 + ch, lr | LFO_ams | LFO_pms, a1, WAIT_47);
    // TODO: Refer DecayRate from tone table
    //hal.write(0x60 + ch, 0x80 | Decay, 0);
}

void YM2608::fm_set_output_lr(uint8_t ch, uint8_t lr) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    hal.write(0xb4 + ch, lr | LFO_ams | LFO_pms, a1, WAIT_47);
}
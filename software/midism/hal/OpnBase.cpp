//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "OpnBase.h"

OpnBase::OpnBase(HAL& hal, float clock, int id)
    : hal(hal),
      ext_clock(clock),
      prescale(6),  // Prescaler 1/6
      timerA_k(clock / (12.0 * prescale)),
      timerB_k(clock / (192.0 * prescale)),
      WAIT_47(47 * 1000.0 / clock + 1),    // us for  47 wait cycle
      WAIT_83(83 * 1000.0 / clock + 1),    // us for  83 wait cycle
      WAIT_576(576 * 1000.0 / clock + 1),  // us for 576 wait cycle
      ch3_mode(0),                         // Set FM CH3 normal mode
      timer_mode(0),                       // Reset timer settings
      LFO_ams(0),
      LFO_pms(0),
      id(id) {
}

void OpnBase::init() {
    ch3_mode   = 0;  // Set FM CH3 normal mode
    timer_mode = 0;  // Reset timer settings
    LFO_ams    = 0;  // Reset LFO AMS
    LFO_pms    = 0;  // Reset LFO PMS

    hal.write(0x2d, 0x00, 0, WAIT_83);  // Set Prescaler 1/6
    hal.write(0x27, 0x30, 0, WAIT_83);  // Normal mode, Reset Timer and IRQ and flags

    hal.write(0x07, 0xff, 0, 1);  // SSG noise/tone off
    hal.write(0x08, 0x00, 0, 1);  // SSG Channel A volume 0
    hal.write(0x09, 0x00, 0, 1);  // SSG Channel B volume 0
    hal.write(0x0a, 0x00, 0, 1);  // SSG Channel C volume 0

    // default for OPN
    for (int ch = 0; ch < 3; ch++) {
        fm_turnoff_key(ch);        // Turn Off Key
        fm_set_fnumber(ch, 0, 0);  // Block/F-Number
        for (int op = 0; op < 4; op++) {
            fm_set_total_level(ch, op, 0x7f);  // Total Level
        }
    }
}

int OpnBase::fm_get_channels() {
    return 3;  // default for OPN
}

/////////////////////////////////////////////////////////
// FM Synthesis
/////////////////////////////////////////////////////////
void OpnBase::fm_set_algorithm(uint8_t ch, uint8_t fb, uint8_t alg) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    hal.write(0xb0 + ch, (fb & 7) << 3 | alg & 0x07, a1, WAIT_47);
}

void OpnBase::fm_set_tone(uint8_t ch, int no) {
    if (no > sizeof(fm_tone_table) / sizeof(fm_tone_table[0])) {
        no = 0;
    }
    fm_set_tone(ch, &fm_tone_table[no][0]);
}

void OpnBase::fm_set_tone(uint8_t ch, const uint8_t* tone) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    int i;
    uint8_t adrs = 0x30 + ch;
    for (i = 0; i < sizeof(fm_tone_table[0]) - 1; i++) {
        hal.write(adrs, tone[i], a1, WAIT_83);
        adrs += 4;
    }
    hal.write(adrs + 0x10, tone[i], a1, WAIT_83);
}

void OpnBase::fm_set_pitch(uint8_t ch, uint8_t p, uint8_t oct, int16_t diff) {
    if (p <= MAXNUM_FM_PITCH && oct <= MAXNUM_OCT) {
        uint8_t a1 = 0;
        if (ch >= 3) {
            ch -= 3;
            a1 = 1;
        }
        int16_t fnum = fm_pitch_table[p] + diff;
        if (fnum < 0x0000) fnum = 0x000;
        if (fnum > 0x07ff) fnum = 0x7ff;
        // Caution: Keep this order when set the registers
        hal.write(0xa4 + ch, fnum >> 8 | oct << 3, a1, WAIT_47);
        hal.write(0xa0 + ch, fnum & 0xff, a1, WAIT_47);
    }
}

void OpnBase::fm_turnon_key(uint8_t ch, uint8_t op) {
    if (ch >= 3) ch = ++ch & 0x07;
    hal.write(0x28, ch | (op << 4), 0, WAIT_83);
}

void OpnBase::fm_turnoff_key(uint8_t ch) {
    if (ch >= 3) ch = ++ch & 0x07;
    hal.write(0x28, ch, 0, WAIT_83);
}

void OpnBase::fm_set_detune_multiple(uint8_t ch, uint8_t op, uint8_t dt, uint8_t ml) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    uint8_t dat = ((dt & 0x7) << 4) | (ml & 0x0f);
    switch (op & 3) {
    case 0:
        hal.write(0x30 + ch, dat, a1, WAIT_83);
        break;
    case 1:
        hal.write(0x38 + ch, dat, a1, WAIT_83);
        break;
    case 2:
        hal.write(0x34 + ch, dat, a1, WAIT_83);
        break;
    case 3:
        hal.write(0x3c + ch, dat, a1, WAIT_83);
        break;
    default:
        break;
    }
}

void OpnBase::fm_set_total_level(uint8_t ch, uint8_t op, uint8_t tl) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    switch (op & 3) {
    case 0:
        hal.write(0x40 + ch, tl, a1, WAIT_83);
        break;
    case 1:
        hal.write(0x48 + ch, tl, a1, WAIT_83);
        break;
    case 2:
        hal.write(0x44 + ch, tl, a1, WAIT_83);
        break;
    case 3:
        hal.write(0x4c + ch, tl, a1, WAIT_83);
        break;
    default:
        break;
    }
}

void OpnBase::fm_set_volume(uint8_t ch, uint8_t no, uint8_t vl) {
    if (no > sizeof(fm_tone_table) / sizeof(fm_tone_table[0])) {
        return;
    }
    int alg = fm_tone_table[no][28] & 0x07;
    switch (alg) {
    case 4:  // Carrier: 2,4
        fm_set_total_level(ch, 1, vl);
    case 0:  // Carrier: 4
    case 1:  // Carrier: 4
    case 2:  // Carrier: 4
    case 3:  // Carrier: 4
        fm_set_total_level(ch, 3, vl);
        break;
    case 7:  // Carrier: 1,2,3,4
        fm_set_total_level(ch, 0, vl);
    case 5:  // Carrier: 2,3,4
    case 6:  // Carrier: 2,3,4
        fm_set_total_level(ch, 1, vl);
        fm_set_total_level(ch, 2, vl);
        fm_set_total_level(ch, 3, vl);
        break;
    default:
        break;
    }
}

void OpnBase::fm_set_envelope(uint8_t ch, uint8_t op, const fm_env& ev) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    switch (op & 3) {
    case 0:
        ch += 0x0;
        break;
    case 1:
        ch += 0x8;
        break;
    case 2:
        ch += 0x4;
        break;
    case 3:
        ch += 0xc;
        break;
    default:
        break;
    }
    hal.write(0x50 + ch, (ev.ks << 6) | (ev.ar & 0x1f), a1, WAIT_83);  // KS/AR
    hal.write(0x60 + ch, ev.dr & 0x1f, a1, WAIT_83);                   // DR
    hal.write(0x70 + ch, ev.sr & 0x1f, a1, WAIT_83);                   // SR
    hal.write(0x80 + ch, (ev.sl << 4) | (ev.rr & 0x0f), a1, WAIT_83);  // SL/RR
}

void OpnBase::fm_set_ssg_envelope(uint8_t ch, uint8_t op, uint8_t type) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    switch (op & 3) {
    case 0:
        ch += 0x0;
        break;
    case 1:
        ch += 0x8;
        break;
    case 2:
        ch += 0x4;
        break;
    case 3:
        ch += 0xc;
        break;
    default:
        break;
    }
    hal.write(0x90 + ch, type & 0x0f, a1, WAIT_83);
}

void OpnBase::fm_set_fnumber(uint8_t ch, uint8_t fnum2, uint8_t fnum1) {
    uint8_t a1 = 0;
    if (ch >= 3) {
        ch -= 3;
        a1 = 1;
    }
    uint8_t adr2, adr1;
    switch (ch) {
    case 0:
        adr2 = 0xa4;
        adr1 = 0xa0;
        break;
    case 1:
        adr2 = 0xa5;
        adr1 = 0xa1;
        break;
    case 2:
        adr2 = 0xa6;
        adr1 = 0xa2;
        break;
    default:
        break;
    }
    // Setting order is important
    hal.write(adr2, fnum2, a1, WAIT_47);
    hal.write(adr1, fnum1, a1, WAIT_47);
}

void OpnBase::fm_set_fnumber_ch3(uint8_t op, uint8_t fnum2, uint8_t fnum1) {
    if (ch3_mode == 0) {
        return;
    }
    uint8_t adr2, adr1;
    switch (op & 3) {
    case 0:
        adr2 = 0xad;
        adr1 = 0xa9;
        break;
    case 1:
        adr2 = 0xae;
        adr1 = 0xaa;
        break;
    case 2:
        adr2 = 0xac;
        adr1 = 0xa8;
        break;
    case 3:
        adr2 = 0xa6;
        adr1 = 0xa2;
        break;
    default:
        break;
    }
    // Setting order is important
    hal.write(adr2, fnum2, 0, WAIT_47);
    hal.write(adr1, fnum1, 0, WAIT_47);
}

/////////////////////////////////////////////////////////
// SSG
/////////////////////////////////////////////////////////
void OpnBase::ssg_set_pitch(uint8_t ch, uint8_t p, uint8_t oct) {
    if (p <= MAXNUM_SSG_PITCH && oct <= MAXNUM_OCT) {
        uint8_t adrs  = (ch % 3) * 2;
        uint16_t data = ssg_pitch_table[p] >> oct;
        hal.write(adrs + 0x00, data & 0xff, 0, 1);
        hal.write(adrs + 0x01, data >> 8, 0, 1);
    }
}

void OpnBase::ssg_set_volume(uint8_t ch, uint8_t vol) {
    uint8_t adrs = (ch % 3) * 2;
    uint8_t data = vol > 0x0f ? 0x10 : vol;
    hal.write(adrs + 0x08, data, 0, 1);
}

void OpnBase::ssg_set_noise(uint8_t noise) {
    hal.write(0x6, noise & 0x1f, 0, 1);
}

void OpnBase::ssg_turnon_key(uint8_t ch, bool noise) {
    uint8_t data = hal.read(0x07, 0);
    if (noise) {
        data &= ~(0x81 << (ch % 3));
        hal.write(0x07, data, 0, 1);
    } else {
        data &= ~(0x01 << (ch % 3));
        hal.write(0x07, data, 0, 1);
    }
    return;
}

void OpnBase::ssg_turnoff_key(uint8_t ch, bool noise) {
    uint8_t data = hal.read(0x07, 0);
    if (noise) {
        data |= (0x81 << (ch % 3));
        hal.write(0x07, data, 0, 1);
    } else {
        data |= (0x01 << (ch % 3));
        hal.write(0x07, data, 0, 1);
    }
    return;
}

void OpnBase::ssg_set_envelope(uint16_t period, uint8_t pattern) {
    hal.write(0x11, period & 0xff, 0, 1);
    hal.write(0x12, period >> 8, 0, 1);
    hal.write(0x13, pattern & 0x0f, 0, 1);
}

/////////////////////////////////////////////////////////
// TIMER
/////////////////////////////////////////////////////////
void OpnBase::set_timer_a(uint16_t value) {
    hal.write(0x25, value & 0x3, 0, WAIT_83);
    hal.write(0x24, (value >> 2) & 0xff, 0, WAIT_83);
}

void OpnBase::set_timer_a_ms(float time) {
    int value = 1024 - (uint16_t)(timerA_k * time);
    if (value < 0) {
        value = 0;
    }
    set_timer_a(value);
}

void OpnBase::set_timer_b(uint8_t value) {
    hal.write(0x26, value, 0, WAIT_83);
}

void OpnBase::set_timer_b_ms(float time) {
    int value = 256 - (uint8_t)(timerB_k * time);
    if (value < 0) {
        value = 0;
    }
    set_timer_b(value);
}

void OpnBase::set_timer_mode(uint8_t mode) {
    timer_mode = mode & 0x3f;
    hal.write(0x27, ch3_mode | timer_mode, 0, WAIT_83);
}

void OpnBase::set_fmch3_mode(uint8_t mode) {
    if (mode < 3) {
        ch3_mode = mode << 6;
        hal.write(0x27, ch3_mode | timer_mode, 0, WAIT_83);
    }
}

/////////////////////////////////////////////////////////
// I/O PORT
/////////////////////////////////////////////////////////
void OpnBase::set_port_direction(bool pa, bool pb) {
    uint8_t data = hal.read(0x07, 0) & 0x3f;
    if (pa) {
        data |= 0x40;
    } else {
        data &= 0xbf;
    }
    if (pb) {
        data |= 0x80;
    } else {
        data &= 0x7f;
    }
    hal.write(0x07, data, 0, 1);
}

void OpnBase::write_port_a(uint8_t data) {
    hal.write(0x0e, data, 0, 1);
}

void OpnBase::write_port_b(uint8_t data) {
    hal.write(0x0f, data, 0, 1);
}

uint8_t OpnBase::read_port_a() {
    return hal.read(0x0e, 0);
}

uint8_t OpnBase::read_port_b() {
    return hal.read(0x0f, 0);
}

/////////////////////////////////////////////////////////
// Status
/////////////////////////////////////////////////////////
uint8_t OpnBase::read_status(int a1) {
    return hal.read_status(a1);
}

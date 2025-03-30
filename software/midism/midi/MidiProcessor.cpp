//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "MidiProcessor.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Debugger.h"
#include "VoiceAllocator.h"

// Debug
#define DUMP_MESSAGE           0
#define MIDIMSG_TIMING_CLOCK   0
#define MIDIMSG_ACTIVE_SENSING 0

MidiProcessor::MidiProcessor(std::array<MidiChannel*, MIDI_CHANNELS>& channels)
    : channels(channels),
      enabled_channels(0xffff),
      note_on_status(0),
      status_byte(0),
      isSysEx(false),
      q_index(0) {
}

MidiProcessor::~MidiProcessor() {
}

void MidiProcessor::EnableChannels(uint16_t states) {
    enabled_channels = states;
}

void MidiProcessor::Reset() {
    // 全Voicesリセット(MIDI Channelより先に行う)
    VoiceAllocator::GetInstance().Reset();

    // 全MIDI Channelsリセット
    for (auto& channel : channels) {
        channel->Reset();
    }

    // NoteOn状態のリセット(MidiPanel用)
    note_on_status = 0;
}

// System Exclusive messageの処理
void MidiProcessor::process_sysex_msg(int length) {
    if (length <= 0) {
        return;
    }

    if (memcmp(GM_SYSTEM_ON, msg_queue, length) == 0 || memcmp(XG_RESET, msg_queue, length) == 0 ||
        memcmp(GS_RESET, msg_queue, length) == 0) {
        // MIDIリセット
        Reset();
    } else if (msg_queue[0] == 0x00) {
        // デバッグ用ダンプ
        if (msg_queue[1] == 0x00) {
            for (auto& ch : channels) {
                ch->dump();   // Channel Parameters
                ch->stats();  // Voice release statistics
            }
        }
        // Voice allocation statistics
        printf("Voice allocation failure: %d\n", VoiceAllocator::GetInstance().GetFailedCount());
        VoiceAllocator::GetInstance().dump();  // Voice parameters
    }
}

bool MidiProcessor::push(uint8_t msg) {
    if (msg == 0xf0) {
        q_index = 0;
        isSysEx = true;
    } else if (msg == 0xf7) {
        process_sysex_msg(q_index - 1);
        isSysEx = false;
    } else if (isSysEx) {
        if (q_index < sizeof(msg_queue)) {
            msg_queue[q_index++] = msg;
        }
    }
    return isSysEx;
}

//
//  MIDI messageのパースと実行
//
uint16_t MidiProcessor::Exec(uint8_t msg[3], int num) {
#if DUMP_MESSAGE
    dump_message(msg, num);
    DPRINTF(1, " | ");
#endif
    if (isSysEx) {
        // System exclusive message
        DPRINTF(1, "      |      : ");
        for (int i = 0; i < 3; i++) {
            DPRINTF(1, "%02x ", msg[i]);
            if ((isSysEx = push(msg[i])) == false) {
                break;
            }
        }
        DPRINTF(1, "\n");
    } else {
        if (msg[0] & 0x80) {
            // Channel Voice Message
            status_byte = msg[0];
            for (int i = 0; i < num; i++) {
                isSysEx = push(msg[i]);
            }
        } else {
            // running status
            msg[1] = msg[0];
            msg[2] = msg[1];
            msg[0] = status_byte;
        }
        process_event(msg);
    }
    return note_on_status;
}

#if DUMP_MESSAGE
//
// MIDI Messageの値表示(デバッグ用)
//
void MidiProcessor::dump_message(const uint8_t msg[3], int num) {
#if !MIDIMSG_TIMING_CLOCK
    if (msg[0] == 0xf8) return;
#endif
#if !MIDIMSG_ACTIVE_SENSING
    if (msg[0] == 0xfe) return;
#endif

    int i;
    for (i = 0; i < num; i++) {
        DPRINTF(1, "%02x", msg[i]);
    }
    for (; i < 3; i++) {
        DPRINTF(1, "--");
    }
    DPRINTF(1, " |");
}
#endif

//
// MIDIイベントの処理
//
void MidiProcessor::process_event(const uint8_t msg[3]) {
    // 処理対象のチャンネルかチェック
    uint8_t ch    = msg[0] & 0xf;
    uint16_t mask = 1 << ch;
    if ((enabled_channels & mask) == 0) {
        return;
    }
    MidiChannel* channel = channels[ch];

    int16_t val;
    int ev = (msg[0] >> 4) & 0xf;
    if (ev != 0xf) {
        DPRINTF(1, "CH:%02d | ", ch);
    }

    switch (ev) {
    case NOTE_ON:
        val = channel->NoteOn(msg[1], msg[2]);
        if (val == 1) {
            // NoteOnした
            note_on_status |= mask;
        } else {
            // NoteOff or NoteOn失敗
            note_on_status &= ~mask;
        }
        DPRINTF(1, "ON : k=%3d v=%3d", msg[1], msg[2]);
        break;
    case NOTE_OFF:
        if (channel->NoteOff(msg[1]) != 1) {
            // NoteOffした
            note_on_status &= ~mask;
        }
        DPRINTF(1, "OFF: k=%d v=%d", msg[1], msg[2]);
        break;
    case PROGRAM_CHANGE:
        channel->SetProgram(msg[1]);
        DPRINTF(1, "PROG: %d", msg[1]);
        break;
    case CONTROL_CHANGE:
        switch (msg[1]) {
        case 1:  // Modulation
            channel->SetModulation(msg[2]);
            break;
        case 7:   // Volume
        case 11:  // Expression
            channel->SetVolume(msg[2]);
            break;
        case 64:  // Hold1
            channel->Hold1(msg[2]);
            break;
        case 98:  // NRPN LSB
            channel->NRPN_LSB(msg[2]);
            break;
        case 99:  // NRPN MSB
            channel->NRPN_MSB(msg[2]);
            break;
        case 100:  // RPN LSB
            channel->RPN_LSB(msg[2]);
            break;
        case 101:  // RPN MSB
            channel->RPN_MSB(msg[2]);
            break;
        case 6:  // Data entry MSB
            channel->DataEntry_MSB(msg[2]);
            break;
        case 38:  // Data entry LSB
            channel->DataEntry_LSB(msg[2]);
            break;
        case 10:  // Pan
            channel->SetPan(msg[2]);
            break;
        case 0:  // Bank select MSB
            channel->BankSelect_MSB(msg[2]);
            break;
        case 32:  // Bank select LSB
            channel->BankSelect_LSB(msg[2]);
            break;
        case 120:  // All Sound Off
        case 123:  // All Note Off
            channel->Reset();
            break;
        }
        DPRINTF(3, "CC: #%d/%d", msg[1], msg[2]);
        break;
    case KEY_AFTER_TOUCH:
        DPRINTF(3, "PolyPress: key=%d velocity=%d", msg[1], msg[2]);
        break;
    case CH_AFTER_TOUCH:
        DPRINTF(3, "ChPress: %d", msg[1]);
        break;
    case PITCH_BEND:
        val = msg[1] + 128 * msg[2] - 8192;
        channel->PitchBend(val);
        DPRINTF(3, "PB: %d", val);
        break;
    case SYSTEM_EXCLUSIVE:
        if (ch == 0) {
            DPRINTF(3, "System| SysEx: %02x %02x %02x\n", msg[0], msg[1], msg[2]);
        } else if (ch == 1) {
            DPRINTF(3, "System| MIDI TC: %d\n", msg[1]);
        } else if (ch == 2) {
            DPRINTF(3, "System| SongPos: %d\n", msg[1] + 128 * msg[2]);
        } else if (ch == 3) {
            DPRINTF(3, "System| SongSelect: %d\n", msg[1]);
        } else if (ch == 6) {
            DPRINTF(3, "System| TuneReqest\n");
        } else if (ch == 8) {
#if MIDIMSG_TIMING_CLOCK
            DPRINTF(4, "System| TimingClock\n");
#endif
        } else if (ch == 10) {
            DPRINTF(3, "System| START\n");
        } else if (ch == 11) {
            DPRINTF(3, "System| CONTINUE\n");
        } else if (ch == 12) {
            DPRINTF(3, "System| STOP\n");
        } else if (ch == 14) {
#if MIDIMSG_ACTIVE_SENSING
            DPRINTF(4, "System| ActiveSensing\n");
#endif
        } else if (ch == 15) {
            DPRINTF(3, "System| RESET\n");
        }
        return;
    default:
        break;
    }
    DPRINTF(1, "\n");
    return;
}

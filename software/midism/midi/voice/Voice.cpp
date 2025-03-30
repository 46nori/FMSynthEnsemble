//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "Voice.h"

Voice::Voice(bool type, int id)
    : type(type), note_on_count(0), midi_ch(-1), bk_program(-1), volume(-1), key(-1), id(id) {
}

Voice::~Voice() {
}

void Voice::Reset() {
    NoteOff();
    midi_ch       = -1;
    bk_program    = -1;
    volume        = -1;
    key           = -1;
    note_on_count = 0;
}

bool Voice::GetType() {
    return type;
}

bool Voice::IsFree() {
    return midi_ch == -1;
}

int Voice::GetChannel() {
    return midi_ch;
}

void Voice::SetChannel(int channel) {
    midi_ch = channel;
}

int Voice::GetKey() {
    return key;
}

void Voice::SetNoteOnCount(int val) {
    note_on_count = val;
}

int Voice::GetNoteOnCount() {
    return note_on_count;
}

int Voice::IncrementNoteOnCount() {
    return ++note_on_count;
}

int Voice::DecrementNoteOnCount() {
    if (--note_on_count < 0) {
        note_on_count = 0;
    }
    return note_on_count;
}

void Voice::dump() {
    // Debug implementation
}

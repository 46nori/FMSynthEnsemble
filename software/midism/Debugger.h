//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <cstdint>
#include <cstdio>

#include "config.h"

/**
 *   Debugger macros
 */
#if ENABLE_DEBUG_PRINT == 1
#define DPRINTF(level, format, ...)        \
    if (level <= Debugger::gDEBUG_LEVEL) { \
        printf(format, ##__VA_ARGS__);     \
    }
#else
#define DPRINTF(level, format, ...)
#endif

namespace Debugger {
/*
 * Debugger commands
 */
constexpr uint8_t DEBUGGER_MIDI_RESET   = 0x01;
constexpr uint8_t DEBUGGER_DUMP_CHANNEL = 0x02;
constexpr uint8_t DEBUGGER_DUMP_VOICE   = 0x03;
constexpr uint8_t DEBUGGER_STATS        = 0x04;

/*
 * Debugger control variables
 */
extern volatile bool gMidiMode;
extern volatile uint8_t gDEBUG_LEVEL;

/*
 * Debugger functions
 */
extern void main(void);
};  // namespace Debugger

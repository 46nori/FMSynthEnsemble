//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "Debugger.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "VoiceAllocator.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

using namespace Debugger;

/*********************************************************
 * グローバル変数
 *********************************************************/
volatile uint8_t Debugger::gDEBUG_LEVEL = 0;
volatile bool Debugger::gMidiMode       = true;

#define DLIMITER   " "
#define MAX_TOKENS 4
#define T_CMD      0
#define T_PARAM1   1
#define T_PARAM2   2
#define T_PARAM3   3
typedef struct token_list {
    int n;                    // number of tokens
    char* token[MAX_TOKENS];  // pointer to token
} token_list;

static token_list* tokenizer(char* str, const char* delim, token_list* t);
static int exec_command(token_list* t);

#define NO_ERROR       (0)
#define ERR_COMMAND    (-1)
#define ERR_PARAM_VAL  (-2)
#define ERR_PARAM_MISS (-3)

/*********************************************************
 * Monitor main
 *********************************************************/
void Debugger::main(void) {
    token_list tokens;
    char cmd_line[32];

    while (1) {
        putchar('>');
        while (fgets(cmd_line, sizeof(cmd_line) - 1, stdin) == NULL);
        cmd_line[strcspn(cmd_line, "\r\n")] = '\0';
        puts(cmd_line);
        tokenizer(cmd_line, DLIMITER, &tokens);
        switch (exec_command(&tokens)) {
        case NO_ERROR:
            break;
        case ERR_COMMAND:
            puts("not found.");
            break;
        case ERR_PARAM_VAL:
            puts("param error.");
            break;
        case ERR_PARAM_MISS:
            puts("missing param.");
            break;
        default:
            puts("error!");
            break;
        }
    }
}

// Tokenizer
static token_list* tokenizer(char* str, const char* delim, token_list* t) {
    char* token;
    t->n  = 0;
    token = strtok(str, delim);
    while (token != NULL && t->n < MAX_TOKENS) {
        t->token[t->n++] = token;
        token            = strtok(NULL, delim);
    }
    return t;
}

// Get token value as an unsigned int
static int get_uint(token_list* t, unsigned int idx, unsigned int* val) {
    if (idx >= t->n) {
        return ERR_PARAM_MISS;
    }

    unsigned int tmp;
    const char* str = t->token[idx];
    if (*str == '$') {
        // Hex
        if (sscanf(str + 1, "%x", &tmp) == 1) {
            *val = tmp;
            return NO_ERROR;
        }
    } else if (sscanf(str, "%u", &tmp) == 1) {
        // Decimal
        *val = tmp;
        return NO_ERROR;
    }
    return ERR_PARAM_VAL;
}

//
// User command handler
//
static int c_help(token_list* t);
static int c_debug_level(token_list* t);
static int c_midi_mode(token_list* t);
static int c_midi_reset(token_list* t);
static int c_midi_reset(token_list* t);
static int c_midi_dump_channel(token_list* t);
static int c_midi_dump_voice(token_list* t);
static int c_midi_stats(token_list* t);

static const struct {
    const char* name;
    int (*func)(token_list*);
} cmd_table[] = {
    {    "dl",       c_debug_level},
    {    "mm",         c_midi_mode},
    {"mreset",        c_midi_reset},
    {    "dc", c_midi_dump_channel},
    {    "dv",   c_midi_dump_voice},
    { "stats",        c_midi_stats},
    {     "h",              c_help},
    {      "",                NULL}
};

// Lookup command
static int exec_command(token_list* t) {
    if (t->n == 0) {
        return NO_ERROR;  // do nothing
    }
    for (int i = 0; cmd_table[i].func != NULL; i++) {
        if (!strcmp(cmd_table[i].name, t->token[T_CMD])) {
            return cmd_table[i].func(t);
        }
    }
    return ERR_COMMAND;  // command not found
}

/*********************************************************
 * Help
 *********************************************************/
static int c_help(token_list* t) {
    // Allocate literal in Flash ROM.
    static constexpr char help_str[] =
        "   <> : mandatory\n"
        "   [] : optional\n"
        "h        : Help\n"
        "dl [0-5] : Set debug print level\n"
        "dc [0-15]: Dump MIDI Channel parameters\n"
        "dv       : Dump MIDI Voice parameters\n"
        "mm [0-1] : MIDI Mode 0:Ignore MIDI, 1:Process MIDI\n"
        "stats    : Statistics\n"
        "mreset   : MIDI Reset\n"
        "";

    puts(help_str);
    return NO_ERROR;
}

/*********************************************************
 * Debug print level
 *********************************************************/
static int c_debug_level(token_list* t) {
    // External memory test
    unsigned int level = 0;
    if (t->n > 1) {
        get_uint(t, T_PARAM1, &level);
        gDEBUG_LEVEL = (uint8_t)level;
    }
    printf("Debug Level: %d\n", gDEBUG_LEVEL);
    return NO_ERROR;
}

/*********************************************************
 * Debug print level
 *********************************************************/
static int c_midi_mode(token_list* t) {
    unsigned int mode = 0;
    if (t->n > 1) {
        get_uint(t, T_PARAM1, &mode);
        gMidiMode = (mode == 0) ? false : true;
    }
    printf("MIDI Mode: %s\n", gMidiMode ? "ON" : "OFF");
    return NO_ERROR;
}

/*********************************************************
 * MIDI Reset
 *********************************************************/
static int c_midi_reset(token_list* t) {
    multicore_fifo_push_blocking(DEBUGGER_MIDI_RESET);
    return NO_ERROR;
}

/*********************************************************
 * Dump MIDI Channel parameters
 *********************************************************/
static int c_midi_dump_channel(token_list* t) {
    if (t->n == 1) {
        // Dummp All channels
        multicore_fifo_push_blocking(DEBUGGER_DUMP_CHANNEL | (0xff << 8));
    } else if (t->n > 1) {
        // Dump specified channel
        unsigned int ch = 0;
        get_uint(t, T_PARAM1, &ch);
        multicore_fifo_push_blocking(DEBUGGER_DUMP_CHANNEL | (ch << 8));
    }
    return NO_ERROR;
}

/*********************************************************
 * Dump MIDI Voice parameters
 *********************************************************/
static int c_midi_dump_voice(token_list* t) {
    multicore_fifo_push_blocking(DEBUGGER_DUMP_VOICE);
    return NO_ERROR;
}

/*********************************************************
 * Statistics
 *********************************************************/
static int c_midi_stats(token_list* t) {
    multicore_fifo_push_blocking(DEBUGGER_STATS);
    return NO_ERROR;
}
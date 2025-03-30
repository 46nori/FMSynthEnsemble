//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include <cstdint>
#include <cstdio>

#include "Debugger.h"
#include "MidiFactory.h"
#include "MidiPanel.h"
#include "MidiProcessor.h"
#include "RP2040.h"
#include "VoiceAllocator.h"
#include "YM2608.h"
//#include "YM2203.h"
#include "bsp/board.h"
#include "config.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"

#if ENABLE_DEUGGER == 1
using namespace Debugger;
static void core1_entry();
static void debug_command(std::array<MidiChannel*, MIDI_CHANNELS>& channels, MidiProcessor& mp);
#endif

/*********************************************************
 * Main (Core0)
 *********************************************************/
int main(int argc, char** argv) {
    // HAL adapterの生成と初期化
    RP2040 hal0(0), hal1(1), hal2(2), hal3(3);
    RP2040::init();

    // FM音源モジュールのインスタンス生成と、HAL adapterの紐付け
    //    YM2203 module_0(hal0, 4000.0, 0);
    //    YM2203 module_1(hal1, 4000.0, 1);
    YM2608 module_0(hal0, 8000.0, 0);
    YM2608 module_1(hal1, 8000.0, 1);
    YM2608 module_2(hal2, 8000.0, 2);
    YM2608 module_3(hal3, 8000.0, 3);

    // FM音源モジュールのDockへの接続
    // 未接続のDockにはnullptrをセットする
    std::array<OpnBase*, 4> modules{
        &module_0,  // Dock0
        &module_1,  // Dock1
        &module_2,  // Dock2
        &module_3   // Dock3
    };

    // MIDIチャンネルのインスタンス生成とリズムチャンネルの設定
    // (ここではmodule_3のYM2608をリズム用に使用している)
    MidiFactory factory(modules);
    auto midi_channels = factory.Create(&module_3);

    // MIDI processorの生成
    MidiProcessor mp(midi_channels);

#if ENABLE_MIDI_PANEL == 1
    // MIDI状態表示パネルのインスタンス生成 (module_0で制御する)
    MidiPanel panel(module_0);
    // パネルのMIDIチャンネルのON/OFF設定を反映
    mp.EnableChannels(panel.GetMidiSwState());
#endif

    // TinyUSB MIDIの初期化
    board_init();
    tusb_init();
    sleep_ms(500);

#if ENABLE_DEUGGER == 1
    // Debuggerの起動(Core1)
    multicore_launch_core1(core1_entry);
#endif

    // MIDIメッセージ処理の開始
    Debugger::gMidiMode = true;  // MIDIモードで起動(以後Deubugerで制御される)
    do {
        tud_task();
        // MIDIメッセージの待機
        if (tud_midi_n_available(0, 0)) {
            uint8_t msg[3]{0};
            int num = tud_midi_n_stream_read(0, 0, msg, sizeof(msg));
            // MIDIメッセージの実行
            if (num && Debugger::gMidiMode) {
#if ENABLE_MIDI_PANEL == 1
                uint16_t keyOn = mp.Exec(msg, num);
                panel.SetLed(keyOn);  // CH毎のKeyOn状態表示
                                      //printf("%04x\n", keyOn);
#else
                mp.Exec(msg, num);
#endif
            }
        }
#if ENABLE_MIDI_PANEL == 1
        // MIDIパネル状態の更新
        panel.Update();
        // MIDIチャンネルのON/OFF設定更新
        mp.EnableChannels(panel.GetMidiSwState());
        // MIDIリセットボタンの処理
        if (panel.IsMidiReset()) {
            panel.SetLed(0);
            mp.Reset();
            printf("MIDI RESET!\n");
        }

#endif
#if ENABLE_DEUGGER == 1
        // Debuggerからのコマンド処理
        if (multicore_fifo_rvalid()) {
            debug_command(midi_channels, mp);
        }
#endif
    } while (1);
}

#if ENABLE_DEUGGER == 1
/*********************************************************
 * Debugger (Core1)
 *********************************************************/
static void core1_entry() {
    printf("\nFMSynthEnsmble\n");
    Debugger::main();
    while (1);
}

static void debug_command(std::array<MidiChannel*, MIDI_CHANNELS>& channels, MidiProcessor& mp) {
    uint32_t cmd = multicore_fifo_pop_blocking();
    switch (cmd & 0xff) {
    case DEBUGGER_MIDI_RESET:  // MIDIリセット
        mp.Reset();
        break;
    case DEBUGGER_DUMP_CHANNEL:  // MIDI Channelのダンプ
        if (((cmd >> 8) & 0xff) == 0xff) {
            for (auto& ch : channels) {
                ch->dump();
            }
        } else {
            channels[(cmd >> 8) & 0xff]->dump();
        }
        break;
    case DEBUGGER_DUMP_VOICE:  // MIDI Voiceのダンプ
        VoiceAllocator::GetInstance().dump();
        break;
    case DEBUGGER_STATS:  // Voiceアロケーションの統計情報
        printf("\nVoice allocation failure: %d\n", VoiceAllocator::GetInstance().GetFailedCount());
        for (auto& ch : channels) {
            ch->stats();
        }
        break;
    default:
        break;
    }
}
#endif

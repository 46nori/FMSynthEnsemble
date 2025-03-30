//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once

// システムでサポートするMIDIチャンネル数(1-16)
constexpr int MIDI_CHANNELS = 16;

// デバッグモードの有効化
#define ENABLE_DEUGGER                         1
#define ENABLE_DEBUG_PRINT                     1

// CSMボイスの有効化
#define ENABLE_CSM                             1

// COARSE TUNEの有効化
#define ENABLE_COARSE_TUNE                     1

// MIDIパネルを接続する場合は1にする
#define ENABLE_MIDI_PANEL                      1
#if ENABLE_MIDI_PANEL == 1
// OPNAモジュールのバグ対策
#define ENABLE_CONNECTOR_WIRING_BUG_WORKAROUND 1
#endif

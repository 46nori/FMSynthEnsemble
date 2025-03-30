//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <functional>

#include "HAL.h"

//
//   GPIO signal Assignment
//
//   bit  31 30 29 28 27 26 25 24   23 22 21 20   19 18 17 16
//        -- -- -- /CS2,1,0 -- --   --/IC -- -- /IRQ3 2  1  0
//   DIR   0  0  0  1  1  1  0  0    0  1  0  0    0  0  0  0
//
//   bit  15 14 13 12 11 10  9  8    7  6  5  4   3  2  1  0
//        D7  6  5  4  3  2  1  0   A1 A0 -- -- /WR/RD -- --
//   DIR   *  *  *  *  *  *  *  *    1  1  0  0   1  1  0  0
//
//   -- : Not used for GPIO
//   DIR: 0(READ), 1(WRITE), *(0/1)
//
#define FM_RD   2
#define FM_WR   3
#define FM_A0   6
#define FM_A1   7
#define FM_D0   8
#define FM_D1   9
#define FM_D2   10
#define FM_D3   11
#define FM_D4   12
#define FM_D5   13
#define FM_D6   14
#define FM_D7   15
#define FM_IRQ0 16
#define FM_IRQ1 17
#define FM_IRQ2 18
#define FM_IRQ3 19
#define FM_IC   22
#define FM_CS0  26
#define FM_CS1  27
#define FM_CS2  28

/**
 * @brief RP2040 adapter class
 */
class RP2040 : public HAL {
private:
    uint32_t cs;
    uint32_t interrupts;

public:
    /**
     * @brief コンストラクタ
     * @param dock Dock番号
     * @details Dock番号に応じて/CSが設定される
     */
    RP2040(int dock);
    virtual ~RP2040();

    RP2040()                         = delete;
    RP2040(const RP2040&)            = delete;
    RP2040& operator=(const RP2040&) = delete;
    RP2040(RP2040&&)                 = delete;
    RP2040& operator=(RP2040&&)      = delete;

    /**
     * @brief RP2040含めたハードウェアの初期化
     * @details 静的メンバ関数なので、RP2040::init()で呼び出す
     */
    static void init();

    uint8_t read(uint8_t adrs, uint8_t a1) override;
    void write(uint8_t adrs, uint8_t data, uint8_t a1, uint8_t wait) override;
    uint8_t read_status(uint8_t a1) override;

private:
    __inline void enable_cs();
    __inline void disable_cs();
    //void wait_until_ready();
};

/**
 * @brief 割り込み処理
 */
extern void attach_isr_callback(int module, std::function<void()> func);
extern void enable_isr_callback(int gpio);
extern void disable_isr_callback(int gpio);

//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#include "RP2040.h"

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

/**
 * @brief GPIOの初期化
 */
static void init_gpio() {
    // Init GPIO
    gpio_init_mask(0b00011100010011111111111111001100);

    // Disable pull up/down
    constexpr uint8_t pins[] = {FM_RD,   FM_WR,   FM_A0, FM_A1,  FM_D0,  FM_D1,   FM_D2,
                                FM_D3,   FM_D4,   FM_D5, FM_D6,  FM_D7,  FM_IRQ0, FM_IRQ1,
                                FM_IRQ2, FM_IRQ3, FM_IC, FM_CS0, FM_CS1, FM_CS2};
    for (int i = 0; i < sizeof(pins) / sizeof(uint8_t); i++) {
        gpio_disable_pulls(pins[i]);
    }

    // Set direction (D7-0 OUT by default)
    gpio_set_dir_masked(0b00011100010011111111111111001100, 0b00011100010000001111111111001100);

    // Set /CS2, /CS1, /CS0, /IC, /A1, /A0, /WR, /RD = H
    gpio_put_masked(0b00011100010000000000000011001100, 0b00011100010000000000000011001100);
}

/**
 * @brief FM音源LSIのハードウェアリセット
 * */
static void reset_fmchip() {
    gpio_put(FM_IC, 0);
    sleep_us(100);  // > 24us(min)@OPNA, >18us(min)@OPN
    gpio_put(FM_IC, 1);
}

/**
 * @brief GPIO割り込みハンドラ
 */
static std::function<void()> cb_func;
static void isr(uint gpio, uint32_t events) {
    // コールバック
    cb_func();
    // 割り込みイベントのクリア
    gpio_acknowledge_irq(gpio, GPIO_IRQ_EDGE_FALL);
}

/**
 * @brief 割り込み時のコールバックの登録
 * @param gpio  GPIO pin
 * @param func  callback function
 */
void attach_isr_callback(int gpio, std::function<void()> func) {
    cb_func = func;                                         // isr()内で呼び出すコールバック登録
    gpio_set_irq_enabled_with_callback(gpio,                // モニタするGPIO
                                       GPIO_IRQ_EDGE_FALL,  // 立ち下がりエッジ
                                       true,                // 割り込みは無効のまま
                                       &isr                 // 割り込みハンドラ
    );
}

/**
 * @brief 割り込みを有効にする
 * @param gpio  GPIO pin
 */
void enable_isr_callback(int gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, true);
}

/**
 * @brief 割り込みを無効にする
 * @param gpio  GPIO pin
 */
void disable_isr_callback(int gpio) {
    gpio_set_irq_enabled(gpio, GPIO_IRQ_EDGE_FALL, false);
}

//
// RP2040
//
RP2040::RP2040(int dock) : cs((uint32_t)(dock & 7) << FM_CS0) {
}

RP2040::~RP2040() {
}

void RP2040::init() {
    stdio_init_all();  // デバッグ用シリアル出力の初期化
    init_gpio();
    reset_fmchip();
}

__inline void RP2040::enable_cs() {
    gpio_put_masked((uint32_t)7 << FM_CS0, cs);
}

__inline void RP2040::disable_cs() {
    gpio_put_masked((uint32_t)7 << FM_CS0, (uint32_t)7 << FM_CS0);
}

uint8_t RP2040::read(uint8_t adrs, uint8_t a1) {
    //wait_until_ready();

    // Disable interrupt to avoid I/O overlapping
    interrupts = save_and_disable_interrupts();

    // Set address to READ
    gpio_put(FM_A1, a1);  // 1: Status1,ADPCM
    gpio_put(FM_A0, 0);   // for address write
    gpio_put(FM_WR, 0);
    enable_cs();
    gpio_put_masked(0x0000ff00, (uint32_t)adrs << 8);
    sleep_us(2);  // Hold time for /CS, /WR > 200ns
                  // Note: sleep_us(1) is insufficient
    gpio_put(FM_WR, 1);
    disable_cs();
    sleep_us(5);  // wait for >4.25us(17cycles@4MHz)

    // Read register
    gpio_set_dir_in_masked(0x0000ff00);  // Set D7-0 IN;
    gpio_put(FM_A0, 1);                  // for register read
    gpio_put(FM_RD, 0);
    enable_cs();
    sleep_us(2);  // Hold time for /CS, /RD > 250ns
                  // Note: sleep_us(1) is insufficient
    uint8_t data = gpio_get_all() >> 8 & 0xff;
    gpio_put(FM_RD, 1);
    disable_cs();
    gpio_set_dir_out_masked(0x0000ff00);  // Set D7-0 OUT

    restore_interrupts(interrupts);
    return data;
}

void RP2040::write(uint8_t adrs, uint8_t data, uint8_t a1, uint8_t wait) {
    //wait_until_ready();

    // Disable interrupt to avoid I/O overlapping
    interrupts = save_and_disable_interrupts();

    // Set address to WRITE
    gpio_put(FM_A1, a1);  // 0:ch1-3 / 1: ch4-6
    gpio_put(FM_A0, 0);   // for address write
    gpio_put(FM_WR, 0);
    enable_cs();
    gpio_put_masked(0x0000ff00, (uint32_t)adrs << 8);
    sleep_us(2);  // Hold time for /CS, /WR > 200ns
                  // Note: sleep_us(1) is insufficient
    gpio_put(FM_WR, 1);
    disable_cs();
    sleep_us(5);  // wait for >4.25us(17cycles@4MHz)

    // Set data
    gpio_put(FM_A0, 1);  // for register write
    gpio_put(FM_WR, 0);
    enable_cs();
    gpio_put_masked(0x0000ff00, (uint32_t)data << 8);
    sleep_us(2);  // Hold time for /CS, /WR > 200ns
                  // Note: sleep_us(1) is insufficient
    gpio_put(FM_WR, 1);
    disable_cs();

    sleep_us(wait);

    restore_interrupts(interrupts);
}

#if 0
void RP2040::wait_until_ready()
{
    // Disable interrupt to avoid I/O overlapping
    interrupts = save_and_disable_interrupts();

    bool isBusy;
    gpio_put(FM_A0, 0);
    gpio_set_dir_in_masked(0x0000ff00); // Set D7-0 IN
    do {
        gpio_put(FM_RD, 0);
        enable_cs();
        sleep_us(2);          // Hold time for /CS, /RD > 250ns
                            // Note: sleep_us(1) is insufficient
        isBusy = gpio_get(15);
        gpio_put(FM_RD, 1);
        disable_cs();
    } while (isBusy);
    gpio_set_dir_out_masked(0x0000ff00); // Set D7-0 OUT

    restore_interrupts(interrupts);
}
#endif

uint8_t RP2040::read_status(uint8_t a1) {
    // Disable interrupt to avoid I/O overlapping
    interrupts = save_and_disable_interrupts();

    gpio_put(FM_A1, a1);
    gpio_put(FM_A0, 0);
    gpio_set_dir_in_masked(0x0000ff00);  // Set D7-0 IN
    gpio_put(FM_RD, 0);
    enable_cs();
    sleep_us(2);  // Hold time for /CS, /RD > 250ns
                  // Note: sleep_us(1) is insufficient
    uint8_t data = (gpio_get_all() >> 8) & 0xff;
    gpio_put(FM_RD, 1);
    disable_cs();
    gpio_set_dir_out_masked(0x0000ff00);  // Set D7-0 OUT

    restore_interrupts(interrupts);
    return data;
}

//
// Copyright (c) 2025 46nori All rights reserved.
//
// This code is licensed under the MIT License.
// See LICENSE file for details.
//
#pragma once
#include <cstdint>

/**
 * @brief HAL(Hardware Abstruction Layer) interface
 */
class HAL {
public:
    /**
     * @brief Read register
     * @param [in] adrs : Register address
     * @param [in] a1   : 1 for YM2608
     * @return register value
     * @details
     *     FM registers is not readable.
     *     SSG, Status, ADPCM(YM2608) registers are readable.
     */
    virtual uint8_t read(uint8_t adrs, uint8_t a1) = 0;

    /**
     * @brief Write register
     * @param [in] adrs : Register address
     * @param [in] data : Data
     * @param [in] a1   : 1 for YM2608
     * @param [in] wait : wait cycle after write
     */
    virtual void write(uint8_t adrs, uint8_t data, uint8_t a1, uint8_t wait) = 0;

    /**
     * @brief Read Status
     * @param [in] a1   : 1 for YM2608
     */
    virtual uint8_t read_status(uint8_t a1) = 0;
};

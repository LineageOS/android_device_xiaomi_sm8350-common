/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

enum SscOemMsgType : uint32_t {
    BRIGHTNESS = 17,
    DISPLAY_FREQUENCY = 20,
};

// Forwards a value to the ambient light sensor on the SSC. Returns 0 on success.
int sendSscOemMsg(SscOemMsgType type, uint32_t value);

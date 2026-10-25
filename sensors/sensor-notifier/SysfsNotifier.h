/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android-base/unique_fd.h>

#include <cstdint>
#include <functional>
#include <string>

// Reads an unsigned integer from a sysfs node each time the node signals a change
class SysfsNotifier {
  public:
    using Callback = std::function<void(uint32_t)>;

    SysfsNotifier(std::string path, Callback callback);

    bool isValid() const { return mFd.ok(); }
    int fd() const { return mFd.get(); }

    // Reads the current value and passes it to the callback. Also re-arms POLLPRI.
    void notify();

  private:
    std::string mPath;
    android::base::unique_fd mFd;
    Callback mCallback;
};

/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SysfsNotifier.h"

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>
#include <fcntl.h>
#include <unistd.h>

SysfsNotifier::SysfsNotifier(std::string path, Callback callback)
    : mPath(std::move(path)),
      mFd(open(mPath.c_str(), O_RDONLY | O_CLOEXEC)),
      mCallback(std::move(callback)) {
    if (!isValid()) {
        PLOG(ERROR) << "failed to open " << mPath;
    }
}

void SysfsNotifier::notify() {
    char buf[32];
    ssize_t len = TEMP_FAILURE_RETRY(pread(mFd.get(), buf, sizeof(buf), 0));
    if (len < 0) {
        PLOG(ERROR) << "failed to read " << mPath;
        return;
    }

    uint32_t value;
    if (!android::base::ParseUint(android::base::Trim(std::string_view(buf, len)), &value)) {
        LOG(ERROR) << "unexpected content in " << mPath;
        return;
    }

    LOG(DEBUG) << mPath << ": " << value;
    mCallback(value);
}

/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <android-base/logging.h>
#include <poll.h>

#include <vector>

#include "SscCalApi.h"
#include "SysfsNotifier.h"

namespace {

struct Node {
    const char* path;
    SscOemMsgType type;
};

constexpr Node kNodes[] = {
        {"/sys/class/backlight/panel0-backlight/brightness", BRIGHTNESS},
        {"/sys/class/mi_display/disp-DSI-0/dynamic_fps", DISPLAY_FREQUENCY},
};

}  // namespace

int main() {
    std::vector<SysfsNotifier> notifiers;
    std::vector<pollfd> fds;
    for (const auto& [path, type] : kNodes) {
        SysfsNotifier notifier(path, [type](uint32_t value) { sendSscOemMsg(type, value); });
        if (!notifier.isValid()) {
            continue;
        }
        // Sync the initial state, this also arms POLLPRI
        notifier.notify();
        fds.push_back({.fd = notifier.fd(), .events = POLLPRI | POLLERR});
        notifiers.push_back(std::move(notifier));
    }

    if (notifiers.empty()) {
        LOG(ERROR) << "no nodes to watch";
        return EXIT_FAILURE;
    }

    while (true) {
        if (poll(fds.data(), fds.size(), -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            PLOG(ERROR) << "poll failed";
            return EXIT_FAILURE;
        }
        for (size_t i = 0; i < fds.size(); i++) {
            if (fds[i].revents & POLLPRI) {
                notifiers[i].notify();
            }
        }
    }
}

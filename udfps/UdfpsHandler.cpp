/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.xiaomi_sm8350"

#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <android-base/logging.h>

#include <fcntl.h>
#include <poll.h>
#include <fstream>
#include <thread>

#include "UdfpsHandler.h"

// Fingerprint hwmodule commands
#define COMMAND_NIT 10
#define PARAM_NIT_UDFPS 1
#define PARAM_NIT_NONE 0

#define COMMAND_FOD_PRESS_STATUS 1
#define PARAM_FOD_PRESSED 1
#define PARAM_FOD_RELEASED 0

// Touchscreen and HBM
#define FOD_HBM_PATH "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_hbm"
#define FOD_UI_PATH "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_ui"

#define FOD_HBM_OFF 0
#define FOD_HBM_ON 1

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

static bool readBool(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
}

class XiaomiUdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) {
        mDevice = device;

        std::thread([this]() {
            int fodUiFd = open(FOD_UI_PATH, O_RDONLY);
            if (fodUiFd < 0) {
                LOG(ERROR) << "failed to open fodUiFd, err: " << fodUiFd;
                return;
            }

            struct pollfd fds[1] = {
                    {fodUiFd, .events = POLLERR | POLLPRI, .revents = 0},
            };

            while (true) {
                int rc = poll(fds, 1, -1);
                if (rc < 0) {
                    if (fds[0].revents & POLLERR) {
                        LOG(ERROR) << "failed to poll fodUiFd, err: " << rc;
                    }
                    continue;
                }

                if (fds[0].revents & (POLLERR | POLLPRI)) {
                    bool nitState = readBool(fodUiFd);
                    mDevice->extCmd(mDevice, COMMAND_NIT,
                                    nitState ? PARAM_NIT_UDFPS : PARAM_NIT_NONE);
                }
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        set(FOD_HBM_PATH, FOD_HBM_ON);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_PRESSED);
    }

    void onFingerUp() {
        set(FOD_HBM_PATH, FOD_HBM_OFF);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_RELEASED);
    }

    void onAcquired(int32_t result, int32_t /*vendorCode*/) {
        switch (static_cast<AcquiredInfo>(result)) {
            case AcquiredInfo::GOOD:
            case AcquiredInfo::PARTIAL:
            case AcquiredInfo::INSUFFICIENT:
            case AcquiredInfo::SENSOR_DIRTY:
            case AcquiredInfo::TOO_SLOW:
            case AcquiredInfo::TOO_FAST:
            case AcquiredInfo::TOO_DARK:
            case AcquiredInfo::TOO_BRIGHT:
            case AcquiredInfo::IMMOBILE:
            case AcquiredInfo::LIFT_TOO_SOON:
                onFingerUp();
                break;
            default:
                break;
        }
    }

    void onAuthenticationSucceeded() { onFingerUp(); }

    void onAuthenticationFailed() { onFingerUp(); }

  private:
    fingerprint_device_t* mDevice;
};

static UdfpsHandler* create() {
    return new XiaomiUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};

/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "SscCalApi.h"

#include <android-base/logging.h>
#include <dlfcn.h>

#include <cerrno>

namespace {

constexpr uint32_t kSensorTypeAmbientLight = 5;

// Layout expected by transfer_ssc_oem_test_msg() in libssccalapi.so
struct SscOemMsg {
    uint32_t sensorType;
    uint32_t msgType;
    uint32_t value;
    uint32_t reserved[15];
};
static_assert(sizeof(SscOemMsg) == 72);

using TransferFn = int (*)(SscOemMsg*);

TransferFn loadTransferFn() {
    // Never dlclose()d: the library stays loaded for the lifetime of the process
    void* handle = dlopen("libssccalapi.so", RTLD_NOW);
    if (handle == nullptr) {
        LOG(ERROR) << "failed to dlopen libssccalapi.so: " << dlerror();
        return nullptr;
    }

    auto fn = reinterpret_cast<TransferFn>(dlsym(handle, "transfer_ssc_oem_test_msg"));
    if (fn == nullptr) {
        LOG(ERROR) << "failed to find transfer_ssc_oem_test_msg: " << dlerror();
    }
    return fn;
}

}  // namespace

int sendSscOemMsg(SscOemMsgType type, uint32_t value) {
    static const TransferFn transfer = loadTransferFn();
    if (transfer == nullptr) {
        return -ENOSYS;
    }

    SscOemMsg msg = {
            .sensorType = kSensorTypeAmbientLight,
            .msgType = type,
            .value = value,
    };
    int rc = transfer(&msg);
    if (rc != 0) {
        LOG(ERROR) << "failed to send message of type " << type << ": " << rc;
    }
    return rc;
}

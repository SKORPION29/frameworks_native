/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <type_traits>
#define LOG_TAG "InputWindow"
#define LOG_NDEBUG 0

#include <android-base/stringprintf.h>
#include <binder/Parcel.h>
#include <input/InputTransport.h>
#include <input/InputWindow.h>

#include <log/log.h>

namespace android {


// --- InputWindowInfo ---
void InputWindowInfo::addTouchableRegion(const Rect& region) {
    touchableRegion.orSelf(region);
}

bool InputWindowInfo::touchableRegionContainsPoint(int32_t x, int32_t y) const {
    return touchableRegion.contains(x,y);
}

bool InputWindowInfo::frameContainsPoint(int32_t x, int32_t y) const {
    return x >= frameLeft && x < frameRight
            && y >= frameTop && y < frameBottom;
}

bool InputWindowInfo::supportsSplitTouch() const {
    return flags.test(Flag::SPLIT_TOUCH);
}

bool InputWindowInfo::overlaps(const InputWindowInfo* other) const {
    return frameLeft < other->frameRight && frameRight > other->frameLeft
            && frameTop < other->frameBottom && frameBottom > other->frameTop;
}

bool InputWindowInfo::operator==(const InputWindowInfo& info) const {
    return info.token == token && info.id == id && info.name == name && info.flags == flags &&
            info.type == type && info.dispatchingTimeout == dispatchingTimeout &&
            info.frameLeft == frameLeft && info.frameTop == frameTop &&
            info.frameRight == frameRight && info.frameBottom == frameBottom &&
            info.surfaceInset == surfaceInset && info.globalScaleFactor == globalScaleFactor &&
            info.windowXScale == windowXScale && info.windowYScale == windowYScale &&
            info.touchableRegion.hasSameRects(touchableRegion) && info.visible == visible &&
            info.canReceiveKeys == canReceiveKeys && info.trustedOverlay == trustedOverlay &&
            info.hasFocus == hasFocus && info.hasWallpaper == hasWallpaper &&
            info.paused == paused && info.ownerPid == ownerPid && info.ownerUid == ownerUid &&
            info.inputFeatures == inputFeatures && info.displayId == displayId &&
            info.portalToDisplayId == portalToDisplayId &&
            info.replaceTouchableRegionWithCrop == replaceTouchableRegionWithCrop &&
            info.applicationInfo.name == applicationInfo.name &&
            info.applicationInfo.token == applicationInfo.token &&
            info.applicationInfo.dispatchingTimeout == applicationInfo.dispatchingTimeout;
}

status_t InputWindowInfo::writeToParcel(android::Parcel* parcel) const {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }
    if (name.empty()) {
        parcel->writeInt32(0);
        return OK;
    }
    parcel->writeInt32(1);

    status_t status = parcel->writeStrongBinder(token) ?:
        parcel->writeInt64(dispatchingTimeout.count()) ?:
        parcel->writeInt32(id) ?:
        parcel->writeUtf8AsUtf16(name) ?:
        parcel->writeInt32(flags.get()) ?:
        parcel->writeInt32(static_cast<std::underlying_type_t<InputWindowInfo::Type>>(type)) ?:
        parcel->writeInt32(frameLeft) ?:
        parcel->writeInt32(frameTop) ?:
        parcel->writeInt32(frameRight) ?:
        parcel->writeInt32(frameBottom) ?:
        parcel->writeInt32(surfaceInset) ?:
        parcel->writeFloat(globalScaleFactor) ?:
        parcel->writeFloat(windowXScale) ?:
        parcel->writeFloat(windowYScale) ?:
        parcel->writeBool(visible) ?:
        parcel->writeBool(canReceiveKeys) ?:
        parcel->writeBool(hasFocus) ?:
        parcel->writeBool(hasWallpaper) ?:
        parcel->writeBool(paused) ?:
        parcel->writeBool(trustedOverlay) ?:
        parcel->writeInt32(ownerPid) ?:
        parcel->writeInt32(ownerUid) ?:
        parcel->writeInt32(inputFeatures.get()) ?:
        parcel->writeInt32(displayId) ?:
        parcel->writeInt32(portalToDisplayId) ?:
        applicationInfo.writeToParcel(parcel) ?:
        parcel->write(touchableRegion) ?:
        parcel->writeBool(replaceTouchableRegionWithCrop) ?:
        parcel->writeStrongBinder(touchableRegionCropHandle.promote());

    return status;
}

status_t InputWindowInfo::readFromParcel(const android::Parcel* parcel) {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }
    if (parcel->readInt32() == 0) {
        return OK;
    }

    token = parcel->readStrongBinder();
    dispatchingTimeout = static_cast<decltype(dispatchingTimeout)>(parcel->readInt64());
    status_t status = parcel->readInt32(&id) ?: parcel->readUtf8FromUtf16(&name);
    if (status != OK) {
        return status;
    }

    flags = Flags<Flag>(parcel->readInt32());
    type = static_cast<Type>(parcel->readInt32());
    status = parcel->readInt32(&frameLeft) ?:
        parcel->readInt32(&frameTop) ?:
        parcel->readInt32(&frameRight) ?:
        parcel->readInt32(&frameBottom) ?:
        parcel->readInt32(&surfaceInset) ?:
        parcel->readFloat(&globalScaleFactor) ?:
        parcel->readFloat(&windowXScale) ?:
        parcel->readFloat(&windowYScale) ?:
        parcel->readBool(&visible) ?:
        parcel->readBool(&canReceiveKeys) ?:
        parcel->readBool(&hasFocus) ?:
        parcel->readBool(&hasWallpaper) ?:
        parcel->readBool(&paused) ?:
        parcel->readBool(&trustedOverlay) ?:
        parcel->readInt32(&ownerPid) ?:
        parcel->readInt32(&ownerUid);

    if (status != OK) {
        return status;
    }

    inputFeatures = Flags<Feature>(parcel->readInt32());
    status = parcel->readInt32(&displayId) ?:
        parcel->readInt32(&portalToDisplayId) ?:
        applicationInfo.readFromParcel(parcel) ?:
        parcel->read(touchableRegion) ?:
        parcel->readBool(&replaceTouchableRegionWithCrop);

    if (status != OK) {
        return status;
    }

    touchableRegionCropHandle = parcel->readStrongBinder();

    return OK;
}

// --- InputWindowHandle ---

InputWindowHandle::InputWindowHandle() {}

InputWindowHandle::~InputWindowHandle() {}

InputWindowHandle::InputWindowHandle(const InputWindowHandle& other) : mInfo(other.mInfo) {}

InputWindowHandle::InputWindowHandle(const InputWindowInfo& other) : mInfo(other) {}

status_t InputWindowHandle::writeToParcel(android::Parcel* parcel) const {
    return mInfo.writeToParcel(parcel);
}

status_t InputWindowHandle::readFromParcel(const android::Parcel* parcel) {
    return mInfo.readFromParcel(parcel);
}

void InputWindowHandle::releaseChannel() {
    mInfo.token.clear();
}

sp<IBinder> InputWindowHandle::getToken() const {
    return mInfo.token;
}

void InputWindowHandle::updateFrom(sp<InputWindowHandle> handle) {
    mInfo = handle->mInfo;
}
} // namespace android

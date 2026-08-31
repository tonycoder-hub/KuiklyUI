/*
 * Tencent is pleased to support the open source community by making KuiklyUI
 * available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 * Licensed under the License of KuiklyUI;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://github.com/Tencent-TDS/KuiklyUI/blob/main/LICENSE
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "KRFrameMonitor.h"
#include "libohos_render/utils/KRRenderLoger.h"

const char KRFrameMonitor::kMonitorName[] = "KRFrameMonitor";
constexpr char kTag[] = "KRFrameMonitor";

KRFrameMonitor::KRFrameMonitor() {
    native_vsync_ = OH_NativeVSync_Create(kMonitorName, strlen(kMonitorName));
}

KRFrameMonitor::~KRFrameMonitor() {
    Stop();
    if (native_vsync_) {
        OH_NativeVSync_Destroy(native_vsync_);
        native_vsync_ = nullptr;
    }
}

void KRFrameMonitor::OnFirstFramePaint() {
    Start();
}

void KRFrameMonitor::Start() {
    KR_LOG_INFO_WITH_TAG(kTag) << "Start";
    if (!flags_.Start()) {
        KR_LOG_INFO_WITH_TAG(kTag) << "has start before.";
        return;
    }
    frame_data_.Reset();
    RequestNextVSync();
}

void KRFrameMonitor::Stop() {
    if (!flags_.is_started) {
        KR_LOG_INFO_WITH_TAG(kTag) << "stop, not start yet.";
        return;
    }
    flags_.Stop();
}

void KRFrameMonitor::OnResume() {
    if (!flags_.OnResume()) {
        KR_LOG_INFO_WITH_TAG(kTag) << "resume, isStarted: " << flags_.is_started << "isResumed: " << flags_.is_resumed;
        return;
    }
    RequestNextVSync();
}

void KRFrameMonitor::OnPause() {
    if (!flags_.OnPause()) {
        KR_LOG_INFO_WITH_TAG(kTag) << "pause, isStarted: " << flags_.is_started << "isResumed: " << flags_.is_resumed;
        return;
    }
    // flags_.OnPause() cleared last_frame_time_nanos and is_resumed.
    // RequestNextVSync is gated by ShouldRequestFrames(), so frames stop.
}

void KRFrameMonitor::OnDestroy() {
    Stop();
}

std::string KRFrameMonitor::GetMonitorData() {
    return frame_data_.ToJSONString();
}

void KRFrameMonitor::RequestNextVSync() {
    if (native_vsync_ && flags_.ShouldRequestFrames()) {
        OH_NativeVSync_RequestFrame(native_vsync_, &KRFrameMonitor::OnVSync, this);
    }
}

void KRFrameMonitor::OnVSync(long long timestamp, void* data) {
    auto* monitor = static_cast<KRFrameMonitor*>(data);
    if (!monitor || !monitor->flags_.ShouldRequestFrames()) {
        return;
    }
    if (monitor->flags_.last_frame_time_nanos > 0) {
        long long frameDuration = timestamp - monitor->flags_.last_frame_time_nanos;
        // 1. 累计总耗时
        long long frameDurationMillis = frameDuration / ONE_MILLI_SECOND_IN_NANOS;
        monitor->frame_data_.total_duration += frameDuration / ONE_MILLI_SECOND_IN_NANOS;
        monitor->frame_data_.frame_count++;
        // 2. 计算掉帧/卡顿
        // 如果一帧超过 16.6ms，多出来的部分记为卡顿
        long long hitches = 0;
        if (frameDuration > FRAME_INTERVAL_NANOS) {
            hitches = (frameDuration - FRAME_INTERVAL_NANOS) / ONE_MILLI_SECOND_IN_NANOS;
            monitor->frame_data_.hitches_duration += hitches;
        }

    }
    monitor->flags_.last_frame_time_nanos = timestamp;
    // 请求下一帧
    monitor->RequestNextVSync();
}
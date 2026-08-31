/*
 * Tencent is pleased to support the open source community by making KuiklyUI
 * available.
 * Copyright (C) 2025 Tencent. All rights reserved.
 * Licensed under the License of KuiklyUI;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * https://github.com/Tencent-TDS/KuiklyUI/blob/main/LICENSE
 * Unless required by applicable law or agreed to in writing, software
    10| * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CORE_RENDER_OHOS_KRFRAMEMONITORFLAGS_H
#define CORE_RENDER_OHOS_KRFRAMEMONITORFLAGS_H

// Header-only Start/OnResume/OnPause flag guards for KRFrameMonitor.
// Harmony NativeVSync is not required so host tests can compile this.
class KRFrameMonitorFlags {
public:
    bool is_started = false;
    bool is_resumed = false;
    long long last_frame_time_nanos = 0;

    // Returns true if start actually transitioned (caller should request frames).
    bool Start() {
        if (is_started) {
            return false;
        }
        is_started = true;
        is_resumed = true;
        last_frame_time_nanos = 0;
        return true;
    }

    void Stop() {
        if (!is_started) {
            return;
        }
        is_started = false;
        is_resumed = false;
        last_frame_time_nanos = 0;
    }

    // Returns true if resume actually transitioned (caller should request frames).
    bool OnResume() {
        if (!is_started || is_resumed) {
            return false;
        }
        is_resumed = true;
        last_frame_time_nanos = 0;
        return true;
    }

    // Inverse of OnResume: skip when not started or already paused.
    // Leftover polarity was `is_resumed` (same as OnResume), so Start then
    // OnPause returned early and never cleared is_resumed.
    // Returns true if pause took effect (caller should stop requesting frames).
    bool OnPause() {
        if (!is_started || !is_resumed) {
            return false;
        }
        last_frame_time_nanos = 0;
        is_resumed = false;
        return true;
    }

    bool ShouldRequestFrames() const {
        return is_started && is_resumed;
    }
};

#endif  // CORE_RENDER_OHOS_KRFRAMEMONITORFLAGS_H

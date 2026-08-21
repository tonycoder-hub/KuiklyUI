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

#ifndef CORE_RENDER_OHOS_KRTHREADCREATE_H
#define CORE_RENDER_OHOS_KRTHREADCREATE_H

#include <errno.h>
#include <pthread.h>
#include <cstddef>

// OpenHarmony 默认工作线程栈约 132KiB，Compose 深层 measure/place 会栈溢出。
// 与 Android KRThreadAdapter 建议一致，context worker 显式使用 8MiB 栈。
inline constexpr size_t kKRContextWorkerStackSize = static_cast<size_t>(8) * 1024 * 1024;

// 创建 joinable 的 context worker：pthread_attr_setstacksize(8MiB) + pthread_create。
// 返回 pthread_create / attr 的 errno（0 成功）。调用方负责 pthread_join。
inline int KRCreateContextWorkerThread(pthread_t *thread, void *(*start_routine)(void *), void *arg) {
    if (thread == nullptr || start_routine == nullptr) {
        return EINVAL;
    }
    pthread_attr_t attr;
    int err = pthread_attr_init(&attr);
    if (err != 0) {
        return err;
    }
    err = pthread_attr_setstacksize(&attr, kKRContextWorkerStackSize);
    if (err != 0) {
        pthread_attr_destroy(&attr);
        return err;
    }
    err = pthread_create(thread, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    return err;
}

#endif  // CORE_RENDER_OHOS_KRTHREADCREATE_H

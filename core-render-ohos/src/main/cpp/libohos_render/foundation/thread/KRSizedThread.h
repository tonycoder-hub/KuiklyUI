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

#ifndef CORE_RENDER_OHOS_KRSIZEDTHREAD_H
#define CORE_RENDER_OHOS_KRSIZEDTHREAD_H

#include <errno.h>
#include <pthread.h>
#include <cstddef>
#include <exception>
#include <system_error>
#include <type_traits>
#include <utility>

// OpenHarmony 默认工作线程栈约 132KiB，Compose 深层 measure/place 会栈溢出。
// 与 Android KRThreadAdapter 建议一致，context worker 显式使用 8MiB 栈。
inline constexpr size_t kKRContextWorkerStackSize = static_cast<size_t>(8) * 1024 * 1024;

// drop-in 替换 std::thread：API 对齐 construct / join / joinable / native_handle / 移动语义。
// 析构不自动 join（与 std::thread 一致：仍 joinable 则 terminate）。
// 栈大小封装在内部，调用方按 std::thread 用法写即可。
class KRSizedThread {
 public:
    using native_handle_type = pthread_t;

    KRSizedThread() noexcept = default;

    template <typename Callable, typename = typename std::enable_if<
                                     !std::is_same<typename std::decay<Callable>::type, KRSizedThread>::value>::type>
    explicit KRSizedThread(Callable &&fn) {
        using Decayed = typename std::decay<Callable>::type;
        auto *task = new Task<Decayed>{std::forward<Callable>(fn)};
        int err = CreateWithStack(&thread_, &Task<Decayed>::Run, task);
        if (err != 0) {
            delete task;
            throw std::system_error(err, std::generic_category(), "KRSizedThread pthread_create");
        }
        joinable_ = true;
    }

    ~KRSizedThread() {
        if (joinable_) {
            std::terminate();
        }
    }

    KRSizedThread(const KRSizedThread &) = delete;
    KRSizedThread &operator=(const KRSizedThread &) = delete;

    KRSizedThread(KRSizedThread &&other) noexcept : thread_(other.thread_), joinable_(other.joinable_) {
        other.joinable_ = false;
        other.thread_ = pthread_t{};
    }

    KRSizedThread &operator=(KRSizedThread &&other) noexcept {
        if (this != &other) {
            if (joinable_) {
                std::terminate();
            }
            thread_ = other.thread_;
            joinable_ = other.joinable_;
            other.joinable_ = false;
            other.thread_ = pthread_t{};
        }
        return *this;
    }

    bool joinable() const noexcept {
        return joinable_;
    }

    void join() {
        if (!joinable_) {
            throw std::system_error(EINVAL, std::generic_category(), "KRSizedThread::join");
        }
        int err = pthread_join(thread_, nullptr);
        joinable_ = false;
        thread_ = pthread_t{};
        if (err != 0) {
            throw std::system_error(err, std::generic_category(), "KRSizedThread pthread_join");
        }
    }

    native_handle_type native_handle() {
        return thread_;
    }

 private:
    template <typename Callable>
    struct Task {
        Callable fn;
        static void *Run(void *arg) {
            auto *task = static_cast<Task *>(arg);
            Callable fn = std::move(task->fn);
            delete task;
            fn();
            return nullptr;
        }
    };

    static int CreateWithStack(pthread_t *thread, void *(*start_routine)(void *), void *arg) {
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

    pthread_t thread_{};
    bool joinable_{false};
};

#endif  // CORE_RENDER_OHOS_KRSIZEDTHREAD_H

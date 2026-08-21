// Host unit test: context worker stack is at least 8 MiB.
//
// Proves KRSizedThread (drop-in std::thread used by KRThread) creates a
// live pthread whose stack, as reported by pthread_getattr_np +
// pthread_attr_getstacksize, is >= 8 MiB.
//
// No Harmony device. Links only pthread.
//
// Compile + run:
//   ./run_krthread_stack.sh
//
// The runner applies a small RLIMIT_STACK (ulimit -s 256) so a default
// pthread / old std::thread worker is well below 8 MiB. That is how this
// test fails before the fix and passes after.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "libohos_render/foundation/thread/KRSizedThread.h"

#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

namespace {

constexpr size_t kMinWorkerStack = static_cast<size_t>(8) * 1024 * 1024;

struct Park {
    std::mutex mu;
    std::condition_variable cv;
    bool ready = false;
    bool stop = false;
};

void ParkMain(Park *park) {
    {
        std::lock_guard<std::mutex> lock(park->mu);
        park->ready = true;
    }
    park->cv.notify_all();
    std::unique_lock<std::mutex> lock(park->mu);
    park->cv.wait(lock, [park]() { return park->stop; });
}

void *ParkPthread(void *arg) {
    ParkMain(static_cast<Park *>(arg));
    return nullptr;
}

void Unpark(Park *park) {
    {
        std::lock_guard<std::mutex> lock(park->mu);
        park->stop = true;
    }
    park->cv.notify_all();
}

void WaitReady(Park *park) {
    std::unique_lock<std::mutex> lock(park->mu);
    park->cv.wait(lock, [park]() { return park->ready; });
}

bool ReadLiveStackSize(pthread_t thread, size_t *out_size, std::string *err) {
    pthread_attr_t attr;
    int rc = pthread_getattr_np(thread, &attr);
    if (rc != 0) {
        *err = "pthread_getattr_np failed, err=" + std::to_string(rc);
        return false;
    }
    size_t size = 0;
    rc = pthread_attr_getstacksize(&attr, &size);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        *err = "pthread_attr_getstacksize failed, err=" + std::to_string(rc);
        return false;
    }
    *out_size = size;
    return true;
}

int Fail(const char *msg) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

}  // namespace

int main() {
    static_assert(kKRContextWorkerStackSize == kMinWorkerStack,
                  "kKRContextWorkerStackSize must be 8 * 1024 * 1024");

    // Control: default pthread_create (same as old std::thread worker).
    Park default_park;
    pthread_t default_thread{};
    int rc = pthread_create(&default_thread, nullptr, &ParkPthread, &default_park);
    if (rc != 0) {
        return Fail(("default pthread_create failed, err=" + std::to_string(rc)).c_str());
    }
    WaitReady(&default_park);
    std::string err;
    size_t default_stack = 0;
    if (!ReadLiveStackSize(default_thread, &default_stack, &err)) {
        Unpark(&default_park);
        pthread_join(default_thread, nullptr);
        return Fail(err.c_str());
    }
    Unpark(&default_park);
    pthread_join(default_thread, nullptr);

    // Production path: same construct / join usage as KRThread + std::thread.
    Park worker_park;
    KRSizedThread worker([&worker_park]() { ParkMain(&worker_park); });
    WaitReady(&worker_park);
    size_t worker_stack = 0;
    if (!ReadLiveStackSize(worker.native_handle(), &worker_stack, &err)) {
        Unpark(&worker_park);
        worker.join();
        return Fail(err.c_str());
    }
    Unpark(&worker_park);
    worker.join();

    std::printf("default pthread stack: %zu bytes (%.2f KiB)\n", default_stack,
                static_cast<double>(default_stack) / 1024.0);
    std::printf("KRSizedThread stack: %zu bytes (%.2f KiB)\n", worker_stack,
                static_cast<double>(worker_stack) / 1024.0);
    std::printf("required minimum: %zu bytes (8192.00 KiB)\n", kMinWorkerStack);

    if (worker_stack < kMinWorkerStack) {
        std::fprintf(stderr,
                     "FAIL: context worker stack %zu < 8 MiB; "
                     "OpenHarmony Compose layout will SIGSEGV\n",
                     worker_stack);
        return 1;
    }

    // When the runner applies a small RLIMIT_STACK, the old std::thread path
    // would land on this default size and fail the 8 MiB check above.
    if (default_stack >= kMinWorkerStack) {
        std::fprintf(stderr,
                     "WARN: default pthread stack is already >= 8 MiB "
                     "(RLIMIT_STACK not reduced); worker check still passed.\n");
    } else {
        std::printf("default stack is below 8 MiB, so the old std::thread path would fail.\n");
    }

    std::printf("PASS: live context worker stack is at least 8 MiB\n");
    return 0;
}

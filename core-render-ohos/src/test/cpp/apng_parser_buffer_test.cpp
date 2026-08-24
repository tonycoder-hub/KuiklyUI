// Host unit test for leftover ApngParser subBuffer / fdAT / eachChunk bounds.
//
// subBuffer did vector(begin+start, begin+start+length) with no range check.
// fdAT did size_t(length - 4): length 0/2 underflows to a huge copy / crash.
// eachChunk advanced off += 12+length without ensuring the chunk fits.
//
// Helpers live in header-only ApngParserBuffer.h so this compiles without
// Harmony / ArkUI. Build + run (from this directory):
//   ./run_apng_parser_buffer_test.sh
//   ./run_apng_parser_buffer_test.sh asan

#include "ApngParserBuffer.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

static int g_failed = 0;

static void expect_true(const char *name, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++g_failed;
    } else {
        std::printf("PASS %s\n", name);
    }
}

static bool subBufferThrows(const std::vector<uint8_t> &bytes, size_t start, size_t length) {
    try {
        (void)subBuffer(bytes, start, length);
        return false;
    } catch (const std::out_of_range &) {
        return true;
    }
}

static std::vector<uint8_t> MakeBeU32(uint32_t value) {
    return {static_cast<uint8_t>((value >> 24) & 0xFF), static_cast<uint8_t>((value >> 16) & 0xFF),
            static_cast<uint8_t>((value >> 8) & 0xFF), static_cast<uint8_t>(value & 0xFF)};
}

static std::vector<uint8_t> MakePngPrefix() {
    return {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
}

int main() {
    const std::vector<uint8_t> bytes = {0, 1, 2, 3, 4};

    // leftover: in-range slice still copies
    {
        auto got = subBuffer(bytes, 1, 3);
        expect_true("subBuffer(1,3) size", got.size() == 3 && got[0] == 1 && got[2] == 3);
        expect_true("SubBufferInRange(1,3)", SubBufferInRange(bytes.size(), 1, 3));
    }

    // leftover: start > size / length > size-start must reject (no OOB iterator)
    {
        expect_true("subBuffer start > size throws", subBufferThrows(bytes, 6, 1));
        expect_true("subBuffer start == size+1 throws", subBufferThrows(bytes, bytes.size() + 1, 0));
        expect_true("subBuffer length > size-start throws", subBufferThrows(bytes, 3, 3));
        expect_true("SubBufferInRange start > size", !SubBufferInRange(bytes.size(), 6, 1));
        expect_true("SubBufferInRange length OOB", !SubBufferInRange(bytes.size(), 3, 3));
    }

    // leftover: readString off+i was unchecked
    {
        bool threw = false;
        try {
            (void)readString(bytes, 4, 4);
        } catch (const std::out_of_range &) {
            threw = true;
        }
        expect_true("readString OOB throws", threw);
        expect_true("readString in-range", readString(bytes, 1, 3) == "\x01\x02\x03");
    }

    // leftover: fdAT length=0 / length=2 must reject without size_t underflow
    {
        // off=0, need at least 12 bytes of chunk header+seq so start=12 is defined.
        std::vector<uint8_t> chunk(16, 0xAB);
        std::vector<uint8_t> out = {0xFF};

        expect_true("tryFdATPayload length=0 rejects", !tryFdATPayload(chunk, 0, 0, out) && out.empty());
        out = {0xFF};
        expect_true("tryFdATPayload length=2 rejects", !tryFdATPayload(chunk, 0, 2, out) && out.empty());

        // length=4 → empty payload after dropping the sequence number
        expect_true("tryFdATPayload length=4 empty payload",
                    tryFdATPayload(chunk, 0, 4, out) && out.empty());

        // length=6 → two payload bytes at off+12
        chunk[12] = 0x11;
        chunk[13] = 0x22;
        expect_true("tryFdATPayload length=6 payload",
                    tryFdATPayload(chunk, 0, 6, out) && out.size() == 2 && out[0] == 0x11 && out[1] == 0x22);
    }

    // leftover: eachChunk must stop when off+8 > size (truncated after signature)
    {
        auto short_png = MakePngPrefix();
        short_png.insert(short_png.end(), {0x00, 0x00});  // only 2 of 8 header bytes
        int calls = 0;
        eachChunk(short_png, [&](const std::string &, std::vector<uint8_t> &, size_t, size_t) {
            ++calls;
            return true;
        });
        expect_true("eachChunk off+8 > size stops", calls == 0);
    }

    // leftover: eachChunk must stop when 12+length would overflow past size
    {
        auto truncated = MakePngPrefix();
        auto len = MakeBeU32(8);  // claims 8 data bytes
        truncated.insert(truncated.end(), len.begin(), len.end());
        truncated.insert(truncated.end(), {'I', 'D', 'A', 'T'});
        truncated.insert(truncated.end(), {0x01, 0x02});  // only 2 of 8+4
        int calls = 0;
        eachChunk(truncated, [&](const std::string &, std::vector<uint8_t> &, size_t, size_t) {
            ++calls;
            return true;
        });
        expect_true("eachChunk 12+length past size stops", calls == 0);
    }

    // leftover: a complete empty IEND chunk is still visited
    {
        auto iend = MakePngPrefix();
        auto len = MakeBeU32(0);
        iend.insert(iend.end(), len.begin(), len.end());
        iend.insert(iend.end(), {'I', 'E', 'N', 'D'});
        iend.insert(iend.end(), {0, 0, 0, 0});  // crc
        int calls = 0;
        std::string seen;
        eachChunk(iend, [&](const std::string &type, std::vector<uint8_t> &, size_t, size_t length) {
            ++calls;
            seen = type;
            return length == 0;
        });
        expect_true("eachChunk complete IEND", calls == 1 && seen == "IEND");
    }

    if (g_failed != 0) {
        std::fprintf(stderr, "%d test(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}

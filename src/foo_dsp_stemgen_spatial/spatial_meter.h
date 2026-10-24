#pragma once
#include "stdafx.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <windows.h>

namespace spatial_meter {

// Legacy/current snapshot protocol. Keep this byte-for-byte compatible with
// compatible snapshot readers.
constexpr std::uint32_t kMagic = 0x4D373153U; // "S17M"
constexpr std::uint32_t kVersion = 3U;
constexpr wchar_t kMappingName[] = L"Local\\StemgenRTSpatial71Meter";

constexpr std::uint32_t kFlagFrontStereo = 1U << 0;
constexpr std::uint32_t kFlagFallback = 1U << 1;
constexpr std::uint32_t kFlagAiActive = 1U << 2;
constexpr std::uint32_t kFlagBypass = 1U << 3;
constexpr std::uint32_t kFlagNativeMultich = 1U << 4;
constexpr std::uint32_t kFlagSyncAdapter = 1U << 5;
constexpr std::uint32_t kFlagSrcActive = 1U << 6;

struct SharedSpatialMeter {
    std::uint32_t magic;
    std::uint32_t version;
    volatile LONG sequence;

    std::uint32_t channels;
    std::uint32_t sampleRate;
    std::uint32_t blockSize;
    std::uint32_t flags;

    float peak[8];
    float rms[8];

    std::uint64_t underrunSamples;
    std::uint64_t dueBoundaryMisses;
    std::uint64_t queueFullDrops;
    std::uint64_t ringOverflowSamples;

    std::uint32_t ringFill;
    std::uint32_t activeLatency;
    std::uint32_t requiredLatency;
    std::uint32_t preparedBlock;

    std::uint32_t inferenceUs;
    std::uint32_t maxInferenceUs;
    std::uint32_t providerCode;
};
static_assert(sizeof(SharedSpatialMeter) == 160U);

// Timeline protocol for smooth visualization.
//
// Foobar may hand the DSP ~1 second of PCM at once. The synchronous DSP then
// computes hundreds of 128-sample hops in a short burst while output playback
// remains buffered and continuous. A single "latest value" snapshot therefore
// looks frozen between host chunks. Preserve every hop here; the viewer drains
// this ring at audio time so the display follows the buffered playback
// timeline instead of the host's bursty callback cadence.
constexpr std::uint32_t kRingMagic = 0x524D3731U; // private meter-ring magic
constexpr std::uint32_t kRingVersion = 1U;
constexpr wchar_t kRingMappingName[] =
    L"Local\\StemgenRTSpatial71MeterRing";
constexpr std::uint32_t kMeterHopFrames = 128U;
constexpr std::uint32_t kMeterRingCapacity = 2048U;

struct MeterRingEntry {
    volatile LONG sequence;
    std::uint32_t frames;
    std::uint32_t sampleRate;
    std::uint32_t flags;
    std::uint32_t inferenceUs;
    std::uint32_t maxInferenceUs;
    std::uint32_t providerCode;
    std::uint32_t channels;
    float peak[8];
    float rms[8];
};
static_assert(sizeof(MeterRingEntry) == 96U);

struct alignas(8) SharedMeterRing {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint32_t capacity;
    std::uint32_t hopFrames;
    volatile LONG64 writeCounter;
    volatile LONG streamEpoch;
    std::uint32_t reserved;
    MeterRingEntry entries[kMeterRingCapacity];
};
static_assert(offsetof(SharedMeterRing, entries) == 32U);

inline int displaySlot(unsigned channelFlag) noexcept {
    switch (channelFlag) {
    case audio_chunk::channel_front_left:   return 0;
    case audio_chunk::channel_front_right:  return 1;
    case audio_chunk::channel_front_center: return 2;
    case audio_chunk::channel_lfe:          return 3;
    case audio_chunk::channel_side_left:    return 4;
    case audio_chunk::channel_side_right:   return 5;
    case audio_chunk::channel_back_left:    return 6;
    case audio_chunk::channel_back_right:   return 7;
    default: return -1;
    }
}

class Publisher {
public:
    Publisher() { ensure(); }

    ~Publisher() {
        if (ringState_ != nullptr) {
            UnmapViewOfFile(ringState_);
            ringState_ = nullptr;
        }
        if (ringMapping_ != nullptr) {
            CloseHandle(ringMapping_);
            ringMapping_ = nullptr;
        }
        if (state_ != nullptr) {
            UnmapViewOfFile(state_);
            state_ = nullptr;
        }
        if (mapping_ != nullptr) {
            CloseHandle(mapping_);
            mapping_ = nullptr;
        }
    }

    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    void ensure() noexcept {
        ensureSnapshot();
        ensureRing();
    }

    void resetTimeline() noexcept {
        ensureRing();
        if (ringState_ == nullptr) return;

        InterlockedIncrement(&ringState_->streamEpoch);
        MemoryBarrier();
        writeCounter_ = 0;
        InterlockedExchange64(&ringState_->writeCounter, 0);
    }

    void publishInterleaved(const audio_sample* data,
                            size_t frames,
                            unsigned channels,
                            unsigned channelConfig,
                            unsigned sampleRate,
                            std::uint32_t flags,
                            std::uint32_t activeLatency,
                            std::uint32_t requiredLatency,
                            std::uint32_t preparedBlock,
                            std::uint32_t inferenceUs,
                            std::uint32_t maxInferenceUs,
                            std::uint32_t providerCode) noexcept {
        ensure();
        if (state_ == nullptr || data == nullptr || frames == 0 || channels == 0)
            return;

        std::array<float, 8> aggregatePeak{};
        std::array<double, 8> aggregateSquares{};
        std::array<bool, 8> aggregatePresent{};

        size_t offset = 0;
        while (offset < frames) {
            const size_t segmentFrames =
                std::min<size_t>(kMeterHopFrames, frames - offset);
            std::array<float, 8> peak{};
            std::array<double, 8> squares{};
            std::array<bool, 8> present{};

            for (unsigned srcCh = 0; srcCh < channels; ++srcCh) {
                const unsigned flag =
                    audio_chunk::g_extract_channel_flag(channelConfig, srcCh);
                const int dst = displaySlot(flag);
                if (dst < 0) continue;
                const size_t d = static_cast<size_t>(dst);
                present[d] = true;
                aggregatePresent[d] = true;

                float p = 0.0f;
                double ss = 0.0;
                for (size_t i = 0; i < segmentFrames; ++i) {
                    const size_t frameIndex = offset + i;
                    const float x0 =
                        static_cast<float>(data[frameIndex * channels + srcCh]);
                    const float x = std::isfinite(x0) ? x0 : 0.0f;
                    const float ax = x < 0.0f ? -x : x;
                    if (ax > p) p = ax;
                    ss += static_cast<double>(x) * static_cast<double>(x);
                }
                peak[d] = p;
                squares[d] = ss;
                aggregatePeak[d] = std::max(aggregatePeak[d], p);
                aggregateSquares[d] += ss;
            }

            std::array<float, 8> rms{};
            for (size_t i = 0; i < 8; ++i) {
                rms[i] = present[i]
                    ? static_cast<float>(
                          std::sqrt(squares[i] /
                                    static_cast<double>(segmentFrames)))
                    : 0.0f;
            }

            writeRingEntry(
                peak, rms, static_cast<std::uint32_t>(segmentFrames),
                sampleRate, flags, inferenceUs, maxInferenceUs, providerCode,
                channels);
            offset += segmentFrames;
        }

        InterlockedIncrement(&state_->sequence); // odd
        state_->channels = channels;
        state_->sampleRate = sampleRate;
        state_->blockSize = static_cast<std::uint32_t>(frames);
        state_->flags = flags;

        for (size_t i = 0; i < 8; ++i) {
            state_->peak[i] = aggregatePeak[i];
            state_->rms[i] = aggregatePresent[i]
                ? static_cast<float>(
                      std::sqrt(aggregateSquares[i] /
                                static_cast<double>(frames)))
                : 0.0f;
        }

        state_->underrunSamples = 0;
        state_->dueBoundaryMisses = 0;
        state_->queueFullDrops = 0;
        state_->ringOverflowSamples = 0;
        state_->ringFill = 0;
        state_->activeLatency = activeLatency;
        state_->requiredLatency = requiredLatency;
        state_->preparedBlock = preparedBlock;
        state_->inferenceUs = inferenceUs;
        state_->maxInferenceUs = maxInferenceUs;
        state_->providerCode = providerCode;
        InterlockedIncrement(&state_->sequence); // even
    }

private:
    void ensureSnapshot() noexcept {
        if (state_ != nullptr) return;

        mapping_ = CreateFileMappingW(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
            sizeof(SharedSpatialMeter), kMappingName);
        if (mapping_ == nullptr) return;

        state_ = static_cast<SharedSpatialMeter*>(
            MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0,
                          sizeof(SharedSpatialMeter)));
        if (state_ == nullptr) {
            CloseHandle(mapping_);
            mapping_ = nullptr;
            return;
        }

        std::memset(state_, 0, sizeof(SharedSpatialMeter));
        state_->magic = kMagic;
        state_->version = kVersion;
    }

    void ensureRing() noexcept {
        if (ringState_ != nullptr) return;

        ringMapping_ = CreateFileMappingW(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
            sizeof(SharedMeterRing), kRingMappingName);
        if (ringMapping_ == nullptr) return;

        ringState_ = static_cast<SharedMeterRing*>(
            MapViewOfFile(ringMapping_, FILE_MAP_ALL_ACCESS, 0, 0,
                          sizeof(SharedMeterRing)));
        if (ringState_ == nullptr) {
            CloseHandle(ringMapping_);
            ringMapping_ = nullptr;
            return;
        }

        LONG nextEpoch = 1;
        if (ringState_->magic == kRingMagic &&
            ringState_->version == kRingVersion) {
            nextEpoch = ringState_->streamEpoch + 1;
            if (nextEpoch <= 0) nextEpoch = 1;
        }

        std::memset(ringState_, 0, sizeof(SharedMeterRing));
        ringState_->magic = kRingMagic;
        ringState_->version = kRingVersion;
        ringState_->capacity = kMeterRingCapacity;
        ringState_->hopFrames = kMeterHopFrames;
        ringState_->streamEpoch = nextEpoch;
        ringState_->writeCounter = 0;
        writeCounter_ = 0;
    }

    void writeRingEntry(const std::array<float, 8>& peak,
                        const std::array<float, 8>& rms,
                        std::uint32_t frames,
                        std::uint32_t sampleRate,
                        std::uint32_t flags,
                        std::uint32_t inferenceUs,
                        std::uint32_t maxInferenceUs,
                        std::uint32_t providerCode,
                        std::uint32_t channels) noexcept {
        if (ringState_ == nullptr) return;

        const LONG64 next = writeCounter_ + 1;
        const size_t index =
            static_cast<size_t>(writeCounter_ %
                                static_cast<LONG64>(kMeterRingCapacity));
        MeterRingEntry& entry = ringState_->entries[index];

        InterlockedIncrement(&entry.sequence); // odd
        entry.frames = frames;
        entry.sampleRate = sampleRate;
        entry.flags = flags;
        entry.inferenceUs = inferenceUs;
        entry.maxInferenceUs = maxInferenceUs;
        entry.providerCode = providerCode;
        entry.channels = channels;
        for (size_t i = 0; i < 8; ++i) {
            entry.peak[i] = peak[i];
            entry.rms[i] = rms[i];
        }
        MemoryBarrier();
        InterlockedIncrement(&entry.sequence); // even
        MemoryBarrier();

        writeCounter_ = next;
        InterlockedExchange64(&ringState_->writeCounter, next);
    }

    HANDLE mapping_{nullptr};
    SharedSpatialMeter* state_{nullptr};

    HANDLE ringMapping_{nullptr};
    SharedMeterRing* ringState_{nullptr};
    LONG64 writeCounter_{0};
};

inline Publisher& publisher() {
    static Publisher instance;
    return instance;
}

} // namespace spatial_meter

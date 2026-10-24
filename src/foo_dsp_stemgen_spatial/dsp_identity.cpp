#include "stdafx.h"
#include "component_ids.h"
#include "spatial_meter.h"
#include <StemgenRT/OnnxRuntime.h>
#include <StemgenRT/StreamingSampleRateAdapter.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

namespace {

constexpr size_t kHop = 128;
constexpr unsigned kModelRate = 44100U;
constexpr double kRate = static_cast<double>(kModelRate);
constexpr float kHeadroom = 0.70710678f;

// StableMix C: stable front/Center geometry, modest AI ambience.
constexpr float kStableDrumsSide = 0.12f;
constexpr float kStableOtherSide = 0.20f;
constexpr float kStableOtherRear = 0.11f;

constexpr float kFrontMidRetain = 0.96f;
constexpr float kCenterMidGain = 0.39597980f;

// Fixed release scheduling: one ORT thread, no spinning, dedicated worker.
constexpr unsigned kPrebufferMs = 250;

std::filesystem::path componentDirectory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&componentDirectory), &module)) {
        return {};
    }

    std::array<wchar_t, 32768> path{};
    const DWORD n = GetModuleFileNameW(
        module, path.data(), static_cast<DWORD>(path.size()));
    if (n == 0 || n >= path.size()) return {};
    return std::filesystem::path(path.data()).parent_path();
}

// Runs one job at a time on a dedicated thread; the caller blocks until the
// job has finished. Because the caller always waits, the DSP state is never
// touched by two threads at once - the work simply happens on another
// thread, in the same order.
class InferenceWorker final {
public:
    explicit InferenceWorker(int priority)
        : thread_([this, priority] { loop(priority); }) {}

    ~InferenceWorker() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            quit_ = true;
        }
        wake_.notify_all();
        if (thread_.joinable()) thread_.join();
    }

    InferenceWorker(const InferenceWorker&) = delete;
    InferenceWorker& operator=(const InferenceWorker&) = delete;

    void run(const std::function<void()>& job) {
        std::exception_ptr failure;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            job_ = &job;
            done_ = false;
            wake_.notify_all();
            finished_.wait(lock, [this] { return done_; });
            job_ = nullptr;
            failure = failure_;
            failure_ = nullptr;
        }
        if (failure) std::rethrow_exception(failure);
    }

private:
    void loop(int priority) {
        SetThreadPriority(GetCurrentThread(), priority);

        std::unique_lock<std::mutex> lock(mutex_);
        for (;;) {
            wake_.wait(lock, [this] {
                return quit_ || (job_ != nullptr && !done_);
            });
            if (quit_) return;

            const std::function<void()>* job = job_;
            lock.unlock();
            std::exception_ptr failure;
            try {
                (*job)();
            } catch (...) {
                failure = std::current_exception();
            }
            lock.lock();
            failure_ = failure;
            done_ = true;
            finished_.notify_all();
        }
    }

    std::mutex mutex_;
    std::condition_variable wake_;
    std::condition_variable finished_;
    const std::function<void()>* job_{nullptr};
    bool done_{true};
    bool quit_{false};
    std::exception_ptr failure_{};
    std::thread thread_; // last: starts after the members above exist
};

} // namespace

class dsp_stemgen_spatial : public dsp_impl_base {
public:
    dsp_stemgen_spatial() {
        for (auto& ch : modelInput_) ch.resize(kHop);
        for (auto& ch : alignedInput_) ch.resize(kHop);
        for (auto& stem : stems_) {
            for (auto& ch : stem) ch.resize(kHop);
        }

        const auto dir = componentDirectory();
        const auto model = dir / L"model.onnx";
        juce::String error;
        runtimeReady_ =
            !dir.empty() && runtime_.isInitialized() &&
            runtime_.loadModel(
                juce::String(model.c_str()), error,
                1, false) &&
            runtime_.prepareForInference(error);

        if (runtimeReady_) {
            const auto provider = runtime_.getExecutionProvider();
            providerCode_ =
                provider.find("CUDA") != std::string::npos ? 1U : 0U;
        }

        if (runtimeReady_) {
            // A failed worker creation propagates; never move inference onto the host thread.
            worker_ = std::make_unique<InferenceWorker>(THREAD_PRIORITY_HIGHEST);
        }

        if (!runtimeReady_) {
            const std::string why = error.toRawUTF8();
            if (!why.empty()) {
                console::print(
                    (std::string("StemSpace: ") + why).c_str());
            }
        }

        spatial_meter::publisher().ensure();
    }

    static GUID g_get_guid() { return kStemgenSpatialDspGuid; }

    static void g_get_name(pfc::string_base& out) {
        out = "StemSpace";
    }

    bool on_chunk(audio_chunk* chunk, abort_callback&) override {
        if (chunk == nullptr) return false;
        const bool stereo =
            chunk->get_channels() == 2 &&
            chunk->get_channel_config() ==
                audio_chunk::channel_config_stereo;
        const unsigned rate = chunk->get_srate();
        const bool supportedStereo =
            stereo && isSupportedStereoRate(rate);

        if (!supportedStereo || !runtimeReady_) {
            if (streamActive_) drainBuffered();

            activeAiPath_ = false;
            publishBypass(*chunk);

            return true;
        }

        if (!rateConfigured_ || hostRate_ != rate) {
            if (streamActive_) drainBuffered();

            if (!configureRate(rate)) {
                activeAiPath_ = false;
                publishBypass(*chunk);

                return true;
            }
        }

        activeAiPath_ = true;
        streamActive_ = true;

        const auto* src = chunk->get_data();
        const size_t frames = chunk->get_sample_count();
        if (src == nullptr || frames == 0) return false;

        bool fed = false;
        runHeavy([&] { fed = convertAndFeed(src, frames); });
        if (!fed) return false;

        // Everything this input chunk produced leaves as ONE 7.1 chunk
        // (previously one tiny chunk per 128-sample model hop). At the start
        // of a stream the output is held until the pre-buffer is full.
        if (!prebuffering_ ||
            batchFrames_ >= prebufferTargetFrames()) {
            prebuffering_ = false;
            flushBatch();
        }

        return false;
    }

    void on_endofplayback(abort_callback&) override {
        if (streamActive_) drainBuffered();
        activeAiPath_ = false;
    }

    void on_endoftrack(abort_callback&) override {
    }

    void flush() override {
        batchFrames_ = 0; // insert_chunk is not allowed here; just drop.
        prebuffering_ = true;

        pendingCount_ = 0;
        haveSubmitted_ = false;
        streamActive_ = false;
        bridgeFailed_ = false;
        activeAiPath_ = false;

        if (rateConfigured_) {
            inputBridge_.reset();
            outputBridge_.reset();
        }

        spatial_meter::publisher().resetTimeline();

        if (runtimeReady_ && !runtimeFailed_) {
            runtime_.resetStreamingState();
        }
        runtimeFailed_ = false;
    }

    double get_latency() override {
        if (!runtimeReady_ || !activeAiPath_) return 0.0;
        const double base = rateConfigured_
            ? latencySeconds_
            : static_cast<double>(kHop) / kRate;
        // Audio still held back by the pre-buffer is DSP latency too.
        const double held =
            (batchFrames_ != 0 && batchRate_ != 0)
                ? static_cast<double>(batchFrames_) /
                      static_cast<double>(batchRate_)
                : 0.0;
        return base + held;
    }

    bool need_track_change_mark() override {
        return false;
    }

private:
    size_t prebufferTargetFrames() const noexcept {
        return static_cast<size_t>(
            static_cast<std::uint64_t>(kPrebufferMs) *
            hostRate_ / 1000U);
    }

    using StereoHop =
        std::array<std::array<float, kHop>, 2>;
    using ModelSpatial =
        std::array<std::array<float, kHop>, 8>;

    static float finiteSample(audio_sample v) noexcept {
        const float x = static_cast<float>(v);
        return std::isfinite(x) ? x : 0.0f;
    }

    static bool isSupportedStereoRate(
        unsigned rate) noexcept {
        return rate >=
                   audio_plugin::
                       StreamingSampleRateAdapter::
                           kMinimumSampleRate &&
               rate <=
                   audio_plugin::
                       StreamingSampleRateAdapter::
                           kMaximumSampleRate &&
               audio_plugin::
                   StreamingSampleRateAdapter::
                       isRatePairSupported(
                           static_cast<double>(rate),
                           kRate);
    }

    bool configureRate(unsigned rate) {
        if (!runtimeReady_ ||
            !isSupportedStereoRate(rate)) {
            return false;
        }

        audio_plugin::StreamingSampleRateAdapter
            newInput;
        audio_plugin::StreamingSampleRateAdapter
            baseOutput;
        audio_plugin::StreamingSampleRateAdapter
            newOutput;

        if (!newInput.prepare(
                static_cast<double>(rate),
                kRate, 2U)) {
            return false;
        }

        if (!baseOutput.prepare(
                kRate,
                static_cast<double>(rate),
                8U)) {
            return false;
        }

        // Make the complete synchronous bridge delay land
        // exactly on a host-sample boundary:
        // input FIR + one model hop + output FIR.
        const double naturalHostDelay =
            newInput.filterGroupDelaySeconds() *
                static_cast<double>(rate) +
            static_cast<double>(kHop) *
                static_cast<double>(rate) / kRate +
            baseOutput.filterGroupDelayOutputSamples();

        const double integralHostDelay =
            std::ceil(naturalHostDelay - 1.0e-9);
        const double correction =
            integralHostDelay - naturalHostDelay;

        if (!std::isfinite(correction) ||
            correction < -1.0e-9 ||
            correction >= 1.0) {
            return false;
        }

        if (!newOutput.prepare(
                kRate,
                static_cast<double>(rate),
                8U,
                std::max(0.0, correction))) {
            return false;
        }

        inputBridge_ = std::move(newInput);
        outputBridge_ = std::move(newOutput);

        hostRate_ = rate;
        rateConfigured_ = true;
        bridgeFailed_ = false;

        latencyHostSamples_ =
            static_cast<std::uint64_t>(
                integralHostDelay);
        latencySeconds_ =
            static_cast<double>(
                latencyHostSamples_) /
            static_cast<double>(hostRate_);

        pendingCount_ = 0;
        haveSubmitted_ = false;
        runtimeFailed_ = false;
        prebuffering_ = true;
        inputBridge_.reset();
        outputBridge_.reset();
        runtime_.resetStreamingState();
        spatial_meter::publisher().resetTimeline();

        return true;
    }

    bool hasStreamState() const noexcept {
        return streamActive_ ||
               pendingCount_ != 0 ||
               haveSubmitted_;
    }

    void copyToModel(const StereoHop& hop) {
        for (size_t ch = 0; ch < 2; ++ch) {
            for (size_t i = 0; i < kHop; ++i) {
                modelInput_[ch][i] = hop[ch][i];
            }
        }
    }

    void rememberSubmitted(
        const StereoHop& hop) noexcept {
        previousSubmitted_ = hop;
        haveSubmitted_ = true;
    }

    // Host-rate stereo -> model-rate hops -> 7.1 into the output batch.
    // Heavy part of on_chunk; runs on the worker thread (see runHeavy).
    bool convertAndFeed(
        const audio_sample* src,
        size_t frames) {
        for (auto& ch : hostInputScratch_) {
            ch.resize(frames);
        }

        for (size_t i = 0; i < frames; ++i) {
            hostInputScratch_[0][i] =
                finiteSample(src[i * 2 + 0]);
            hostInputScratch_[1][i] =
                finiteSample(src[i * 2 + 1]);
        }

        const size_t modelCapacity =
            inputBridge_.maxOutputForInput(frames);
        if (modelCapacity ==
            std::numeric_limits<size_t>::max()) {
            bridgeFailed_ = true;
            return false;
        }

        for (auto& ch : modelBridgeScratch_) {
            ch.resize(modelCapacity);
        }

        const std::array<const float*, 2> inPtrs = {
            hostInputScratch_[0].data(),
            hostInputScratch_[1].data()
        };
        const std::array<float*, 2> outPtrs = {
            modelBridgeScratch_[0].data(),
            modelBridgeScratch_[1].data()
        };

        const auto converted = inputBridge_.process(
            inPtrs.data(), frames,
            outPtrs.data(), modelCapacity);

        if (!converted.ok ||
            converted.inputConsumed != frames) {
            bridgeFailed_ = true;
            return false;
        }

        feedModelSamples(
            modelBridgeScratch_,
            converted.outputProduced);
        return true;
    }

    // Runs the job on StemSpace's own (non-MMCSS) thread and waits for it,
    void runHeavy(const std::function<void()>& job) {
        worker_->run(job);
    }

    void feedModelSamples(
        const std::array<std::vector<float>, 2>& data,
        size_t frames) {
        for (size_t i = 0; i < frames; ++i) {
            pending_[0][pendingCount_] =
                data[0][i];
            pending_[1][pendingCount_] =
                data[1][i];

            if (++pendingCount_ == kHop) {
                processHop(pending_, kHop);
                pendingCount_ = 0;
            }
        }
    }

    void processHop(
        const StereoHop& hop,
        size_t outputFrames) {
        if (runtimeFailed_) {
            if (haveSubmitted_) {
                emitFrontOnly(
                    previousSubmitted_,
                    outputFrames);
            }
            rememberSubmitted(hop);
            return;
        }

        copyToModel(hop);

        bool outputValid = false;
        const auto inferenceStart =
            std::chrono::steady_clock::now();

        const bool ok =
            runtime_.runInference(
                modelInput_, stems_,
                alignedInput_, outputValid);

        const auto inferenceEnd =
            std::chrono::steady_clock::now();

        const auto elapsedUs =
            std::chrono::duration_cast<
                std::chrono::microseconds>(
                    inferenceEnd -
                    inferenceStart).count();

        lastInferenceUs_ =
            elapsedUs > 0xFFFFFFFFLL
                ? 0xFFFFFFFFU
                : static_cast<std::uint32_t>(
                      elapsedUs < 0
                          ? 0
                          : elapsedUs);

        if (lastInferenceUs_ >
            maxInferenceUs_) {
            maxInferenceUs_ =
                lastInferenceUs_;
        }

        if (!ok) {
            runtimeFailed_ = true;
            if (haveSubmitted_) {
                emitFrontOnly(
                    previousSubmitted_,
                    outputFrames);
            }
            rememberSubmitted(hop);
            return;
        }

        if (outputValid) {
            emitSpatial(outputFrames);
        }

        rememberSubmitted(hop);
    }

    void emitSpatial(size_t frames) {
        ModelSpatial model{};

        for (size_t i = 0; i < frames; ++i) {
            const float mainL =
                alignedInput_[0][i];
            const float mainR =
                alignedInput_[1][i];

            const float drumsL =
                stems_[audio_plugin::kStemDrums]
                      [0][i];
            const float drumsR =
                stems_[audio_plugin::kStemDrums]
                      [1][i];

            const float otherL =
                stems_[audio_plugin::kStemOther]
                      [0][i];
            const float otherR =
                stems_[audio_plugin::kStemOther]
                      [1][i];

            const float mid =
                0.5f * (mainL + mainR);
            const float side =
                0.5f * (mainL - mainR);

            model[0][i] =
                kHeadroom *
                (kFrontMidRetain * mid + side);
            model[1][i] =
                kHeadroom *
                (kFrontMidRetain * mid - side);
            model[2][i] =
                kHeadroom *
                kCenterMidGain * mid;
            model[3][i] = 0.0f;

            // foobar canonical 7.1 order is
            // FL FR C LFE BL BR SL SR.
            model[4][i] =
                kHeadroom *
                kStableOtherRear * otherL;
            model[5][i] =
                kHeadroom *
                kStableOtherRear * otherR;
            model[6][i] =
                kHeadroom *
                (kStableDrumsSide * drumsL +
                 kStableOtherSide * otherL);
            model[7][i] =
                kHeadroom *
                (kStableDrumsSide * drumsR +
                 kStableOtherSide * otherR);
        }

        emitModelSpatial(
            model, frames,
            spatial_meter::kFlagAiActive |
                spatial_meter::kFlagSyncAdapter |
                (hostRate_ != kModelRate
                     ? spatial_meter::kFlagSrcActive
                     : 0U),
            lastInferenceUs_,
            maxInferenceUs_);
    }

    void emitFrontOnly(
        const StereoHop& hop,
        size_t frames) {
        ModelSpatial model{};

        for (size_t i = 0; i < frames; ++i) {
            model[0][i] =
                kHeadroom * hop[0][i];
            model[1][i] =
                kHeadroom * hop[1][i];
        }

        emitModelSpatial(
            model, frames,
            spatial_meter::kFlagFallback |
                spatial_meter::kFlagFrontStereo |
                spatial_meter::kFlagSyncAdapter |
                (hostRate_ != kModelRate
                     ? spatial_meter::kFlagSrcActive
                     : 0U),
            lastInferenceUs_,
            maxInferenceUs_);
    }

    void emitModelSpatial(
        const ModelSpatial& model,
        size_t frames,
        std::uint32_t flags,
        std::uint32_t inferenceUs,
        std::uint32_t maxInferenceUs) {

        if (!rateConfigured_ ||
            frames == 0) {
            return;
        }

        const size_t hostCapacity =
            outputBridge_.maxOutputForInput(
                frames);

        if (hostCapacity ==
            std::numeric_limits<size_t>::max()) {
            bridgeFailed_ = true;
            return;
        }

        for (auto& ch : hostSpatialScratch_) {
            ch.resize(hostCapacity);
        }

        std::array<const float*, 8> inPtrs{};
        std::array<float*, 8> outPtrs{};

        for (size_t ch = 0; ch < 8; ++ch) {
            inPtrs[ch] = model[ch].data();
            outPtrs[ch] =
                hostSpatialScratch_[ch].data();
        }

        const auto converted =
            outputBridge_.process(
                inPtrs.data(), frames,
                outPtrs.data(), hostCapacity);

        if (!converted.ok) {
            bridgeFailed_ = true;
            return;
        }

        const size_t produced =
            converted.outputProduced;
        if (produced == 0) return;

        interleavedOutput_.resize(
            produced * 8U);

        for (size_t i = 0; i < produced; ++i) {
            const size_t base = i * 8U;
            for (size_t ch = 0; ch < 8; ++ch) {
                interleavedOutput_[base + ch] =
                    static_cast<audio_sample>(
                        hostSpatialScratch_[ch][i]);
            }
        }

        spatial_meter::publisher().
            publishInterleaved(
                interleavedOutput_.data(),
                produced, 8,
                audio_chunk::
                    channel_config_7point1,
                hostRate_,
                flags,
                static_cast<std::uint32_t>(
                    std::min<std::uint64_t>(
                        latencyHostSamples_,
                        0xFFFFFFFFULL)),
                static_cast<std::uint32_t>(
                    std::min<std::uint64_t>(
                        latencyHostSamples_,
                        0xFFFFFFFFULL)),
                static_cast<std::uint32_t>(
                    produced),
                inferenceUs,
                maxInferenceUs,
                providerCode_);

        appendToBatch(
            interleavedOutput_.data(),
            produced);
    }

    void appendToBatch(
        const audio_sample* interleaved8,
        size_t frames) {
        if (frames == 0) return;
        if (batchFrames_ == 0) batchRate_ = hostRate_;

        const size_t offset = batchFrames_ * 8U;
        const size_t needed = offset + frames * 8U;
        if (batchOutput_.size() < needed) {
            batchOutput_.resize(needed);
        }
        std::copy(
            interleaved8,
            interleaved8 + frames * 8U,
            batchOutput_.begin() +
                static_cast<std::ptrdiff_t>(offset));
        batchFrames_ += frames;
    }

    // Emits everything queued since the last call as a single chunk, placed
    // before the chunk currently being processed (same order as before).
    // Must only run from on_chunk / on_endofplayback paths, never flush().
    void flushBatch() {
        if (batchFrames_ == 0) return;
        auto* c = insert_chunk(batchFrames_ * 8U);
        c->set_data(
            batchOutput_.data(),
            batchFrames_, 8,
            batchRate_,
            audio_chunk::
                channel_config_7point1);
        batchFrames_ = 0;
    }

    void flushInputBridgeTail() {
        if (!rateConfigured_ ||
            inputBridge_.isBypassed()) {
            return;
        }

        size_t remaining =
            inputBridge_.filterLength() + 2U;

        constexpr size_t kFlushBlock = 1024U;
        std::array<std::vector<float>, 2>
            zeros{
                std::vector<float>(
                    kFlushBlock, 0.0f),
                std::vector<float>(
                    kFlushBlock, 0.0f)
            };

        while (remaining > 0) {
            const size_t count =
                std::min(
                    remaining,
                    kFlushBlock);

            const size_t capacity =
                inputBridge_.
                    maxOutputForInput(
                        count);

            if (capacity ==
                std::numeric_limits<
                    size_t>::max()) {
                bridgeFailed_ = true;
                return;
            }

            for (auto& ch :
                 modelBridgeScratch_) {
                ch.resize(capacity);
            }

            const std::array<
                const float*, 2> inPtrs = {
                    zeros[0].data(),
                    zeros[1].data()
                };
            const std::array<float*, 2>
                outPtrs = {
                    modelBridgeScratch_[0].
                        data(),
                    modelBridgeScratch_[1].
                        data()
                };

            const auto result =
                inputBridge_.process(
                    inPtrs.data(), count,
                    outPtrs.data(),
                    capacity);

            if (!result.ok) {
                bridgeFailed_ = true;
                return;
            }

            feedModelSamples(
                modelBridgeScratch_,
                result.outputProduced);

            remaining -= count;
        }
    }

    void flushOutputBridgeTail() {
        if (!rateConfigured_ ||
            outputBridge_.isBypassed()) {
            return;
        }

        size_t remaining =
            outputBridge_.filterLength() + 2U;

        ModelSpatial zeros{};

        while (remaining > 0) {
            const size_t count =
                std::min<size_t>(
                    remaining, kHop);

            emitModelSpatial(
                zeros, count,
                spatial_meter::kFlagAiActive |
                    spatial_meter::
                        kFlagSyncAdapter |
                    spatial_meter::kFlagSrcActive,
                0U,
                maxInferenceUs_);

            remaining -= count;
        }
    }

    void drainModelPending() {
        if (pendingCount_ != 0) {
            const size_t partial =
                pendingCount_;

            for (size_t ch = 0;
                 ch < 2; ++ch) {
                for (size_t i = partial;
                     i < kHop; ++i) {
                    pending_[ch][i] = 0.0f;
                }
            }

            processHop(
                pending_, kHop);

            StereoHop zeros{};
            processHop(
                zeros, partial);
        } else if (haveSubmitted_) {
            StereoHop zeros{};
            processHop(
                zeros, kHop);
        }

        pendingCount_ = 0;
        haveSubmitted_ = false;
    }

    void drainBuffered() {
        if (!rateConfigured_) {
            pendingCount_ = 0;
            haveSubmitted_ = false;
            streamActive_ = false;
            return;
        }

        runHeavy([this] {
            // 1) Let the downsampling FIR release
            //    the end of the host-rate source.
            flushInputBridgeTail();

            // 2) Release the model's preceding-hop
            //    output and any partial model hop.
            drainModelPending();

            // 3) Release the final upsampling FIR tail.
            flushOutputBridgeTail();
        });

        // 4) Hand the whole drained tail (and anything still held by the
        //    pre-buffer) to foobar2000 as one chunk.
        flushBatch();
        prebuffering_ = true;

        inputBridge_.reset();
        outputBridge_.reset();

        pendingCount_ = 0;
        haveSubmitted_ = false;
        streamActive_ = false;

        if (!runtimeFailed_) {
            runtime_.resetStreamingState();
        }
        runtimeFailed_ = false;
    }

    void publishBypass(
        const audio_chunk& chunk) {
        std::uint32_t flags =
            spatial_meter::kFlagBypass |
            spatial_meter::kFlagSyncAdapter;

        if (chunk.get_channels() == 2 &&
            chunk.get_channel_config() ==
                audio_chunk::
                    channel_config_stereo) {
            flags |=
                spatial_meter::
                    kFlagFrontStereo;
        }

        if (chunk.get_channels() > 2) {
            flags |=
                spatial_meter::
                    kFlagNativeMultich;
        }

        spatial_meter::publisher().
            publishInterleaved(
                chunk.get_data(),
                chunk.get_sample_count(),
                chunk.get_channels(),
                chunk.get_channel_config(),
                chunk.get_srate(),
                flags,
                0U, 0U,
                static_cast<std::uint32_t>(
                    chunk.get_sample_count()),
                0U,
                maxInferenceUs_,
                providerCode_);
    }

    // One output chunk per on_chunk call (see flushBatch).
    std::vector<audio_sample> batchOutput_{};
    size_t batchFrames_{0};
    unsigned batchRate_{kModelRate};

    bool prebuffering_{true};

    audio_plugin::OnnxRuntime runtime_;
    bool runtimeReady_{false};
    bool runtimeFailed_{false};

    audio_plugin::StreamingSampleRateAdapter
        inputBridge_;
    audio_plugin::StreamingSampleRateAdapter
        outputBridge_;

    bool rateConfigured_{false};
    bool bridgeFailed_{false};
    bool streamActive_{false};
    bool activeAiPath_{false};
    unsigned hostRate_{kModelRate};

    std::uint64_t latencyHostSamples_{kHop};
    double latencySeconds_{
        static_cast<double>(kHop) / kRate
    };

    std::uint32_t lastInferenceUs_{0};
    std::uint32_t maxInferenceUs_{0};
    std::uint32_t providerCode_{0};

    StereoHop pending_{};
    size_t pendingCount_{0};
    StereoHop previousSubmitted_{};
    bool haveSubmitted_{false};

    std::array<
        std::vector<float>, 2>
        hostInputScratch_{};
    std::array<
        std::vector<float>, 2>
        modelBridgeScratch_{};
    std::array<
        std::vector<float>, 8>
        hostSpatialScratch_{};

    std::vector<audio_sample>
        interleavedOutput_{};

    std::array<
        std::vector<float>,
        audio_plugin::kNumChannels>
        modelInput_{};

    std::array<
        std::vector<float>,
        audio_plugin::kNumChannels>
        alignedInput_{};

    std::array<
        std::array<
            std::vector<float>,
            audio_plugin::kNumChannels>,
        audio_plugin::kNumStems>
        stems_{};

    // Declared last: destroyed (thread joined) before everything it uses.
    std::unique_ptr<InferenceWorker> worker_;
};

static dsp_factory_nopreset_t<
    dsp_stemgen_spatial>
    g_stemgen_spatial_factory;

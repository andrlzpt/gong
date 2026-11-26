//go:build android && cgo

#include <android/log.h>
#include <oboe/Oboe.h>
#include <memory>
#include <cmath>
#include <atomic>

class ToneCallback : public oboe::AudioStreamDataCallback {
private:
    static constexpr double twoPi = 6.283185307179586;
    static constexpr double frequency = 440.0;
    static constexpr float amplitude = 0.03f;
    
    double phase = 0.0;

public:
    std::atomic<uint32_t> calls{0};

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* stream,
            void* audioData,
            int32_t numFrames) override {
        
        auto* samples = static_cast<float*>(audioData);

        const double phaseStep = twoPi * frequency / stream->getSampleRate();

        const int32_t channels = stream->getChannelCount();

        for (int32_t frame = 0; frame < numFrames; ++frame) {
            const float sample =
                    amplitude * static_cast<float>(std::sin(phase));

            for (int32_t channel = 0; channel < channels; ++channel) {
                samples[frame * channels + channel] = sample;
            }

            phase += phaseStep;
            if (phase >= twoPi) {
                phase -= twoPi;
            }
        }

        calls.fetch_add(1, std::memory_order_relaxed);

        return oboe::DataCallbackResult::Continue;
    }
};

class InputCallback : public oboe::AudioStreamDataCallback {
public:
    std::atomic<float> peak{0.0f};
    std::atomic<uint32_t> calls{0};

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* stream,
            void* audioData,
            int32_t numFrames) override {
        const auto* samples = static_cast<const float*>(audioData);
        const int32_t sampleCount = numFrames *stream->getChannelCount();

        float blockPeak = 0.0f;

        for (int32_t i = 0; i < sampleCount; ++i) {
            const float magnitude = std::fabs(samples[i]);
            if (magnitude > blockPeak) {
                blockPeak = magnitude;
            }
        }

        float previous = peak.load(std::memory_order_relaxed);

        while (blockPeak > previous &&
                !peak.compare_exchange_weak(
                    previous, blockPeak, std::memory_order_relaxed
                )
            ) {
        }

        calls.fetch_add(1, std::memory_order_relaxed);

        return oboe::DataCallbackResult::Continue;
    }
};

static_assert(std::atomic<uint32_t>::is_always_lock_free);

static_assert(std::atomic<float>::is_always_lock_free);

class AudioEngine {
public:
    int start();
    int stop();
    int startInput();
    int stopInput();
    float readInputPeak();

private:
    std::shared_ptr<ToneCallback> callback;
    std::shared_ptr<oboe::AudioStream> stream;
    std::shared_ptr<InputCallback> inputCallback;
    std::shared_ptr<oboe::AudioStream> inputStream;
};

static AudioEngine engine;

static void logBuffers(const char* name, oboe::AudioStream* audioStream) {
    const int32_t rate = audioStream->getSampleRate();
    const int32_t burst = audioStream->getFramesPerBurst();
    const int32_t size = audioStream->getBufferSizeInFrames();
    const int32_t capacity = audioStream->getBufferCapacityInFrames();

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "%s buffers: burst=%d, size=%d, capacity=%d frames",
        name,
        static_cast<int>(burst),
        static_cast<int>(size),
        static_cast<int>(capacity)
    );

    if (rate > 0 && size > 0) {
        __android_log_print(
            ANDROID_LOG_INFO, "GONG",
            "%s buffer duration: %.2f ms",
            name, 1000.0 * size / rate
        );
    }
}

int AudioEngine::start() {
    if (stream != nullptr) {
        return 0;
    }
    __android_log_print(
        ANDROID_LOG_INFO,
        "GONG",
        "Go -> C++ -> Oboe: %s",
        oboe::getVersionText()
    );

    callback = std::make_shared<ToneCallback>();

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Mono);

    builder.setDataCallback(callback);

    oboe::Result result = builder.openStream(stream);

    if (result != oboe::Result::OK) {
        __android_log_print(
            ANDROID_LOG_ERROR, "GONG",
            "Open output failed: %s", oboe::convertToText(result)
        );
        stream.reset();
        callback.reset();

        return -1;
    }

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Output opened: API=%s, rate=%d, channels=%d, mode=%s, sharing=%s",
        oboe::convertToText(stream->getAudioApi()),
        stream->getSampleRate(),
        stream->getChannelCount(),
        oboe::convertToText(stream->getPerformanceMode()),
        oboe::convertToText(stream->getSharingMode())
    );

    result = stream->requestStart();

    if (result != oboe::Result::OK) {
        __android_log_print(
            ANDROID_LOG_ERROR, "GONG",
            "Start output failed: %s", oboe::convertToText(result)
        );
        stream->close();
        stream.reset();
        callback.reset();
        return -1;
    }

    logBuffers("Output", stream.get());

    return 0;
}

static void logXRuns(const char* name, oboe::AudioStream* audioStream) {
    const auto result = audioStream->getXRunCount();

    if (result) {
        __android_log_print(
            ANDROID_LOG_INFO, "GONG",
            "%s xruns: %d",
            name, static_cast<int>(result.value())
        );
    } else {
        __android_log_print(
            ANDROID_LOG_WARN, "GONG",
            "%s xrun count unavailable: %s",
            name, oboe::convertToText(result.error())
        );
    }
}

int AudioEngine::stop() {
    if (stream == nullptr) {
        return 0;
    }

    logXRuns("Output", stream.get());

    const oboe::Result result = stream->close();

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Output close: %s", oboe::convertToText(result)
    );

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Output callbacks: %u",
        static_cast<unsigned int>(
            callback->calls.load(std::memory_order_relaxed))
    );

    if (result != oboe::Result::OK) {
        return -1;
    }

    stream.reset();
    callback.reset();
    return 0;
}

int AudioEngine::startInput() {
    if (inputStream != nullptr) {
        return 0;
    }

    inputCallback = std::make_shared<InputCallback>();

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Input);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Mono);
    builder.setDataCallback(inputCallback);

    oboe::Result result = builder.openStream(inputStream);

    if (result != oboe::Result::OK) {
        __android_log_print(
            ANDROID_LOG_ERROR, "GONG",
            "Open input failed: %s", oboe::convertToText(result)
        );
        inputStream.reset();
        inputCallback.reset();
        return -1;
    }

    result = inputStream->requestStart();

    if (result != oboe::Result::OK) {
        __android_log_print(
            ANDROID_LOG_ERROR, "GONG",
            "Start input failed: %s", oboe::convertToText(result)
        );
        inputStream->close();
        inputStream.reset();
        inputCallback.reset();
        return -1;
    }

    logBuffers("Input", inputStream.get());

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Input started: rate=%d, channels=%d, mode=%s, sharing=%s",
        inputStream->getSampleRate(),
        inputStream->getChannelCount(),
        oboe::convertToText(inputStream->getPerformanceMode()),
        oboe::convertToText(inputStream->getSharingMode())
    );

    return 0;
}

int AudioEngine::stopInput() {
    if (inputStream == nullptr) {
        return 0;
    }

    logXRuns("Input", inputStream.get());

    const oboe::Result result = inputStream->close();

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Input close: %s; callbacks: %u",
        oboe::convertToText(result),
        static_cast<unsigned int>(
            inputCallback->calls.load(std::memory_order_relaxed))
    );

    if (result != oboe::Result::OK) {
        return -1;
    }

    inputStream.reset();
    inputCallback.reset();
    return 0;
}

float AudioEngine::readInputPeak() {
    if (inputCallback == nullptr ||
        inputCallback->calls.load(std::memory_order_relaxed) == 0) {
        
            return -1.0f;
    }

    return inputCallback->peak.exchange(0.0f, std::memory_order_relaxed);
}

extern "C" int gong_audio_start(void) {
    return engine.start();
}

extern "C" int gong_audio_stop(void) {
    return engine.stop();
}

extern "C" int gong_audio_input_start(void) {
    return engine.startInput();
}

extern "C" int gong_audio_input_stop(void) {
    return engine.stopInput();
}

extern "C" float gong_audio_input_peak(void) {
    return engine.readInputPeak();
}
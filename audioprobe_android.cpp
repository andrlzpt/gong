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

static_assert(std::atomic<uint32_t>::is_always_lock_free);

class AudioEngine {
public:
    int start();
    int stop();

private:
    std::shared_ptr<ToneCallback> callback;
    std::shared_ptr<oboe::AudioStream> stream;
};

static AudioEngine engine;

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

    return 0;
}

int AudioEngine::stop() {
    if (stream == nullptr) {
        return 0;
    }

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

extern "C" int gong_audio_start(void) {
    return engine.start();
}

extern "C" int gong_audio_stop(void) {
    return engine.stop();
}

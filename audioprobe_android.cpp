//go:build android && cgo

#include <android/log.h>
#include <oboe/Oboe.h>
#include <memory>

extern "C" int gong_audio_probe(void) {
    __android_log_print(
        ANDROID_LOG_INFO,
        "GONG",
        "Go -> C++ -> Oboe: %s",
        oboe::getVersionText()
    );

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output);
    builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
    builder.setSharingMode(oboe::SharingMode::Exclusive);
    builder.setFormat(oboe::AudioFormat::Float);
    builder.setChannelCount(oboe::ChannelCount::Mono);

    std::shared_ptr<oboe::AudioStream> stream;
    oboe::Result result = builder.openStream(stream);

    if (result != oboe::Result::OK) {
        __android_log_print(
            ANDROID_LOG_ERROR, "GONG",
            "Open output failed: %s", oboe::convertToText(result)
        );
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

    result = stream->close();

    __android_log_print(
        ANDROID_LOG_INFO, "GONG",
        "Output close: %s", oboe::convertToText(result)
    );

    return result == oboe::Result::OK ? 0 : -1;
}
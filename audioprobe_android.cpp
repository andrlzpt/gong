//go:build android && cgo

#include <android/log.h>
#include <oboe/Version.h>

extern "C" int gong_audio_probe(void) {
    __android_log_print(
        ANDROID_LOG_INFO,
        "GONG",
        "Go -> C++ -> Oboe: %s",
        oboe::getVersionText()
    );

    return 42;
}
//go:build android && cgo

#include <android/log.h>

extern "C" int gong_audio_probe(void) {
    __android_log_print(
        ANDROID_LOG_INFO,
        "GONG",
        "Go chamou o teste C++"
    );

    return 42;
}
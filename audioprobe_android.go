//go:build android && cgo

package main

/*
#cgo CXXFLAGS: -std=c++17 -I${SRCDIR}/third_party/oboe/include
#cgo android,arm64 LDFLAGS: -L${SRCDIR}/build/android/build/oboe/arm64-v8a -loboe -lOpenSLES -llog
int gong_audio_probe(void);
int gong_audio_start(void);
int gong_audio_stop(void);
*/
import "C"
import "fmt"

func runAudioProbe() error {
	result := int(C.gong_audio_probe())
	if result != 0 {
		return fmt.Errorf("native audio output test failed: code: %d", result)
	}

	return nil
}

func startAudioOutput() error {
	if result := int(C.gong_audio_start()); result != 0 {
		return fmt.Errorf("start audio output failed: code %d", result)
	}
	return nil
}

func stopAudioOutput() error {
	if result := int(C.gong_audio_stop()); result != 0 {
		return fmt.Errorf("stop audio output failed: code %d", result)
	}
	return nil
}

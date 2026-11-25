//go:build android && cgo

package main

/*
#cgo CXXFLAGS: -std=c++17 -I${SRCDIR}/third_party/oboe/include
#cgo android,arm64 LDFLAGS: -L${SRCDIR}/build/android/build/oboe/arm64-v8a -loboe -lOpenSLES -llog
int gong_audio_start(void);
int gong_audio_stop(void);
int gong_audio_input_start(void);
int gong_audio_input_stop(void);
float gong_audio_input_peak(void);
*/
import "C"
import "fmt"

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

func startAudioInput() error {
	if result := int(C.gong_audio_input_start()); result != 0 {
		return fmt.Errorf("start audio input failed: code %d", result)
	}
	return nil
}

func stopAudioInput() error {
	if result := int(C.gong_audio_input_stop()); result != 0 {
		return fmt.Errorf("stop audio input failed: code %d", result)
	}

	return nil
}

func readAudioInputPeak() (float64, error) {
	peak := float64(C.gong_audio_input_peak())
	if peak < 0 {
		return 0, fmt.Errorf("microphone input has no reading available")
	}
	return peak, nil
}

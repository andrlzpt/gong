//go:build android && cgo

package main

/*
#cgo CXXFLAGS: -std=c++17 -I${SRCDIR}/third_party/oboe/include
#cgo android,arm64 LDFLAGS: -L${SRCDIR}/build/android/build/oboe/arm64-v8a -loboe -lOpenSLES -llog
int gong_audio_probe(void);
*/
import "C"
import "log"

func init() {
	result := int(C.gong_audio_probe())
	log.Printf("GONG: teste GO -> C++ = %d", result)
}

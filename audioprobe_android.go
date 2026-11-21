//go:build android && cgo

package main

/*
#cgo LDFLAGS: -llog
int gong_audio_probe(void);
*/
import "C"
import "log"

func init() {
	result := int(C.gong_audio_probe())
	log.Printf("GONG: teste GO -> C++ = %d", result)
}

//go:build !android || !cgo

package main

import "errors"

func startAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

func stopAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

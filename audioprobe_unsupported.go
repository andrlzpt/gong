//go:build !android || !cgo

package main

import "errors"

func startAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

func stopAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

func startAudioInput() error {
	return errors.New("native audio input is available only on Android")
}

func stopAudioInput() error {
	return errors.New("native audio input is available only on Android")
}

func readAudioInputPeak() (float64, error) {
	return 0, errors.New("native audio input is available only on Android")
}

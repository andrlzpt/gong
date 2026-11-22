//go:build !android || !cgo

package main

import "errors"

func runAudioProbe() error {
	return errors.New("native audio output test is avaiable only on Android")
}

func startAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

func stopAudioOutput() error {
	return errors.New("native audio output is available only on Android")
}

//go:build !android || !cgo

package main

import "errors"

func runAudioProbe() error {
	return errors.New("native audio output test is avaiable only on Android")
}

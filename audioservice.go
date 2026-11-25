package main

import "sync"

type AudioService struct {
	mu sync.Mutex
}

func (s *AudioService) StartOutput() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	return startAudioOutput()
}

func (s *AudioService) StopOutput() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	return stopAudioOutput()
}

func (s *AudioService) StartInput() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	return startAudioInput()
}

func (s *AudioService) StopInput() error {
	s.mu.Lock()
	defer s.mu.Unlock()

	return stopAudioInput()
}

func (s *AudioService) ReadInputPeak() (float64, error) {
	s.mu.Lock()
	defer s.mu.Unlock()

	return readAudioInputPeak()
}

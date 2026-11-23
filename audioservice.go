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

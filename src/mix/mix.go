package mix

import "mbc/sdl"

//so:include <SDL3_mixer/SDL_mixer.h>

//so:extern MIX_Mixer
type Mixer struct{}

//so:extern MIX_Track
type Track struct{}

//so:extern MIX_Audio
type Audio struct{}

type TrackStoppedCallback func(userdata any, track *Track)

//so:extern MIX_Init
func Init() bool

//so:extern MIX_CreateMixerDevice
func CreateMixerDevice(devId uint32, spec *sdl.AudioSpec) *Mixer

//so:extern MIX_CreateTrack
func CreateTrack(*Mixer) *Track

//so:extern MIX_LoadAudio
func LoadAudio(mixer *Mixer, path string, preDecode bool) *Audio

//so:extern MIX_LoadAudioIO
func LoadAudioIO(mixer *Mixer, io *sdl.IOStream, preDecode bool, closeio bool) *Audio

//so:extern MIX_SetTrackIOStream
func SetTrackIOStream(track *Track, io *sdl.IOStream, closeio bool) bool

//so:extern MIX_SetTrackAudio
func SetTrackAudio(track *Track, audio *Audio) bool

//so:extern MIX_PlayTrack
func PlayTrack(track *Track,options uint32)

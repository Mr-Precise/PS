#ifndef PS_H
#define PS_H

#include <stdio.h>

#define CHANNEL_NUM (int)8 // Number of channels (synth tracks)
#define TICK_SIZE (int)6000
#define ECHO_SIZE (TICK_SIZE * 3)
#define REVERB 16
#define MAX_FLANGER_SIZE 100
#define REC_SIZE (TICK_SIZE * 16)

enum {
    SYNTH_DRUM = 1,
    SYNTH_HAT,
    SYNTH_BASS,
    SYNTH_BASS_TINY,
    SYNTH_ACID_BASS,
    SYNTH_EFFECT,
    SYNTH_POLY,
    SYNTH_PAD
};

int RandInt();

int Tick = 0; // 0...pattern size (number of lines)
int Timer = 0; // 0...one_tick_size
int Fadeout = 0;
int OutMode = 0; // 0 - SDL; 1 - WAV EXPORT;
int Srate = 44100;
int Buffsize = 1024;
int CurPattern = 0; // 0...number of patterns
int ExitRequest = 0;

float MaxVolume = 1;
float Volume = 1.3;
float FadeoutVol = 1;
float dc_sl = 0, dc_sr = 0;
float dc_psl = 0, dc_psr = 0;

const char *export_file_name = NULL;

float drum_timer[CHANNEL_NUM];
float drum_timer2[CHANNEL_NUM];
float drum_timer3[CHANNEL_NUM];
float hat_timer[CHANNEL_NUM];
float hat_old1[CHANNEL_NUM];
float hat_old2[CHANNEL_NUM];
float bass_freq[CHANNEL_NUM];
float bass_tdelta[CHANNEL_NUM];
float bass_delta[CHANNEL_NUM];
float bass_timer[CHANNEL_NUM];
float effect_timer[CHANNEL_NUM];
float effect_timer2[CHANNEL_NUM];
float echo_buf1[ECHO_SIZE * 2];
float echo_buf2[ECHO_SIZE * 2];

int syn[CHANNEL_NUM] = {SYNTH_DRUM, SYNTH_HAT, SYNTH_BASS, SYNTH_BASS_TINY, SYNTH_EFFECT, SYNTH_POLY, SYNTH_PAD, SYNTH_PAD};
int effect_lowfilter[CHANNEL_NUM];
int slow_reverb[REVERB];
int echo_ptr = 0;

float flanger_buf1[MAX_FLANGER_SIZE * 3 * 6];
float flanger_buf2[MAX_FLANGER_SIZE * 3 * 6];
float flanger_ptr = 0;
float flanger_size = 0;
float flanger_timer = 0;

int time_effect = 1;

int start_recorder = 0;
int rec_ptr = 0;
int rec_play = 0;

float rec1[REC_SIZE];
float rec2[REC_SIZE];

#endif // PS_H

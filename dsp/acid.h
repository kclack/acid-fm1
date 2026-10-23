/* SPDX-License-Identifier: MIT
 * Original ACID FM1 synthesis and sequencing engine. No Arturia code or assets. */
#ifndef ACID_FM1_H
#define ACID_FM1_H
#include <stdint.h>

#define ACID_STEPS 64
#define ACID_MODS 3
#define ACID_DELAY 32768
#define ACID_REV 2048
#define ACID_VERSION 1u
enum {
    P_CUTOFF, P_RESONANCE, P_ENV, P_DECAY, P_ACCENT, P_SLIDE,
    P_WAVE, P_PULSE, P_TUNE, P_SUB, P_DRIVE, P_DIST, P_DIST_MIX,
    P_TEMPO, P_SWING, P_GATE, P_LENGTH, P_DIRECTION, P_ROOT, P_SCALE,
    P_DELAY, P_FEEDBACK, P_DELAY_TIME, P_REVERB, P_CHORUS, P_VOLUME,
    P_BASS, P_NOISE, P_VIB_RATE, P_VIB_DEPTH, P_MODE, P_CLOCK,
    P_MOD1_RATE, P_MOD1_DEPTH, P_MOD1_TARGET, P_MOD1_SHAPE, P_MOD1_LENGTH,
    P_MOD2_RATE, P_MOD2_DEPTH, P_MOD2_TARGET, P_MOD2_SHAPE, P_MOD2_LENGTH,
    P_MOD3_RATE, P_MOD3_DEPTH, P_MOD3_TARGET, P_MOD3_SHAPE, P_MOD3_LENGTH,
    P_ACC_ATTACK, P_TRACKING, P_CLIP, P_ACC_LENGTH, P_SLIDE_LENGTH, P_MUTATION,
    P_COUNT
};
typedef struct { const char *name; float lo, hi, initial; uint8_t discrete; } AcidParamInfo;
extern const AcidParamInfo acid_params[P_COUNT];
/* Notes are offsets from ROOT. Gate 0 means rest; lock < 0 means no cutoff lock. */
typedef struct {
    int8_t note;
    uint8_t gate, accent, slide, probability, ratchet;
    uint16_t reserved;
    float cutoff;
} AcidStep;
typedef struct {
    uint32_t magic, version;
    float p[P_COUNT];
    AcidStep step[ACID_STEPS];
    float curve[ACID_MODS][8];
    uint32_t seed;
} AcidProject;
#define ACID_MAGIC 0x41434944u

typedef struct {
    AcidProject project;
    float fs;
    uint32_t rng, samples, clock_age, control, ticks;
    int playing, position, direction, step_serial, ratchet_index, pending;
    float remaining, duration, elapsed, estimated_step, clock_elapsed;
    float pitch, target_pitch, frequency, phase, sub_phase, amp, amp_target;
    float env, accent_env, accent_target, envelope_coeff, glide_coeff, amp_coeff;
    float cutoff_lock, coeff, resonance, drive, level, pulse, sub, noise;
    float z[4], dc_x, dc_y, low;
    int gate, accented, note, external_lost, step_active;
    uint8_t held[128];
    uint32_t order[128], serial;
    int held_count, arp_index;
    float mod_phase[ACID_MODS], mod_random[ACID_MODS], mod_value[ACID_MODS];
    float mod_pitch, mod_volume, vibrato_phase, chorus_phase, delay_smooth;
    float peak;
    uint32_t delay_pos, chorus_pos, rev_pos[4];
    int16_t delay_buf[ACID_DELAY];
    float chorus_buf[2048], reverb_buf[4][ACID_REV], rev_damp[4];
} Acid;

void acid_init(Acid *a, float sample_rate);
void acid_set(Acid *a, int id, float value);
void acid_step_set(Acid *a, int step, int field, float value);
void acid_curve_set(Acid *a, int mod, int point, float value);
void acid_preset(Acid *a, int preset);
const char *acid_preset_name(int preset);
#define ACID_PRESETS 8
void acid_generate(Acid *a, uint32_t seed, float density);
void acid_mutate(Acid *a, uint32_t seed, float amount);
void acid_start(Acid *a);
void acid_stop(Acid *a);
void acid_panic(Acid *a);
void acid_note_on(Acid *a, int note, int velocity);
void acid_note_off(Acid *a, int note);
void acid_midi(Acid *a, uint8_t status, uint8_t d1, uint8_t d2);
void acid_process(Acid *a, float *interleaved, uint32_t frames);
int acid_project_valid(const AcidProject *p);
int acid_load(Acid *a, const AcidProject *p);
int acid_quantize(int semitone, int scale);

#endif

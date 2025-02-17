//////////////////////////////////////////////////////////////////////////////
// OPNA emulator
// Copyright (C) 2015-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)

#include "opna.h"
#include "YM2203_Timbre.h"

//////////////////////////////////////////////////////////////////////////////
// Number of Key 

#define KEY_C           0   // C
#define KEY_CS          1   // C#
#define KEY_D           2   // D
#define KEY_DS          3   // D#
#define KEY_E           4   // E
#define KEY_F           5   // F
#define KEY_FS          6   // F#
#define KEY_G           7   // G
#define KEY_GS          8   // G#
#define KEY_A           9   // A
#define KEY_AS          10  // A#
#define KEY_B           11  // B
#define KEY_NUM         12  // 12 keys in a octave

//////////////////////////////////////////////////////////////////////////////

#define FM_CH_NUM       3   // YM2203 has 3 FM channels
#define SSG_CH_NUM      3   // YM2203 has 3 SSG channels

//////////////////////////////////////////////////////////////////////////////
// Tone/Noise mode of SSG channels

#define TONE_MODE       0   // tone output mode (default)
#define NOISE_MODE      1   // noise output mode
#define TONE_NOISE_MODE 2   // tone & noise output mode

//////////////////////////////////////////////////////////////////////////////
// register address (SSG)

#define ADDR_SSG_TONE_FREQ_L    0x00
#define ADDR_SSG_TONE_FREQ_H    0x01
#define ADDR_SSG_NOISE_FREQ     0x06
#define ADDR_SSG_MIXING         0x07
#define ADDR_SSG_LEVEL_ENV      0x08
#define ADDR_SSG_ENV_FREQ_L     0x0B
#define ADDR_SSG_ENV_FREQ_H     0x0C
#define ADDR_SSG_ENV_TYPE       0x0D

//////////////////////////////////////////////////////////////////////////////
// register address (FM)

#define ADDR_FM_LFO_ON_SPEED    0x22
#define ADDR_FM_KEYON           0x28
#define ADDR_FM_PRESCALER_1     0x2D
#define ADDR_FM_PRESCALER_2     0x2E
#define ADDR_FM_PRESCALER_3     0x2F
#define ADDR_FM_DETUNE_MULTI    0x30
#define ADDR_FM_TL              0x40
#define ADDR_FM_AR_KEYSCALE     0x50
#define ADDR_FM_DR              0x60
#define ADDR_FM_SR              0x70
#define ADDR_FM_SL_RR           0x80
#define ADDR_FM_FREQ_L          0xA0
#define ADDR_FM_FREQ_H          0xA4
#define ADDR_FM_FB_ALGORITHM    0xB0
#define ADDR_FM_LR_AMS_PMS      0xB4

//////////////////////////////////////////////////////////////////////////////
// YM2203

struct YM2203 {
    YM2203();
    ~YM2203() { }

    void init(uint32_t clock, uint32_t rate, const char* rhythmpath);

    void fm_key_on(int fm_ich);
    void fm_key_off(int fm_ich);
    void fm_set_pitch(int fm_ich, int octave, int key, int adj = 0);
    void fm_set_volume(int fm_ich, int volume, int adj[4]);
    void fm_set_volume(int fm_ich, int volume) {
        int adj[4] = {0, 0, 0, 0};
        fm_set_volume(fm_ich, volume, adj);
    }
    void fm_set_timbre(int fm_ich, YM2203_Timbre *timbre);

    void ssg_key_on(int ssg_ich);
    void ssg_key_off(int ssg_ich);
    void ssg_set_pitch(int ssg_ich, int octave, int key, int adj = 0);
    void ssg_set_volume(int ssg_ich, int volume);
    void ssg_set_envelope(int ssg_ich, int type, uint16_t interval);
    void ssg_set_tone_or_noise(int ssg_ich, int mode);

    bool load_rhythm_data(const char *path) {
        return m_opna.LoadRhythmSample(path);
    }
    void mix(FM_SAMPLETYPE *dest, int nsamples) {
        m_opna.Mix(dest, nsamples);
    }
    bool count(uint32_t microsec) {
        return m_opna.Count(microsec);
    }
    uint32_t get_next_event() {
        return m_opna.GetNextEvent();
    }
    void reset() {
        m_opna.Reset();
    }

    void write_reg(uint32_t addr, uint32_t data) {
        m_opna.SetReg(addr, data);
    }

protected:
    FM::OPNA        m_opna;
    YM2203_Timbre * m_fm_timbres[FM_CH_NUM];
    uint8_t         m_fm_volumes[FM_CH_NUM];
    bool            m_ssg_enveloped[SSG_CH_NUM];
    uint8_t         m_ssg_tone_noise[SSG_CH_NUM];
    uint8_t         m_ssg_key_on;
    uint8_t         m_ssg_envelope_type;

    static const uint16_t FM_PITCH_TABLE[KEY_NUM];
    static const uint16_t SSG_PITCH_TABLE[KEY_NUM];
}; // struct YM2203

//////////////////////////////////////////////////////////////////////////////

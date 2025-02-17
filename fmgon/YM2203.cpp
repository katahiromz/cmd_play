//////////////////////////////////////////////////////////////////////////////
// OPNA emulator
// Copyright (C) 2015-2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)

#include "fmgon.h"
#include "YM2203.h"

//////////////////////////////////////////////////////////////////////////////

/*static*/ const uint16_t YM2203::FM_PITCH_TABLE[KEY_NUM] = {
    617, 654, 693, 734, 778, 824, 873, 925, 980, 1038, 1100, 1165
};

/*static*/ const uint16_t YM2203::SSG_PITCH_TABLE[KEY_NUM] = {
    7645, 7215, 6810, 6428, 6067, 5727, 5405, 5102, 4816, 4545, 4290, 4050
};

//////////////////////////////////////////////////////////////////////////////

YM2203::YM2203() {
    // initial value
    for (int ich = 0; ich < 3; ++ich) {
        m_fm_timbres[ich] = NULL;
        m_fm_volumes[ich] = 0;
        m_ssg_enveloped[ich] = false;
    }
    m_ssg_tone_noise[0] = 0x01;
    m_ssg_tone_noise[1] = 0x02;
    m_ssg_tone_noise[2] = 0x04;
}

void YM2203::init(uint32_t clock, uint32_t rate, const char* rhythmpath) {
    m_opna.Init(clock, rate, false, rhythmpath);
    m_opna.Reset();
    m_ssg_key_on = 0x3F;
    uint32_t addr = ADDR_SSG_MIXING;
    uint32_t data = 0x3F;
    write_reg(addr, data);
}

void YM2203::fm_key_on(int fm_ich) {
    assert(0 <= fm_ich && fm_ich < FM_CH_NUM);
    if (m_fm_timbres[fm_ich] == NULL) {
        assert(0);
        return;
    }
    uint32_t addr = ADDR_FM_KEYON;
    uint32_t data = (m_fm_timbres[fm_ich]->opMask << 4) | fm_ich;
    write_reg(addr, data);
}

void YM2203::ssg_key_on(int ssg_ich) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);
    m_ssg_key_on &= ~m_ssg_tone_noise[ssg_ich];
    uint32_t addr = ADDR_SSG_MIXING;
    uint32_t data = m_ssg_key_on;
    write_reg(addr, data);
    if (m_ssg_enveloped[ssg_ich]) {
        switch (m_ssg_envelope_type) {
        case 9: case 15:
            addr = ADDR_SSG_ENV_TYPE;
            data = m_ssg_envelope_type;
            write_reg(addr, data);
            break;
        default:
            break;
        }
    }
}

void YM2203::fm_key_off(int fm_ich) {
    assert(0 <= fm_ich && fm_ich < FM_CH_NUM);
    uint32_t addr = ADDR_FM_KEYON;
    uint32_t data = 0 | fm_ich;
    write_reg(addr, data);
}

void YM2203::ssg_key_off(int ssg_ich) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);
    m_ssg_key_on |= m_ssg_tone_noise[ssg_ich];
    uint32_t addr = ADDR_SSG_MIXING;
    uint32_t data = m_ssg_key_on;
    write_reg(addr, data);
}

void YM2203::fm_set_pitch(int fm_ich, int octave, int key, int adj) {
    assert(0 <= fm_ich && fm_ich < FM_CH_NUM);
    uint32_t addr = ADDR_FM_FREQ_H + fm_ich;
    uint32_t data = (
        ((octave & 0x07) << 3) |
        (((FM_PITCH_TABLE[key] + adj) >> 8) & 0x07)
    );
    write_reg(addr, data);

    addr = ADDR_FM_FREQ_L + fm_ich;
    data = (uint8_t)(FM_PITCH_TABLE[key] + adj);
    write_reg(addr, data);
}

void YM2203::ssg_set_pitch(int ssg_ich, int octave, int key, int adj) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);
    uint16_t ssg_f = SSG_PITCH_TABLE[key];
    if (octave > 0) {
        ssg_f >>= (octave - 1);
        ssg_f = (ssg_f >> 1) + (ssg_f & 0x0001);
    }

    uint32_t addr = ADDR_SSG_TONE_FREQ_L + ssg_ich * 2;
    uint32_t data = ssg_f;
    write_reg(addr, data);

    addr = ADDR_SSG_TONE_FREQ_H + ssg_ich * 2;
    data = (ssg_f >> 8) & 0x0F;
    write_reg(addr, data);
}

static const uint8_t OP_OFFSET[] = {0x00, 0x08, 0x04, 0x0C};

void YM2203::fm_set_volume(int fm_ich, int volume, int adj[4]) {
    assert(0 <= fm_ich && fm_ich < FM_CH_NUM);
    assert((0 <= volume) && (volume <= 15));

    if (m_fm_timbres[fm_ich] == NULL) {
        assert(0);
        return;
    }

    uint8_t algorithm = m_fm_timbres[fm_ich]->algorithm;
    uint8_t attenate = uint8_t(uint8_t(15 - volume) * 3);

    uint32_t addr = ADDR_FM_TL + uint8_t(fm_ich) + OP_OFFSET[OPERATOR_4];
    uint32_t data = ((m_fm_timbres[fm_ich]->tl[OPERATOR_4] + attenate - adj[3]) & 0x7F);
    write_reg(addr, data);

    if (algorithm >= ALGORITHM_4) {
        addr = ADDR_FM_TL + uint8_t(fm_ich) + OP_OFFSET[OPERATOR_2];
        data = ((m_fm_timbres[fm_ich]->tl[OPERATOR_2] + attenate - adj[1]) & 0x7F);
        write_reg(addr, data);
    }

    if (algorithm >= ALGORITHM_5) {
        addr = ADDR_FM_TL + uint8_t(fm_ich) + OP_OFFSET[OPERATOR_3];
        data = ((m_fm_timbres[fm_ich]->tl[OPERATOR_3] + attenate - adj[2]) & 0x7F);
        write_reg(addr, data);
    }

    if (algorithm == ALGORITHM_7) {
        addr = ADDR_FM_TL + (uint8_t)fm_ich + OP_OFFSET[OPERATOR_1];
        data = ((m_fm_timbres[fm_ich]->tl[OPERATOR_1] + attenate - adj[0]) & 0x7F);
        write_reg(addr, data);
    }
}

void YM2203::ssg_set_volume(int ssg_ich, int volume) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);
    assert((0 <= volume) && (volume <= 15));

    uint32_t addr = ADDR_SSG_LEVEL_ENV + ssg_ich;
    uint32_t data = volume & 0x0F;
    write_reg(addr, data);
}

void YM2203::ssg_set_envelope(int ssg_ich, int type, uint16_t interval) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);

    uint32_t addr = ADDR_SSG_LEVEL_ENV + ssg_ich;
    uint32_t data = 0x10;
    write_reg(addr, data);
    m_ssg_enveloped[ssg_ich] = true;

    addr = ADDR_SSG_ENV_TYPE;
    data = (type & 0x0F);
    write_reg(addr, data);
    m_ssg_envelope_type = data;

    addr = ADDR_SSG_ENV_FREQ_L;
    data = (interval & 0xFF);
    write_reg(addr, data);

    addr = ADDR_SSG_ENV_FREQ_H;
    data = (interval >> 8) & 0xFF;
    write_reg(addr, data);
}

void YM2203::ssg_set_tone_or_noise(int ssg_ich, int mode) {
    assert(0 <= ssg_ich && ssg_ich < SSG_CH_NUM);

    static const uint8_t TONE_MASK[3] = {0x01, 0x02, 0x04};
    static const uint8_t NOISE_MASK[3] = {0x08, 0x10, 0x20};

    switch (mode) {
    case TONE_MODE:
        m_ssg_tone_noise[ssg_ich] = TONE_MASK[ssg_ich];
        break;
    case NOISE_MODE:
        m_ssg_tone_noise[ssg_ich] = NOISE_MASK[ssg_ich];
        break;
    case TONE_NOISE_MODE:
        m_ssg_tone_noise[ssg_ich] = TONE_MASK[ssg_ich] + NOISE_MASK[ssg_ich];
        break;
    }
}

void YM2203::fm_set_timbre(int fm_ich, YM2203_Timbre *timbre) {
    assert(0 <= fm_ich && fm_ich < FM_CH_NUM);

    static const uint8_t OP_OFFSET[] = {0x00, 0x08, 0x04, 0x0C};

    uint32_t addr, data;
    for (int op = OPERATOR_1; op <= OPERATOR_4; ++op) {
        uint8_t tl = timbre->tl[op];
        uint8_t ar = timbre->ar[op];
        uint8_t dr = timbre->dr[op];
        uint8_t sr = timbre->sr[op];
        uint8_t sl = timbre->sl[op];
        uint8_t rr = timbre->rr[op];
        int8_t sDetune = timbre->detune[op];
        uint8_t detune;
        if (sDetune >= 0) {
            detune = uint8_t(sDetune);
        } else {
            detune = uint8_t(4-sDetune);
        }
        uint8_t multiple = timbre->multiple[op];
        uint8_t keyScale = timbre->keyScale[op];
        uint8_t offset = (uint8_t)fm_ich + OP_OFFSET[op];

        addr = ADDR_FM_DETUNE_MULTI + offset;
        data = ((detune & 0x07) << 4) | (multiple & 0x0F);
        write_reg(addr, data);

        addr = ADDR_FM_TL + offset;
        data = (tl & 0x7F);
        write_reg(addr, data);

        addr = ADDR_FM_AR_KEYSCALE + offset;
        data = (((keyScale & 0x03) << 6) | (ar & 0x1F));
        write_reg(addr, data);

        addr = ADDR_FM_DR + offset;
        data = (dr & 0x1F);
        write_reg(addr,data);

        addr = ADDR_FM_SR + offset;
        data = (sr & 0x1F);
        write_reg(addr, data);

        addr = ADDR_FM_SL_RR + offset;
        data = (((sl & 0x0F) << 4) | (rr & 0x0F));
        write_reg(addr, data);
    }

    addr = ADDR_FM_FB_ALGORITHM + fm_ich;
    data = (((timbre->feedback & 0x07) << 3) | (timbre->algorithm & 0x07));
    write_reg(addr, data);

    // TODO: LFO
    //addr = ADDR_FM_LFO_ON_SPEED;
    //data = ...
    //write_reg(addr, data);
    //
    //addr = ADDR_FM_LR_AMS_PMS;
    //data = ...;
    //write_reg(addr, data);

    m_fm_timbres[fm_ich] = timbre;
} // YM2203::fm_set_timbre

//////////////////////////////////////////////////////////////////////////////

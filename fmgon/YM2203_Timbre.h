//////////////////////////////////////////////////////////////////////////////
// fmgengen YM2203 (OPN) emulator timbre
// Copyright (C) 2015 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com).
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Number of FM operators

#define OPERATOR_1      0
#define OPERATOR_2      1
#define OPERATOR_3      2
#define OPERATOR_4      3
#define OPERATOR_NUM    4

//////////////////////////////////////////////////////////////////////////////
// Bit mask for YM2203_Timbre#opMask

#define MASK_OP1        0x01
#define MASK_OP2        0x02
#define MASK_OP3        0x04
#define MASK_OP4        0x08
#define MASK_ALL        0x0F

//////////////////////////////////////////////////////////////////////////////
// Algorism number

#define ALGORITHM_0     0
#define ALGORITHM_1     1
#define ALGORITHM_2     2
#define ALGORITHM_3     3
#define ALGORITHM_4     4
#define ALGORITHM_5     5
#define ALGORITHM_6     6
#define ALGORITHM_7     7

//////////////////////////////////////////////////////////////////////////////
// timbre array

typedef int16_t timbre_array_t[5][10];

//////////////////////////////////////////////////////////////////////////////
// YM2203 FM synthesizer timbre structure.

struct YM2203_Timbre {
    uint8_t algorithm;                  // Algorithm
    uint8_t feedback;                   // Feedback
    uint8_t opMask;                     // Operator Mask
    uint8_t ar[OPERATOR_NUM];           // Attack Rate
    uint8_t dr[OPERATOR_NUM];           // Decay Rate
    uint8_t sr[OPERATOR_NUM];           // Sustain Rate
    uint8_t rr[OPERATOR_NUM];           // Release Rate
    uint8_t sl[OPERATOR_NUM];           // Systain Level
    uint8_t tl[OPERATOR_NUM];           // Total Level
    uint8_t keyScale[OPERATOR_NUM];     // Key Scale
    uint8_t multiple[OPERATOR_NUM];     // Multiple
    int8_t  detune[OPERATOR_NUM];       // Detune

    // Amplitude Modulation Sensitivity (AMS)
    uint8_t     ams[OPERATOR_NUM]; 

    uint8_t     waveForm;               // Wave Form
    uint8_t     sync;                   // LFO Sync
    uint16_t    speed;                  // LFO Speed
    int8_t      pmd;                    // Pitch Modulation Depth (PMD)
    int8_t      amd;                    // Amplitude Modulation Depth (AMD)
    uint8_t     pms;                    // Pitch Modulation Sensitivity (PMS)

    YM2203_Timbre();
    YM2203_Timbre(const timbre_array_t& array);
    void set(const timbre_array_t& array);

    void setAR(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setDR(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setSR(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setRR(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setSL(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setTL(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setKS(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setML(uint8_t op1, uint8_t op2, uint8_t op3, uint8_t op4);
    void setDT( int8_t op1,  int8_t op2,  int8_t op3,  int8_t op4);
}; // struct YM2203_Timbre

//////////////////////////////////////////////////////////////////////////////

// the number of the tones
#define NUM_TONES   62

// tone data
extern const timbre_array_t ym2203_tone_table[NUM_TONES];

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// soundplayer --- an fmgon sound player
// Copyright (C) 2015 Katayama Hirofumi MZ. All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>
#include <vector>
#include <memory>
#include <unordered_map>

#ifdef _WIN32
    #define UNBOOST_USE_WIN32_THREAD
    #include "unboost/mutex.hpp"
    #include "unboost/thread.hpp"
#else
    #define UNBOOST_USE_POSIX_THREAD
    #include "unboost/mutex.hpp"
    #include "unboost/thread.hpp"
#endif

//////////////////////////////////////////////////////////////////////////////
// OpenAL --- portable audio library

#include "al.h"

//////////////////////////////////////////////////////////////////////////////
// pevent --- portable event objects

#include "pevent/pevent.h"

//////////////////////////////////////////////////////////////////////////////
// YM2203

#include "fmgon/YM2203.h"

//////////////////////////////////////////////////////////////////////////////
// VskNote

enum SpecialKeys {
    KEY_REST = -1,             // 休符
    KEY_SPECIAL_ACTION = -2,   // スペシャルアクション
    KEY_TONE = -3,             // トーン変更
    KEY_SPECIAL_REST = -4,     // 特殊な休符
    KEY_REG = -5,              // レジスタ書き込み
    KEY_ENVELOP_INTERVAL = -6, // エンベロープ周期
    KEY_ENVELOP_TYPE = -7,     // エンベロープ形状
};

struct VskNote {
    int         m_tempo;
    int         m_octave;
    int         m_key;
    bool        m_dot;
    float       m_length;
    char        m_sign;
    float       m_sec;
    float       m_gate;
    float       m_volume;   // in 0 to 15
    int         m_quantity; // in 0 to 8
    bool        m_and;
    int         m_reg;
    int         m_data;

    VskNote(int tempo, int octave, int note,
            bool dot = false, float length = 24, char sign = 0,
            float volume = 8, int quantity = 8,
            bool and_ = false, int reg = -1, int data = -1)
    {
        m_tempo = tempo;
        m_octave = octave;
        m_dot = dot;
        m_length = length;
        m_sign = sign;
        m_sec = get_sec(m_tempo, m_length);
        set_key_from_char(note);
        m_volume = volume;
        m_quantity = quantity;
        m_and = and_;
        m_reg = reg;
        m_data = data;
    }

    float get_sec(int tempo, float length) const;
    void set_key_from_char(char note);

private:
    VskNote();
}; // struct VskNote

//////////////////////////////////////////////////////////////////////////////
// VskSoundSetting

struct VskSoundSetting {
    int                 m_tempo;    // tempo
    int                 m_octave;   // octave
    float               m_length;   // 24 is the length of a quarter note
    YM2203_Timbre       m_timbre;   // see YM2203_Timbre
    bool                m_fm;       // whether it is FM or not?
    float               m_volume;   // in 0 to 15
    int                 m_quantity; // in 0 to 8

    VskSoundSetting(int tempo = 120, int octave = 4 - 1, float length = 24,
                    int tone = 0, bool fm = false) :
        m_tempo(tempo), m_octave(octave), m_length(length),
        m_fm(fm)
    {
        m_volume = 8;
        m_quantity = 8;
    }

    void reset() {
        m_tempo = 120;
        m_octave = 4 - 1;
        m_length = 24;
        m_fm = false;
        m_volume = 8;
        m_quantity = 8;
    }
}; // struct VskSoundSetting

//////////////////////////////////////////////////////////////////////////////
// VskPhrase

struct VskSoundPlayer;

struct VskPhrase {
    float                               m_goal = 0;
    ALuint                              m_buffer = -1;
    ALuint                              m_source = -1;
    VskSoundSetting                     m_setting;
    std::vector<VskNote>                m_notes;

    VskSoundPlayer*                     m_player;
    std::vector<std::pair<float, int>>  m_gate_to_special_action_no;
    size_t                              m_remaining_actions;

    VskPhrase() { }
    VskPhrase(const VskSoundSetting& setting) : m_setting(setting) { }

    ~VskPhrase() {
        destroy();
    }

    void add_note(char note) {
        add_note(note, false);
    }
    void add_note(char note, bool dot) {
        add_note(note, dot, m_setting.m_length);
    }
    void add_note(char note, bool dot, float length) {
        add_note(note, dot, length, 0);
    }
    void add_note(char note, bool dot, float length, char sign) {
        add_note(note, dot, length, sign, m_setting.m_quantity);
    }
    void add_note(char note, bool dot, float length, char sign, int quantity) {
        add_note(note, dot, length, sign, quantity, false);
    }
    void add_note(char note, bool dot, float length, char sign, int quantity, bool and_) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, dot, length, sign, m_setting.m_volume,
            quantity, and_);
    }
    void add_action_node(char note, int action_no) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, false, 0, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, action_no);
    }
    void add_tone(char note, int tone_no) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, false, 0, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, tone_no);
    }
    void add_reg(char note, int reg, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, false, 0, 0, m_setting.m_volume,
            m_setting.m_quantity, false, reg, data);
    }
    void add_envelop_interval(char note, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, false, 0, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, data);
    }
    void add_envelop_type(char note, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave,
            note, false, 0, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, data);
    }
    void add_key(int key) {
        add_key(key, false);
    }
    void add_key(int key, bool dot) {
        add_key(key, dot, m_setting.m_length);
    }
    void add_key(int key, bool dot, float length) {
        add_key(key, dot, length, 0);
    }
    void add_key(int key, bool dot, float length, char sign) {
        add_key(key, dot, length, sign, m_setting.m_quantity);
    }
    void add_key(int key, bool dot, float length, char sign, int quantity) {
        if (key == 96) {
            key = 0;
        }
        VskNote note(m_setting.m_tempo, 0,
            0, dot, length, sign, m_setting.m_volume, quantity);
        note.m_key = key;
        m_notes.push_back(note);
    }

    void schedule_special_action(float gate, int action_no);
    void execute_special_actions();

    void rescan_notes();
    void calc_total();
    void realize(VskSoundPlayer *player);
    void realize(VskSoundPlayer *player, FM_SAMPLETYPE*& data, size_t& data_size);
    bool gen_source_and_buffer(const FM_SAMPLETYPE *data, size_t data_size);
    void destroy();
}; // struct VskPhrase

//////////////////////////////////////////////////////////////////////////////

typedef std::vector<std::shared_ptr<VskPhrase>>  VskScoreBlock;

// The function pointer type of special action
typedef void (*VskSpecialActionFn)(int action_number);

//////////////////////////////////////////////////////////////////////////////

struct VskSoundPlayer {
    bool                                        m_playing_music;
    PE_event                                    m_stopping_event;
    std::deque<VskScoreBlock>                   m_melody_line;
    unboost::mutex                              m_play_lock;
    std::vector<std::shared_ptr<VskNote>>       m_notes;
    YM2203                                      m_ym;
    std::unordered_map<int, VskSpecialActionFn> m_action_no_to_special_action;
    std::vector<FM_SAMPLETYPE>                  m_samples;

    VskSoundPlayer() : m_playing_music(false), m_stopping_event(false, false) { }
    virtual ~VskSoundPlayer() { }

    void play(VskScoreBlock& block);
    bool wait_for_stop(uint32_t milliseconds = -1);
    bool play_and_wait(VskScoreBlock& block, uint32_t milliseconds = -1);
    void stop();
    bool save_as_wav(VskScoreBlock& block, const wchar_t *filename);
    bool generate_pcm_raw(VskScoreBlock& block, std::vector<FM_SAMPLETYPE>& samples);

    void register_special_action(int action_no, VskSpecialActionFn fn = nullptr);
    void do_special_action(int action_no);

    void write_reg(uint32_t addr, uint32_t data) {
        m_ym.write_reg(addr, data);
    }
}; // struct VskSoundPlayer

//////////////////////////////////////////////////////////////////////////////

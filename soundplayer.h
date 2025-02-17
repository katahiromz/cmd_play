//////////////////////////////////////////////////////////////////////////////
// soundplayer --- an fmgon sound player
// Copyright (C) 2015-2025 Katayama Hirofumi MZ. All Rights Reserved.
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

#define VSK_PCM16_VALUE int16_t

//////////////////////////////////////////////////////////////////////////////
// pevent --- portable event objects

#include "pevent/pevent.h"

//////////////////////////////////////////////////////////////////////////////
// YM2203

#include "fmgon/YM2203.h"

//////////////////////////////////////////////////////////////////////////////
// VskNote - 音符、休符、その他の何か

// 特殊キー
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
    int         m_tempo;            // テンポ
    int         m_octave;           // オクターブ
    uint8_t     m_LR;               // 左右 (left/right)
    int         m_key;              // キー
    bool        m_dot;              // 付点
    float       m_length;           // 長さ
    char        m_sign;             // シャープかフラットか
    float       m_sec;              // 秒数
    float       m_gate;             // 開始時刻
    float       m_volume;           // 音量 (0～15)
    int         m_quantity;         // 音符の長さの割合 (0～8)
    bool        m_and;              // タイか？
    int         m_reg;              // レジスタのアドレス
    int         m_data;             // 汎用データ

    VskNote(int tempo, int octave, uint8_t LR, int note,
            bool dot = false, float length = 24.0f, char sign = 0,
            float volume = 8.0f, int quantity = 8,
            bool and_ = false, int reg = -1, int data = -1)
    {
        m_tempo = tempo;
        m_octave = octave;
        m_LR = LR;
        m_dot = dot;
        m_length = length;
        m_sign = sign;
        m_sec = get_sec(tempo, length, dot);
        m_key = get_key_from_char(note, sign);
        m_volume = volume;
        m_quantity = quantity;
        m_and = and_;
        m_reg = reg;
        m_data = data;
    }

    static float get_sec(int tempo, float length, bool dot);
    static int get_key_from_char(char note, char sign);

private:
    VskNote();
}; // struct VskNote

//////////////////////////////////////////////////////////////////////////////
// VskSoundSetting - 音声の設定

struct VskSoundSetting {
    int                 m_tempo;    // テンポ
    int                 m_octave;   // オクターブ
    float               m_length;   // 音符の長さ (24は四分音符の長さ)
    bool                m_fm;       // FMかどうか
    float               m_volume;   // 音量 (0～15)
    int                 m_quantity; // 音符の長さの割合 (0～8)
    int                 m_tone;     // 音色
    uint8_t             m_LR;       // 左右 (left/right)
    YM2203_Timbre       m_timbre;   // see YM2203_Timbre

    VskSoundSetting(int tempo = 120, int octave = 4 - 1, float length = 24,
                    int tone = 0, bool fm = false) :
        m_tempo(tempo), m_octave(octave), m_length(length),
        m_fm(fm)
    {
        m_volume = 8;
        m_quantity = 8;
        m_tone = tone;
        m_LR = 0x3;
    }

    void reset() {
        m_tempo = 120;
        m_octave = 4 - 1;
        m_length = 24;
        m_fm = false;
        m_volume = 8;
        m_quantity = 8;
        m_tone = 0;
        m_LR = 0x3;
        m_timbre.set(ym2203_tone_table[0]);
    }
};

//////////////////////////////////////////////////////////////////////////////
// VskPhrase - フレーズ

struct VskSoundPlayer;

struct VskPhrase {
    float                               m_goal = 0;     // 演奏終了時刻（秒）
    VskSoundSetting&                    m_setting;      // 音声設定
    std::vector<VskNote>                m_notes;        // 音符、休符、その他の何か

    VskSoundPlayer*                     m_player;       // サウンドプレーヤー

    // 時刻からスペシャルアクションへの写像
    std::vector<std::pair<float, int>>  m_gate_to_special_action_no;

    size_t                              m_remaining_actions;    // 残りのスペシャルアクションの個数

    VskPhrase(VskSoundSetting& setting) : m_setting(setting) { }

    // 音符を追加
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
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, dot, length, sign, m_setting.m_volume,
            quantity, and_);
    }

    // スペシャルアクションを追加
    void add_action_node(char note, int action_no) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, false, 0.0f, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, action_no);
    }

    // 音色を追加
    void add_tone(char note, int tone_no) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, false, 0.0f, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, tone_no);
    }

    // レジスタ書き込みを追加
    void add_reg(char note, int reg, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, false, 0.0f, 0, m_setting.m_volume,
            m_setting.m_quantity, false, reg, data);
    }

    // 波形の間隔を追加
    void add_envelop_interval(char note, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, false, 0.0f, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, data);
    }

    // 波形の種類を追加
    void add_envelop_type(char note, int data) {
        m_notes.emplace_back(
            m_setting.m_tempo, m_setting.m_octave, m_setting.m_LR,
            note, false, 0.0f, 0, m_setting.m_volume,
            m_setting.m_quantity, false, -1, data);
    }

    // キーを追加
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
        VskNote note(m_setting.m_tempo, 0, m_setting.m_LR, 0, dot, length, sign,
                     m_setting.m_volume, quantity);
        note.m_key = key;
        m_notes.push_back(note);
    }

    void schedule_special_action(float gate, int action_no);
    void execute_special_actions();
    void realize(VskSoundPlayer* player, int ich, VSK_PCM16_VALUE*& data, size_t *pdata_size);

protected:
    void rescan_notes();
    void calc_gate_and_goal();
}; // struct VskPhrase

//////////////////////////////////////////////////////////////////////////////

// 楽譜のブロックは、フレーズの集合
typedef std::vector<std::shared_ptr<VskPhrase>>  VskScoreBlock;

// スペシャルアクションの関数
typedef void (*VskSpecialActionFn)(int action_number);

//////////////////////////////////////////////////////////////////////////////
// VskSoundPlayer - サウンドプレーヤー

struct VskSoundPlayer {
    bool                                        m_playing_music;    // 演奏中か？
    PE_event                                    m_stopping_event;   // 演奏停止用のイベント
    std::deque<VskScoreBlock>                   m_melody_line;      // メロディーライン
    unboost::mutex                              m_play_lock;        // 排他制御のミューテックス
    std::vector<std::shared_ptr<VskNote>>       m_notes;            // 音符、休符、その他の何かの配列
    YM2203                                      m_ym;               // 音源エミュレータ
    std::vector<VSK_PCM16_VALUE>                m_pcm_values;       // 実際の波形

    // アクション番号からスペシャルアクションへの写像
    std::unordered_map<int, VskSpecialActionFn> m_action_no_to_special_action;

    VskSoundPlayer(const char *rhythm_path = NULL);
    virtual ~VskSoundPlayer() { }

    void play(VskScoreBlock& block, bool stereo);
    bool wait_for_stop(uint32_t milliseconds);
    bool play_and_wait(VskScoreBlock& block, uint32_t milliseconds, bool stereo);
    void stop();
    bool save_as_wav(VskScoreBlock& block, const wchar_t *filename, bool stereo);
    bool generate_pcm_raw(VskScoreBlock& block, std::vector<VSK_PCM16_VALUE>& values, bool stereo);

    void register_special_action(int action_no, VskSpecialActionFn fn = nullptr);
    void do_special_action(int action_no);

    void write_reg(uint32_t addr, uint32_t data) {
        m_ym.write_reg(addr, data);
    }
}; // struct VskSoundPlayer

//////////////////////////////////////////////////////////////////////////////

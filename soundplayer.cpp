﻿//////////////////////////////////////////////////////////////////////////////
// soundplayer --- an fmgon sound player
// Copyright (C) 2015-2025 Katayama Hirofumi MZ. All Rights Reserved.

#include "fmgon/fmgon.h"
#include "soundplayer.h"
#include "sound.h"
#include <mmsystem.h>
#include <map>
#include <cstdio>
#include <limits>

#define CLOCK       8000000     // クロック数
#define SAMPLERATE  44100       // サンプルレート (Hz)

#define LFO_INTERVAL 150

class VskLFOCtrl {
    int     m_waveform;
    int     m_qperiod; // quarter of period
    int     m_count;
    int     m_phase; // 0, 1, 2 or 3
    float   m_adj_p_max;
    float   m_adj_v_max[4];
    float   m_adj_p_diff;
    float   m_adj_v_diff[4];
public:
    float   m_adj_p; // for pitch
    float   m_adj_v[4]; // for volume

public:
    VskLFOCtrl() {
        m_adj_p = 0;
        memset(m_adj_v_diff, 0, sizeof(m_adj_v_diff));
    }

    void init_for_timbre(YM2203_Timbre *p_timbre) {
        int i;
        m_waveform = p_timbre->waveForm;
        if (p_timbre->speed) {
            m_qperiod = 900 * LFO_INTERVAL / (4*p_timbre->speed);
        } else {
            m_qperiod = 0;
        }
        //m_count = 0;
        m_phase = 0;
        m_adj_p_max = p_timbre->pmd * (float)p_timbre->pms / 2.0f; // TBD
        for (i = 0; i < 4; ++i) {
            m_adj_v_max[i] =
                p_timbre->amd * (float)p_timbre->ams[i] / 2; // TBD
        }
        init_for_phase(true);
    }

    void init_for_keyon(YM2203_Timbre *p_timbre) {
        if (p_timbre->sync) {
            m_phase = 0;
            init_for_phase();
        }
    }

    void increment() {
        int i;
        if (0 == m_qperiod) {
            return;
        }
        m_count++;
        if (m_count < m_qperiod) {
            m_adj_p += m_adj_p_diff;
            for (i = 0; i < 4; ++i) {
                m_adj_v[i] += m_adj_v_diff[i];
            }
        } else {
            m_phase = (m_phase + 1) & 3;
            init_for_phase();
        }
    }

protected:
    void init_for_phase(bool flag_first = false) {
        int i;
        m_count = 0;
        if (flag_first) {
            switch (m_waveform) {
            case 0: // saw
                m_adj_p = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = 0;
                }
                m_adj_p_diff = m_adj_p_max / (m_qperiod * 2);
                for (i = 0; i < 4; ++i) {
                    m_adj_v_diff[i] = m_adj_v_max[i] / (m_qperiod * 2);
                }
                break;
            case 1: // square
                m_adj_p = -m_adj_p_max;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = -m_adj_v_max[i];
                }
                m_adj_p_diff = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v_diff[i] = 0;
                }
                break;
            case 2: // triangle
                m_adj_p = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = 0;
                }
                m_adj_p_diff = m_adj_p_max / m_qperiod;
                for (i = 0; i < 4; ++i) {
                    m_adj_v_diff[i] = m_adj_v_max[i] / m_qperiod;
                }
                break;
            default: // sample and hold
                //m_adj_p = m_adj_p_max * (rand() * 2.0 / RAND_MAX - 1);
                //for (i = 0; i < 4; ++i) {
                //    m_adj_v[i] = m_adj_v_max[i] * (rand() * 2.0 / RAND_MAX - 1);
                //}
                m_adj_p_diff = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v_diff[i] = 0;
                }
                break;
            }
        }
        switch (m_waveform) {
        case 0: // saw
            if (0 == m_phase) {
                m_adj_p = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = 0;
                }
            } else if (2 == m_phase) {
                m_adj_p = -m_adj_p;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = -m_adj_v[i];
                }
            }
            break;
        case 1: // square
            if (0 == (m_phase & 1)) {
                m_adj_p = -m_adj_p;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = -m_adj_v[i];
                }
            }
            break;
        case 2: // triangle
            if (0 == m_phase) {
                m_adj_p = 0;
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = 0;
                }
            } else if (1 == (m_phase & 1)) {
                m_adj_p_diff = -m_adj_p_diff;
                for (i = 0; i < 4; ++i) {
                    m_adj_v_diff[i] = -m_adj_v_diff[i];
                }
            }
            break;
        default: // sample and hold
            if (0 == (m_phase & 1)) {
                m_adj_p = float(
                    m_adj_p_max * (rand() * 2.0 / RAND_MAX - 1)
                );
                for (i = 0; i < 4; ++i) {
                    m_adj_v[i] = float(
                        m_adj_v_max[i] * (rand() * 2.0 / RAND_MAX - 1)
                    );
                }
            }
            break;
        }
    }
}; // class VskLFOCtrl

//////////////////////////////////////////////////////////////////////////////
// VskNote - 音符、休符、その他の何か

// 秒数を計算
float VskNote::get_sec(int tempo, float length, bool dot) {
    float sec;
    assert(tempo != 0);
    // NOTE: 24 is the length of a quarter note
    if (dot) {
        sec = length * (60.0f * 1.5f / 24.0f) / tempo;
    } else {
        sec = length * (60.0f / 24.0f) / tempo;
    }
    return sec;
}

// 文字からキーを取得
int VskNote::get_key_from_char(char ch, char sign) {
    if (ch == 'R' || ch == 0)
        return KEY_REST;
    if (ch == '@')
        return KEY_TONE;
    if (ch == 'W')
        return KEY_SPECIAL_REST;
    if (ch == 'Y')
        return KEY_REG;
    if (ch == 'M')
        return KEY_ENVELOP_INTERVAL;
    if (ch == 'S')
        return KEY_ENVELOP_TYPE;
    if (ch == 'X')
        return KEY_SPECIAL_ACTION;
    if (ch == 'Z')
        return KEY_MIDI;

    static const char keys[KEY_NUM + 1] = "C+D+EF+G+A+B";

    const char *ptr = strchr(keys, ch);
    assert(ptr != NULL);
    assert(*ptr == ch);
    int key = int(ptr - keys);

    switch (sign) {
    case '+': case '#':
        if (key == KEY_B) {
            key = KEY_C;
        } else {
            ++key;
        }
        break;
    case '-':
        if (key == KEY_C) {
            key = KEY_B;
        } else {
            --key;
        }
        break;
    default:
        break;
    }

    return key;
}

//////////////////////////////////////////////////////////////////////////////
// VskPhrase - フレーズ

// スペシャルアクションを予約する
void VskPhrase::schedule_special_action(float gate, int action_no) {
    m_gate_to_special_action_no.push_back(std::make_pair(gate, action_no));
}

// スペシャルアクションを実行する
void VskPhrase::execute_special_actions() {
    assert(m_player);

    // 残りの未実行のアクション数を設定
    // 入力が"CDX0X1"などで再生完了後にスペシャルアクションを実行する延長時間の調整に使用される
    m_remaining_actions = m_gate_to_special_action_no.size();

    // アクションがない場合は何もしない
    if (m_remaining_actions == 0) {
        return;
    }

    // gateに合わせてスペシャルアクションを実行するための制御スレッド
    unboost::thread(
        [this](int dummy) {
            // 前回実行したスペシャルアクションのgateを保持、初期値は0
            // gate、last_gateは秒を小数点で表しています
            float last_gate = 0;

            // gateが同じスペシャルアクションをまとめる
            std::map<float, std::vector<int>> gate_to_actions;
            for (const auto& pair : m_gate_to_special_action_no) {
                gate_to_actions[pair.first].push_back(pair.second);
            }

            // スペシャルアクションをgateごとにまとめて実行
            // std::mapのiteratorはkeyの昇順でiterateするし、
            // アクションも順番通りでvectorに追加したため、順番は保証されている
            for (auto& pair2 : gate_to_actions) {
                auto gate = pair2.first;
                auto action_numbers = pair2.second;

                // 前のgateからの待機時間を計算して待機
                if (!m_player->wait_for_stop(uint32_t(gate - last_gate) * 1000)) {
                    // 待機中にstopされた場合、ループを抜ける
                    break;
                }

                // スペシャルアクションを別のスレッドで実行
                unboost::thread(
                    [this, action_numbers](int dummy) {
                        // gateが同じスペシャルアクションをループ実行
                        for (const auto& action_no : action_numbers) {
                            m_player->do_special_action(action_no);
                        }
                    },
                    0
                ).detach();
                // 残りの未実行のアクション数をを減らす
                m_remaining_actions -= action_numbers.size();

                last_gate = gate;
            }
        },
        0
    ).detach();
}

// 音楽記号のタイを実現するために、フレーズデータを再スキャンする
void VskPhrase::rescan_notes() {
    std::vector<VskNote> new_notes;
    for (size_t i = 0; i < m_notes.size(); ++i) {
        // 有効な"&"か？
        if (!m_notes[i].m_and || !(i < m_notes.size()) ||
            !(KEY_C <= m_notes[i].m_key && m_notes[i].m_key <= KEY_B) ||
            m_notes[i].m_octave != m_notes[i + 1].m_octave ||
            m_notes[i].m_key != m_notes[i + 1].m_key)
        {
            // 有効な"&"ではなかった
            new_notes.push_back(m_notes[i]);
            continue;
        }

        // 有効な"&"があった。長さと秒数を再計算
        size_t k = 0;
        float length = 0, sec = 0;
        do {
            length += m_notes[i + k].m_length;
            sec += m_notes[i + k].m_sec;
            ++k;
        } while (m_notes[i + k].m_and);
        length += m_notes[i + k].m_length;
        sec += m_notes[i + k].m_sec;

        // 新しい長さと秒数を格納
        m_notes[i].m_length = length;
        m_notes[i].m_sec = sec;

        new_notes.push_back(m_notes[i]);
        i += k; // 次の場所へ
    }

    m_notes = std::move(new_notes);
} // VskPhrase::rescan_notes

// 音符再生の時刻とゴール（演奏終了）の時刻を更新する
void VskPhrase::calc_gate_and_goal() {
    float gate = 0;
    for (auto& note : m_notes) {
        note.m_gate = gate;
        gate += note.m_sec;
    }
    m_goal = gate;
}

// 波形を実現する（ステレオ）
std::unique_ptr<VSK_PCM16_VALUE[]> VskPhrase::fm_realize(int ich, size_t *pdata_size)
{
    assert(m_player != nullptr);
    assert(m_setting.m_audio_type == AUDIO_TYPE_FM);

    // チャンネルに応じてチップに振り分ける
    YM2203& ym = (ich >= 3) ? m_player->m_ym1 : m_player->m_ym0;
    if (ich >= 3)
        ich -= 3;

    // メモリーを割り当て
    auto count = uint32_t(m_goal * SAMPLERATE + 1) * 2; // stereo
    *pdata_size = count * sizeof(VSK_PCM16_VALUE);
    auto data = std::make_unique<VSK_PCM16_VALUE[]>(count);
    std::memset(&data[0], 0, *pdata_size);

    uint32_t isample = 0;

    auto& timbre = m_setting.m_timbre;
    ym.fm_set_timbre(ich, &timbre);

    VskLFOCtrl lc;
    lc.init_for_timbre(&timbre);

    for (auto& note : m_notes) { // For each note
        if (note.m_key == KEY_SPECIAL_ACTION) { // Special action?
            schedule_special_action(note.m_gate, note.m_data);
            continue;
        }

        if (note.m_key == KEY_TONE) { // Tone change?
            const auto new_tone = note.m_data;
            assert((0 <= new_tone) && (new_tone < NUM_TONES));
            timbre = ym2203_tone_table[new_tone];
            ym.fm_set_timbre(ich, &timbre);
            lc.init_for_timbre(&timbre);
            continue;
        }

        if (note.m_key == KEY_REG) { // Register?
            ym.write_reg(note.m_reg, note.m_data);
            continue;
        }

        if (note.m_key == KEY_ENVELOP_INTERVAL) {
            auto interval = note.m_data;
            ym.write_reg(ADDR_SSG_ENV_FREQ_L, (interval & 0xFF));
            ym.write_reg(ADDR_SSG_ENV_FREQ_H, ((interval >> 8) & 0xFF));
            continue;
        }

        if (note.m_key == KEY_ENVELOP_TYPE) {
            auto type = note.m_data;
            ym.write_reg(ADDR_SSG_ENV_TYPE, (type & 0x0F));
            continue;
        }

        if (note.m_key == KEY_OFF || note.m_key == KEY_MIDI) {
            continue;
        }

        // 左右を設定する
        auto LR = note.m_LR;
        auto pms = timbre.pms;
        for (int i = 0; i < 3; ++i) {
            auto ams = timbre.ams[i];
            uint8_t value = (uint8_t)((LR << 6) | (ams << 4) | pms);
            ym.write_reg(0xB4 + i, value);
        }

        if (note.m_key != KEY_SPECIAL_REST) { // Not special rest?
            // do key on
            if (note.m_key != KEY_REST) { // Has key?
                ym.fm_set_pitch(ich, note.m_octave, note.m_key);
                ym.fm_set_volume(ich, int(note.m_volume), int(note.m_volume_at));
                ym.fm_key_on(ich);
            }

            lc.init_for_keyon(&timbre);
        }

        // render sound
        auto sec = note.m_sec * note.m_quantity / 8.0f;
        auto nsamples = int(SAMPLERATE * sec);
        int unit;
        while (nsamples) {
            unit = SAMPLERATE / LFO_INTERVAL;
            if (unit > nsamples) {
                unit = nsamples;
            }
            ym.mix(&data[isample * 2], unit);
            isample += unit;
            // @56 のLFO効果が非常におかしいので特別に回避策をすることにした
            if (note.m_key != KEY_REST && note.m_key != KEY_SPECIAL_REST && m_setting.m_tone != 56) {
                lc.increment();
                int adj[4] = {
                    int(lc.m_adj_v[0]), int(lc.m_adj_v[1]),
                    int(lc.m_adj_v[2]), int(lc.m_adj_v[3]),
                };
                ym.fm_set_volume(ich, int(note.m_volume), int(note.m_volume_at), adj);
                ym.fm_set_pitch(ich, note.m_octave, note.m_key, int(lc.m_adj_p));
            }
            nsamples -= unit;
        }
        ym.count(uint32_t(sec * 1000 * 1000));
        isample += nsamples;

        sec = note.m_sec * (8.0f - note.m_quantity) / 8.0f;
        nsamples = int(SAMPLERATE * sec);
        if (note.m_key != KEY_SPECIAL_REST) {
            // do key off
            ym.fm_key_off(ich);
        }
        unit = SAMPLERATE;
        if (unit > nsamples) {
            unit = nsamples;
        }
        ym.mix(&data[isample * 2], unit);
        ym.count(uint32_t(sec * 1000 * 1000));
        isample += nsamples;
    }

    return data;
}

// 波形を実現する（ステレオ）
std::unique_ptr<VSK_PCM16_VALUE[]> VskPhrase::ssg_realize(int ich, size_t *pdata_size)
{
    assert(m_player != nullptr);
    assert(m_setting.m_audio_type == AUDIO_TYPE_SSG);

    // チャンネルに応じてチップに振り分ける
    YM2203& ym = (ich >= 3) ? m_player->m_ym1 : m_player->m_ym0;
    if (ich >= 3)
        ich -= 3;

    // メモリーを割り当て
    auto count = uint32_t(m_goal * SAMPLERATE + 1) * 2; // stereo
    *pdata_size = count * sizeof(VSK_PCM16_VALUE);
    auto data = std::make_unique<VSK_PCM16_VALUE[]>(count);
    std::memset(&data[0], 0, *pdata_size);

    uint32_t isample = 0;

    for (auto& note : m_notes) {
        if (note.m_key == KEY_SPECIAL_ACTION) { // Special action?
            schedule_special_action(note.m_gate, note.m_data);
            continue;
        }

        if (note.m_key == KEY_REG) { // Register?
            ym.write_reg(note.m_reg, note.m_data);
            continue;
        }

        if (note.m_key == KEY_ENVELOP_INTERVAL) {
            auto interval = note.m_data;
            ym.write_reg(ADDR_SSG_ENV_FREQ_L, (interval & 0xFF));
            ym.write_reg(ADDR_SSG_ENV_FREQ_H, ((interval >> 8) & 0xFF));
            continue;
        }

        if (note.m_key == KEY_ENVELOP_TYPE) {
            auto type = note.m_data;
            ym.write_reg(ADDR_SSG_ENV_TYPE, (type & 0x0F));
            continue;
        }

        if (note.m_key == KEY_OFF || note.m_key == KEY_MIDI) {
            continue;
        }

        // do key on
        if (note.m_key != KEY_REST && note.m_key != KEY_SPECIAL_REST) {
            ym.ssg_set_pitch(ich, note.m_octave, note.m_key);
            ym.ssg_set_volume(ich, int(note.m_volume));
            ym.ssg_key_on(ich);
        }

        // render sound
        auto sec = note.m_sec * note.m_quantity / 8.0f;
        auto nsamples = int(SAMPLERATE * sec);
        ym.mix(&data[isample * 2], nsamples);
        ym.count(uint32_t(sec * 1000 * 1000));
        isample += nsamples;

        sec = note.m_sec * (8.0f - note.m_quantity) / 8.0f;
        nsamples = int(SAMPLERATE * sec);
        if (note.m_key != KEY_SPECIAL_REST) {
            // do key off
            ym.ssg_key_off(ich);
        }
        ym.mix(&data[isample * 2], nsamples);
        ym.count(uint32_t(sec * 1000 * 1000));
        isample += nsamples;
    }

    return data;
}

//////////////////////////////////////////////////////////////////////////////
// WAVEヘッダ

#define WAV_HEADER_SIZE 44 // WAVEヘッダのバイトサイズ

// WAVEヘッダを取得する
static uint8_t*
get_wav_header(uint32_t data_size, uint32_t sample_rate, uint16_t bit_depth, bool stereo)
{
    static uint8_t wav_header[WAV_HEADER_SIZE] = { 0 };

    std::memcpy(&wav_header[0], "RIFF", 4);
    std::memcpy(&wav_header[8], "WAVE", 4);
    std::memcpy(&wav_header[12], "fmt ", 4);
    std::memcpy(&wav_header[36], "data", 4);

    uint16_t num_channels = (stereo ? 2 : 1);
    uint16_t block_align = num_channels * (bit_depth / 8);
    uint32_t byte_rate = sample_rate * num_channels * (bit_depth / 8);

    uint32_t chunk_size = data_size + WAV_HEADER_SIZE - 8;
    uint32_t subchunk1_size = 16;
    uint16_t audio_format = 1; // PCM

    // Windows なのでリトルエンディアンを仮定する
    std::memcpy(&wav_header[4], &chunk_size, 4);
    std::memcpy(&wav_header[16], &subchunk1_size, 4);
    std::memcpy(&wav_header[20], &audio_format, 2);
    std::memcpy(&wav_header[22], &num_channels, 2);
    std::memcpy(&wav_header[24], &sample_rate, 4);
    std::memcpy(&wav_header[28], &byte_rate, 4);
    std::memcpy(&wav_header[32], &block_align, 2);
    std::memcpy(&wav_header[34], &bit_depth, 2);
    std::memcpy(&wav_header[40], &data_size, 4);

    return wav_header;
}

//////////////////////////////////////////////////////////////////////////////
// VskSoundPlayer - サウンドプレーヤー

VskSoundPlayer::VskSoundPlayer(const char *rhythm_path)
    : m_playing_music(false)
    , m_stopping_event(false, false)
{
    // YMを初期化
    m_ym0.init(CLOCK, SAMPLERATE, rhythm_path);
    m_ym1.init(CLOCK, SAMPLERATE, rhythm_path);

    for (int ich = 0; ich < SSG_CH_NUM; ++ich)
    {
        m_ym0.ssg_set_tone_or_noise(ich, TONE_MODE);
        m_ym1.ssg_set_tone_or_noise(ich, TONE_MODE);
    }
}

bool VskSoundPlayer::wait_for_stop(uint32_t milliseconds) {
    // 演奏が終わるか、指定時間が経過するまで待つ
    return m_stopping_event.wait_for_event(milliseconds);
}

// 実際に演奏する
void VskSoundPlayer::play(VskScoreBlock& block, bool stereo) {
    // 波形を生成
    generate_pcm_raw(block, m_pcm_values, stereo);

    // スペシャルアクションを実行
    for (auto& phrase : block)
        phrase->execute_special_actions();

    // 波形に基づいて演奏
    vsk_sound_play(m_pcm_values.data(), m_pcm_values.size() * sizeof(VSK_PCM16_VALUE), stereo);

    // 終わるのを待つ
    wait_for_stop(-1);
}

// MIDIイベントを送信
static MMRESULT
send_midi_event(HMIDIOUT hmo, int status, int data1, int data2)
{
    union {
        DWORD dwData;
        BYTE bData[4];
    } u;

    u.bData[0] = (BYTE)status;  // MIDI status byte
    u.bData[1] = (BYTE)data1;   // first MIDI data byte
    u.bData[2] = (BYTE)data2;   // second MIDI data byte
    u.bData[3] = 0;
    return midiOutShortMsg(hmo, u.dwData);
}

void VskPhrase::add_key_offs()
{
    std::vector<VskNote> notes;
    for (auto& note : m_notes) {
        switch (note.m_key) {
        case KEY_REST: case KEY_SPECIAL_REST: // 休符
        case KEY_TONE: // トーン変更
        case KEY_SPECIAL_ACTION: // スペシャルアクション
        case KEY_REG: // レジスタ書き込み
        case KEY_ENVELOP_INTERVAL: // エンベロープ周期
        case KEY_ENVELOP_TYPE: // エンベロープ形状
            notes.push_back(note);
            break;
        default: // 普通の音符
            {
                notes.push_back(note);

                // 長さゼロのKEY_OFFを追加する
                int key = note.m_key;
                float sec = note.m_sec;
                note.m_key = KEY_OFF;
                note.m_sec = 0;
                note.m_length = 0;
                note.m_data = key;
                note.m_gate += sec * note.m_quantity / 8;
                notes.push_back(note);
            }
            break;
        }
    }
    m_notes = std::move(notes);
}

// MIDIを演奏する
void VskSoundPlayer::play_midi(VskScoreBlock& block)
{
    // MIDI出力を開く
    HMIDIOUT hMidiOut;
    MMRESULT result = midiOutOpen(&hMidiOut, 0, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        printf("Error opening MIDI output\n");
        return;
    }

    // 演奏する前の準備
    int ch = 0;
    std::vector<VskNote> notes;
    float goal = 0;
    for (auto& phrase : block) {
        assert(phrase);
        phrase->calc_gate_and_goal();
        phrase->add_key_offs();
        phrase->rescan_notes();
        phrase->set_player(this);
        assert(phrase->m_setting.m_audio_type == AUDIO_TYPE_MIDI);
        for (auto& note : phrase->m_notes) {
            note.m_misc = ch;
            notes.push_back(note);
        }
        if (goal < phrase->m_goal)
            goal = phrase->m_goal;
        ++ch;
    }

    // 音符の開始時間でソート
    std::sort(notes.begin(), notes.end(), [](const VskNote& x, const VskNote& y) {
        return x.m_gate < y.m_gate;
    });

    // リセット
    ch = 0;
    for (auto& phrase : block) {
        assert(phrase);
        send_midi_event(hMidiOut, 0xB0 + ch, 0x79, 0x00); // リセットオールコントローラ
        send_midi_event(hMidiOut, 0xC0 + ch, phrase->m_setting.m_tone, 0); // プログラムチェンジ（音色）
        ++ch;
    }

    // 実際に音を出す
    float gate = 0, volume = 8.0f, volume_at = -1;
    const int max_quantity = 8, default_length = 24;
    for (auto& note : notes) {
        int octave = note.m_octave;
        int quantity = note.m_quantity;
        int length = note.m_length;
        int ch = note.m_misc;
        int tempo = note.m_tempo;

        if (note.m_gate > gate) {
            Sleep(DWORD(1000 * (note.m_gate - gate)));
            gate = note.m_gate;
        }

        // 音量設定
        if (volume_at != note.m_volume_at && volume_at != -1) {
            volume_at = note.m_volume_at;
            send_midi_event(hMidiOut, 0xB0 + ch, 0x07, uint8_t(volume_at));
        } else if (volume != note.m_volume) {
            volume = note.m_volume;
            send_midi_event(hMidiOut, 0xB0 + ch, 0x07, uint8_t(volume * 127.0f / 15.0f));
        }

        switch (note.m_key) {
        case KEY_TONE: // トーン変更
            send_midi_event(hMidiOut, 0xC0 + ch, note.m_data, 0); // プログラムチェンジ（音色）
            break;
        case KEY_REST: case KEY_SPECIAL_REST: // 休符
        case KEY_SPECIAL_ACTION: // スペシャルアクション
        case KEY_REG: // レジスタ書き込み
        case KEY_ENVELOP_INTERVAL: // エンベロープ周期
        case KEY_ENVELOP_TYPE: // エンベロープ形状
            // 無視
            break;
        case KEY_OFF: // キーオフ
            send_midi_event(hMidiOut, 0x80 + ch, note.m_data + 12 * octave, 127); // ノートオフ（ベロシティ 127）
            break;
        case KEY_MIDI: // MIDIメッセージ
            switch (note.m_reg & 0xF0) {
            case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
                // 第２データバイトあり
                {
                    auto status = note.m_reg;
                    auto data1 = HIBYTE(note.m_data);
                    auto data2 = LOBYTE(note.m_data);
                    send_midi_event(hMidiOut, status, data1, data2);
                }
                break;
            case 0xC0: case 0xD0:
                // 第２データバイトなし
                {
                    auto status = note.m_reg;
                    auto data1 = HIBYTE(note.m_data);
                    send_midi_event(hMidiOut, status, data1, 0);
                }
                break;
            case 0xF0:
                // サポートしない。
                assert(0);
                break;
            }
            break;
        default: // 普通の音符
            send_midi_event(hMidiOut, 0x90 + ch, note.m_key + 12 * octave, 127); // ノートオン（ベロシティ 127）
            break;
        }
    }

    // 終わりまで待つ
    if (goal > gate) {
        Sleep(DWORD(1000 * (goal - gate)));
    }

    // 音が終わっていない？ もうちょっと待とうか？
    Sleep(100);

    midiOutClose(hMidiOut); // MIDI出力を閉じる
}

// PCM波形を生成する
bool VskSoundPlayer::generate_pcm_raw(VskScoreBlock& block, std::vector<VSK_PCM16_VALUE>& values, bool stereo) {
    std::vector<std::unique_ptr<VSK_PCM16_VALUE[]>> raw_data;
    std::vector<size_t> data_sizes;

    // ステレオ音声として波形を実現する
    int ich = 0;
    const int source_num_channels = 2;
    for (auto& phrase : block) {
        if (phrase) {
            phrase->calc_gate_and_goal();
            phrase->rescan_notes();
            phrase->set_player(this);

            size_t data_size;
            std::unique_ptr<VSK_PCM16_VALUE[]> data;
            switch (phrase->m_setting.m_audio_type) {
            case AUDIO_TYPE_SSG:
                data = phrase->ssg_realize(ich, &data_size);
                break;
            case AUDIO_TYPE_FM:
                data = phrase->fm_realize(ich, &data_size);
                break;
            default:
                assert(0);
            }
            assert(data != nullptr);

            raw_data.push_back(std::move(data));
            data_sizes.push_back(data_size);
        }
        ++ich;
    }

    // 最大のデータサイズを計算
    size_t data_size = 0;
    for (size_t i = 0; i < raw_data.size(); ++i) {
        if (data_size < data_sizes[i])
            data_size = data_sizes[i];
    }

    // 転送元のデータを計算
    const size_t source_num_samples = data_size / sizeof(VSK_PCM16_VALUE) / source_num_channels;
    const size_t source_num_values = source_num_samples * source_num_channels;

    // 転送先の波形データを確保
    const int num_channels = (stereo ? 2 : 1);
    values.resize(source_num_samples * num_channels);

    // 波形データを構築
    VSK_PCM16_VALUE prev_value = 0;
    for (size_t ivalue = 0; ivalue < source_num_values; ++ivalue) {
        // Mixing
        int32_t value = 0;
        for (size_t i = 0; i < raw_data.size(); ++i) {
            if (ivalue < data_sizes[i] / sizeof(VSK_PCM16_VALUE))
                value += raw_data[i][ivalue];
        }

        // Clipping value
        if (value < std::numeric_limits<VSK_PCM16_VALUE>::min())
            value = std::numeric_limits<VSK_PCM16_VALUE>::min();
        else if (value > std::numeric_limits<VSK_PCM16_VALUE>::max())
            value = std::numeric_limits<VSK_PCM16_VALUE>::max();

        // 転送先に格納
        int32_t sample_value = ((ivalue < data_size) ? value : 0);
        if (stereo) { // ステレオの場合
            values[ivalue] = (VSK_PCM16_VALUE)sample_value;
        } else { // モノラルの場合
            if (ivalue & 1) // 奇数の場合
                values[ivalue >> 1] = (VSK_PCM16_VALUE)((prev_value + sample_value) >> 1);
            else // 偶数の場合
                prev_value = sample_value;
        }
    }

    return true;
}

// 音声をWAVファイルとして保存
bool VskSoundPlayer::save_wav(VskScoreBlock& block, const wchar_t *filename, bool stereo) {
    // 波形を生成する
    std::vector<VSK_PCM16_VALUE> values;
    generate_pcm_raw(block, values, stereo);
    size_t data_size = values.size() * sizeof(VSK_PCM16_VALUE);

    // WAVファイルを書き込み用として開く
    FILE *fout = _wfopen(filename, L"wb");
    if (!fout)
        return false;

    // WAVファイルに書き込み、閉じる
    auto wav_header = get_wav_header(data_size, SAMPLERATE, 16, stereo);
    std::fwrite(wav_header, WAV_HEADER_SIZE, 1, fout);
    std::fwrite(values.data(), data_size, 1, fout);
    std::fclose(fout);

    return true;
}

static void write_u16(FILE *fout, uint16_t value)
{
    std::fputc(uint8_t(value >> 8), fout);
    std::fputc(uint8_t(value), fout);
}

static void write_u32(FILE *fout, uint32_t value)
{
    std::fputc(uint8_t(value >> 24), fout);
    std::fputc(uint8_t(value >> 16), fout);
    std::fputc(uint8_t(value >> 8), fout);
    std::fputc(uint8_t(value), fout);
}

static void write_variable_length(std::vector<unsigned char> &trackData, uint32_t value)
{
    unsigned char buffer[4];
    int len = 0;
    buffer[len++] = value & 0x7F;
    while (value >>= 7) {
        buffer[len++] = (value & 0x7F) | 0x80;
    }
    for (int i = len - 1; i >= 0; --i) {
        trackData.push_back(buffer[i]);
    }
}

// MIDIデータをファイルに書き込む
bool VskSoundPlayer::write_mid_file(FILE *fout, VskScoreBlock& block)
{
    uint8_t num_tracks = uint8_t(1 + block.size());
    const uint16_t ticks_per_quarter_note = 480;
    const int max_quantity = 8, default_length = 24;

    // ヘッダーチャンク
    std::fwrite("MThd", 4, 1, fout);
    write_u32(fout, 6);
    write_u16(fout, 1);  // フォーマット 1
    write_u16(fout, num_tracks);
    write_u16(fout, ticks_per_quarter_note);

    std::vector<uint8_t> trackData;

    // テンポ情報のトラック 1を書き込む
    {
        int tempo = 120, delta_time = 0;
        uint32_t microseconds_per_quarter_note = 60 * 1000 * 1000 / tempo;

        std::fwrite("MTrk", 4, 1, fout);
        size_t trackSizePos = ftell(fout);
        write_u32(fout, 0);

        trackData.push_back(0x00);
        trackData.push_back(0xFF);
        trackData.push_back(0x51);
        trackData.push_back(0x03);
        trackData.push_back(uint8_t(microseconds_per_quarter_note >> 16));
        trackData.push_back(uint8_t(microseconds_per_quarter_note >> 8));
        trackData.push_back(uint8_t(microseconds_per_quarter_note));

        if (block.size()) {
            auto ch = 0;
            auto& phrase = block[ch];
            for (auto& note : phrase->m_notes) {
                if (tempo != note.m_tempo) {
                    tempo = note.m_tempo;
                    microseconds_per_quarter_note = 60 * 1000 * 1000 / tempo;
                    write_variable_length(trackData, delta_time);
                    trackData.push_back(0xFF);
                    trackData.push_back(0x51);
                    trackData.push_back(0x03);
                    trackData.push_back(uint8_t(microseconds_per_quarter_note >> 16));
                    trackData.push_back(uint8_t(microseconds_per_quarter_note >> 8));
                    trackData.push_back(uint8_t(microseconds_per_quarter_note));
                    delta_time = 0;
                } else {
                    delta_time += ticks_per_quarter_note * default_length / note.m_length;
                }
            }
        }

        // エンドオブトラック
        write_variable_length(trackData, delta_time);
        trackData.push_back(0xFF);
        trackData.push_back(0x2F);
        trackData.push_back(0x00);

        // トラックサイズとトラックデータを書き込む
        size_t trackSize = trackData.size();
        fseek(fout, trackSizePos, SEEK_SET);
        write_u32(fout, trackSize);
        fseek(fout, 0, SEEK_END);
        std::fwrite(trackData.data(), trackSize, 1, fout);
        trackData.clear();
    }

    // 演奏内容を含むトラックを書き込む
    for (size_t ch = 0; ch < block.size(); ++ch) {
        int tempo = 120;
        float volume = 8.0f, volume_at = -1;
        uint32_t microseconds_per_quarter_note = 60 * 1000 * 1000 / tempo;

        auto& phrase = block[ch];
        std::fwrite("MTrk", 4, 1, fout);
        size_t trackSizePos = ftell(fout);
        write_u32(fout, 0);

        int delta_time = 0, LR = 0x3;

        // プログラムチェンジ
        trackData.push_back(0x00);
        trackData.push_back(0xC0 + ch);
        trackData.push_back(phrase->m_setting.m_tone);

        // 音量設定
        trackData.push_back(0x00);
        trackData.push_back(0xB0 + ch);
        trackData.push_back(0x07);
        trackData.push_back(uint8_t(volume * 127.0f / 15.0f));

        for (auto& note : phrase->m_notes) {
            // テンポ設定
            if (tempo != note.m_tempo) {
                tempo = note.m_tempo;
                microseconds_per_quarter_note = 60 * 1000 * 1000 / tempo;
            }

            switch (note.m_key) {
            case KEY_REST: case KEY_SPECIAL_REST: // 休符
                delta_time += ticks_per_quarter_note * note.m_length / default_length;
                break;
            case KEY_TONE: // トーン変更
                // プログラムチェンジ
                write_variable_length(trackData, delta_time);
                trackData.push_back(0xC0 + ch);
                trackData.push_back(note.m_data);
                delta_time = 0;
                break;
            case KEY_SPECIAL_ACTION: // スペシャルアクション
            case KEY_REG: // レジスタ書き込み
            case KEY_ENVELOP_INTERVAL: // エンベロープ周期
            case KEY_ENVELOP_TYPE: // エンベロープ形状
            case KEY_OFF: // キーオフ
                // 無視
                break;
            case KEY_MIDI: // MIDIメッセージ
                switch (note.m_reg & 0xF0) {
                case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
                    // 第２データバイトあり
                    write_variable_length(trackData, delta_time);
                    trackData.push_back(note.m_reg);
                    trackData.push_back(HIBYTE(note.m_data));
                    trackData.push_back(LOBYTE(note.m_data));
                    delta_time = 0;
                    break;
                case 0xC0: case 0xD0:
                    // 第２データバイトなし
                    write_variable_length(trackData, delta_time);
                    trackData.push_back(note.m_reg);
                    trackData.push_back(HIBYTE(note.m_data));
                    delta_time = 0;
                    break;
                case 0xF0:
                    // サポートしない。
                    assert(0);
                    break;
                }
                break;
            default: // 普通の音符
                {
                    // 音量設定
                    if (volume_at != note.m_volume_at && volume_at != -1) {
                        volume_at = note.m_volume_at;
                        write_variable_length(trackData, delta_time);
                        trackData.push_back(0xB0 + ch);
                        trackData.push_back(0x07);
                        trackData.push_back(uint8_t(volume_at));
                        delta_time = 0;
                    } else if (volume != note.m_volume) {
                        volume = note.m_volume;
                        write_variable_length(trackData, delta_time);
                        trackData.push_back(0xB0 + ch);
                        trackData.push_back(0x07);
                        trackData.push_back(uint8_t(volume * 127.0f / 15.0f));
                        delta_time = 0;
                    }

                    // パン
                    if (LR != note.m_LR) {
                        LR = note.m_LR;
                        write_variable_length(trackData, delta_time);
                        trackData.push_back(0xB0 + ch);
                        trackData.push_back(0x0A);
                        if (LR == 0x3)
                            trackData.push_back(64); // 中央
                        else if (LR == 0x2)
                            trackData.push_back(0); // 左
                        else if (LR == 0x1)
                            trackData.push_back(127); // 右
                        else
                            assert(0);
                        delta_time = 0;
                    }

                    auto octave = note.m_octave;
                    auto length = note.m_length;
                    auto quantity = note.m_quantity;

                    // ノートオン
                    write_variable_length(trackData, delta_time);
                    trackData.push_back(0x90 + ch);
                    trackData.push_back(uint8_t(12 * octave + note.m_key));
                    trackData.push_back(127);
                    delta_time = ticks_per_quarter_note * length * quantity / default_length / max_quantity;

                    // ノートオフ
                    write_variable_length(trackData, delta_time);
                    trackData.push_back(0x80 + ch);
                    trackData.push_back(uint8_t(12 * octave + note.m_key));
                    trackData.push_back(127);
                    delta_time = ticks_per_quarter_note * length * (max_quantity - quantity) / default_length / max_quantity;
                }
                break;
            }
        }

        // エンドオブトラック
        write_variable_length(trackData, delta_time);
        trackData.push_back(0xFF);
        trackData.push_back(0x2F);
        trackData.push_back(0x00);

        // トラックサイズとトラックデータを書き込む
        size_t trackSize = trackData.size();
        fseek(fout, trackSizePos, SEEK_SET);
        write_u32(fout, trackSize);
        fseek(fout, 0, SEEK_END);
        std::fwrite(trackData.data(), trackSize, 1, fout);
        trackData.clear();
    }

    return true;
}

// MIDファイルとして保存
bool VskSoundPlayer::save_mid(VskScoreBlock& block, const wchar_t *filename)
{
    // MIDファイルを書き込み用として開く
    FILE *fout = _wfopen(filename, L"wb");
    if (!fout)
        return false;

    bool ret = write_mid_file(fout, block);
    std::fclose(fout);

    return ret;
}

// 演奏を停止
void VskSoundPlayer::stop()
{
    // 演奏停止を知らせる
    m_playing_music = false;
    m_stopping_event.pulse();

    // メロディーラインをクリア
    m_play_lock.lock();
    m_melody_line.clear();
    m_play_lock.unlock();
}

// スペシャルアクションを登録
void VskSoundPlayer::register_special_action(int action_no, VskSpecialActionFn fn)
{
    m_action_no_to_special_action[action_no] = fn;
}

// 特定のスペシャルアクションを実行する
void VskSoundPlayer::do_special_action(int action_no)
{
    auto fn = m_action_no_to_special_action[action_no];
    if (fn)
        (*fn)(action_no);
    else
        std::printf("special action X%d\n", action_no);
}

//////////////////////////////////////////////////////////////////////////////

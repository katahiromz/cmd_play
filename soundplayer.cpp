﻿//////////////////////////////////////////////////////////////////////////////
// soundplayer --- an fmgon sound player
// Copyright (C) 2015-2025 Katayama Hirofumi MZ. All Rights Reserved.

#include "fmgon/fmgon.h"
#include "soundplayer.h"
#include "sound.h"
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
        if (!m_notes[i].m_and) {
            new_notes.push_back(m_notes[i]);
            continue;
        }

        // タイがあった。長さと秒数を再計算
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
void VskPhrase::realize(VskSoundPlayer *player, VSK_PCM16_VALUE*& data, size_t *pdata_size) {
    calc_gate_and_goal();
    rescan_notes();

    m_player = player;
    YM2203& ym = player->m_ym;

    // Allocate the wave data
    auto count = uint32_t(m_goal * SAMPLERATE * 2); // stereo
    if (count % 2)
        ++count;
    *pdata_size = count * sizeof(VSK_PCM16_VALUE);
    data = new VSK_PCM16_VALUE[count];
    std::memset(&data[0], 0, *pdata_size);

    uint32_t isample = 0;
    if (m_setting.m_fm) { // FM sound?
        int ch = FM_CH1;

        auto& timbre = m_setting.m_timbre;
        timbre.set(ym2203_tone_table[m_setting.m_tone]);
        ym.set_timbre(ch, &timbre);
        VskLFOCtrl lc;

        for (auto& note : m_notes) { // For each note
            if (note.m_key == KEY_SPECIAL_ACTION) { // Special action?
                schedule_special_action(note.m_gate, note.m_data);
                continue;
            }

            if (note.m_key == KEY_TONE) { // Tone change?
                const auto new_tone = note.m_data;
                assert((0 <= new_tone) && (new_tone < NUM_TONES));
                timbre.set(ym2203_tone_table[new_tone]);
                ym.set_timbre(ch, &timbre);
                lc.init_for_timbre(&timbre);
                continue;
            }

            if (note.m_key == KEY_REG) { // Register?
                m_player->write_reg(note.m_reg, note.m_data);
                continue;
            }

            if (note.m_key == KEY_ENVELOP_INTERVAL) {
                auto interval = note.m_data;
                m_player->write_reg(ADDR_SSG_ENV_FREQ_L, (interval & 0xFF));
                m_player->write_reg(ADDR_SSG_ENV_FREQ_H, ((interval >> 8) & 0xFF));
                continue;
            }

            if (note.m_key == KEY_ENVELOP_TYPE) {
                auto type = note.m_data;
                m_player->write_reg(ADDR_SSG_ENV_TYPE, (type & 0x0F));
                continue;
            }

            if (note.m_key != KEY_SPECIAL_REST) { // Not special rest?
                // do key on
                if (note.m_key != KEY_REST) { // Has key?
                    ym.set_pitch(ch, note.m_octave, note.m_key);
                    ym.set_volume(ch, int(note.m_volume));
                    ym.note_on(ch);
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
                if (note.m_key != KEY_REST && note.m_key != KEY_SPECIAL_REST) {
                    lc.increment();
                    int adj[4] = {
                        int(lc.m_adj_v[0]), int(lc.m_adj_v[1]),
                        int(lc.m_adj_v[2]), int(lc.m_adj_v[3]),
                    };
                    ym.set_volume(ch, int(note.m_volume), adj);
                    ym.set_pitch(ch, note.m_octave, note.m_key, int(lc.m_adj_p));
                }
                nsamples -= unit;
            }
            ym.count(uint32_t(sec * 1000 * 1000));
            isample += nsamples;

            sec = note.m_sec * (8.0f - note.m_quantity) / 8.0f;
            nsamples = int(SAMPLERATE * sec);
            if (note.m_key != KEY_SPECIAL_REST) {
                // do key off
                ym.note_off(ch);
            }
            unit = SAMPLERATE;
            if (unit > nsamples) {
                unit = nsamples;
            }
            ym.mix(&data[isample * 2], unit);
            ym.count(uint32_t(sec * 1000 * 1000));
            isample += nsamples;
        }
    } else { // SSG sound?
        int ch = SSG_CH_A;

        ym.set_tone_or_noise(ch, TONE_MODE);

        for (auto& note : m_notes) {
            if (note.m_key == KEY_SPECIAL_ACTION) { // Special action?
                schedule_special_action(note.m_gate, note.m_data);
                continue;
            }

            if (note.m_key == KEY_REG) { // Register?
                m_player->write_reg(note.m_reg, note.m_data);
                continue;
            }

            if (note.m_key == KEY_ENVELOP_INTERVAL) {
                auto interval = note.m_data;
                m_player->write_reg(ADDR_SSG_ENV_FREQ_L, (interval & 0xFF));
                m_player->write_reg(ADDR_SSG_ENV_FREQ_H, ((interval >> 8) & 0xFF));
                continue;
            }

            if (note.m_key == KEY_ENVELOP_TYPE) {
                auto type = note.m_data;
                m_player->write_reg(ADDR_SSG_ENV_TYPE, (type & 0x0F));
                continue;
            }

            // do key on
            if (note.m_key != KEY_REST && note.m_key != KEY_SPECIAL_REST) {
                ym.set_pitch(ch, note.m_octave, note.m_key);
                ym.set_volume(ch, int(note.m_volume));
                ym.note_on(ch);
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
                ym.note_off(ch);
            }
            ym.mix(&data[isample * 2], nsamples);
            ym.count(uint32_t(sec * 1000 * 1000));
            isample += nsamples;
        }
    }
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
    m_ym.init(CLOCK, SAMPLERATE, rhythm_path);
}

bool VskSoundPlayer::wait_for_stop(uint32_t milliseconds) {
    // 演奏が終わるか、指定時間が経過するまで待つ
    return m_stopping_event.wait_for_event(milliseconds);
}

bool VskSoundPlayer::play_and_wait(VskScoreBlock& block, uint32_t milliseconds, bool stereo) {
    // 演奏して終わるのを待つ
    play(block, stereo);
    return wait_for_stop(milliseconds);
}

// PCM波形を生成する
bool VskSoundPlayer::generate_pcm_raw(VskScoreBlock& block, std::vector<VSK_PCM16_VALUE>& values, bool stereo) {
    // ステレオ音声として波形を実現する
    const int source_num_channels = 2;
    std::vector<VSK_PCM16_VALUE*> raw_data;
    std::vector<size_t> data_sizes;
    for (auto& phrase : block) {
        if (phrase) {
            VSK_PCM16_VALUE *data;
            size_t data_size;
            phrase->realize(this, data, &data_size);
            raw_data.push_back(data);
            data_sizes.push_back(data_size);
        }
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
    for (size_t ivalue = 0; ivalue < source_num_values; ++ivalue) {
        if (!stereo && (ivalue & 1))
            continue; // モノラルでivalueが奇数の場合は無視

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

        // 作成する波形がステレオでなければモノラルにする。転送先に格納
        int32_t sample_value = ((ivalue < data_size) ? value : 0);
        if (stereo) {
            values[ivalue] = sample_value;
        } else {
            values[ivalue / 2] = sample_value;
        }
    }

    // 後始末
    for (auto entry : raw_data) {
        delete[] entry;
    }

    return true;
}

// 音声をWAVファイルとして保存
bool VskSoundPlayer::save_as_wav(VskScoreBlock& block, const wchar_t *filename, bool stereo) {
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

// 演奏を開始する
void VskSoundPlayer::play(VskScoreBlock& block, bool stereo) {
    // 波形を生成
    generate_pcm_raw(block, m_pcm_values, stereo);

    // スペシャルアクションを実行
    for (auto& phrase : block) {
        phrase->execute_special_actions();
    }

    // 波形に基づいて演奏
    vsk_sound_play(m_pcm_values.data(), m_pcm_values.size() * sizeof(VSK_PCM16_VALUE), stereo);
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

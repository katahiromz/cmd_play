﻿#include "types.h"
#include "sound.h"
#include "encoding.h"
#include "ast.h"
#include <unordered_set>
#include <cstdio>
#include <cassert>

#include "soundplayer.h"                // サウンドプレーヤー
#include "scanner.h"                    // VskScanner

// サウンドプレーヤー
extern std::shared_ptr<VskSoundPlayer> vsk_sound_player;

#define VSK_MAX_CHANNEL 6

// CMD PLAYの現在の設定
VskSoundSetting vsk_fm_sound_settings[VSK_MAX_CHANNEL];
VskSoundSetting vsk_ssg_sound_settings[VSK_MAX_CHANNEL];
VskSoundSetting vsk_midi_sound_settings[VSK_MAX_CHANNEL];

// 設定のリセット１
void vsk_cmd_play_stopm(void)
{
    for (auto& item : vsk_fm_sound_settings)
        item.stopm();
    for (auto& item : vsk_ssg_sound_settings)
        item.stopm();
    for (auto& item : vsk_midi_sound_settings)
        item.stopm();
}

// 設定のリセット２
void vsk_cmd_play_reset_settings(void)
{
    for (auto& item : vsk_fm_sound_settings)
        item = VskSoundSetting();
    for (auto& item : vsk_ssg_sound_settings)
        item = VskSoundSetting();
    for (auto& item : vsk_midi_sound_settings)
        item = VskSoundSetting();
}

// 設定のサイズ
size_t vsk_cmd_play_get_setting_size(void)
{
    return sizeof(VskSoundSetting);
}

// 設定の取得
bool vsk_cmd_play_get_setting(int ch, std::vector<uint8_t>& data)
{
    data.resize(sizeof(VskSoundSetting));
    switch (ch)
    {
    case 0: case 1: case 2: case 3: case 4: case 5:
        std::memcpy(data.data(), &vsk_fm_sound_settings[ch], sizeof(VskSoundSetting));
        return true;
    case 6: case 7: case 8: case 9: case 10: case 11:
        std::memcpy(data.data(), &vsk_ssg_sound_settings[ch - 6], sizeof(VskSoundSetting));
        return true;
    case 12: case 13: case 14: case 15: case 16: case 17:
        std::memcpy(data.data(), &vsk_midi_sound_settings[ch - 12], sizeof(VskSoundSetting));
        return true;
    default:
        return false;
    }
}

// 音色を適用する
bool vsk_cmd_play_voice(int ich, const void *data, size_t data_size)
{
    if (ich < 0 || ich >= VSK_MAX_CHANNEL)
    {
        assert(0);
        return false;
    }

    timbre_array_t array;
    if (sizeof(array) != data_size)
    {
        assert(0);
        return false;
    }

    std::memcpy(array, data, data_size);
    vsk_fm_sound_settings[ich].m_timbre.set(array);
    return true;
}

// 設定の設定
bool vsk_cmd_play_set_setting(int ch, const std::vector<uint8_t>& data)
{
    if (data.size() != sizeof(VskSoundSetting))
        return false;
    switch (ch)
    {
    case 0: case 1: case 2: case 3: case 4: case 5:
        std::memcpy(&vsk_fm_sound_settings[ch], data.data(), sizeof(VskSoundSetting));
        return true;
    case 6: case 7: case 8: case 9: case 10: case 11:
        std::memcpy(&vsk_ssg_sound_settings[ch - 6], data.data(), sizeof(VskSoundSetting));
        return true;
    case 12: case 13: case 14: case 15: case 16: case 17:
        std::memcpy(&vsk_midi_sound_settings[ch - 12], data.data(), sizeof(VskSoundSetting));
        return true;
    default:
        return false;
    }
}

// 再帰的に「[変数名]」を変数の値に置き換える関数
std::string
vsk_replace_play_placeholders(const std::string& str, std::unordered_set<std::string>& visited) {
    std::string result = str;
    size_t start_pos = 0;

    while ((start_pos = result.find("[", start_pos)) != result.npos) {
        size_t end_pos = result.find("]", start_pos);
        if (end_pos == std::string::npos)
            break; // 閉じカッコが見つからない場合は終了

        std::string key = result.substr(start_pos + 1, end_pos - start_pos - 1);
        CharUpperA(&key[0]);
        if (visited.find(key) != visited.end()) {
            // 循環参照を検出した場合はエラーとして処理する
            throw std::runtime_error("circular reference detected");
        }
        visited.insert(key);

        auto it = g_variables.find(key);
        if (it != g_variables.end()) {
            // ここで再帰的に置き換えを行う
            std::string value = vsk_replace_play_placeholders(it->second, visited);
            result.replace(start_pos, end_pos - start_pos + 1, value);
            start_pos += value.length(); // 置き換えた後の新しい開始位置に移動
        } else {
            result.replace(start_pos, end_pos - start_pos + 1, "");
        }
        visited.erase(key);
    }

    return result;
}

// 再帰的に「[変数名]」を変数の値に置き換える関数
std::string vsk_replace_play_placeholders(const std::string& str)
{
    std::unordered_set<std::string> visited;
    return vsk_replace_play_placeholders(str, visited);
}

//////////////////////////////////////////////////////////////////////////////
// VskPlayItem --- CMD PLAY 用の演奏項目

struct VskPlayItem
{
    VskString               m_subcommand;   // コマンド
    VskString               m_param;        // 引数1
    VskString               m_param2;       // 引数2
    char                    m_sign;         // +, -, #
    bool                    m_dot;          // .
    bool                    m_and;          // &
    int                     m_plet_count;   // 連符のカウント
    int                     m_plet_L;       // 連符の長さ

    VskPlayItem() { clear(); }

    void clear() {
        m_subcommand.clear();
        m_param.clear();
        m_param2.clear();
        m_sign = 0;
        m_dot = false;
        m_and = false;
        m_plet_count = 1;
        m_plet_L = 0;
    }
};

// 演奏項目を再スキャン (Pass 2)
bool vsk_rescan_play_items(std::vector<VskPlayItem>& items)
{
    // 連符の設定
    size_t k = VskString::npos;
    int level = 0, real_note_count = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].m_subcommand == "{") { // 連符の始まり
            k = i;
            if (level == 0)
                real_note_count = 0;
            ++level;
        } else if (items[i].m_subcommand == "}") {
            --level;
            if (level < 0)
                return false;
            if (level == 0) { // 連符の終わり
                if (items[i].m_param.empty()) {
                    for (size_t m = k + 1; m < i; ++m) {
                        items[m].m_plet_count = real_note_count;
                        items[m].m_plet_L = 0;
                    }
                } else {
                    int plet_L = atoi(items[i].m_param.c_str());
                    for (size_t m = k + 1; m < i; ++m) {
                        items[m].m_plet_count = real_note_count;
                        items[m].m_plet_L = plet_L;
                    }
                }
                items.erase(items.begin() + i);
                items.erase(items.begin() + k);
                i -= 2;
            }
        } else { // 連符の中の本当の音符の数を数える
            char ch = items[i].m_subcommand[0];
            switch (ch) {
            case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
            case 'R': case 'N':
                ++real_note_count;
                break;
            default:
                if (items[i].m_subcommand == "@W")
                    ++real_note_count;
            }
        }
    }
    return true;
} // vsk_rescan_play_items

// 演奏項目をスキャン (Pass 1)
bool vsk_scan_play_param(const char *& pch, VskPlayItem& item)
{
    while (vsk_isblank(*pch)) ++pch;

    if (vsk_isdigit(*pch)) {
        for (;; pch++) {
            if (vsk_isblank(*pch))
                continue;
            if (!vsk_isdigit(*pch))
                break;
            item.m_param += *pch;
        }
    } else if (*pch == '=') {
        for (++pch; *pch && *pch != ';'; ++pch) {
            if (!vsk_isblank(*pch))
                item.m_param += *pch;
        }

        if (!*pch)
            return false;
        ++pch;
    }

    return true;
} // vsk_scan_play_param

// 演奏項目を評価する
bool vsk_eval_cmd_play_items(std::vector<VskPlayItem>& items, const VskString& expr)
{
    VskString str = vsk_replace_play_placeholders(expr);
    const char *pch = str.c_str();
    items.clear();

    // MMLのパース
    VskPlayItem item;
    char ch, prev_ch = 0;
    while (*pch != 0) {
        prev_ch = ch;
        ch = vsk_toupper(*pch++); // 大文字にする

        // 'Y'コマンドが不完全だと失敗
        if (prev_ch == 'Y' && ch != ',')
            return false;

        // 終端文字なら終了
        if (ch == 0)
            break;

        // 空白は無視
        if (vsk_isblank(ch))
            continue;

        switch (ch) {
        case ' ': case '\t': // blank
            continue;
        case '&': case '^':
            // タイ
            if (items.size()) {
                items.back().m_and = true;
            }
            continue;
        case '<': case '>':
            // オクターブを増減する
            item.m_subcommand = {ch};
            break;
        case '{':
            // n連符の始め
            item.m_subcommand = {ch};
            break;
        case '}':
            // n連符の終わり
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            break;
        case 'Y':
            // OPNレジスタ
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            break;
        case ',':
            // OPNレジスタ
            if (prev_ch != 'Y')
                return false;
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            break;
        case 'R':
            // 休符
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            // 付点
            if (*pch == '.') {
                item.m_dot = true;
                ++pch;
            }
            break;
        case 'N':
            // 指定された高さの音
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            // 付点
            if (*pch == '.') {
                item.m_dot = true;
                ++pch;
            }
            break;
        case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
            // 音符
            item.m_subcommand = {ch};
            while (vsk_isblank(*pch)) ++pch;
            // シャープとフラット
            switch (*pch) {
            case '-': case '#': case '+':
                item.m_sign = *pch++;
                break;
            }
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            // 付点
            if (*pch == '.') {
                item.m_dot = true;
                ++pch;
            }
            break;
        case '@':
            ch = vsk_toupper(*pch++);
            switch (ch) {
            case 'V': case 'W':
                // "@V", "@W"
                item.m_subcommand = {'@', ch};
                // パラメータ
                if (!vsk_scan_play_param(pch, item))
                    return false;
                break;
            case 'L': case 'M': case 'R':
                // "@L", "@M", "@R"
                item.m_subcommand = {'@', ch};
                break;
            default:
                // "@": 音色を変える
                --pch;
                item.m_subcommand = "@";
                // パラメータ
                if (!vsk_scan_play_param(pch, item))
                    return false;
                break;
            }
            break;
        case 'M': case 'S': case 'V': case 'L': case 'Q':
        case 'O': case 'T': case 'Z':
            // その他のMML
            item.m_subcommand = {ch};
            // パラメータ
            if (!vsk_scan_play_param(pch, item))
                return false;
            break;
        default:
            return false;
        }
        items.push_back(item);
        item.clear();
    }

    return vsk_rescan_play_items(items);
} // vsk_eval_cmd_play_items

VskAstPtr vsk_get_play_param(const VskPlayItem& item)
{
    if (item.m_param.empty())
        return nullptr;
    return vsk_eval_cmd_play_text(item.m_param);
} // vsk_get_play_param

bool vsk_phrase_from_cmd_play_items(std::shared_ptr<VskPhrase> phrase, const std::vector<VskPlayItem>& items)
{
    float length;
    int key = 0;
    char ch = 0;
    static int s_r = -1; // コマンド'Y'チェック用
    static int s_iZ = 0; // コマンド'Z'チェック用
    for (auto& item : items) {
        ch = item.m_subcommand[0];

        if (ch != ',')
            s_r = -1;

        if (ch != 'Z')
            s_iZ = 0;

        bool length_set;
        switch (ch) {
        case ' ': case '\t': // blank
            continue;
        case 'M':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if (1 <= i0 && i0 <= 65535) {
                    phrase->add_envelop_interval(ch, i0);
                    continue;
                }
            } else {
                phrase->add_envelop_interval(ch, 255);
                continue;
            }
            return false;
        case 'S':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if (0 <= i0 && i0 <= 15) {
                    phrase->add_envelop_type(ch, i0);
                    continue;
                }
            } else {
                phrase->add_envelop_type(ch, 1);
                continue;
            }
            return false;
        case 'V':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((0 <= i0) && (i0 <= 15)) {
                    phrase->m_setting.m_volume = (float)i0;
                    continue;
                }
                return false;
            } else {
                phrase->m_setting.m_volume = 8;
            }
            continue;
        case 'L':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((1 <= i0) && (i0 <= 64)) {
                    phrase->m_setting.m_length = (24.0f * 4.0f) / i0;
                    continue;
                }
                return false;
            } else {
                phrase->m_setting.m_length = (24.0f * 4.0f) / 4;
            }
            continue;
        case 'Q':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((0 <= i0) && (i0 <= 8)) {
                    phrase->m_setting.m_quantity = i0;
                    continue;
                }
                return false;
            } else {
                phrase->m_setting.m_quantity = 8;
            }
            continue;
        case 'O':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((1 <= i0) && (i0 <= 8)) {
                    phrase->m_setting.m_octave = i0 - 1;
                    continue;
                }
                return false;
            } else {
                phrase->m_setting.m_octave = 4 - 1;
            }
            continue;
        case '<':
            if (0 < phrase->m_setting.m_octave) {
                (phrase->m_setting.m_octave)--;
                continue;
            }
            return false;
        case '>':
            if (phrase->m_setting.m_octave < 8) {
                (phrase->m_setting.m_octave)++;
                continue;
            }
            return false;
        case 'N':
            length = phrase->m_setting.m_length;

            length_set = false;
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((0 <= i0) && (i0 <= 96)) {
                    key = i0;
                    if (key >= 96) {
                        key = 0;
                    }
                    length_set = true;
                } else {
                    return false;
                }
            } else {
                return false;
            }

            // 連符ならば、連符の長さを調整
            if (!length_set && item.m_plet_count > 1) {
                auto L = item.m_plet_L;
                if (L == 0) {
                    length = phrase->m_setting.m_length;
                } else if ((1 <= L) && (L <= 64)) {
                    length = 24.0f * 4 / L;
                } else {
                    return false;
                }
                length /= item.m_plet_count;
            }

            phrase->add_key(key, item.m_dot, length, item.m_sign);
            phrase->m_notes.back().m_and = item.m_and;
            continue;
        case 'T':
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((32 <= i0) && (i0 <= 255)) {
                    phrase->m_setting.m_tempo = i0;
                    continue;
                }
                return false;
            } else {
                phrase->m_setting.m_tempo = 120;
            }
            continue;
        case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'A': case 'B': case 'R':
            length = phrase->m_setting.m_length;
            length_set = false;
            if (auto ast = vsk_get_play_param(item)) {
                auto L = ast->to_int();
                // NOTE: 24 is the length of a quarter note
                if ((1 <= L) && (L <= 64)) {
                    length = float(24 * 4 / L);
                    length_set = true;
                } else {
                    return false;
                }
            }

            // 連符ならば連符の長さを調整
            if (!length_set && item.m_plet_count > 1) {
                auto L = item.m_plet_L;
                if (L == 0) {
                    length = phrase->m_setting.m_length;
                } else if ((1 <= L) && (L <= 64)) {
                    length = 24.0f * 4 / L;
                } else {
                    return false;
                }
                length /= item.m_plet_count;
            }

            phrase->add_note(ch, item.m_dot, length, item.m_sign);
            phrase->m_notes.back().m_and = item.m_and;
            continue;
        case '@':
            if (item.m_subcommand == "@") {
                if (auto ast = vsk_get_play_param(item)) {
                    auto i0 = ast->to_int();
                    switch (phrase->m_setting.m_audio_type) {
                    case AUDIO_TYPE_SSG:
                        // SSG音源は音色を変えられない
                        continue;
                    case AUDIO_TYPE_FM:
                        if ((0 <= i0) && (i0 <= 61)) {
                            phrase->add_tone(ch, i0);
                            phrase->m_setting.m_tone = i0;
                            continue;
                        }
                        break;
                    case AUDIO_TYPE_MIDI:
                        if ((0 <= i0) && (i0 <= 127)) {
                            phrase->add_tone(ch, i0);
                            phrase->m_setting.m_tone = i0;
                            continue;
                        }
                        break;
                    default:
                        assert(0);
                    }
                }
            } else if (item.m_subcommand == "@V") {
                if (auto ast = vsk_get_play_param(item)) {
                    auto i0 = ast->to_int();
                    if ((0 <= i0) && (i0 <= 127)) {
                        phrase->m_setting.m_volume_at = i0;
                        continue;
                    }
                }
            } else if (item.m_subcommand == "@W") { // 特殊な休符
                length = phrase->m_setting.m_length;
                length_set = false;
                if (auto ast = vsk_get_play_param(item)) {
                    auto L = ast->to_int();
                    // NOTE: 24 is the length of a quarter note
                    if ((1 <= L) && (L <= 64)) {
                        length = float(24 * 4 / L);
                        length_set = true;
                    } else {
                        return false;
                    }
                }

                // 連符ならば、連符の長さを調整
                if (!length_set && item.m_plet_count > 1) {
                    auto L = item.m_plet_L;
                    if (L == 0) {
                        length = phrase->m_setting.m_length;
                    } else if ((1 <= L) && (L <= 64)) {
                        length = 24.0f * 4 / L;
                    } else {
                        return false;
                    }
                    length /= item.m_plet_count;
                }

                phrase->add_note('W', item.m_dot, length, item.m_sign);
                phrase->m_notes.back().m_and = item.m_and;
                continue;
            } else if (item.m_subcommand == "@L") { // LEFT (左)
                phrase->m_setting.m_LR = 0x2;
                continue;
            } else if (item.m_subcommand == "@M") { // MIDDLE (中央)
                phrase->m_setting.m_LR = 0x3;
                continue;
            } else if (item.m_subcommand == "@R") { // RIGHT (右)
                phrase->m_setting.m_LR = 0x1;
                continue;
            }
            return false;
        case 'Y': case ',': // OPNメッセージ
            if (ch == 'Y') {
                if (auto ast = vsk_get_play_param(item)) {
                    s_r = ast->to_int();
                    if ((0 <= s_r) && (s_r <= 178))
                        continue;
                }
            } else {
                if (auto ast = vsk_get_play_param(item)) {
                    int d = ast->to_int();
                    if ((0 <= s_r) && (s_r <= 178) && (0 <= d) && (d <= 255)) {
                        phrase->add_reg('Y', s_r, d);
                        s_r = -1;
                        continue;
                    }
                }
            }
            return false;
        case 'Z': // MIDIメッセージ
            {
                static int s_status = 0;
                static int s_data1 = 0, s_data2 = 0;
                static bool s_has_data2 = false;
                switch (s_iZ) {
                case 0:
                    if (auto ast = vsk_get_play_param(item)) {
                        s_status = ast->to_int();
                        switch (s_status & 0xF0) {
                            case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:
                                // 第２データバイトあり
                                s_has_data2 = true;
                                break;
                            case 0xC0: case 0xD0:
                                // 第２データバイトなし
                                s_has_data2 = false;
                                break;
                            default:
                                return false;
                        }
                        ++s_iZ;
                        continue;
                    }
                    break;
                case 1:
                    if (auto ast = vsk_get_play_param(item)) {
                        s_data1 = ast->to_int();
                        if ((0 <= s_data1) && (s_data1 <= 256)) {
                            if (s_has_data2) {
                                ++s_iZ;
                            } else {
                                phrase->add_reg('Z', s_status, MAKEWORD(0, s_data1));
                                s_iZ = 0;
                            }
                            continue;
                        }
                    }
                    break;
                case 2:
                    if (auto ast = vsk_get_play_param(item)) {
                        s_data2 = ast->to_int();
                        assert(!s_has_data2);
                        if ((0 <= s_data2) && (s_data2 <= 256)) {
                            phrase->add_reg('Z', s_status, MAKEWORD(s_data2, s_data1));
                            s_iZ = 0;
                            continue;
                        }
                    }
                    break;
                }
                return false;
            }
            break;
        default:
            assert(0);
            break;
        }
    }

    return ch != 'Y' && s_iZ == 0;
} // vsk_phrase_from_cmd_play_items

//////////////////////////////////////////////////////////////////////////////

// SSG音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_ssg(const std::vector<VskString>& strs, bool stereo, bool no_sound)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_ssg_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_SSG;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!no_sound)
    {
        // play now
        vsk_sound_player->play(block, stereo);
    }

    return VSK_SOUND_ERR_SUCCESS;
}

// FM+SSG音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg(const std::vector<VskString>& strs, bool stereo, bool no_sound)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(
            (iChannel < 3) ? vsk_fm_sound_settings[iChannel] : vsk_ssg_sound_settings[iChannel - 3]
        );
        phrase->m_setting.m_audio_type = ((iChannel < 3) ? AUDIO_TYPE_FM : AUDIO_TYPE_SSG);
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!no_sound)
    {
        // play now
        vsk_sound_player->play(block, stereo);
    }

    return VSK_SOUND_ERR_SUCCESS;
}

// FM音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_fm(const std::vector<VskString>& strs, bool stereo, bool no_sound)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_fm_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_FM;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!no_sound)
    {
        // play now
        vsk_sound_player->play(block, stereo);
    }

    return VSK_SOUND_ERR_SUCCESS;
}

// MIDI音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_midi(const std::vector<VskString>& strs, bool no_sound)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_midi_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_MIDI;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!no_sound)
    {
        // play now
        vsk_sound_player->play_midi(block);
    }

    return VSK_SOUND_ERR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////

// SSG音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_ssg_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_SSG;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR;

    return VSK_SOUND_ERR_SUCCESS;
}

// FM+SSG音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL;

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(
            (iChannel < 3) ? vsk_fm_sound_settings[iChannel] : vsk_ssg_sound_settings[iChannel - 3]
        );
        phrase->m_setting.m_audio_type = ((iChannel < 3) ? AUDIO_TYPE_FM : AUDIO_TYPE_SSG);
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

// FM音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_fm_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL; // 失敗

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_fm_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_FM;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL; // 失敗

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

// MIDI音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_midi_save(const std::vector<VskString>& strs, const wchar_t *filename)
{
    assert(strs.size() <= VSK_MAX_CHANNEL);
    size_t iChannel = 0;

    // add phrases to block
    VskScoreBlock block;
    // for each channel strings
    for (auto& str : strs) {
        // get play items
        std::vector<VskPlayItem> items;
        if (!vsk_eval_cmd_play_items(items, str))
            return VSK_SOUND_ERR_ILLEGAL; // 失敗

        // create phrase
        auto phrase = std::make_shared<VskPhrase>(vsk_midi_sound_settings[iChannel]);
        phrase->m_setting.m_audio_type = AUDIO_TYPE_MIDI;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL; // 失敗

        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_mid(block, filename))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

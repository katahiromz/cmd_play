#include "types.h"
#include "sound.h"
#include "encoding.h"
#include "ast.h"
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
    default:
        return false;
    }
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
    default:
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////////
// VskPlayItem --- CMD PLAY 用の演奏項目

struct VskPlayItem
{
    VskString               m_subcommand;
    VskString               m_param;
    VskString               m_param2;
    char                    m_sign;     // +, -, #
    bool                    m_dot;      // .
    bool                    m_and;      // &
    int                     m_plet_count;
    int                     m_plet_L;

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
    size_t k = VskString::npos;
    int level = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].m_subcommand == "{") {
            k = i;
            ++level;
        }
        if (items[i].m_subcommand == "}") {
            --level;
            if (level == 0) {
                int plet_count = int(i - (k + 1));
                int plet_L = atoi(items[i].m_param.c_str());
                for (size_t m = k + 1; m < i; ++m) {
                    items[m].m_plet_count = plet_count;
                    items[m].m_plet_L = plet_L;
                }
                items.erase(items.begin() + i);
                items.erase(items.begin() + k);
                --i;
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

std::string vsk_replace_placeholders2(const std::string& str);

// 演奏項目を評価する
bool vsk_eval_cmd_play_items(std::vector<VskPlayItem>& items, const VskString& expr)
{
    VskString str = vsk_replace_placeholders2(expr);
    const char *pch = str.c_str();
    items.clear();

    // MMLのパース
    VskPlayItem item;
    while (*pch != 0) {
        char ch = vsk_toupper(*pch++);
        if (ch == 0)
            break;
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
        case 'Y': case ',':
            // OPNレジスタ
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
    for (auto& item : items) {
        char ch = item.m_subcommand[0];
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
                phrase->m_setting.m_length = (24.0f * 4.0f);
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
            if (auto ast = vsk_get_play_param(item)) {
                auto i0 = ast->to_int();
                if ((0 <= i0) && (i0 <= 96)) {
                    key = i0;
                    if (key >= 96) {
                        key = 0;
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
            if ((item.m_plet_count > 1) && (item.m_plet_L != 0)) {
                auto L = item.m_plet_L;
                if ((1 <= L) && (L <= 64)) {
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
            if (auto ast = vsk_get_play_param(item)) {
                auto L = ast->to_int();
                // NOTE: 24 is the length of a quarter note
                if ((1 <= L) && (L <= 64)) {
                    length = float(24 * 4 / L);
                } else {
                    return false;
                }
            }
            if ((item.m_plet_count > 1) && (item.m_plet_L != 0)) {
                auto L = item.m_plet_L;
                if ((1 <= L) && (L <= 64)) {
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
                    if ((0 <= i0) && (i0 <= 61)) {
                        phrase->add_tone(ch, i0);
                        phrase->m_setting.m_tone = i0;
                        continue;
                    }
                }
            } else if (item.m_subcommand == "@V") {
                if (auto ast = vsk_get_play_param(item)) {
                    auto i0 = ast->to_int();
                    if ((0 <= i0) && (i0 <= 127)) {
                        phrase->m_setting.m_volume =  i0 * (15.0f / 127.0f);
                        continue;
                    }
                }
            } else if (item.m_subcommand == "@W") { // 特殊な休符
                length = phrase->m_setting.m_length;
                if (auto ast = vsk_get_play_param(item)) {
                    auto L = ast->to_int();
                    // NOTE: 24 is the length of a quarter note
                    if ((1 <= L) && (L <= 64)) {
                        length = float(24 * 4 / L);
                    } else {
                        return false;
                    }
                }
                if ((item.m_plet_count > 1) && (item.m_plet_L != 0)) {
                    auto L = item.m_plet_L;
                    if ((1 <= L) && (L <= 64)) {
                        length = 24.0f * 4 / L;
                    } else {
                        return false;
                    }
                    length /= item.m_plet_count;
                }
                phrase->add_note('W', item.m_dot, length, item.m_sign);
                phrase->m_notes.back().m_and = item.m_and;
                continue;
            }
            return false;
        case 'Y':
        case ',':
            {
                static int r = 0;
                if (ch == 'Y') {
                    if (auto ast = vsk_get_play_param(item)) {
                        r = ast->to_int();
                        continue;
                    }
                    return false;
                } else {
                    if (auto ast = vsk_get_play_param(item)) {
                        int d = ast->to_int();
                        phrase->add_reg('Y', r, d);
                        continue;
                    }
                    return false;
                }
            }
            continue;
        default:
            assert(0);
            break;
        }
    }
    return true;
} // vsk_phrase_from_cmd_play_items

//////////////////////////////////////////////////////////////////////////////

// SSG音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_ssg(const std::vector<VskString>& strs, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        phrase->m_setting = vsk_ssg_sound_settings[iChannel];
        phrase->m_setting.m_fm = false;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // apply settings
        vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        // add phrase
        block.push_back(phrase);
        // apply settings
        vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        // next channel
        ++iChannel;
    }

    // play now
    vsk_sound_player->play(block, stereo);

    return VSK_SOUND_ERR_SUCCESS;
}

// FM+SSG音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg(const std::vector<VskString>& strs, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        if (iChannel < 3) {
            phrase->m_setting = vsk_fm_sound_settings[iChannel];
            phrase->m_setting.m_fm = true;
        } else {
            phrase->m_setting = vsk_ssg_sound_settings[iChannel - 3];
            phrase->m_setting.m_fm = false;
        }
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // apply settings
        if (iChannel < 3) {
            vsk_fm_sound_settings[iChannel] = phrase->m_setting;
        } else {
            vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        }
        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    // play now
    vsk_sound_player->play(block, stereo);

    return VSK_SOUND_ERR_SUCCESS;
}

// FM音源で音楽再生
VSK_SOUND_ERR vsk_sound_cmd_play_fm(const std::vector<VskString>& strs, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        phrase->m_setting = vsk_fm_sound_settings[iChannel];
        phrase->m_setting.m_fm = true;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // apply settings
        vsk_fm_sound_settings[iChannel] = phrase->m_setting;
        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    // play now
    vsk_sound_player->play(block, stereo);

    return VSK_SOUND_ERR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////

// SSG音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        phrase->m_setting = vsk_ssg_sound_settings[iChannel];
        phrase->m_setting.m_fm = false;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // apply settings
        vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        // add phrase
        block.push_back(phrase);
        // apply settings
        vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_as_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR;

    return VSK_SOUND_ERR_SUCCESS;
}

// FM+SSG音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        if (iChannel < 3) {
            phrase->m_setting = vsk_fm_sound_settings[iChannel];
            phrase->m_setting.m_fm = true;
        } else {
            phrase->m_setting = vsk_ssg_sound_settings[iChannel - 3];
            phrase->m_setting.m_fm = false;
        }
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL;

        // apply settings
        if (iChannel < 3) {
            vsk_fm_sound_settings[iChannel] = phrase->m_setting;
        } else {
            vsk_ssg_sound_settings[iChannel] = phrase->m_setting;
        }
        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_as_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

// FM音源で音楽保存
VSK_SOUND_ERR vsk_sound_cmd_play_fm_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo)
{
    assert(strs.size() < VSK_MAX_CHANNEL);
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
        auto phrase = std::make_shared<VskPhrase>();
        phrase->m_setting = vsk_fm_sound_settings[iChannel];
        phrase->m_setting.m_fm = true;
        if (!vsk_phrase_from_cmd_play_items(phrase, items))
            return VSK_SOUND_ERR_ILLEGAL; // 失敗

        // apply settings
        vsk_fm_sound_settings[iChannel] = phrase->m_setting;
        // add phrase
        block.push_back(phrase);
        // next channel
        ++iChannel;
    }

    if (!vsk_sound_player->save_as_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

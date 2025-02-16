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
// CMD SINGの現在の設定
static VskSoundSetting vsk_cmd_sing_settings;

// 設定をリセット
void vsk_cmd_sing_reset_settings(void)
{
    vsk_cmd_sing_settings = VskSoundSetting();
}

// 設定のサイズ
size_t vsk_cmd_sing_get_setting_size(void)
{
    return sizeof(VskSoundSetting);
}

// 設定の取得
bool vsk_cmd_sing_get_setting(std::vector<uint8_t>& data)
{
    data.resize(sizeof(VskSoundSetting));
    std::memcpy(data.data(), &vsk_cmd_sing_settings, sizeof(VskSoundSetting));
    return true;
}

// 設定の設定
bool vsk_cmd_sing_set_setting(const std::vector<uint8_t>& data)
{
    if (data.size() != sizeof(VskSoundSetting))
        return false;
    std::memcpy(&vsk_cmd_sing_settings, data.data(), sizeof(VskSoundSetting));
    return true;
}

// VskSingItem --- CMD SING 用の演奏項目
struct VskSingItem
{
    VskString               m_subcommand;
    VskString               m_param;
    char                    m_sign;
    bool                    m_dot;

    VskSingItem() { clear(); }

    void clear() {
        m_subcommand.clear();
        m_param.clear();
        m_sign = 0;
        m_dot = false;
    }

    // デバッグ用
    VskString to_str() const {
        VskString ret = m_subcommand;
        if (m_sign)
            ret += m_sign;
        ret += m_param;
        if (m_dot)
            ret += '.';
        return ret;
    }
};

// デバッグ用
VskString vsk_string_from_sing_items(const std::vector<VskSingItem>& items)
{
    VskString ret;
    for (auto& item : items)
        ret += item.to_str();
    return ret;
}

// CMD SINGのパラメータを取得する
VskAstPtr vsk_get_sing_param(const VskSingItem& item)
{
    if (item.m_param.empty())
        return nullptr;
    return vsk_eval_text(item.m_param);
} // vsk_get_sing_param

// CMD SINGの項目群からフレーズを作成する
bool vsk_phrase_from_sing_items(std::shared_ptr<VskPhrase> phrase, const std::vector<VskSingItem>& items)
{
    float length;
    for (auto& item : items) {
        char ch = item.m_subcommand[0];
        switch (ch) {
        case 'T': // Tempo (テンポ)
            if (auto ast = vsk_get_sing_param(item)) {
                auto i0 = ast->to_int();
                if ((48 <= i0) && (i0 <= 255)) {
                    phrase->m_setting.m_tempo = i0;
                    continue;
                }
            }
            return false;
        case 'O': // Octave (オクターブ)
            if (auto ast = vsk_get_sing_param(item)) {
                auto i0 = ast->to_int();
                if ((3 <= i0) && (i0 <= 6)) {
                    phrase->m_setting.m_octave = i0 - 1;
                    continue;
                }
            }
            return false;
        case 'L': // Length (音符・休符の長さ)
            if (auto ast = vsk_get_sing_param(item)) {
                auto i0 = ast->to_int();
                if ((1 <= i0) && (i0 <= 32)) {
                    phrase->m_setting.m_length = (24.0f * 4) / i0;
                    continue;
                }
            }
            return false;
        case 'R':
            if (item.m_subcommand == "RP") { // 繰り返し？
                break;
            }
            // ...FALL THROUGH...
        case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
            // 音符(CDEFGAB)か休符(Rest)
            if (auto ast = vsk_get_sing_param(item)) {
                auto L = ast->to_int();
                // NOTE: 24 is the length of a quarter note
                if ((1 <= L) && (L <= 32)) {
                    length = 24.0f * 4 / L;
                } else {
                    return false;
                }
            } else {
                length = phrase->m_setting.m_length;
            }
            phrase->add_note(ch, item.m_dot, length, item.m_sign);
            continue;
        case 'X':
            // スペシャルアクション
            if (auto ast = vsk_get_sing_param(item)) {
                int action_no = ast->to_int();
                phrase->add_action_node(ch, action_no);
                continue;
            }
            return false;
        }
    }
    return true;
} // vsk_phrase_from_sing_items

// CMD SINGの項目の繰り返し(RP)を展開する。
bool vsk_expand_sing_items_repeat(std::vector<VskSingItem>& items)
{
retry:;
    int level = 0, repeat = 0;
    auto it_repeat = items.end();
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->m_subcommand == "RP") { // 繰り返し（repeat）
            if (auto ast = vsk_get_sing_param(*it)) { // RPの引数
                repeat = ast->to_int();
                if (repeat < 0 || 255 < repeat) { // 繰り返しの回数が不正？
                    assert(0);
                    return false;
                }
                it_repeat = it; // RPのある位置を覚えておく
                ++it;
                if (it == items.end()) {
                    assert(0);
                    return false; // 文法エラー
                }
                if (it->m_subcommand == "[") { // 繰り返しの始まり
                    ++level;
                    if (level >= 8) { // 多重ループが限界を超えた？
                        assert(0);
                        return false; // 不正
                    }
                } else { // 繰り返しの始まりがなかった？
                    assert(0);
                    return false; // 文法エラー
                }
            } else { // 引数がなかった？
                assert(0);
                return false; // 文法エラー
            }
            continue;
        }
        if (it->m_subcommand == "]") { // 繰り返しの終わり
            --level;
            if (it_repeat == items.end()) { // 繰り返しの始まりがなかった？
                assert(0);
                return false; // 文法エラー
            }
            std::vector<VskSingItem> sub(it_repeat + 2, it);
            auto insert_position = items.erase(it_repeat, it + 1);
            for (int m = 0; m < repeat; ++m) {
                insert_position = items.insert(insert_position, sub.begin(), sub.end());
            }
            goto retry; // 一回展開したら最初からやり直し
        }
    }
    return true; // 成功
} // vsk_expand_sing_items_repeat

// 文字列からCMD SINGの項目を取得する
bool vsk_sing_items_from_string(std::vector<VskSingItem>& items, const VskString& expr)
{
    // 大文字にする
    auto str = expr;
    vsk_upper(str);

    // スキャナーを使って字句解析を始める
    VskScanner scanner(str);
    items.clear();
    VskSingItem item;
    VskString subcommand;
    while (!scanner.eof()) { // 文字列の終わりまで
        char ch = scanner.getch(); // 一文字取得
        if (vsk_isblank(ch)) continue; // 空白は無視
        if (ch == ';') continue;

        if (ch == '[') { // 繰り返しのカッコはじめ？
            item.m_subcommand = "[";
            items.push_back(item);
            item.clear();
            continue;
        }
        if (ch == ']') { // 繰り返しのカッコ終わり？
            item.m_subcommand = "]";
            items.push_back(item);
            item.clear();
            continue;
        }

        int status = 0;
        if (vsk_isupper(ch)) {
            subcommand.push_back(ch);
            if (subcommand.size() == 1) {
                switch (subcommand[0]) {
                case 'T': case 'O': case 'L': 
                case 'C': case 'D': case 'E': case 'F': case 'G': case 'A': case 'B':
                case 'X':
                    status = 1; // 引数が１つあるかもしれない
                    break;
                case 'R':
                    status = 1; // 引数が１つあるかもしれない
                    if (scanner.peek() == 'P') { // "RP" (repeat)？
                        subcommand += scanner.getch();
                    }
                    break;
                default:
                    break;
                }
            } else {
                continue;
            }
        }
        if (status == 0) {
            return false; // 引数がないのにここに来るのはおかしい
        }
        item.m_subcommand = subcommand;
        subcommand.clear();

        // シャープかフラット
        ch = scanner.peek();
        if ((ch == '+') || (ch == '#') || (ch == '-')) {
            item.m_sign = scanner.getch();
            ch = scanner.peek();
        }

        // パラメータを取得する
        if (scanner.peek() == '(') {
            int level = 0;
            for (;;) {
                ch = scanner.peek();
                if (ch == 0)
                    break;
                if (ch == '(') {
                    ++level;
                    item.m_param.push_back(ch);
                    scanner.getch();
                    continue;
                }
                if (ch == ')') {
                    item.m_param.push_back(ch);
                    scanner.getch();
                    if (--level == 0)
                        break;
                    continue;
                }
                scanner.getch();
                if (!vsk_isblank(ch)) {
                    item.m_param.push_back(ch);
                }
            }
        } else {
            if (vsk_isdigit(ch)) {
                while (!scanner.eof()) {
                    ch = scanner.getch();
                    if (!vsk_isdigit(ch)) {
                        scanner.ungetch();
                        break;
                    }
                    item.m_param.push_back(ch);
                }
            }
        }

        // 付点（ドット）
        ch = scanner.peek();
        if (ch == '.') {
            item.m_dot = true;
            scanner.getch();
        }

        // 項目を追加
        items.push_back(item);
        item.clear();
    }

    // 繰り返しを展開する
    return vsk_expand_sing_items_repeat(items);
} // vsk_sing_items_from_string

VskString vsk_replace_placeholders(const VskString& str);

// CMD SING文実装の本体
VSK_SOUND_ERR vsk_sound_cmd_sing(const char *str, bool stereo)
{
    VskString s = vsk_replace_placeholders(str); // {文字列変数名}を展開する

    // 文字列からCMD SINGの項目を取得する
    std::vector<VskSingItem> items;
    if (!vsk_sing_items_from_string(items, s))
        return VSK_SOUND_ERR_ILLEGAL; // 失敗

    // フレーズを作成する
    auto phrase = std::make_shared<VskPhrase>();
    phrase->m_setting = vsk_cmd_sing_settings;
    phrase->m_setting.m_fm = false;
    if (!vsk_phrase_from_sing_items(phrase, items))
        return VSK_SOUND_ERR_ILLEGAL; // 失敗

    // フレーズを演奏する
    VskScoreBlock block = { phrase };
    vsk_sound_player->play(block, stereo);

    // 設定を保存する
    vsk_cmd_sing_settings = phrase->m_setting;

    return VSK_SOUND_ERR_SUCCESS; // 成功
}

// CMD SING文の出力をWAVファイルに保存する
VSK_SOUND_ERR vsk_sound_cmd_sing_save(const char *str, const wchar_t *filename, bool stereo)
{
    VskString s = vsk_replace_placeholders(str); // {文字列変数名}を展開する

    // 文字列からCMD SINGの項目を取得する
    std::vector<VskSingItem> items;
    if (!vsk_sing_items_from_string(items, s))
        return VSK_SOUND_ERR_ILLEGAL; // 失敗

    // フレーズを作成する
    auto phrase = std::make_shared<VskPhrase>();
    phrase->m_setting = vsk_cmd_sing_settings;
    phrase->m_setting.m_fm = false;
    if (!vsk_phrase_from_sing_items(phrase, items))
        return VSK_SOUND_ERR_ILLEGAL; // 失敗

    // フレーズを演奏する
    VskScoreBlock block = { phrase };
    if (!vsk_sound_player->save_as_wav(block, filename, stereo))
        return VSK_SOUND_ERR_IO_ERROR; // 失敗

    return VSK_SOUND_ERR_SUCCESS;
}

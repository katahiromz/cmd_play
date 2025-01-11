#pragma once

#include "types.h"
#include "mstr.h"

// 2つのバイト値からワード値を作る
inline VskWord vsk_make_word(VskByte low, VskByte high)
{
    return low | ((VskWord)(high) << 8);
}

// 2つのワード値からダブルワード値を作る
inline VskDword vsk_make_dword(VskWord low, VskWord high)
{
    return low | ((VskDword)(high) << 16);
}

// ワードの下位バイトを取得
inline VskByte vsk_low_byte(VskWord w)
{
    return (VskByte)w;
}

// ワードの上位バイトを取得
inline VskByte vsk_high_byte(VskWord w)
{
    return (VskByte)(w >> 8);
}

// ダブルワードの下位ワードを取得
inline VskWord vsk_low_word(VskDword dw)
{
    return (VskWord)dw;
}

// ダブルワードの上位ワードを取得
inline VskWord vsk_high_word(VskDword dw)
{
    return (VskWord)(dw >> 16);
}

// ASCIIの印刷可能文字か？
inline bool vsk_isprint(VskByte ch)
{
    return ((0x20 <= ch) && (ch <= 0x7E));
}

// 大文字か？
inline bool vsk_isupper(VskByte ch)
{
    return ('A' <= ch) && (ch <= 'Z');
}

// 小文字か？
inline bool vsk_islower(VskByte ch)
{
    return ('a' <= ch) && (ch <= 'z');
}

// 数字か？
inline bool vsk_isdigit(VskByte ch)
{
    return ('0' <= ch) && (ch <= '9');
}

// 16進数の文字か？
inline bool vsk_isxdigit(VskByte ch)
{
    bool ret;
    if (vsk_isdigit(ch))
        ret = true;
    else if (('A' <= ch) && (ch <= 'F'))
        ret = true;
    else if (('a' <= ch) && (ch <= 'f'))
        ret = true;
    else
        ret = false;
    return ret;
}

// 8進数の文字か？
inline bool vsk_isoctal(VskByte ch)
{
    return ('0' <= ch) && (ch <= '7');
}

// ASCIIのアルファベットか？
inline bool vsk_isalpha(VskByte ch)
{
    return (vsk_isupper(ch) || vsk_islower(ch));
}

// ASCIIのアルファベットか数字か？
inline bool vsk_isalnum(VskByte ch)
{
    return (vsk_isalpha(ch) || vsk_isdigit(ch));
}

// C言語の識別子の最初の文字か？
inline bool vsk_iscsymf(VskByte ch)
{
    bool ret;
    if (vsk_isupper(ch) || vsk_islower(ch))
        ret = true;
    else if (ch == '_')
        ret = true;
    else
        ret = false;
    return ret;
}

// C言語の識別子の文字か？
inline bool vsk_iscsym(VskByte ch)
{
    bool ret;
    if (vsk_isupper(ch) || vsk_islower(ch) || vsk_isdigit(ch))
        ret = true;
    else if (ch == '_')
        ret = true;
    else
        ret = false;
    return ret;
}

// 余白か？
inline bool vsk_isblank(VskByte ch)
{
    return ((ch == ' ') || (ch == '\t'));
}

// 空白か？
inline bool vsk_isspace(VskByte ch)
{
    return (std::strchr(" \t\n\v\f\r", ch) != NULL);
}

// 1文字を大文字に
inline VskByte vsk_toupper(VskByte ch)
{
    if (vsk_islower(ch))
        return (VskByte)((ch - 'a') + 'A');
    return ch;
}

// 1文字を小文字に
inline VskByte vsk_tolower(VskByte ch)
{
    if (vsk_isupper(ch))
        return (VskByte)((ch - 'A') + 'a');
    return ch;
}

// 大文字に変換
template <typename T_STRING>
inline void vsk_upper(T_STRING& str)
{
    for (auto& ch : str)
    {
        if ('a' <= ch && ch <= 'z')
            ch += 'A' - 'a';
    }
}

// 小文字に変換
template <typename T_STRING>
inline void vsk_lower(T_STRING& str)
{
    for (auto& ch : str)
    {
        if ('a' <= ch && ch <= 'z')
            ch += 'a' - 'A';
    }
}

template <typename T_STRING>
inline void vsk_quote(T_STRING& str)
{
    T_STRING ret;
    ret += '"';
    ret += str;
    ret += '"';
    str = std::move(ret);
}

template <typename T_STRING>
inline void vsk_unquote(T_STRING& str)
{
    if (str.empty() || str[0] != '"')
        return;

    if (str[str.size() - 1] == '"')
        str = str.substr(1, str.size() - 2);
    else
        str = str.substr(1);
}

///////////////////////////////////////////////////////////////////////////////
// kanji-related

// JIS全角文字のバイトか？
inline bool vsk_is_jis_byte(VskByte ch)
{
    return ((0x21 <= ch) && (ch <= 0x7E));
}

// JIS全角文字のワードか？
inline bool vsk_is_jis_code(VskWord w)
{
    VskByte high = VskByte(w >> 8), low = VskByte(w);
    return (vsk_is_jis_byte(high) && vsk_is_jis_byte(low));
}

// JISの2バイト系半角文字か？
inline bool vsk_is_jis_2byte_halfwidth_code(VskByte high, VskByte low)
{
    return high == 0x8E && 0xA1 <= low && low <= 0xDF;
}

// JISの2バイト系半角文字か？
inline bool vsk_is_jis_2byte_halfwidth_code(VskWord jis)
{
    return vsk_is_jis_2byte_halfwidth_code(vsk_high_byte(jis), vsk_low_byte(jis));
}

// SJIS全角文字の最初のバイトか？
inline bool vsk_is_sjis_lead(VskByte ch)
{
    return ((0x81 <= ch) && (ch <= 0x9F)) ||
           ((0xE0 <= ch) && (ch <= 0xEF));
}

// SJIS全角文字の二番目のバイトか？
inline bool vsk_is_sjis_trail(VskByte ch)
{
    return ((0x40 <= ch) && (ch <= 0x7E)) ||
           ((0x80 <= ch) && (ch <= 0xFC));
}

// JIS全角文字をSJIS全角文字に変換
VskWord vsk_jis2sjis(VskByte high, VskByte c1);

// JIS全角文字をSJIS全角文字に変換
inline VskWord vsk_jis2sjis(VskWord jis_code)
{
    return vsk_jis2sjis(vsk_high_byte(jis_code), vsk_low_byte(jis_code));
}

// SJIS全角文字か？
inline bool vsk_is_sjis_code(VskByte ch0, VskByte ch1)
{
    return vsk_is_sjis_lead(ch0) && vsk_is_sjis_trail(ch1);
}

// SJIS全角文字か？
inline bool vsk_is_sjis_code(VskWord w)
{
    return vsk_is_sjis_code(vsk_high_byte(w), vsk_low_byte(w));
}

// SJIS全角文字をJIS全角文字に変換
VskWord vsk_sjis2jis(VskByte high, VskByte low);

// SJIS全角文字をJIS全角文字に変換
inline VskWord vsk_sjis2jis(VskWord w)
{
    return vsk_sjis2jis(vsk_high_byte(w), vsk_low_byte(w));
}

// SJISの半角カナか？
inline bool vsk_is_hankaku_kana(VskByte b)
{
    return ((0xA1 <= b) && (b <= 0xDF));
}

#define VSK_JIS_KANJI_IN  "\x1B\x4B" // Kanji-In
#define VSK_JIS_KANJI_OUT "\x1B\x48" // Kanji-Out

inline bool vsk_is_ki_code(VskWord w)
{
    return w == 0x1B4B;
}
inline bool vsk_is_ko_code(VskWord w)
{
    return w == 0x1B48;
}

// 半角文字を全角文字に変換する
VskString vsk_ank_to_kanji(const VskString& str);
// 全角文字を半角文字に変換する
VskString vsk_kanji_to_ank(const VskString& str);

// SJIS文字列をJIS文字列（KI/KO方式）に変換
VskString vsk_sjis2jis(const VskString& sjis);

// JIS文字列（KI/KO方式）をSJIS文字列に変換
VskString vsk_jis2sjis(const VskString& jis, bool kanji_in = false);

// 文字列からKI/KOコードを取り除く
inline VskString vsk_remove_ki_ko(const VskString& jis)
{
    VskString ret = jis;
    mstr_replace_all(ret, VSK_JIS_KANJI_IN, "");
    mstr_replace_all(ret, VSK_JIS_KANJI_OUT, "");
    return ret;
}

// 特定の文字種のJIS文字列の文字数を返す。KLEN関数を参照
int vsk_jis_kanji_len(VskString jis, int type);

// 特定の文字種のSJIS文字列の文字数を返す。KLEN関数を参照
int vsk_sjis_kanji_len(VskString sjis, int type);

// JIS文字列の先頭からの文字数からバイトインデックスに変換
size_t vsk_jis_kpos2ib(VskString jis, size_t kpos, bool *pkanji_in = nullptr);

// SJIS文字列の先頭からの文字数からバイトインデックスに変換
size_t vsk_sjis_kpos2ib(VskString sjis, size_t kpos);

// バイトインデックスからSJIS文字列の先頭文字数に変換
size_t vsk_sjis_ib2kpos(VskString sjis, size_t ib0);

// KTYPE関数を参照
int vsk_jis_ktype(const char *str);
// KTYPE関数を参照
int vsk_sjis_ktype(const char *str);
// KEXT$関数を参照
VskString vsk_sjis_kanji_extract(const VskString& str, bool is_zenkaku);

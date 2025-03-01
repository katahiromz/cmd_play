﻿// Detect memory leaks (for Debug and MSVC)
#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include <cstdio>
#include <cstring>
#include <cassert>
#include <map>
#include <unordered_set>
#include <stdexcept>
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "sound.h"
#include "mstr.h"
#include "server/server.h" // 非同期演奏のためのサーバー

enum RET { // exit code of this program
    RET_SUCCESS = 0,
    RET_BAD_CALL = 2,
    RET_BAD_CMDLINE = 3,
    RET_CANT_OPEN_FILE = 4,
    RET_BAD_SOUND_INIT = 5,
    RET_CANCELED = 6,
};

inline WORD get_lang_id(void)
{
    return PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
}

static bool g_no_beep = false;

void do_beep(void)
{
#ifdef ENABLE_BEEP
    if (!g_no_beep)
        Beep(2000, 500);
#endif
}

void my_puts(LPCTSTR pszText, FILE *fout)
{
#ifdef UNICODE
    _ftprintf(fout, _T("%ls"), pszText);
#else
    fputs(pszText, fout);
#endif
}

void my_printf(FILE *fout, LPCTSTR  fmt, ...)
{
    static TCHAR szText[2048];
    va_list va;

    va_start(va, fmt);
    StringCchVPrintf(szText, _countof(szText), fmt, va);
    my_puts(szText, fout);
    va_end(va);
}

bool vsk_get_int_array(std::vector<int16_t>& array, const std::wstring& str, std::vector<int16_t> *pdefault_values = nullptr) {
    auto s = str;
    mstr_replace_all(s, L" ", L"");
    mstr_replace_all(s, L"\t", L"");
    mstr_replace_all(s, L"{", L"");
    mstr_replace_all(s, L"}", L"");

    std::vector<std::wstring> vec;
    mstr_split(vec, s, L",");

    size_t i = 0;
    for (auto& item : vec) {
        int value;
        if (item.empty()) {
            if (!pdefault_values)
                return false;
            if (i < pdefault_values->size())
                value = (*pdefault_values)[i];
            else
                return false;
        } else {
            wchar_t *endptr;
            value = wcstol(item.c_str(), &endptr, 10);
            if (*endptr)
                return false;
        }
        array.push_back(int16_t(value));
        ++i;
    }
    return true;
}

// Text ID
enum TEXT_ID {
    IDT_VERSION,
    IDT_HELP,
    IDT_TOO_MANY_ARGS,
    IDT_MODE_OUT_OF_RANGE,
    IDT_BAD_CALL,
    IDT_NEEDS_OPERAND,
    IDT_NEEDS_TWO_OPERANDS,
    IDT_INVALID_OPTION,
    IDT_SOUND_INIT_FAILED,
    IDT_CIRCULAR_REFERENCE,
    IDT_CANT_OPEN_FILE,
    IDT_CANT_COPY_TONE,
    IDT_CH_OUT_OF_RANGE,
};

// localization
LPCTSTR get_text(INT id)
{
#ifdef JAPAN
    if (get_lang_id() == LANG_JAPANESE) // Japone for Japone
    {
        switch (id)
        {
        case IDT_VERSION: return TEXT("cmd_play バージョン 2.5 by 片山博文MZ\n");
        case IDT_HELP:
            return
                TEXT("使い方: cmd_play [オプション] [#n] [文字列1] [文字列2] [文字列3] [文字列4] [文字列5] [文字列6]\n")
                TEXT("\n")
                TEXT("オプション:\n")
                TEXT("  -D変数名=値                変数に代入。\n")
                TEXT("  -save-wav 出力.wav         WAVファイルとして保存（MIDI音源を除く）。\n")
                TEXT("  -save-mid 出力.mid         MIDファイルとして保存（MIDI音源のみ）。\n")
                TEXT("  -reset                     音楽を止めて設定をリセット。\n")
                TEXT("  -stopm                     音色を変えない以外は -reset と同じ。\n")
                TEXT("  -stereo                    音をステレオにする（デフォルト）。\n")
                TEXT("  -mono                      音をモノラルにする。\n")
                TEXT("  -voice CH FILE.voi         ファイルからチャンネルCHに音色を読み込む（FM音源）。\n")
                TEXT("  -voice CH \"CSV\"            配列からチャンネルCHに音色を読み込む（FM音源）。\n")
                TEXT("  -voice-copy TONE FILE.voi  音色をファイルにコピーする（FM音源）。\n")
                TEXT("  -bgm 0                     演奏が終わるまで待つ。\n")
                TEXT("  -bgm 1                     演奏が終わるまで待たない（デフォルト）。\n")
                TEXT("  -help                      このメッセージを表示する。\n")
                TEXT("  -version                   バージョン情報を表示する。\n")
                TEXT("\n")
                TEXT("数値変数は、「L=数値変数名;」のように等号とセミコロンではさめば指定できます。\n")
                TEXT("文字列変数は「[A$]」のように [ ] で囲えば展開できます。\n");
        case IDT_TOO_MANY_ARGS: return TEXT("エラー: 引数が多すぎます。\n");
        case IDT_MODE_OUT_OF_RANGE: return TEXT("エラー: 音源モード (#n) の値が範囲外です。\n");
        case IDT_BAD_CALL: return TEXT("エラー: 不正な関数呼び出しです。\n");
        case IDT_NEEDS_OPERAND: return TEXT("エラー: オプション「%s」は引数が必要です。\n");
        case IDT_NEEDS_TWO_OPERANDS: return TEXT("エラー: オプション「%s」は２つの引数が必要です。\n");
        case IDT_INVALID_OPTION: return TEXT("エラー: 「%s」は、無効なオプションです。\n");
        case IDT_SOUND_INIT_FAILED: return TEXT("エラー: vsk_sound_initが失敗しました。\n");
        case IDT_CIRCULAR_REFERENCE: return TEXT("エラー: 変数の循環参照を検出しました。\n");
        case IDT_CANT_OPEN_FILE: return TEXT("エラー: ファイル「%s」が開けません。\n");
        case IDT_CANT_COPY_TONE: return TEXT("エラー: 音色をコピーできませんでした。\n");
        case IDT_CH_OUT_OF_RANGE: return TEXT("エラー: チャンネルの値が範囲外です。\n");
        }
    }
    else // The others are Let's la English
#endif
    {
        switch (id)
        {
        case IDT_VERSION: return TEXT("cmd_play version 2.5 by katahiromz\n");
        case IDT_HELP:
            return
                TEXT("Usage: cmd_play [Options] [#n] [string1] [string2] [string3] [string4] [string5] [string6]\n")
                TEXT("\n")
                TEXT("Options:\n")
                TEXT("  -DVAR=VALUE                Assign to a variable.\n")
                TEXT("  -save-wav output.wav       Save as WAV file (except MIDI sound).\n")
                TEXT("  -save-mid output.mid       Save as MID file (MIDI sound only).\n")
                TEXT("  -reset                     Stop music and reset settings.\n")
                TEXT("  -stopm                     Same as -reset except that the tone is not changed.\n")
                TEXT("  -stereo                    Make sound stereo (default).\n")
                TEXT("  -mono                      Make sound mono.\n")
                TEXT("  -voice CH FILE.voi         Load a tone from a file to channel CH (FM sound).\n")
                TEXT("  -voice CH \"CSV\"            Load a tone from an array to channel CH (FM sound).\n")
                TEXT("  -voice-copy TONE FILE.voi  Copy the tone to a file (FM sound).\n")
                TEXT("  -bgm 0                     Wait until the performance is over.\n")
                TEXT("  -bgm 1                     Don't wait until the performance is over (default).\n")
                TEXT("  -help                      Display this message.\n")
                TEXT("  -version                   Display version info.\n")
                TEXT("\n")
                TEXT("Numeric variables can be specified by enclosing them between an equal sign and a semicolon: \"L=variable name;\".\n")
                TEXT("String variables can be expanded by enclosing them in [ ].\n");
        case IDT_TOO_MANY_ARGS: return TEXT("ERROR: Too many arguments.\n");
        case IDT_MODE_OUT_OF_RANGE: return TEXT("ERROR: The audio mode value (#n) is out of range.\n");
        case IDT_BAD_CALL: return TEXT("ERROR: Illegal function call\n");
        case IDT_NEEDS_OPERAND: return TEXT("ERROR: Option '%s' needs an operand.\n");
        case IDT_NEEDS_TWO_OPERANDS: return TEXT("ERROR: Option '%s' needs two operands.\n");
        case IDT_INVALID_OPTION: return TEXT("ERROR: '%s' is an invalid option.\n");
        case IDT_SOUND_INIT_FAILED: return TEXT("ERROR: vsk_sound_init failed.\n");
        case IDT_CIRCULAR_REFERENCE: return TEXT("ERROR: Circular variable reference detected.\n");
        case IDT_CANT_OPEN_FILE: return TEXT("ERROR: Unable to open file '%s'.\n");
        case IDT_CANT_COPY_TONE: return TEXT("ERROR: Unable to copy tone.\n");
        case IDT_CH_OUT_OF_RANGE: return TEXT("ERROR: The channel value is out of range.\n");
        }
    }

    assert(0);
    return nullptr;
}

void version(void)
{
    my_puts(get_text(IDT_VERSION), stdout);
}

void usage(void)
{
    my_puts(get_text(IDT_HELP), stdout);
}

#define VSK_MAX_CHANNEL 6

struct VOICE_INFO
{
    INT m_ch;
    std::wstring m_data;
};

struct CMD_PLAY
{
    bool m_help = false;
    bool m_version = false;
    std::vector<std::string> m_str_to_play;
    std::map<VskString, VskString> m_variables;
    std::wstring m_save_wav;
    std::wstring m_save_mid;
    int m_audio_mode = 2;
    bool m_reset = false;
    bool m_stopm = false;
    bool m_stereo = true;
    bool m_no_reg = false;
    bool m_bgm = true;
    bool m_bgm2 = true;
    HWND m_hwndServer = nullptr;
    std::vector<VOICE_INFO> m_voices;

    RET parse_cmd_line(int argc, wchar_t **argv);
    RET run(int argc, wchar_t **argv);
    bool start_server();
    bool load_settings();
    bool save_settings();
    bool save_bgm_and_vars_only();

    std::wstring build_server_cmd_line(int argc, wchar_t **argv);
    VSK_SOUND_ERR save_wav();
    VSK_SOUND_ERR save_mid();
    VSK_SOUND_ERR play_str(bool no_sound);
};

#define NUM_SETTINGS 18

// レジストリから設定を読み込む
bool CMD_PLAY::load_settings()
{
    if (m_no_reg)
        return false;

    // レジストリを開く
    HKEY hKey;
    LSTATUS error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play", 0,
                                  KEY_READ, &hKey);
    if (error)
        return false;

    // ステレオかモノラルか？
    {
        DWORD dwValue, cbValue = sizeof(dwValue);
        error = RegQueryValueExW(hKey, L"Stereo", NULL, NULL, (BYTE*)&dwValue, &cbValue);
        if (!error)
            m_stereo = !!dwValue;
    }

    // BGMか？
    {
        DWORD dwValue, cbValue = sizeof(dwValue);
        error = RegQueryValueExW(hKey, L"BGM", NULL, NULL, (BYTE*)&dwValue, &cbValue);
        if (!error)
            m_bgm = !!dwValue;
    }

    // 音声の設定のサイズ
    size_t size = vsk_cmd_play_get_setting_size();

    // 設定項目を読み込む
    std::vector<uint8_t> setting;
    setting.resize(size);
    for (int ch = 0; ch < NUM_SETTINGS; ++ch)
    {
        WCHAR szValueName[64];
        StringCchPrintfW(szValueName, _countof(szValueName), L"setting%u", ch);

        DWORD cbValue = size;
        error = RegQueryValueExW(hKey, szValueName, NULL, NULL, setting.data(), &cbValue);
        if (!error && cbValue == size)
            vsk_cmd_play_set_setting(ch, setting);
    }

    // 変数項目を読み込む
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        CHAR szName[MAX_PATH], szValue[512];
        DWORD cchName = _countof(szName);
        DWORD cbValue = sizeof(szValue);
        error = RegEnumValueA(hKey, dwIndex, szName, &cchName, NULL, NULL, (BYTE *)szValue, &cbValue);
        szName[_countof(szName) - 1] = 0; // Avoid buffer overrun
        szValue[_countof(szValue) - 1] = 0; // Avoid buffer overrun
        if (error)
            break;

        if (std::memcmp(szName, "VAR_", 4 * sizeof(CHAR)) != 0)
            continue;

        CharUpperA(szName);
        CharUpperA(szValue);
        g_variables[&szName[4]] = szValue;
    }

    // レジストリを閉じる
    RegCloseKey(hKey);

    return true;
}

// レジストリへ設定を書き込む
bool CMD_PLAY::save_bgm_and_vars_only()
{
    if (m_no_reg)
        return false;

    // レジストリを作成
    HKEY hKey;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play", 0,
                                    NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);
    if (error)
        return false;

    // BGMか？
    {
        DWORD dwValue = !!m_bgm, cbValue = sizeof(dwValue);
        RegSetValueExW(hKey, L"BGM", 0, REG_DWORD, (BYTE *)&dwValue, cbValue);
    }

    // レジストリの変数項目を消す
retry:
    CHAR szName[MAX_PATH];
    DWORD cchName;
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        cchName = _countof(szName);
        error = RegEnumValueA(hKey, dwIndex, szName, &cchName, NULL, NULL, NULL, NULL);
        szName[_countof(szName) - 1] = 0; // Avoid buffer overrun
        if (error)
            break;

        if (std::memcmp(szName, "VAR_", 4 * sizeof(CHAR)) != 0)
            continue;

        RegDeleteValueA(hKey, szName);
        goto retry;
    }

    // 新しい変数項目群を書き込む
    std::string name;
    for (auto& pair : g_variables)
    {
        name = "VAR_" + pair.first;
        DWORD cbValue = (pair.second.size() + 1) * sizeof(CHAR);
        RegSetValueExA(hKey, name.c_str(), 0, REG_SZ, (BYTE *)pair.second.c_str(), cbValue);
    }

    // レジストリを閉じる
    RegCloseKey(hKey);

    return true;
}

// レジストリへ設定を書き込む
bool CMD_PLAY::save_settings()
{
    if (m_no_reg)
        return false;

    // レジストリを作成
    HKEY hKey;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play", 0,
                                    NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);
    if (error)
        return false;

    // ステレオかモノラルか？
    {
        DWORD dwValue = !!m_stereo, cbValue = sizeof(dwValue);
        RegSetValueExW(hKey, L"Stereo", 0, REG_DWORD, (BYTE *)&dwValue, cbValue);
    }
    // BGMか？
    {
        DWORD dwValue = !!m_bgm, cbValue = sizeof(dwValue);
        RegSetValueExW(hKey, L"BGM", 0, REG_DWORD, (BYTE *)&dwValue, cbValue);
    }

    // 音声の設定を書き込む
    std::vector<uint8_t> setting;
    WCHAR szValueName[64];
    for (int ch = 0; ch < NUM_SETTINGS; ++ch)
    {
        vsk_cmd_play_get_setting(ch, setting);

        StringCchPrintfW(szValueName, _countof(szValueName), L"setting%u", ch);

        RegSetValueExW(hKey, szValueName, 0, REG_BINARY, setting.data(), (DWORD)setting.size());
    }

    // レジストリの変数項目を消す
retry:
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        CHAR szName[MAX_PATH];
        DWORD cchName = _countof(szName);
        error = RegEnumValueA(hKey, dwIndex, szName, &cchName, NULL, NULL, NULL, NULL);
        szName[_countof(szName) - 1] = 0; // Avoid buffer overrun
        if (error)
            break;

        if (std::memcmp(szName, "VAR_", 4 * sizeof(CHAR)) != 0)
            continue;

        RegDeleteValueA(hKey, szName);
        goto retry;
    }

    // 新しい変数項目群を書き込む
    for (auto& pair : g_variables)
    {
        std::string name = "VAR_";
        name += pair.first;
        DWORD cbValue = (pair.second.size() + 1) * sizeof(CHAR);
        RegSetValueExA(hKey, name.c_str(), 0, REG_SZ, (BYTE *)pair.second.c_str(), cbValue);
    }

    // レジストリを閉じる
    RegCloseKey(hKey);

    return true;
}

// アプリのレジストリキーを消す
void erase_reg_settings(void)
{
    RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play");
}

RET CMD_PLAY::parse_cmd_line(int argc, wchar_t **argv)
{
    if (argc <= 1)
    {
        m_help = true;
        return RET_SUCCESS;
    }

    for (int iarg = 1; iarg < argc; ++iarg)
    {
        LPWSTR arg = argv[iarg];

        if (_wcsicmp(arg, L"-help") == 0 || _wcsicmp(arg, L"--help") == 0)
        {
            m_help = true;
            return RET_SUCCESS;
        }

        if (_wcsicmp(arg, L"-version") == 0 || _wcsicmp(arg, L"--version") == 0)
        {
            m_version = true;
            return RET_SUCCESS;
        }

        if (arg[0] == '-' && (arg[1] == 'd' || arg[1] == 'D'))
        {
            VskString str = vsk_sjis_from_wide(&arg[2]);
            auto ich = str.find('=');
            if (ich == str.npos)
            {
                my_printf(stderr, get_text(IDT_INVALID_OPTION), arg);
                return RET_BAD_CMDLINE;
            }
            auto var = str.substr(0, ich);
            auto value = str.substr(ich + 1);
            CharUpperA(&var[0]);
            CharUpperA(&value[0]);
            m_variables[var] = value;
            continue;
        }

        if (_wcsicmp(arg, L"-save-wav") == 0 || _wcsicmp(arg, L"--save-wav") == 0 ||
            _wcsicmp(arg, L"-save_wav") == 0 || _wcsicmp(arg, L"--save_wav") == 0)
        {
            if (iarg + 1 < argc)
            {
                m_save_wav = argv[++iarg];
                continue;
            }
            else
            {
                my_printf(stderr, get_text(IDT_NEEDS_OPERAND), arg);
                return RET_BAD_CMDLINE;
            }
        }

        if (_wcsicmp(arg, L"-save-mid") == 0 || _wcsicmp(arg, L"--save-mid") == 0)
        {
            if (iarg + 1 < argc)
            {
                m_save_mid = argv[++iarg];
                continue;
            }
            else
            {
                my_printf(stderr, get_text(IDT_NEEDS_OPERAND), arg);
                return RET_BAD_CMDLINE;
            }
        }

        if (_wcsicmp(arg, L"-reset") == 0 || _wcsicmp(arg, L"--reset") == 0)
        {
            m_reset = true;
            continue;
        }

        if (_wcsicmp(arg, L"-stopm") == 0 || _wcsicmp(arg, L"--stopm") == 0)
        {
            m_stopm = true;
            continue;
        }

        if (_wcsicmp(arg, L"-mono") == 0 || _wcsicmp(arg, L"--mono") == 0)
        {
            m_stereo = false;
            continue;
        }

        if (_wcsicmp(arg, L"-stereo") == 0 || _wcsicmp(arg, L"--stereo") == 0)
        {
            m_stereo = true;
            continue;
        }

        if (_wcsicmp(arg, L"-voice-copy") == 0 || _wcsicmp(arg, L"--voice-copy") == 0)
        {
            if (iarg + 2 < argc)
            {
                int tone = _wtoi(argv[++iarg]);
                std::wstring file = argv[++iarg];

                // 音色をコピーする
                std::vector<uint8_t> data;
                if (!vsk_sound_voice_copy(tone, data))
                {
                    my_printf(stderr, get_text(IDT_CANT_COPY_TONE));
                    return RET_BAD_CALL;
                }

                // 音色をファイルに書き込む
                FILE *fout = _wfopen(file.c_str(), L"wb");
                if (!fout)
                {
                    my_printf(stderr, get_text(IDT_CANT_OPEN_FILE), file.c_str());
                    return RET_BAD_CALL;
                }
                fwrite(data.data(), data.size(), 1, fout);
                fclose(fout);

                continue;
            }
            else
            {
                my_printf(stderr, get_text(IDT_NEEDS_TWO_OPERANDS), arg);
                return RET_BAD_CMDLINE;
            }
        }

        if (_wcsicmp(arg, L"-voice") == 0 || _wcsicmp(arg, L"--voice") == 0)
        {
            if (iarg + 2 < argc)
            {
                int ch = _wtoi(argv[++iarg]);
                std::wstring data = argv[++iarg];

                if (!(1 <= ch && ch <= 6)) {
                    my_printf(stderr, get_text(IDT_CH_OUT_OF_RANGE));
                    return RET_BAD_CALL;
                }

                m_voices.push_back({ ch, data });
                continue;
            }
            else
            {
                my_printf(stderr, get_text(IDT_NEEDS_TWO_OPERANDS), arg);
                return RET_BAD_CMDLINE;
            }
        }

        // hidden feature
        if (_wcsicmp(arg, L"-no-beep") == 0 || _wcsicmp(arg, L"--no-beep") == 0)
        {
            g_no_beep = true;
            continue;
        }

        // hidden feature
        if (_wcsicmp(arg, L"-no-reg") == 0 || _wcsicmp(arg, L"--no-reg") == 0)
        {
            m_no_reg = true;
            continue;
        }

        if (_wcsicmp(arg, L"-bgm") == 0 || _wcsicmp(arg, L"--bgm") == 0)
        {
            if (iarg + 1 < argc)
            {
                m_bgm2 = m_bgm = !!_wtoi(argv[++iarg]);
                continue;
            }
            else
            {
                my_printf(stderr, get_text(IDT_NEEDS_OPERAND), arg);
                return RET_BAD_CMDLINE;
            }
        }

#ifndef NDEBUG
        if (_wcsicmp(arg, L"-print-checksums") == 0 || _wcsicmp(arg, L"--print-checksums") == 0)
        {
            void vsk_print_timbre_checksums(void);
            vsk_print_timbre_checksums();
            continue;
        }
#endif

        if (arg[0] == '-')
        {
            my_printf(stderr, get_text(IDT_INVALID_OPTION), arg);
            return RET_BAD_CMDLINE;
        }

        if (arg[0] == '#')
        {
            m_audio_mode = _wtoi(&arg[1]);
            if (!(0 <= m_audio_mode && m_audio_mode <= 5))
            {
                my_puts(get_text(IDT_MODE_OUT_OF_RANGE), stderr);
                return RET_BAD_CMDLINE;
            }
            continue;
        }

        if (m_str_to_play.size() < VSK_MAX_CHANNEL)
        {
            m_str_to_play.push_back(vsk_sjis_from_wide(arg).c_str());
            continue;
        }

        my_puts(get_text(IDT_TOO_MANY_ARGS), stderr);
        return RET_BAD_CMDLINE;
    }

    return RET_SUCCESS;
}

std::wstring CMD_PLAY::build_server_cmd_line(int argc, wchar_t **argv)
{
    std::wstring ret;

    for (int iarg = 1; iarg < argc; ++iarg)
    {
        auto arg = argv[iarg];
        if (wcschr(arg, L' ')) {
            ret += L'"';
            ret += arg;
            ret += L'"';
        } else {
            ret += arg;
        }
        ret += L' ';
    }

    return ret;
}

// サーバーを起動する
bool CMD_PLAY::start_server()
{
    // サーバーへのパスファイル名を構築する
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"cmd_play_server.exe");

    // シェルでサーバーを起動する
    SHELLEXECUTEINFOW info = { sizeof(info) };
    info.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
    info.lpFile = szPath;
    info.nShow = SW_HIDE;
    if (!ShellExecuteExW(&info))
        return false;

    WaitForInputIdle(info.hProcess, INFINITE);
    CloseHandle(info.hProcess);

    return true;
}

VSK_SOUND_ERR CMD_PLAY::play_str(bool no_sound)
{
    switch (m_audio_mode)
    {
    case 0:
        return vsk_sound_cmd_play_ssg(m_str_to_play, m_stereo, no_sound);
    case 1:
        return vsk_sound_cmd_play_midi(m_str_to_play, no_sound);
    case 2:
    case 3:
    case 4:
        return vsk_sound_cmd_play_fm_and_ssg(m_str_to_play, m_stereo, no_sound);
    case 5:
        return vsk_sound_cmd_play_fm(m_str_to_play, m_stereo, no_sound);
    }

    return VSK_SOUND_ERR_ILLEGAL;
}

VSK_SOUND_ERR CMD_PLAY::save_wav()
{
    switch (m_audio_mode)
    {
    case 0:
        return vsk_sound_cmd_play_ssg_save(m_str_to_play, m_save_wav.c_str(), m_stereo);
    case 2:
    case 3:
    case 4:
        return vsk_sound_cmd_play_fm_and_ssg_save(m_str_to_play, m_save_wav.c_str(), m_stereo);
    case 5:
        return vsk_sound_cmd_play_fm_save(m_str_to_play, m_save_wav.c_str(), m_stereo);
    case 1:
    default:
        return VSK_SOUND_ERR_ILLEGAL;
    }
}

VSK_SOUND_ERR CMD_PLAY::save_mid()
{
    if (m_audio_mode != 1)
        return VSK_SOUND_ERR_ILLEGAL;

    return vsk_sound_cmd_play_midi_save(m_str_to_play, m_save_mid.c_str());
}

RET CMD_PLAY::run(int argc, wchar_t **argv)
{
    if (m_help)
    {
        usage();
        return RET_SUCCESS;
    }

    if (m_version)
    {
        version();
        return RET_SUCCESS;
    }

    if (!vsk_sound_init(m_stereo))
    {
        my_puts(get_text(IDT_SOUND_INIT_FAILED), stderr);
        return RET_BAD_SOUND_INIT;
    }

    if (m_stopm) // 音色を変えない以外は -reset と同じ
    {
        // コマンドラインの -bgm 設定を優先する
        m_bgm = m_bgm2;
        // サーバーを停止
        if (HWND hwndServer = find_server_window())
            SendMessageW(hwndServer, WM_CLOSE, 0, 0);
        // 変数をクリア
        g_variables.clear();
        // 音色以外の設定をクリア
        vsk_cmd_play_stopm();
        // 設定を保存
        save_settings();
    }

    if (m_reset) // 音楽を止めて設定をリセットする
    {
        // コマンドラインの -bgm 設定を優先する
        m_bgm = m_bgm2;
        // サーバーを停止
        if (HWND hwndServer = find_server_window())
            SendMessageW(hwndServer, WM_CLOSE, 0, 0);
        // 変数をクリア
        g_variables.clear();
        // 設定をクリア
        vsk_cmd_play_reset_settings();
        // 設定を保存
        save_settings();
    }

    if (m_bgm && m_save_wav.empty() && m_save_mid.empty()) // 非同期に演奏か？
    {
        // 急げ！ 音が遅れてはいけない。
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        // 文法をチェックする
        auto err = play_str(true);
        // g_variablesをm_variablesで上書き
        for (auto& pair : m_variables)
            g_variables[pair.first] = pair.second;
        // BGMと変数だけ設定を保存
        save_bgm_and_vars_only();

        // エラーチェック
        switch (err) {
        case VSK_SOUND_ERR_ILLEGAL:
            // エラーメッセージを出力
            my_puts(get_text(IDT_BAD_CALL), stderr);
            do_beep(); // BEEP音
            // 音声モジュールを解放
            vsk_sound_exit();
            return RET_BAD_CALL;
        case VSK_SOUND_ERR_IO_ERROR:
            // エラーメッセージを出力
            my_puts(get_text(IDT_CANT_OPEN_FILE), stderr);
            do_beep(); // BEEP音
            // 音声モジュールを解放
            vsk_sound_exit();
            return RET_CANT_OPEN_FILE;
        default:
            break;
        }

        // サイバーが起動してなければ起動
        if (!m_hwndServer || !::IsWindow(m_hwndServer))
            m_hwndServer = find_server_window();
        if (!m_hwndServer) {
            start_server();
            m_hwndServer = find_server_window();
        }

        if (m_hwndServer) { // サーバーを起動できた？
            // サーバー側のコマンドラインを構築
            auto cmd_line = build_server_cmd_line(argc, argv);

            // サーバーにWM_COPYDATAメッセージを送信
            COPYDATASTRUCT copyData;
            copyData.dwData = 0xDEADFACE;
            copyData.cbData = (cmd_line.size() + 1) * sizeof(WCHAR);
            copyData.lpData = (PVOID)cmd_line.c_str();
            SendMessageW(m_hwndServer, WM_COPYDATA, 0, (LPARAM)&copyData);

            // 音声モジュールを解放
            vsk_sound_exit();

            return RET_SUCCESS;
        }

        // サーバーが起動できないときは、同期的に演奏
    }

    // g_variablesをm_variablesで上書き
    for (auto& pair : m_variables)
    {
        g_variables[pair.first] = pair.second;
    }

    // 音色を適用する
    for (auto& voice : m_voices)
    {
        size_t size = vsk_sound_voice_size();
        char buf[size];

        std::vector<int16_t> array;
        if (vsk_get_int_array(array, voice.m_data) && array.size() * sizeof(int16_t) == size)
        {
            // 配列から音色を読み込む
            std::memcpy(buf, array.data(), size);
        }
        else
        {
            // ファイルから音色を読み込む
            FILE *fin = _wfopen(voice.m_data.c_str(), L"rb");
            if (!fin) {
                my_printf(stderr, get_text(IDT_CANT_OPEN_FILE), voice.m_data.c_str());
                vsk_sound_exit();
                return RET_BAD_CALL;
            }
            size_t count = fread(buf, size, 1, fin);
            fclose(fin);

            if (!count) {
                my_printf(stderr, get_text(IDT_CANT_COPY_TONE));
                return RET_BAD_CALL;
            }
        }

        // 音色を適用
        if (!vsk_cmd_play_voice(voice.m_ch - 1, buf, size)) {
            my_printf(stderr, get_text(IDT_CANT_COPY_TONE));
            return RET_BAD_CALL;
        }
    }

    if (m_save_wav.size())
    {
        auto err = save_wav();
        switch (err)
        {
        case VSK_SOUND_ERR_SUCCESS:
            break;
        case VSK_SOUND_ERR_ILLEGAL:
            my_puts(get_text(IDT_BAD_CALL), stderr);
            do_beep();
            save_settings();
            vsk_sound_exit();
            return RET_BAD_CALL;
        case VSK_SOUND_ERR_IO_ERROR:
            my_printf(stderr, get_text(IDT_CANT_OPEN_FILE), m_save_wav.c_str());
            do_beep();
            save_settings();
            vsk_sound_exit();
            return RET_CANT_OPEN_FILE;
        }

        return RET_SUCCESS;
    }

    if (m_save_mid.size())
    {
        auto err = save_mid();
        switch (err)
        {
        case VSK_SOUND_ERR_SUCCESS:
            break;
        case VSK_SOUND_ERR_ILLEGAL:
            my_puts(get_text(IDT_BAD_CALL), stderr);
            do_beep();
            save_settings();
            vsk_sound_exit();
            return RET_BAD_CALL;
        case VSK_SOUND_ERR_IO_ERROR:
            my_printf(stderr, get_text(IDT_CANT_OPEN_FILE), m_save_mid.c_str());
            do_beep();
            save_settings();
            vsk_sound_exit();
            return RET_CANT_OPEN_FILE;
        }

        return RET_SUCCESS;
    }

    auto err = play_str(false);
    if (err)
    {
        my_puts(get_text(IDT_BAD_CALL), stderr);
        do_beep();
        save_settings();
        vsk_sound_exit();
        return RET_BAD_CALL;
    }

    vsk_sound_wait(-1);

    save_settings();
    vsk_sound_exit();

    return RET_SUCCESS;
}

#ifdef CMD_PLAY_EXE
static bool g_canceled = false; // Ctrl+Cなどが押されたか？

static BOOL WINAPI HandlerRoutine(DWORD signal)
{
    switch (signal)
    {
    case CTRL_C_EVENT: // Ctrl+C
    case CTRL_BREAK_EVENT: // Ctrl+Break
        g_canceled = true;
        vsk_sound_stop();
        //std::printf("^C\nBreak\nOk\n"); // このハンドラで時間を掛けちゃダメだ。
        //std::fflush(stdout); // このハンドラで時間を掛けちゃダメだ。
        //do_beep(); // このハンドラで時間を掛けちゃダメだ。
        return TRUE;
    }
    return FALSE;
}

int wmain(int argc, wchar_t **argv)
{
    SetConsoleCtrlHandler(HandlerRoutine, TRUE); // Ctrl+C

    CMD_PLAY play;

    play.load_settings();

    if (RET ret = play.parse_cmd_line(argc, argv))
    {
        do_beep();
        return ret;
    }

    if (RET ret = play.run(argc, argv))
        return ret;

    return RET_SUCCESS;
}

#include <clocale>

int main(void)
{
    // Unicode console output support
    std::setlocale(LC_ALL, "");

    int ret;
    try
    {
        int argc;
        LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        ret = wmain(argc, argv);
        LocalFree(argv);
    } catch (const std::exception& e) {
        std::string what = e.what();
        if (what == "circular reference detected") {
            my_puts(get_text(IDT_CIRCULAR_REFERENCE), stderr);
            do_beep();
        } else {
            printf("ERROR: %s\n", what.c_str());
        }
        ret = RET_BAD_CALL;
        exit(ret);
    }

    // Detect handle leaks (for Debug)
#if (_WIN32_WINNT >= 0x0500) && !defined(NDEBUG)
    TCHAR szText[MAX_PATH];
    wnsprintf(szText, _countof(szText), TEXT("GDI Objects: %ld, User Objects: %ld\n"),
              GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS),
              GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS));
    OutputDebugString(szText);
#endif

    // Detect memory leaks (for Debug and MSVC)
#if defined(_MSC_VER) && !defined(NDEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    if (g_canceled)
    {
        std::printf("^C\nBreak\nOk\n");
        std::fflush(stdout);
        do_beep();
        if (HWND hwndServer = find_server_window())
            PostMessageW(hwndServer, WM_CLOSE, 0, 0);
        erase_reg_settings();
        return RET_CANCELED;
    }

    std::printf("Ok\n");
    return ret;
}
#endif  // def CMD_PLAY_EXE

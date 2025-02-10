// Detect memory leaks (for Debug and MSVC)
#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif

#include "sound.h"
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

inline WORD get_lang_id(void)
{
    return PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale()));
}

// localization
LPCTSTR get_text(INT id)
{
#ifdef JAPAN
    if (get_lang_id() == LANG_JAPANESE) // Japone for Japone
    {
        switch (id)
        {
        case 0: return TEXT("cmd_play バージョン 1.2 by 片山博文MZ");
        case 1:
            return
                TEXT("使い方: cmd_play [オプション] [#n] [文字列1] [文字列2] [文字列3] [文字列4] [文字列5] [文字列6]\n")
                TEXT("\n")
                TEXT("オプション:\n")
                TEXT("  -D変数名=値            変数に代入。\n")
                TEXT("  -save_wav 出力.wav     WAVファイルとして保存。\n")
                TEXT("  -help                  このメッセージを表示する。\n")
                TEXT("  -version               バージョン情報を表示する。\n")
                TEXT("\n")
                TEXT("文字列変数は [ ] で囲えば展開できます。");
        case 2: return TEXT("エラー: 引数が多すぎます。\n");
        case 3: return TEXT("エラー: 演奏する文字列が未指定です。\n");
        case 4: return TEXT("エラー: 音源モード (#n) の値が範囲外です。\n");
        case 5: return TEXT("エラー: Illegal function call\n");
        case 6: return TEXT("エラー: オプション -save_wav は引数が必要です。\n");
        case 7: return TEXT("エラー: 「%ls」は、無効なオプションです。\n");
        case 8: return TEXT("エラー: vsk_sound_initが失敗しました。\n");
        }
    }
    else // The others are Let's la English
#endif
    {
        switch (id)
        {
        case 0: return TEXT("cmd_play version 1.2 by katahiromz");
        case 1:
            return
                TEXT("Usage: cmd_play [Options] [#n] [string1] [string2] [string3] [string4] [string5] [string6]\n")
                TEXT("\n")
                TEXT("Options:\n")
                TEXT("  -DVAR=VALUE            Assign to a variable.\n")
                TEXT("  -save_wav output.wav   Save as WAV file.\n")
                TEXT("  -help                  Display this message.\n")
                TEXT("  -version               Display version info.\n")
                TEXT("\n")
                TEXT("String variables can be expanded by enclosing them in [ ].");
        case 2: return TEXT("ERROR: Too many arguments.\n");
        case 3: return TEXT("ERROR: No string to play specified.\n");
        case 4: return TEXT("ERROR: The audio mode value (#n) is out of range.\n");
        case 5: return TEXT("ERROR: Illegal function call\n");
        case 6: return TEXT("ERROR: Option -save_wav needs an operand.\n");
        case 7: return TEXT("ERROR: '%ls' is an invalid option.\n");
        case 8: return TEXT("ERROR: vsk_sound_init failed.\n");
        }
    }

    assert(0);
    return nullptr;
}

void version(void)
{
    _putts(get_text(0));
}

void usage(void)
{
    _putts(get_text(1));
}

// 変数
std::map<VskString, VskString> g_variables;

// 再帰的に「{変数名}」を変数の値に置き換える関数
std::string
vsk_replace_placeholders(const std::string& str, std::unordered_set<std::string>& visited) {
    std::string result = str;
    size_t start_pos = 0;

    while ((start_pos = result.find("{", start_pos)) != result.npos) {
        size_t end_pos = result.find("}", start_pos);
        if (end_pos == std::string::npos)
            break; // 閉じカッコが見つからない場合は終了

        std::string key = result.substr(start_pos + 1, end_pos - start_pos - 1);
        CharUpperA(&key[0]);
        if (visited.find(key) != visited.end()) {
            // 循環参照を検出した場合はエラーとして処理する
            throw std::runtime_error("Circular reference detected: " + key);
        }
        visited.insert(key);

        auto it = g_variables.find(key);
        if (it != g_variables.end()) {
            // ここで再帰的に置き換えを行う
            std::string value = vsk_replace_placeholders(it->second, visited);
            result.replace(start_pos, end_pos - start_pos + 1, value);
            start_pos += value.length(); // 置き換えた後の新しい開始位置に移動
        } else {
            result.replace(start_pos, end_pos - start_pos + 1, "");
        }
        visited.erase(key);
    }

    return result;
}

// 再帰的に「{変数名}」を変数の値に置き換える関数
std::string vsk_replace_placeholders(const std::string& str)
{
    std::unordered_set<std::string> visited;
    return vsk_replace_placeholders(str, visited);
}

// 再帰的に「[変数名]」を変数の値に置き換える関数
std::string
vsk_replace_placeholders2(const std::string& str, std::unordered_set<std::string>& visited) {
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
            throw std::runtime_error("Circular reference detected: " + key);
        }
        visited.insert(key);

        auto it = g_variables.find(key);
        if (it != g_variables.end()) {
            // ここで再帰的に置き換えを行う
            std::string value = vsk_replace_placeholders2(it->second, visited);
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
std::string vsk_replace_placeholders2(const std::string& str)
{
    std::unordered_set<std::string> visited;
    return vsk_replace_placeholders2(str, visited);
}

// ワイド文字列をSJIS文字列に変換
std::string vsk_sjis_from_wide(const wchar_t *wide)
{
    int size = WideCharToMultiByte(932, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size == 0)
        return "";
    std::string str;
    str.resize(size - 1);
    WideCharToMultiByte(932, 0, wide, -1, &str[0], size, nullptr, nullptr);
    return str;
}

#define VSK_MAX_CHANNEL 6

struct CMD_PLAY
{
    std::vector<std::string> m_str_to_play;
    std::wstring m_output_file;
    int m_audio_mode = 2;

    int parse_cmd_line(int argc, wchar_t **argv);
    int run();
    bool load_settings();
    bool save_settings();
};

static LPCWSTR s_setting_key[] = {
    L"setting0",
    L"setting1",
    L"setting2",
    L"setting3",
    L"setting4",
    L"setting5",
};

bool CMD_PLAY::load_settings()
{
    HKEY hKey;

    LSTATUS error = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play", 0,
                                  KEY_READ, &hKey);
    if (error)
        return false;

    size_t size = vsk_cmd_play_get_setting_size();

    std::vector<uint8_t> setting[6];
    for (int ch = 0; ch < 6; ++ch)
    {
        setting[ch].resize(size);

        DWORD cbValue = size;
        error = RegQueryValueExW(hKey, s_setting_key[ch], NULL, NULL, setting[ch].data(), &cbValue);
        if (!error)
            vsk_cmd_play_set_setting(ch, setting[ch]);
    }

    RegCloseKey(hKey);

    return true;
}

bool CMD_PLAY::save_settings()
{
    HKEY hKey;
    LSTATUS error = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Katayama Hirofumi MZ\\cmd_play", 0,
                                    NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (error)
        return false;

    for (int ch = 0; ch < 6; ++ch)
    {
        std::vector<uint8_t> setting;
        vsk_cmd_play_get_setting(ch, setting);
        RegSetValueExW(hKey, s_setting_key[ch], 0, REG_BINARY, setting.data(), (DWORD)setting.size());
    }
    RegCloseKey(hKey);

    return true;
}

int CMD_PLAY::parse_cmd_line(int argc, wchar_t **argv)
{
    if (argc <= 1)
    {
        usage();
        return 1;
    }

    for (int iarg = 1; iarg < argc; ++iarg)
    {
        LPWSTR arg = argv[iarg];

        if (_wcsicmp(arg, L"-help") == 0 || _wcsicmp(arg, L"--help") == 0)
        {
            usage();
            return 1;
        }

        if (_wcsicmp(arg, L"-version") == 0 || _wcsicmp(arg, L"--version") == 0)
        {
            version();
            return 1;
        }

        if (arg[0] == '-' && (arg[1] == 'd' || arg[1] == 'D'))
        {
            VskString str = vsk_sjis_from_wide(&arg[2]);
            auto ich = str.find('=');
            if (ich == str.npos)
            {
                TCHAR text[256];
                StringCchPrintf(text, _countof(text), get_text(7), arg);
                _ftprintf(stderr, TEXT("%ls"), text);
                return 1;
            }
            auto var = str.substr(0, ich);
            auto value = str.substr(ich + 1);
            CharUpperA(&var[0]);
            CharUpperA(&value[0]);
            g_variables[var] = value;
            continue;
        }

        if (_wcsicmp(arg, L"-save_wav") == 0 || _wcsicmp(arg, L"--save_wav") == 0)
        {
            if (iarg + 1 < argc)
            {
                m_output_file = argv[++iarg];
                continue;
            }
            else
            {
                _ftprintf(stderr, TEXT("%ls"), get_text(6));
                return 1;
            }
        }

        if (arg[0] == '-')
        {
            TCHAR text[256];
            StringCchPrintf(text, _countof(text), get_text(3), arg);
            _ftprintf(stderr, TEXT("%ls"), text);
            return 1;
        }

        if (arg[0] == '#')
        {
            m_audio_mode = _wtoi(&arg[1]);
            if (!(0 <= m_audio_mode && m_audio_mode <= 4))
            {
                _ftprintf(stderr, TEXT("%ls"), get_text(4));
                return 1;
            }
            continue;
        }

        if (m_str_to_play.size() < VSK_MAX_CHANNEL)
        {
            m_str_to_play.push_back(vsk_sjis_from_wide(arg).c_str());
            continue;
        }

        _ftprintf(stderr, get_text(2));
        return 1;
    }

    if (m_str_to_play.empty())
    {
        _ftprintf(stderr, get_text(3));
        return 1;
    }

    return 0;
}

int CMD_PLAY::run()
{
    if (!vsk_sound_init())
    {
        _ftprintf(stderr, get_text(8));
        return 1;
    }

    load_settings();

    if (m_output_file.size())
    {
        switch (m_audio_mode)
        {
        case 0:
            if (!vsk_sound_cmd_play_ssg_save(m_str_to_play, m_output_file.c_str()))
            {
                _ftprintf(stderr, TEXT("%ls"), get_text(5));
                vsk_sound_exit();
                return 1;
            }
            break;
        case 2:
        case 3:
        case 4:
            if (!vsk_sound_cmd_play_fm_and_ssg_save(m_str_to_play, m_output_file.c_str()))
            {
                _ftprintf(stderr, TEXT("%ls"), get_text(5));
                vsk_sound_exit();
                return 1;
            }
            break;
        }
        return 0;
    }

    switch (m_audio_mode)
    {
    case 0:
        if (!vsk_sound_cmd_play_ssg(m_str_to_play))
        {
            _ftprintf(stderr, TEXT("%ls"), get_text(5));
            vsk_sound_exit();
            return 1;
        }
        break;
    case 2:
    case 3:
    case 4:
        if (!vsk_sound_cmd_play_fm_and_ssg(m_str_to_play))
        {
            _ftprintf(stderr, TEXT("%ls"), get_text(5));
            vsk_sound_exit();
            return 1;
        }
        break;
    }

    vsk_sound_wait(-1);

    save_settings();

    vsk_sound_exit();

    return 0;
}

int wmain(int argc, wchar_t **argv)
{
    CMD_PLAY play;
    if (int ret = play.parse_cmd_line(argc, argv))
        return ret;

    if (int ret = play.run())
        return ret;

    return 0;
}

#include <clocale>

int main(void)
{
    // Unicode console output support
    std::setlocale(LC_ALL, "");

    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    int ret = wmain(argc, argv);
    LocalFree(argv);

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

    return ret;
}

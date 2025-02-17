#include "types.h"
#include "sound.h"
#include <cstdio>
#include <windows.h>
#include <mmsystem.h>
#include <shlwapi.h>

#include "soundplayer.h"                // サウンドプレーヤー

// サウンドプレーヤー
std::shared_ptr<VskSoundPlayer> vsk_sound_player;

// WAVE出力用の変数
WAVEFORMATEX vsk_wfx;
HWAVEOUT vsk_hWaveOut = nullptr;
WAVEHDR vsk_waveHdr = { nullptr };

// WAVE出力用のコールバック関数
static void CALLBACK
waveOutProc(
    HWAVEOUT hWaveOut,
    UINT uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE) {
        // バッファの再生が完了したときの処理
        Sleep(50);
        vsk_sound_player->m_stopping_event.set();
    }
}

//////////////////////////////////////////////////////////////////////////////

// リズム音源データのある場所を取得する
bool vsk_get_rhythm_path(char *path, size_t path_max)
{
    GetModuleFileNameA(NULL, path, path_max); // EXEファイルのパスファイル名を取得

    // rhythmフォルダを探す
    PathRemoveFileSpecA(path); // パスの最後の項目を削除
    PathAppendA(path, "rhythm\\");
    if (PathIsDirectoryA(path))
        return true;

    PathRemoveFileSpecA(path);
    PathRemoveFileSpecA(path);
    PathAppendA(path, "rhythm\\");
    if (PathIsDirectoryA(path))
        return true;

    PathRemoveFileSpecA(path);
    PathRemoveFileSpecA(path);
    PathRemoveFileSpecA(path);
    PathAppendA(path, "rhythm\\");

    if (PathIsDirectoryA(path))
        return true;

    path[0] = 0;
    return false; // 見つからなかった
}

// 音源を初期化する
bool vsk_sound_init(bool stereo)
{
    // リズム音源のある場所を取得
    char rhythm_path[MAX_PATH];
    vsk_get_rhythm_path(rhythm_path, _countof(rhythm_path));

    // サウンドプレーヤーを作成
    vsk_sound_player = std::make_shared<VskSoundPlayer>(rhythm_path);

    // WAVEFORMATEX構造体を初期化
    ZeroMemory(&vsk_wfx, sizeof(vsk_wfx));
    vsk_wfx.wFormatTag = WAVE_FORMAT_PCM; // PCM
    vsk_wfx.nChannels = (stereo ? 2 : 1); // チャンネル数
    vsk_wfx.nSamplesPerSec = 44100; // サンプリングレート
    vsk_wfx.wBitsPerSample = 16; // ビット深度
    vsk_wfx.nBlockAlign = (vsk_wfx.nChannels * vsk_wfx.wBitsPerSample) / 8;
    vsk_wfx.nAvgBytesPerSec = vsk_wfx.nSamplesPerSec * vsk_wfx.nBlockAlign;
    vsk_wfx.cbSize = 0;

    // Wave出力を開く
    ZeroMemory(&vsk_waveHdr, sizeof(vsk_waveHdr));
    vsk_waveHdr.dwFlags |= WHDR_DONE;
    MMRESULT result = waveOutOpen(&vsk_hWaveOut, WAVE_MAPPER, &vsk_wfx, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION | WAVE_ALLOWSYNC);
    return (result == MMSYSERR_NOERROR);
}

// 音源を停止する
void vsk_sound_stop(void)
{
    // Wave出力をリセット
    vsk_waveHdr.dwFlags |= WHDR_DONE;
    waveOutReset(vsk_hWaveOut);

    vsk_sound_player->m_stopping_event.set();
}

// 音声を再生する
void vsk_sound_play(const void *data, size_t data_size, bool stereo)
{
    // いったん音声を止める
    vsk_sound_stop();
    vsk_sound_player->m_stopping_event.reset();

    // Wave出力の準備をする
    ZeroMemory(&vsk_waveHdr, sizeof(vsk_waveHdr));
    vsk_waveHdr.lpData = (LPSTR)data;
    vsk_waveHdr.dwBufferLength = data_size;
    vsk_waveHdr.dwFlags = 0;
    vsk_waveHdr.dwLoops = 0;
    waveOutPrepareHeader(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));

    // Waveデータを出力する
    waveOutWrite(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));
}

// 音源を破棄する
void vsk_sound_exit(void)
{
    // 音を止める
    vsk_sound_stop();

    // Wave出力を閉じる
    waveOutUnprepareHeader(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));
    waveOutClose(vsk_hWaveOut);
    vsk_hWaveOut = nullptr;

    // サウンドプレーヤーを解放する
    vsk_sound_player = nullptr;
}

// 音源が演奏中か？
bool vsk_sound_is_playing(void)
{
    return !(vsk_waveHdr.dwFlags & WHDR_DONE);
}

// 音源を待つ。単位はミリ秒
bool vsk_sound_wait(VskDword milliseconds)
{
    if (!vsk_sound_is_playing())
        return false;

    return vsk_sound_player->wait_for_stop(milliseconds);
}

// OPNのレジスタにデータを設定する
bool vsk_sound_voice_reg(int addr, int data)
{
    if (!vsk_sound_player)
        return false;

    vsk_sound_player->write_reg(addr, data);
    return true;
}

// 音色のサイズを取得する
size_t vsk_sound_voice_size(void)
{
    return sizeof(ym2203_tone_table[0]);
}

// 音色をコピーする
bool vsk_sound_voice_copy(int tone, std::vector<uint8_t>& data)
{
    if (tone < 0 || tone >= NUM_TONES)
        return false;

    size_t size = sizeof(ym2203_tone_table[tone]);
    data.resize(size);

    std::memcpy(data.data(), ym2203_tone_table[tone], size);
    return true;
}

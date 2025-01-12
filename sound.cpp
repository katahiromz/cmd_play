#include "types.h"
#include "sound.h"
#include <cstdio>
#include <windows.h>
#include <mmsystem.h>

#include "freealut/include/AL/alut.h"   // OpenAL utility
#include "soundplayer.h"                // サウンドプレーヤー

// サウンドプレーヤー
std::shared_ptr<VskSoundPlayer> vsk_sound_player;

WAVEFORMATEX vsk_wfx;
HWAVEOUT vsk_hWaveOut = nullptr;
WAVEHDR vsk_waveHdr = { nullptr };

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
        vsk_sound_player->m_stopping_event.pulse();
    }
}

//////////////////////////////////////////////////////////////////////////////

// 音源を初期化する
bool vsk_sound_init(void)
{
    if (!alutInit(NULL, NULL))
    {
        ALenum error = alutGetError();
        std::fprintf(stderr, "%s\n", alutGetErrorString(error));
        return false;
    }
    vsk_sound_player = std::make_shared<VskSoundPlayer>();

    ZeroMemory(&vsk_wfx, sizeof(vsk_wfx));
    vsk_wfx.wFormatTag = WAVE_FORMAT_PCM;
    vsk_wfx.nChannels = 1; // モノラル
    vsk_wfx.nSamplesPerSec = 44100 * 2; // サンプリングレート
    vsk_wfx.wBitsPerSample = 16; // ビット深度
    vsk_wfx.nBlockAlign = (vsk_wfx.nChannels * vsk_wfx.wBitsPerSample) / 8;
    vsk_wfx.nAvgBytesPerSec = vsk_wfx.nSamplesPerSec * vsk_wfx.nBlockAlign;
    vsk_wfx.cbSize = 0;

    ZeroMemory(&vsk_waveHdr, sizeof(vsk_waveHdr));
    vsk_waveHdr.dwFlags |= WHDR_DONE;

    MMRESULT result = waveOutOpen(&vsk_hWaveOut, WAVE_MAPPER, &vsk_wfx, (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION | WAVE_ALLOWSYNC);
    return (result == MMSYSERR_NOERROR);
}

// 音源を停止する
void vsk_sound_stop(void)
{
    vsk_waveHdr.dwFlags |= WHDR_DONE;
    waveOutReset(vsk_hWaveOut);
    vsk_sound_player->m_stopping_event.pulse();
}

// 音声を再生する
void vsk_sound_play(const void *data, size_t data_size)
{
    vsk_sound_stop();

    ZeroMemory(&vsk_waveHdr, sizeof(vsk_waveHdr));
    vsk_waveHdr.lpData = (LPSTR)data;
    vsk_waveHdr.dwBufferLength = data_size;
    vsk_waveHdr.dwFlags = 0;
    vsk_waveHdr.dwLoops = 0;

    waveOutPrepareHeader(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));
    waveOutWrite(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));
}

// 音源を破棄する
void vsk_sound_exit(void)
{
    vsk_sound_stop();
    waveOutUnprepareHeader(vsk_hWaveOut, &vsk_waveHdr, sizeof(WAVEHDR));
    waveOutClose(vsk_hWaveOut);
    alutExit();
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
    if (vsk_sound_is_playing())
        return vsk_sound_player->wait_for_stop(milliseconds);
    return false;
}

// OpenALのエラー文字列を取得する
const char *vsk_get_openal_error(int error)
{
    switch (error)
    {
    case AL_NO_ERROR:           return "No Error";
    case AL_INVALID_NAME:       return "Invalid Name";
    case AL_INVALID_ENUM:       return "Invalid Enum";
    case AL_INVALID_VALUE:      return "Invalid Value";
    case AL_INVALID_OPERATION:  return "Invalid Operation";
    case AL_OUT_OF_MEMORY:      return "Out of Memory";
    default:                    return "Unknown Error";
    }
}

// OpenALのエラー文字列を出力する
void vsk_print_openal_error(int error)
{
    std::printf(vsk_get_openal_error(error));
}

// OPNのレジスタにデータを設定する
bool vsk_sound_voice_reg(int addr, int data)
{
    if (vsk_sound_player) {
        vsk_sound_player->write_reg(addr, data);
        return true;
    }
    return false;
}

#include "types.h"
#include "sound.h"
#include <cstdio>

#include "freealut/include/AL/alut.h"   // OpenAL utility
#include "fmgon/soundplayer.h"          // サウンドプレーヤー

// サウンドプレーヤー
std::shared_ptr<VskSoundPlayer> vsk_sound_player;

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
    return true;
}

// 音源を停止する
void vsk_sound_stop(void)
{
    if (vsk_sound_player)
        vsk_sound_player->stop();
}

// 音源を破棄する
void vsk_sound_exit(void)
{
    vsk_sound_stop();
    alutExit();
    vsk_sound_player = nullptr;
}

// 音源が演奏中か？
bool vsk_sound_is_playing(void)
{
    return vsk_sound_player && vsk_sound_player->m_playing_music;
}

// 音源を待つ。単位はミリ秒
bool vsk_sound_wait(VskDword milliseconds)
{
    if (vsk_sound_is_playing())
        return vsk_sound_player->wait_for_stop(milliseconds);
    return false;
}

// BEEP音を出す
void vsk_sound_beep(int i)
{
    assert(vsk_sound_player);
    if (vsk_sound_player) {
        vsk_sound_player->beep(i);
    }
}

// BEEP音を出しているか？
bool vsk_sound_is_beeping(void)
{
    return vsk_sound_player && vsk_sound_player->is_beeping();
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

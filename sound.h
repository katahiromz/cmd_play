#pragma once

#include "types.h"

bool vsk_sound_init(void);
void vsk_sound_exit(void);
void vsk_sound_play(const void *data, size_t data_size);
bool vsk_sound_is_playing(void);
bool vsk_sound_wait(VskDword milliseconds);
void vsk_sound_stop(void);
const char *vsk_get_openal_error(int error);
void vsk_print_openal_error(int error);
bool vsk_sound_voice_reg(int addr, int data);

enum VSK_SOUND_ERR
{
    VSK_SOUND_ERR_SUCCESS = 0,
    VSK_SOUND_ERR_ILLEGAL,
    VSK_SOUND_ERR_IO_ERROR,
};

// CMD SING
VSK_SOUND_ERR vsk_sound_cmd_sing(const char *str);
VSK_SOUND_ERR vsk_sound_cmd_sing(const wchar_t *str);
VSK_SOUND_ERR vsk_sound_cmd_sing_save(const char *str, const wchar_t *filename);
VSK_SOUND_ERR vsk_sound_cmd_sing_save(const wchar_t *wstr, const wchar_t *filename);

// CMD PLAY
bool vsk_sound_cmd_play_ssg(const std::vector<VskString>& strs);
bool vsk_sound_cmd_play_fm_and_ssg(const std::vector<VskString>& strs);
bool vsk_sound_cmd_play_fm(const std::vector<VskString>& strs);
bool vsk_sound_cmd_play_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename);
bool vsk_sound_cmd_play_fm_and_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename);
bool vsk_sound_cmd_play_fm_save(const std::vector<VskString>& strs, const wchar_t *filename);

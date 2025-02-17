#pragma once

#include "types.h"
#include <map>

bool vsk_sound_init(bool stereo);
void vsk_sound_exit(void);
void vsk_sound_play(const void *data, size_t data_size, bool stereo);
bool vsk_sound_is_playing(void);
bool vsk_sound_wait(VskDword milliseconds);
void vsk_sound_stop(void);
bool vsk_sound_voice_reg(int addr, int data);
size_t vsk_sound_voice_size(void);
bool vsk_sound_voice_copy(int tone, std::vector<uint8_t>& data);
#ifndef VEYSICK
std::string vsk_sjis_from_wide(const wchar_t *wide);
#endif

enum VSK_SOUND_ERR
{
    VSK_SOUND_ERR_SUCCESS = 0,
    VSK_SOUND_ERR_ILLEGAL,
    VSK_SOUND_ERR_IO_ERROR,
};

// CMD SING
VSK_SOUND_ERR vsk_sound_cmd_sing(const char *str, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_sing(const wchar_t *str, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_sing_save(const char *str, const wchar_t *filename, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_sing_save(const wchar_t *wstr, const wchar_t *filename, bool stereo);
void vsk_cmd_sing_reset_settings(void);
size_t vsk_cmd_sing_get_setting_size(void);
bool vsk_cmd_sing_get_setting(std::vector<uint8_t>& data);
bool vsk_cmd_sing_set_setting(const std::vector<uint8_t>& data);

// CMD PLAY
VSK_SOUND_ERR vsk_sound_cmd_play_ssg(const std::vector<VskString>& strs, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg(const std::vector<VskString>& strs, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_play_fm(const std::vector<VskString>& strs, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_play_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_play_fm_and_ssg_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo);
VSK_SOUND_ERR vsk_sound_cmd_play_fm_save(const std::vector<VskString>& strs, const wchar_t *filename, bool stereo);
void vsk_cmd_play_reset_settings(void);
size_t vsk_cmd_play_get_setting_size(void);
bool vsk_cmd_play_get_setting(int ch, std::vector<uint8_t>& data);
bool vsk_cmd_play_set_setting(int ch, const std::vector<uint8_t>& data);
bool vsk_cmd_play_voice(int ich, const void *data, size_t data_size);

// variables
extern std::map<VskString, VskString> g_variables;

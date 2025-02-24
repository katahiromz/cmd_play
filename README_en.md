(English)

# cmd_play by katahiromz

## Overview

This is a program that emulates (reproduces) the `CMD PLAY` statement of the N88-BASIC language of the old PC-8801 computer on a modern Windows computer.

## Usage

    Usage: cmd_play [Options] [#n] [string1] [string2] [string3] [string4] [string5] [string6]

    Options:
      -DVAR=VALUE                Assign to a variable.
      -save-wav output.wav       Save as WAV file (except MIDI sound).
      -save-mid output.mid       Save as MID file (MIDI sound only).
      -reset                     Stop music and reset settings.
      -stopm                     Same as -reset except that the tone is not changed.
      -stereo                    Make sound stereo (default).
      -mono                      Make sound mono.
      -voice CH FILE.voi         Load a tone from a file to channel CH (FM sound).
      -voice CH "CSV"            Load a tone from an array to channel CH (FM sound).
      -voice-copy TONE FILE.voi  Copy the tone to a file (FM sound).
      -bgm 0                     Wait until the performance is over.
      -bgm 1                     Don't wait until the performance is over (default).
      -help                      Display this message.
      -version                   Display version info.

    Numeric variables can be specified by enclosing them between an equal sign and a semicolon: "L=variable name;".
    String variables can be expanded by enclosing them in [ ].

## `CMD PLAY` statement (8801 only) (CMD extension)

- [Function] Plays music.
- [Etymology] Command play
- [Format] `CMD PLAY [#`*sound source mode*`,] [` *string 1* `][,` *string 2* `][,` *string 3* `][,` *string 4* `][,` *string 5* `][,` *string 6* `]`
- [Example] `CMD PLAY "CDE"` ⇒ Plays Do-Re-Mi
- [Explanation] Can play up to 6 chords. *string 1*, *string 2*, *string 3* correspond to channels 1, 2, 3, *string 4*, *string 5*, *string 6* correspond to channels 4, 5, 6.

*sound source mode* is a value between `0` and `5`, and has the following meanings.

- `#0` ... Uses an external SSG sound source. Channels 1 to 6 use the SSG sound source.
- `#1` ... Uses an external MIDI sound source. Channels 1 to 6 use the MIDI sound source.
- `#2` ... Channels 1 to 3 use the FM sound source, and channels 4 to 6 use the SSG sound source.
- `#3` ... Switches OPN to sound effect mode. Sound is produced by the `Y` statement of `CMD PLAY`.
- `#4` ... Switches OPN to CSM (sine wave) mode. Sound is produced by the `Y` statement of `CMD PLAY`.
- `#5` ... Uses FM sound source. Channels 1 to 6 use the FM sound source.

If *sound source mode* is omitted, `#2` is selected. `#5` can only be selected on sound board 2.
Normally, use *sound source mode* at `#0`, `#1`, `#2`, or `#5`. Rhythm sound sources are not yet supported.

The character strings for each channel are called MML (Music Macro Language), and have the following meanings:

| String                          | Meaning                                                                                                       | Default |
|:--------------------------------|:--------------------------------------------------------------------------------------------------------------|:--------|
| Mx (SSG only)                   | Sets the envelope period (1≦x≦65535). x is optional.                                                        | M255    |
| Sx (SSG only)                   | Sets the envelope shape (0≦x≦15). x is optional.                                                            | S1      |
| Vx                              | Sets the volume (0≦x≦15). x is optional.                                                                    | V8      |
| Lx                              | Sets the default length of notes and rests (1≦x≦64). x is optional.                                         | L4      |
| Qx                              | Sets the length ratio of notes (1≦x≦8). x is optional.                                                      | Q8      |
| Ox                              | Sets the octave (1≦x≦8). x is optional.                                                                     | O4      |
| >                               | Raises the octave by one.                                                                                     |         |
| <                               | Lowers the octave by one.                                                                                     |         |
| Nx                              | Plays the note of the height specified by x (0≦x≦96). x is not optional.                                    |         |
| Tx                              | Sets the tempo (32≦x≦255). x is optional.                                                                   | T120    |
| Rx                              | Plays a rest (1≦x≦64). x is the length of the rest. x is optional.                                          | R4      |
| C～B[+/-][x][.]                 | Plays a note (1≦x≦64). x is the length of the note. x is optional.                                          |         |
| + (#)                           | Raises the previous note by a semitone.                                                                       |         |
| -                               | Lowers the previous note by a semitone.                                                                       |         |
| .                               | Adds a dot to the previous note or rest, lengthening it by 1.5 times.                                         |         |
| &                               | Connects the preceding and following notes.                                                                   |         |
| { ... }x                        | Plays a tuple of notes equally divided by x in the specified length. x is optional.                           |         |
| @x (FM/MIDI only)               | Switches to the tone specified by the tone number x. x is not optional.                                       | @0      |
| Yr,d (OPN only)                 | Sets the content of OPN register r to d (0≦r≦178 and 0≦d≦255).                                            |         |
| Zd (MIDI only)                  | Sends 1-byte data d to MIDI (0≦d≦255).                                                                      |         |
| @M (Soundboard 2, FM/MIDI only) | Sets the output to both left and right channels.                                                              |         |
| @L (Soundboard 2, FM/MIDI only) | Sets the output to the left channel.                                                                          |         |
| @R (Soundboard 2, FM/MIDI only) | Sets the output to the right channel.                                                                         |         |
| @Vx (FM/MIDI only)              | Fine-tunes the volume (0≦x≦127). x is not optional.                                                         | @V106   |
| @Wx                             | Maintains the state for the length specified by x (1≦x≦64). x is optional and defaults to the value of `L`. |         |

- The musical notes `CDEFGAB` correspond to "Do Re Mi Fa So La Si".
- For tone numbers, see the tone list.
- `@x` cannot be specified in MIDI on the actual machine. Instead, use `Zd` multiple times to change the program.
- `@M`, `@L`, and `@R` cannot be specified in MIDI on the actual machine. Instead, use `Zd` multiple times to set the pan.
- For more information about MIDI, see the Wikipedia MIDI page.
- On some models, after you set `@V`, `@V` takes precedence over the `V` setting.
- The `CMD PLAY` statement is an extended CMD command and can only be used on the 8801. Before using `CMD PLAY`, you must execute [`NEW CMD`](#new_cmd) (except in unlimited mode). If you specify a string longer than 255 characters, a `String too long` error will occur (except in unlimited mode).

## License Agreement

- Use of this software must comply with the terms of use of the "fmgon license" described in [LICENSE.txt](LICENSE.txt).
- The source code of this software can be downloaded from https://github.com/katahiromz/cmd_play .
- Parts of this software are copyrighted by cisc (cisc@retropc.net).

## List of FM tone generator tones

| Tone number | Tone name        | Explanation                                   |
|------------:|:-----------------|:----------------------------------------------|
| `0`         | `Default VOICE`  | Default tone                                  |
| `1`         | `BRASS 2`        | Brass tone                                    |
| `2`         | `STRING 2`       | String tone                                   |
| `3`         | `EPIANO 3`       | Electronic piano tone                         |
| `4`         | `EBASS 1`        | Electronic bass tone                          |
| `5`         | `EORGAN 1`       | Electronic organ tone                         |
| `6`         | `PORGAN 1`       | Pipe organ tone                               |
| `7`         | `FLUTE`          | Flute                                         |
| `8`         | `OBOE`           | Oboe                                          |
| `9`         | `CLARINET`       | Clarinet                                      |
| `10`        | `VIBRPHN`        | Vibraphone                                    |
| `11`        | `HARPSIC`        | Harpsichord                                   |
| `12`        | `BELL`           | Bell                                          |
| `13`        | `PIANO`          | Piano                                         |
| `14`        | `MUSHI`          | Sound of insects chirping                     |
| `15`        | `DESCENT`        | Sound of descending from high altitude        |
| `16`        | `UFO`            | Sound of a UFO moving away                    |
| `17`        | `GRANPRI`        | Racing car engine sound                       |
| `18`        | `LASER 1`        | Laser gun sound                               |
| `19`        | `LASER 2`        | Laser gun sound                               |
| `20`        | `SIN WAVE`       | Sine wave for tuning                          |
| `21`        | `BRASS 1`        | Brass sound                                   |
| `22`        | `BRASS 2`        | Brass type sound, same as tone 1              |
| `23`        | `TRUMPET`        | Trumpet                                       |
| `24`        | `STRING 1`       | String type sound                             |
| `25`        | `STRING 2`       | String type sound, same as tone 2             |
| `26`        | `EPIANO 1`       | Electronic piano type sound                   |
| `27`        | `EPIANO 2`       | Electronic piano type sound                   |
| `28`        | `EPIANO 3`       | Electronic piano type sound, same as tone 3   |
| `29`        | `GUITAR`         | Guitar                                        |
| `30`        | `EBASS 1`        | Electronic bass type sound, same as tone 4    |
| `31`        | `EBASS 2`        | Electronic bass type sound                    |
| `32`        | `EORGAN 1`       | Electronic organ sound, same as tone 5        |
| `33`        | `EORGAN 2`       | Electronic organ sound                        |
| `34`        | `PORGAN 1`       | Pipe organ sound, same as tone 6              |
| `35`        | `PORGAN 2`       | Pipe organ sound                              |
| `36`        | `FLUTE`          | Flute, same as tone 7                         |
| `37`        | `PICCOLO`        | Piccolo                                       |
| `38`        | `OBOE`           | Oboe, same as tone 8                          |
| `39`        | `CLARINET`       | Clarinet, same as tone 9                      |
| `40`        | `GROCKEN`        | Glockenspiel                                  |
| `41`        | `VIBRPHN`        | Vibraphone, same as tone 10                   |
| `42`        | `XYLOPHN`        | Xylophone                                     |
| `43`        | `KOTO`           | Koto                                          |
| `44`        | `ZITAR`          | Zither                                        |
| `45`        | `CLAV`           | Clavinet                                      |
| `46`        | `HARPSIC`        | Harpsichord, same as tone 0 and 11            |
| `47`        | `BELL`           | Bell, same as tone 12                         |
| `48`        | `HARP`           | Harp                                          |
| `49`        | `BELL/BRASS`     | Bell with staccato, brass with long tone      |
| `50`        | `HARMONICA`      | Harmonica                                     |
| `51`        | `STEEL DRUM`     | Steel drum                                    |
| `52`        | `TIMPANI`        | Timpani                                       |
| `53`        | `TRAIN`          | Train whistle                                 |
| `54`        | `AMBULAN`        | Ambulance                                     |
| `55`        | `TWEET`          | Birds chirping                                |
| `56`        | `RAIN DROP`      | Raindrops falling                             |
| `57`        | `HORN`           | Horn                                          |
| `58`        | `SNARE DRUM`     | Snare drum                                    |
| `59`        | `COW BELL`       | Cowbell                                       |
| `60`        | `PERC 1`         | Percussion sounds                             |
| `61`        | `PERC 2`         | Percussion sounds                             |

## List of MIDI tone generator tones

| Tone number | Tone name      | Explanation                        |
|------------:|:---------------|:-----------------------------------|
| `000`       | `Piano 1`      | Grand piano                        |
| `001`       | `Piano 2`      | Bright piano                       |
| `002`       | `Piano 3`      | Electric grand piano               |
| `003`       | `Piano 4`      | Honky tonk                         |
| `004`       | `E.Piano 1`    | Electric piano 1                   |
| `005`       | `E.Piano 2`    | Electric piano 2                   |
| `006`       | `Harpsichord`  | Harpsichord                        |
| `007`       | `Clavi`        | Clavi                              |
| `008`       | `Celesta`      | Celesta                            |
| `009`       | `Glockenspl`   | Glockenspiel                       |
| `010`       | `MusicBox`     | Music box                          |
| `011`       | `Vibraphone`   | Vibraphone                         |
| `012`       | `Marimba`      | Marimba                            |
| `013`       | `Xylophone`    | Xylophone                          |
| `014`       | `Tubularbell`  | Tubular bell                       |
| `015`       | `Santur`       | Santur                             |
| `016`       | `Organ 1`      | Drawbar organ                      |
| `017`       | `Organ 2`      | Percussive organ                   |
| `018`       | `Organ 3`      | Rock organ                         |
| `019`       | `Church Org1`  | Church organ                       |
| `020`       | `Reed Organ`   | Reed organ                         |
| `021`       | `Accordion`    | Accordion                          |
| `022`       | `Harmonica`    | Harmonica                          |
| `023`       | `Bandoneon`    | Bandoneon                          |
| `024`       | `Nylon Gt.`    | Nylon string guitar                |
| `025`       | `Steel Gt.`    | Steel string guitar                |
| `026`       | `Jazz Gt.`     | Jazz guitar                        |
| `027`       | `Clean Gt.`    | Clean guitar                       |
| `028`       | `Muted Gt.`    | Muted guitar                       |
| `029`       | `OverdriveGt`  | Overdrive guitar                   |
| `030`       | `Dist.Gt.`     | Distortion guitar                  |
| `031`       | `Gt.Harmonix`  | Guitar harmonics                   |
| `032`       | `Acoustic Bs`  | Acoustic Bass                      |
| `033`       | `Fingered Bs`  | Electric Bass 1                    |
| `034`       | `Picked Bass`  | Electric Bass 2                    |
| `035`       | `Fretless Bs`  | Fretless Bass                      |
| `036`       | `Slap Bass 1`  | Slap Bass 1                        |
| `037`       | `Slap Bass 2`  | Slap Bass 2                        |
| `038`       | `Syn.Bass 1`   | Synth Bass 1                       |
| `039`       | `Syn.Bass 2`   | Synth Bass 2                       |
| `040`       | `Violin`       | Violin                             |
| `041`       | `Viola`        | Viola                              |
| `042`       | `Cello`        | Cello                              |
| `043`       | `Contrabass`   | Contrabass                         |
| `044`       | `Tremolo Str`  | Tremolo Strings                    |
| `045`       | `Pizzicato`    | Pizzicato                          |
| `046`       | `Harp`         | Harp                               |
| `047`       | `Timpani`      | Timpani                            |
| `048`       | `Strings`      | Strings 1                          |
| `049`       | `SlowStrings`  | Strings 2                          |
| `050`       | `SynStrings1`  | Synth Strings 1                    |
| `051`       | `SynStrings2`  | Synth Strings 2                    |
| `052`       | `Choir Aahs`   | Choir                              |
| `053`       | `Voice Oohs`   | Voice                              |
| `054`       | `SynVox`       | Synth Voice                        |
| `055`       | `Orchest.Hit`  | Orchestra Hit                      |
| `056`       | `Trumpet`      | Trumpet                            |
| `057`       | `Trombone`     | Trombone                           |
| `058`       | `Tuba`         | Tuba                               |
| `059`       | `MuteTrumpet`  | Muted Trumpet                      |
| `060`       | `French Horn`  | French Horn                        |
| `061`       | `Brass 1`      | Brass                              |
| `062`       | `Syn.Brass 1`  | Synth Brass 1                      |
| `063`       | `Syn.Brass 2`  | Synth Brass 2                      |
| `064`       | `Soprano Sax`  | Soprano Sax                        |
| `065`       | `Alto Sax`     | Alto Sax                           |
| `066`       | `Tenor Sax`    | Tenor Sax                          |
| `067`       | `Baritone Sax` | Baritone Sax                       |
| `068`       | `Oboe`         | Oboe                               |
| `069`       | `EnglishHorn`  | English Horn                       |
| `070`       | `Bassoon`      | Bassoon                            |
| `071`       | `Clarinet`     | Clarinet                           |
| `072`       | `Piccolo`      | Piccolo                            |
| `073`       | `Flute`        | Flute                              |
| `074`       | `Recorder`     | Recorder                           |
| `075`       | `Pan Flute`    | Pan Flute                          |
| `076`       | `Bottle Blow`  | Bottle Blow                        |
| `077`       | `Shakuhachi`   | Shakuhachi                         |
| `078`       | `Whistle`      | Whistle                            |
| `079`       | `Ocarina`      | Ocarina                            |
| `080`       | `Square Wave`  | Square Wave                        |
| `081`       | `Saw Wave`     | Sawtooth Wave                      |
| `082`       | `SynCalliope`  | Calliope                           |
| `083`       | `ChifferLead`  | ChifferLead                        |
| `084`       | `Charang`      | Charang                            |
| `085`       | `Voice Lead`   | Voice Lead                         |
| `086`       | `5th Saw`      | 5th Saw                            |
| `087`       | `Bass & Lead`  | Bass & Lead                        |
| `088`       | `Fantasia`     | New Age                            |
| `089`       | `Warm Pad`     | Warm Pad                           |
| `090`       | `Polysynth`    | Polysynth                          |
| `091`       | `Space Voice`  | Space Voice                        |
| `092`       | `Bowed Glass`  | Bowed Glass                        |
| `093`       | `Metal Pad`    | Metal Pad                          |
| `094`       | `Halo Pad`     | Halo Pad                           |
| `095`       | `Sweep Pad`    | Sweep Pad                          |
| `096`       | `Ice Rain`     | Raindrops                          |
| `097`       | `Soundtrack`   | Soundtrack                         |
| `098`       | `Crystal`      | Crystal                            |
| `099`       | `Atmosphere`   | Atmosphere                         |
| `100`       | `Brightness`   | Brightness                         |
| `101`       | `Goblin`       | Goblin                             |
| `102`       | `Echo Drops`   | Echo                               |
| `103`       | `Sci-Fi`       | Science Fiction                    |
| `104`       | `Sitar`        | Sitar                              |
| `105`       | `Banjo`        | Banjo                              |
| `106`       | `Shamisen`     | Shamisen                           |
| `107`       | `Koto`         | Koto                               |
| `108`       | `Kalimba`      | Kalimba                            |
| `109`       | `Bagpipe`      | Bagpipe                            |
| `110`       | `Fiddle`       | Fiddle                             |
| `111`       | `Shanai`       | Shanai                             |
| `112`       | `Tinkle Bell`  | Tinkle Bell                        |
| `113`       | `Agogo`        | Agogo                              |
| `114`       | `Steel Drum`   | Steel Drum                         |
| `115`       | `WoodBlock`    | Wood Block                         |
| `116`       | `Taiko`        | Taiko                              |
| `117`       | `Melodic Tom`  | Melodic Tom                        |
| `118`       | `Synth Drum`   | Synth Drum                         |
| `119`       | `Reverse Cym`  | Reverse Cymbal                     |
| `120`       | `Gt.FretNoiz`  | Guitar fret noise                  |
| `121`       | `BreathNoise`  | Breath noise                       |
| `122`       | `Seashore`     | Seashore                           |
| `123`       | `Bird`         | Bird                               |
| `124`       | `Telephone 1`  | Telephone 1                        |
| `125`       | `Helicopter`   | Helicopter                         |
| `126`       | `Applause`     | Applause                           |
| `127`       | `Gun shot`     | Gun shot                           |

## Contact

- Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>

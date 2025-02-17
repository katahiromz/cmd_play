﻿(Japanese, UTF-8)

# cmd_play by 片山博文MZ

## 概要

古いパソコン PC-8801 の N88-BASIC という言語の `CMD PLAY` 文を現代の Windows パソコンでエミュレート（再現）するプログラムです。

## 使い方

```txt
使い方: cmd_play [オプション] [#n] [文字列1] [文字列2] [文字列3] [文字列4] [文字列5] [文字列6]

オプション:
  -D変数名=値                変数に代入。
  -save_wav 出力.wav         WAVファイルとして保存。
  -stopm                     音楽を止めて設定をリセット。
  -stereo                    音をステレオにする(デフォルト)。
  -mono                      音をモノラルにする。
  -voice CH FILE.voi         ファイルからチャンネルCHに音色を読み込む。
  -voice-copy TONE FILE.voi  音色をファイルにコピーする。
  -help                      このメッセージを表示する。
  -version                   バージョン情報を表示する。

文字列変数は [ ] で囲えば展開できます。
```

## `CMD PLAY`文 (8801のみ) (CMD拡張)

- 【機能】 音楽を演奏します。
- 【語源】 Command play
- 【書式】 `CMD PLAY [#`*音源モード*`,] [` *文字列1* `][,` *文字列2* `][,` *文字列3* `][,` *文字列4* `][,` *文字列5* `][,` *文字列6* `]`
- 【文例】 `CMD PLAY "CDE"`　⇒　ドレミを演奏
- 【説明】 6和音まで演奏できます。*文字列1*、*文字列2*、*文字列3*はチャンネル1、2、3に、*文字列4*、*文字列5*、*文字列6*はチャンネル4、5、6に対応します。*音源モード*は`0`～`5`の値で、次のような意味があります。

- `#0` … 別売のミュージックインタフェースボードのSSG音源を使います。チャンネル1～6がSSG音源を使用します。
- `#1` … 別売のミュージックインタフェースボードのMIDIインタフェースを使います(未対応)。
- `#2` … チャンネル1、2、3がFM音源を、チャンネル4、5、6がSSG音源を使用します。
- `#3` … OPNを効果音モードに切り替えます。`CMD PLAY`の`Y`文によって音を出します。
- `#4` … OPNをCSM(サイン波)モードに切り替えます。`CMD PLAY`の`Y`文によって音を出します。
- `#5` … チャンネル1～6がFM音源を使用します。

`cmd_play`では現在、`#1`は選択できません。*音源モード*を省略した場合は、`#2`が選択されます。
`#5`は、スーパーICでなければ選択できません。
通常、*音源モード*は、`#0`、`#2`、または`#5`でお使いください。
リズム音源はまだ未対応です。

各チャンネルの文字列は、MML (Music Macro Language)といい、次のような意味を持ちます。

|文字列                        |意味                                                                            |初期値   |
|:-----------------------------|:-------------------------------------------------------------------------------|:--------|
|`Mx` (SSG音源のみ)            |エンベロープ周期を設定します(1≦`x`≦65535)。                                   |`M255`   |
|`Sx` (SSG音源のみ)            |エンベロープ形状を設定します(0≦`x`≦15)。                                      |`S1`     |
|`Vx`                          |音量を設定します(0≦`x`≦15)。                                                  |`V8`     |
|`Lx`                          |音符や休符の既定値の長さを設定します(1≦`x`≦64)。                              |`L4`     |
|`Qx`                          |音の長さの割合を設定します(1≦`x`≦8)。                                         |`Q8`     |
|`Ox`                          |オクターブを設定します(1≦`x`≦8)。                                             |`O4`     |
|`>`                           |オクターブを１つ上げます。                                                      |         |
|`<`                           |オクターブを１つ下げます。                                                      |         |
|`Nx`                          |`x`で指定された高さの音を発生します(0≦`x`≦96)。                               |         |
|`Tx`                          |テンポを設定します(32≦`x`≦255)。                                              |`T120`   |
|`Rx`                          |休符を演奏します(1≦`x`≦64)。`x`は休符の長さです。                             |`R4`     |
|`C`～`B[+/-][x][.]`           |音符を演奏します(1≦`x`≦64)。`x`は音符の長さです。                             |         |
|`+` (`#`)                     |直前の音符を半音上げます。                                                      |         |
|`-`                           |直前の音符を半音下げます。                                                      |         |
|`.`                           |直前の音符や休符に符点をつけます。長さが1.5倍になります。                       |         |
|`&`                           |前後の音をつなげます。                                                          |         |
|`{ ... }x`                    |指定された長さの`x`分音符を`{` `}`の中の音符の個数で等分した連符を演奏します。  |         |
|`@x` (FM音源のみ)             |音色番号`x`で指定された音色に切り替えます。                                     |`@0`     |
|`Yr,d` (OPNのみ)              |OPNのレジスタ`r`の内容を`d`にします。                                           |         |
|`Zd` (MIDIのみ)               |MIDIにデータ`d`を送ります。                                                     |         |
|`@M` (スーパーIC、FM音源のみ) |出力を左右両側に設定します。                                                    |         |
|`@L` (スーパーIC、FM音源のみ) |出力を左側に設定します。                                                        |         |
|`@R` (スーパーIC、FM音源のみ) |出力を右側に設定します。                                                        |         |
|`@Vx` (FM音源とMIDIのみ)      |音量を細かく調整します(0≦`x`≦127)。                                           |`@V106`  |
|`@Wx`                         |`x`で指定された長さだけ状態を維持します(1≦`x`≦64)。                           |`@W4`    |

- 音符`CDEFGAB`は「ドレミファソラシ」に対応します。
- 音色番号については、音色一覧をご覧ください。
- 【注意】 `CMD PLAY`文はCMD拡張命令であり、8801のみで使用できます。`CMD PLAY`を使用する前に[`NEW CMD`](#new_cmd)を実行する必要があります(無制限モードを除く)。255文字を越える長さの文字列を指定すると、`String too long`エラーが発生します(無制限モードを除く)。

## 音色一覧

|  音色番号  |音色名             |解説                                               |
|-----------:|:------------------|:--------------------------------------------------|
|`0`         |  `Default VOICE`  |  既定の音色                                       |
|`1`         |  `BRASS 2`        |  ブラス系の音                                     |
|`2`         |  `STRING 2`       |  ストリング系の音                                 |
|`3`         |  `EPIANO 3`       |  電子ピアノ系の音                                 |
|`4`         |  `EBASS 1`        |  電子ベース系の音                                 |
|`5`         |  `EORGAN 1`       |  電子オルガン系の音                               |
|`6`         |  `PORGAN 1`       |  パイプオルガン系の音                             |
|`7`         |  `FLUTE`          |  フルート                                         |
|`8`         |  `OBOE`           |  オーボエ                                         |
|`9`         |  `CLARINET`       |  クラリネット                                     |
|`10`        |  `VIBRPHN`        |  ビブラフォン                                     |
|`11`        |  `HARPSIC`        |  ハープシコード                                   |
|`12`        |  `BELL`           |  ベル                                             |
|`13`        |  `PIANO`          |  ピアノ                                           |
|`14`        |  `MUSHI`          |  虫の鳴き声                                       |
|`15`        |  `DESCENT`        |  高空から降下する音                               |
|`16`        |  `UFO`            |  UFOが遠ざかる音                                  |
|`17`        |  `GRANPRI`        |  レーシングカーのエンジン音                       |
|`18`        |  `LASER 1`        |  レーザーガンの音                                 |
|`19`        |  `LASER 2`        |  レーザーガンの音                                 |
|`20`        |  `SIN WAVE`       |  チューニング用の正弦波                           |
|`21`        |  `BRASS 1`        |  ブラス系の音                                     |
|`22`        |  `BRASS 2`        |  ブラス系の音、音色1と同じ                        |
|`23`        |  `TRUMPET`        |  トランペット                                     |
|`24`        |  `STRING 1`       |  ストリング系の音                                 |
|`25`        |  `STRING 2`       |  ストリング系の音、音色2と同じ                    |
|`26`        |  `EPIANO 1`       |  電子ピアノ系の音                                 |
|`27`        |  `EPIANO 2`       |  電子ピアノ系の音                                 |
|`28`        |  `EPIANO 3`       |  電子ピアノ系の音、音色3と同じ                    |
|`29`        |  `GUITAR`         |  ギター                                           |
|`30`        |  `EBASS 1`        |  電子ベース系の音、音色4と同じ                    |
|`31`        |  `EBASS 2`        |  電子ベース系の音                                 |
|`32`        |  `EORGAN 1`       |  電子オルガン系の音、音色5と同じ                  |
|`33`        |  `EORGAN 2`       |  電子オルガン系の音                               |
|`34`        |  `PORGAN 1`       |  パイプオルガン系の音、音色6と同じ                |
|`35`        |  `PORGAN 2`       |  パイプオルガン系の音                             |
|`36`        |  `FLUTE`          |  フルート、音色7と同じ                            |
|`37`        |  `PICCOLO`        |  ピッコロ                                         |
|`38`        |  `OBOE`           |  オーボエ、音色8と同じ                            |
|`39`        |  `CLARINET`       |  クラリネット、音色9と同じ                        |
|`40`        |  `GROCKEN`        |  グロッケン                                       |
|`41`        |  `VIBRPHN`        |  ビブラフォン、音色10と同じ                       |
|`42`        |  `XYLOPHN`        |  シロフォン                                       |
|`43`        |  `KOTO`           |  琴                                               |
|`44`        |  `ZITAR`          |  ツィター                                         |
|`45`        |  `CLAV`           |  クラビネット                                     |
|`46`        |  `HARPSIC`        |  ハープシコード、音色0、11と同じ                  |
|`47`        |  `BELL`           |  ベル、音色12と同じ                               |
|`48`        |  `HARP`           |  ハープ                                           |
|`49`        |  `BELL/BRASS`     |  スタッカートでベル、ロングトーンでブラスの音     |
|`50`        |  `HARMONICA`      |  ハーモニカ                                       |
|`51`        |  `STEEL DRUM`     |  スチールドラム                                   |
|`52`        |  `TIMPANI`        |  ティンパニー                                     |
|`53`        |  `TRAIN`          |  列車の警笛                                       |
|`54`        |  `AMBULAN`        |  救急車                                           |
|`55`        |  `TWEET`          |  小鳥のさえずり                                   |
|`56`        |  `RAIN DROP`      |  雨の落ちる音                                     |
|`57`        |  `HORN`           |  ホルン                                           |
|`58`        |  `SNARE DRUM`     |  スネアドラム                                     |
|`59`        |  `COW BELL`       |  カウベル                                         |
|`60`        |  `PERC 1`         |  打楽器系の音                                     |
|`61`        |  `PERC 2`         |  打楽器系の音                                     |

## 使用許諾

- 本ソフトウェアの使用においては、[LICENSE.txt](LICENSE.txt)に記載されている「fmgon ライセンス」の使用条件に従ってください。
- 本ソフトウェアのソースコードは https://github.com/katahiromz/cmd_play からダウンロードできます。
- 本ソフトウェアの一部は、cisc さん (cisc@retropc.net) が著作権を所有しています。

## 連絡先

- 片山博文MZ <katayama.hirofumi.mz@gmail.com>

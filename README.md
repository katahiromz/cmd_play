﻿(Japanese, UTF-8)

# cmd_play by 片山博文MZ

## 概要

古いパソコン PC-8801 の N88-BASIC という言語の `CMD PLAY` 文を現代の Windows パソコンでエミュレート（再現）するプログラムです。

## 使い方

```txt
使い方: cmd_play [オプション] [#n] [文字列1] [文字列2] [文字列3] [文字列4] [文字列5] [文字列6]

オプション:
  -D変数名=値                変数に代入。
  -save-wav 出力.wav         WAVファイルとして保存（MIDI音源を除く）。
  -save-mid 出力.mid         MIDファイルとして保存（MIDI音源のみ）。
  -stopm                     音楽を止めて設定をリセット。
  -stereo                    音をステレオにする（デフォルト）。
  -mono                      音をモノラルにする。
  -voice CH FILE.voi         ファイルからチャンネルCHに音色を読み込む（FM音源）。
  -voice CH "CSV"            配列からチャンネルCHに音色を読み込む（FM音源）。
  -voice-copy TONE FILE.voi  音色をファイルにコピーする（FM音源）。
  -bgm 0                     演奏が終わるまで待つ。
  -bgm 1                     演奏が終わるまで待たない（デフォルト）。
  -help                      このメッセージを表示する。
  -version                   バージョン情報を表示する。

数値変数は、「L=数値変数名;」のように等号とセミコロンではさめば指定できます。
文字列変数は「[A$]」のように [ ] で囲えば展開できます。
```

## `CMD PLAY`文 (8801のみ) (CMD拡張)

- 【機能】 音楽を演奏します。
- 【語源】 Command play
- 【書式】 `CMD PLAY [#`*音源モード*`,] [` *文字列1* `][,` *文字列2* `][,` *文字列3* `][,` *文字列4* `][,` *文字列5* `][,` *文字列6* `]`
- 【文例】 `CMD PLAY "CDE"`　⇒　ドレミを演奏
- 【説明】 6和音まで演奏できます。*文字列1*、*文字列2*、*文字列3*はチャンネル1、2、3に、*文字列4*、*文字列5*、*文字列6*はチャンネル4、5、6に対応します。*音源モード*は`0`～`5`の値で、次のような意味があります。

- `#0` … SSG音源を使います。チャンネル1～6がSSG音源を使用します。
- `#1` … MIDI音源を使います。チャンネル1～6がMIDI音源を使用します。
- `#2` … チャンネル1～3がFM音源を、チャンネル4～6がSSG音源を使用します。
- `#3` … OPNを効果音モードに切り替えます。`CMD PLAY`の`Y`文によって音を出します。
- `#4` … OPNをCSM(サイン波)モードに切り替えます。`CMD PLAY`の`Y`文によって音を出します。
- `#5` … FM音源を使います。チャンネル1～6がFM音源を使用します。

*音源モード*を省略した場合は、`#2`が選択されます。`#5`は、スーパーICでなければ選択できません。
通常、*音源モード*は、`#0`、`#1`、`#2`、または`#5`でお使いください。
リズム音源はまだ未対応です。

各チャンネルの文字列は、MML (Music Macro Language)といい、次のような意味を持ちます。

|文字列                         |意味                                                                            |初期値   |
|:------------------------------|:-------------------------------------------------------------------------------|:--------|
|`Mx` (SSGのみ)                 |エンベロープ周期を設定します(1≦`x`≦65535)。                                   |`M255`   |
|`Sx` (SSGのみ)                 |エンベロープ形状を設定します(0≦`x`≦15)。                                      |`S1`     |
|`Vx`                           |音量を設定します(0≦`x`≦15)。                                                  |`V8`     |
|`Lx`                           |音符や休符の既定値の長さを設定します(1≦`x`≦64)。                              |`L4`     |
|`Qx`                           |音の長さの割合を設定します(1≦`x`≦8)。                                         |`Q8`     |
|`Ox`                           |オクターブを設定します(1≦`x`≦8)。                                             |`O4`     |
|`>`                            |オクターブを１つ上げます。                                                      |         |
|`<`                            |オクターブを１つ下げます。                                                      |         |
|`Nx`                           |`x`で指定された高さの音を発生します(0≦`x`≦96)。                               |         |
|`Tx`                           |テンポを設定します(32≦`x`≦255)。                                              |`T120`   |
|`Rx`                           |休符を演奏します(1≦`x`≦64)。`x`は休符の長さです。                             |`R4`     |
|`C`～`B[+/-][x][.]`            |音符を演奏します(1≦`x`≦64)。`x`は音符の長さです。                             |         |
|`+` (`#`)                      |直前の音符を半音上げます。                                                      |         |
|`-`                            |直前の音符を半音下げます。                                                      |         |
|`.`                            |直前の音符や休符に符点をつけます。長さが1.5倍になります。                       |         |
|`&`                            |前後の音をつなげます。                                                          |         |
|`{ ... }x`                     |指定された長さの`x`分音符を`{` `}`の中の音符の個数で等分した連符を演奏します。  |         |
|`@x` (FM/MIDIのみ)             |音色番号`x`で指定された音色に切り替えます。                                     |`@0`     |
|`Yr,d` (OPNのみ)               |OPNのレジスタ`r`の内容を`d`にします(0≦`r`≦178 かつ 0≦`d`≦255)。             |         |
|`Zd` (MIDIのみ)                |MIDIに1バイトのデータ`d`を送ります(0≦`d`≦255)。                               |         |
|`@M` (スーパーIC、FM/MIDIのみ) |出力を左右両チャンネルに設定します。                                            |         |
|`@L` (スーパーIC、FM/MIDIのみ) |出力を左チャンネルに設定します。                                                |         |
|`@R` (スーパーIC、FM/MIDIのみ) |出力を右チャンネルに設定します。                                                |         |
|`@Vx` (SSGを除く)              |音量を細かく調整します(0≦`x`≦127)。                                           |`@V106`  |
|`@Wx`                          |`x`で指定された長さだけ状態を維持します(1≦`x`≦64)。                           |`@W4`    |

- 音符`CDEFGAB`は「ドレミファソラシ」に対応します。
- 音色番号については、音色一覧をご覧ください。
- 実機のMIDIでは `@x`は指定できません。代わりに `Zd`を使ってプログラムチェンジしてください。
- MIDIについては、[WikipediaのMIDIのページ](https://ja.wikipedia.org/wiki/MIDI)をご覧ください。
- 【注意】 `CMD PLAY`文はCMD拡張命令であり、8801のみで使用できます。`CMD PLAY`を使用する前に[`NEW CMD`](#new_cmd)を実行する必要があります(無制限モードを除く)。255文字を越える長さの文字列を指定すると、`String too long`エラーが発生します(無制限モードを除く)。

## 使用許諾

- 本ソフトウェアの使用においては、[LICENSE.txt](LICENSE.txt)に記載されている「fmgon ライセンス」の使用条件に従ってください。
- 本ソフトウェアのソースコードは https://github.com/katahiromz/cmd_play からダウンロードできます。
- 本ソフトウェアの一部は、cisc さん (cisc@retropc.net) が著作権を所有しています。

## FM音源の音色一覧

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

## MIDI音源の音色一覧

|  音色番号  |音色名             |解説                                               |
|-----------:|:------------------|:--------------------------------------------------|
|`000`       | `Piano 1`         | グランドピアノ                                    |
|`001`       | `Piano 2`         | ブライトピアノ                                    |
|`002`       | `Piano 3`         | エレクトリックグランドピアノ                      |
|`003`       | `Piano 4`         | ホンキートンク                                    |
|`004`       | `E.Piano 1`       | エレクトリックピアノ 1                            |
|`005`       | `E.Piano 2`       | エレクトリックピアノ 2                            |
|`006`       | `Harpsichord`     | ハープシコード                                    |
|`007`       | `Calvi`           | クラビ                                            |
|`008`       | `Celesta`         | チェレスタ                                        |
|`009`       | `Glockenspl`      | グロッケンシュピール                              |
|`010`       | `MusicBox`        | ミュージックボックス                              |
|`011`       | `Vibraphone`      | ビブラフォン                                      |
|`012`       | `Marinba`         | マリンバ                                          |
|`013`       | `Xylophone`       | シロフォン                                        |
|`014`       | `Tubularbell`     | チューブラーベル                                  |
|`015`       | `Santur`          | サントゥール                                      |
|`016`       | `Organ 1`         | ドローバーオルガン                                |
|`017`       | `Organ 2`         | パーカッシブオルガン                              |
|`018`       | `Organ 3`         | ロックオルガン                                    |
|`019`       | `Church Org1`     | チャーチオルガン                                  |
|`020`       | `Reed Organ`      | リードオルガン                                    |
|`021`       | `Accordion`       | アコーディオン                                    |
|`022`       | `Harmonica`       | ハーモニカ                                        |
|`023`       | `Bandoneon`       | バンドネオン                                      |
|`024`       | `Nylon Gt.`       | ナイロンストリングスギター                        |
|`025`       | `Steel Gt.`       | スチールストリングスギター                        |
|`026`       | `Jazz Gt.`        | ジャズギター                                      |
|`027`       | `Clean Gt.`       | クリーンギター                                    |
|`028`       | `Muted Gt.`       | ミュートギター                                    |
|`029`       | `OverdriveGt`     | オーバードライブギター                            |
|`030`       | `Dist.Gt.`        | ディストーションギター                            |
|`031`       | `Gt.Harmonix`     | ギターハーモニクス                                |
|`032`       | `Acoustic Bs`     | アコースティックベース                            |
|`033`       | `Fingered Bs`     | エレクトリックベース 1                            |
|`034`       | `Picked Bass`     | エレクトリックベース 2                            |
|`035`       | `Fretless Bs`     | フレットレスベース                                |
|`036`       | `Slap Bass 1`     | フラップベース 1                                  |
|`037`       | `Slap Bass 2`     | フラップベース 2                                  |
|`038`       | `Syn.Bass 1`      | シンセベース 1                                    |
|`039`       | `Syn.Bass 2`      | シンセベース 2                                    |
|`040`       | `Violin`          | バイオリン                                        |
|`041`       | `Viola`           | ビオラ                                            |
|`042`       | `Cello`           | チェロ                                            |
|`043`       | `Contrabass`      | コントラバス                                      |
|`044`       | `Tremolo Str`     | トレモロストリングス                              |
|`045`       | `Pizzicato`       | ピチカート                                        |
|`046`       | `Harp`            | ハープ                                            |
|`047`       | `Timpani`         | ティンパニ                                        |
|`048`       | `Strings`         | ストリングス 1                                    |
|`049`       | `SlowStrings`     | ストリングス 2                                    |
|`050`       | `SynStrings1`     | シンセストリングス 1                              |
|`051`       | `SynStrings2`     | シンセストリングス 2                              |
|`052`       | `Choir Aahs`      | クワイア                                          |
|`053`       | `Voice Oohs`      | ボイス                                            |
|`054`       | `SynVox`          | シンセボイス                                      |
|`055`       | `Orchest.Hit`     | オーケストラヒット                                |
|`056`       | `Trumpet`         | トランペット                                      |
|`057`       | `Trombone`        | トロンボーン                                      |
|`058`       | `Tuba`            | チューバ                                          |
|`059`       | `MuteTrumpet`     | ミュートトランペット                              |
|`060`       | `French Horn`     | フレンチホルン                                    |
|`061`       | `Brass 1`         | ブラス                                            |
|`062`       | `Syn.Brass 1`     | シンセブラス 1                                    |
|`063`       | `Syn.Brass 2`     | シンセブラス 2                                    |
|`064`       | `Soprano Sax`     | ソプラノサックス                                  |
|`065`       | `Alto Sax`        | アルトサックス                                    |
|`066`       | `Tenor Sax`       | テナーサックス                                    |
|`067`       | `Baritone Sax`    | バリトンサックス                                  |
|`068`       | `Oboe`            | オーボエ                                          |
|`069`       | `EnglishHorn`     | イングリッシュホルン                              |
|`070`       | `Bassoon`         | バスーン                                          |
|`071`       | `Clarinet`        | クラリネット                                      |
|`072`       | `Piccolo`         | ピッコロ                                          |
|`073`       | `Flute`           | フルート                                          |
|`074`       | `Recorder`        | リコーダー                                        |
|`075`       | `Pan Flute`       | パンフルート                                      |
|`076`       | `Bottle Blow`     | ボトルブロー                                      |
|`077`       | `Shakuhachi`      | 尺八                                              |
|`078`       | `Whistle`         | ホイッスル                                        |
|`079`       | `Ocarina`         | オカリナ                                          |
|`080`       | `Square Wave`     | スクエアウェーブ                                  |
|`081`       | `Saw Wave`        | ソートゥースウェーブ                              |
|`082`       | `SynCalliope`     | カリオペ                                          |
|`083`       | `ChifferLead`     | チフリード                                        |
|`084`       | `Charang`         | チャラン                                          |
|`085`       | `Voice Lead`      | ボイスリード                                      |
|`086`       | `5th Saw`         | フィフスリード                                    |
|`087`       | `Bass & Lead`     | ベース＋リード                                    |
|`088`       | `Fantasia`        | ニューエイジ                                      |
|`089`       | `Warm Pad`        | ウォームパッド                                    |
|`090`       | `Polysynth`       | ポリシンセ                                        |
|`091`       | `Space Voice`     | スペースクワイア                                  |
|`092`       | `Bowed Glass`     | ボウグラス                                        |
|`093`       | `Metal Pad`       | メタリックパッド                                  |
|`094`       | `Halo Pad`        | ヘイロパッド                                      |
|`095`       | `Sweep Pad`       | スイープパッド                                    |
|`096`       | `Ice Rain`        | レインドロップ                                    |
|`097`       | `Soundtrack`      | サウンドトラック                                  |
|`098`       | `Crystal`         | クリスタル                                        |
|`099`       | `Atmosphere`      | アトモスフィア                                    |
|`100`       | `Brightness`      | ブライトネス                                      |
|`101`       | `Goblin`          | ゴブリン                                          |
|`102`       | `Echo Drops`      | エコー                                            |
|`103`       | `Sci-Fi`          | サイエンスフィクション                            |
|`104`       | `Sitar`           | シタール                                          |
|`105`       | `Banjo`           | バンジョー                                        |
|`106`       | `Shamisen`        | 三味線                                            |
|`107`       | `Koto`            | 琴                                                |
|`108`       | `Kalimba`         | カリンバ                                          |
|`109`       | `Bagpipe`         | バグパイプ                                        |
|`110`       | `Fiddle`          | フィドル                                          |
|`111`       | `Shanai`          | シャナイ                                          |
|`112`       | `Tinkle Bell`     | ティンクルベル                                    |
|`113`       | `Agogo`           | アゴゴ                                            |
|`114`       | `Steel Drum`      | スチームドラム                                    |
|`115`       | `WoodBlock`       | ウッドブロック                                    |
|`116`       | `Taiko`           | 太鼓                                              |
|`117`       | `Melodic Tom`     | メロディックタム                                  |
|`118`       | `Synth Drum`      | シンセドラム                                      |
|`119`       | `Reverse Cym`     | リバースシンバル                                  |
|`120`       | `Gt.FretNoiz`     | ギターフレットノイズ                              |
|`121`       | `BreathNoise`     | ブレスノイズ                                      |
|`122`       | `Seashore`        | シーショア―                                      |
|`123`       | `Bird`            | バード                                            |
|`124`       | `Telephone 1`     | テレホン                                          |
|`125`       | `Helicopter`      | ヘリコプター                                      |
|`126`       | `Applause`        | アプローズ                                        |
|`127`       | `Gun shot`        | ガンショット                                      |

## 連絡先

- 片山博文MZ <katayama.hirofumi.mz@gmail.com>

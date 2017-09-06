NB. media/wav - Windows WAV file creation and play
NB.
NB. 12/03/2006 Oleg Kobchenko

require 'dll files'

coclass 'pwav'

NB. =========================================================
NB. playing wave files
PlaySound=: 'winmm PlaySoundA > i *c i i'&cd

'SND_SYNC SND_ASYNC SND_MEMORY SND_LOOP SND_PURGE SND_ALIAS'=: 0 1 4 8 64 65536

NB.*wavplay v plays wav from file or memory
NB.   y  wav file or char vector of wav format
NB.  [x] SND_* flags, SND_SYNC default, see wav.ijs for details
wavplay=: SND_SYNC&$: : (4 : 0)
  PlaySound y;0;x
)

NB. =========================================================
NB. making wave files
BIGEND=. ({.a.)={.1(3!:4)1
'`i2 i4'=: (0 1+2*BIGEND){ (1&ic)`(2&ic)`([: , _2 |.\ 1&ic)`([: , _4 |.\ 2&ic)
RATE=: 11000

NB.*wavmake v wav format from samples
NB.   y  samples vector or (2,N)=$y matrix for stereo
NB.       range is 0..255 or _32768..32767
NB.  [x] sample rate in Hz, RATE default of 11000
wavmake=: 3 : 0
  RATE wavmake y               NB. x: sample rate, eg 11 kHz
:
  bs=. 8 16{~255<>./,y         NB. bits per sample
  ba=. (bs*#$y) <.@% 8         NB. block align
  br=. x * ba                  NB. byte rate (bytes/sec)
  d=. 'data',(,~ i4@#) i2`({&a.)@.(bs=8) ,|:y
  f=. 'fmt ',(i4 16),(i2 1),(i2 #$y),(i4 x),(i4 br),(i2 ba),i2 bs
  'RIFF',(,~ i4@#) 'WAVE',f,d
)

NB. =========================================================
NB. making notes
freq=: 440*2^12%~-&9
note=: 0.25&$: :(4 : '<.255*(%>./)(-<./) 1 o.y*x*2p1*(i.%<:)1+RATE*x')"0

NB.*wavmakenote v make wav format from musical notes
wavmakenote=: [: wavmake ;@(<@note freq)

NB.*wavnote v plays musical notes
NB.   y  tones 0=C, 1=C#, 2=D of main octave
NB.  [x] durations in sec, 0.25 default of 1/4 sec
wavnote=: 4 wavplay wavmakenote


NB. =========================================================
NB. reading PCM wave files

ic=: 3!:4             NB. integer conversion
bigEnd=: 2 : 'u`v@.(1=_1 ic 0 1{a.)'
int=: _2&ic  bigEnd  ( _2 ic [: , _4 |.\ ] )
i24=: (_2 ic [: , (0{a.) ,.~ _3 ]\ ])  bigEnd  ( _2 ic [: , (0{a.) ,. _3 |.\ ] )
sht=: _1&ic  bigEnd  ( _1 ic [: , _4 |.\ ] )
byt=: a.&i.
substr=: ,."1@[ ];.0 ]

oibeg=: 3 : 'OFFS=: 0'  NB. offset iterator
oimov=: 3 : 'y,~y-~OFFS=: OFFS+y'

wavhead=: 3 : 0
  oibeg''
         chunk         =:        y substr~ oimov 4
  assert chunk         -: 'RIFF'
         chunkSize     =: {. int y substr~ oimov 4
         format        =:        y substr~ oimov 4
  assert format        -: 'WAVE'

         subChunk1     =:        y substr~ oimov 4
         chunkSize1    =: {. int y substr~ oimov 4
         format1       =: {. sht y substr~ oimov 2
  assert format1       -: 1

         chanels       =: {. sht y substr~ oimov 2
         sampleRate    =: {. int y substr~ oimov 4
         byteRate      =: {. int y substr~ oimov 4
         blockAlign    =: {. sht y substr~ oimov 2
         bitsPerSample =: {. sht y substr~ oimov 2

         aux=: ''
  while. OFFS<#y do.
         subChunk2     =:        y substr~ oimov 4
         chunkSize2    =: {. int y substr~ oimov 4
    if. 'data'-:subChunk2 do. break. end.
         aux=: aux,<             y substr~ oimov chunkSize2
  end.
         sampleSize    =: (bitsPerSample <.@% 8) { 1 1 2 3 4
         sampleCount   =: chunkSize2 <.@% sampleSize
)

NB.*wavdata v WAVE data from noun
NB.   y  wave format
NB.  [x] samples: (index,count)[, .. ,:index,count]
wavdata=: 3 : 0
  '' wavdata y
:
  wavhead y
  n=. oimov chunkSize2
  if. #x do. n=. (x*sampleSize) +"1 ({.n),0 end.
  f=. ]`byt`sht`i24`int@.sampleSize
  data=: f , n substr y
)

NB.*wavinfo v WAVE info from noun
NB.   y  wave format
wavinfo=: 3 : 0
  wavhead y
  r=.      'File Type       ',chunk
  r=. r,LF,'Total Size      ',":8+chunkSize
  r=. r,LF,'File Format     ',format
  r=. r,LF,'Channels        ',":chanels
  r=. r,LF,'Sample Rate     ',":sampleRate
  r=. r,LF,'Sample Count    ',":sampleCount
  r=. r,LF,'Bits per Sample ',":bitsPerSample
)

NB.*wavfile v WAVE info or data from file
NB.   y  wav file
NB.  [x] what[;samples]
NB.      what:    0-info 1-head 2-data
NB.      samples: see wavdata
wavfile=: 3 : 0
  0 wavfile y
:
  require 'jmf'
  r=. ''
  JCHAR map_jmf_ 'WAVE_pwav_';y;'';1  NB. readonly
  'fn nx'=. 2{.boxopen x
  try.
    select. fn
    case. 1 do. r=.    wavhead WAVE_pwav_
    case. 2 do. r=. nx wavdata WAVE_pwav_
    case.   do. r=.    wavinfo WAVE_pwav_
    end.
  catchd. 0 end.
  unmap_jmf_ 'WAVE_pwav_'
  r
)

NB. =========================================================
wavplay_z_=: wavplay_pwav_
wavmake_z_=: wavmake_pwav_
wavmakenote_z_=: wavmakenote_pwav_
wavnote_z_=: wavnote_pwav_
wavhead_z_=: wavhead_pwav_
wavdata_z_=: wavdata_pwav_
wavinfo_z_=: wavinfo_pwav_
wavfile_z_=: wavfile_pwav_


NB. =========================================================
Note 'Test playing and making waves'
scriptdoc 'media/wav'

wavplay jpath'~addons/media/wav/test1.wav'
1 wavplay jpath'~addons/media/wav/test1.wav'      NB. async
(1+8) wavplay jpath'~addons/media/wav/test1.wav'  NB. async loop
64 wavplay <0                                     NB. stop

load 'files'
(wavmake i.100) fwrite jpath'~addons/media/wav/test2.wav'
(wavmake 1000?@$200) fwrite jpath'~addons/media/wav/test3.wav'
(wavmake ,~^:6 i.200) fwrite jpath'~addons/media/wav/test4.wav'

4 wavplay 2000 wavmake 1000?@$200              NB. memory
4 wavplay wavmake <.255*(%>./)(-<./) 1 o.440*    2p1*(i.%<:) RATE_pwav_
4 wavplay wavmake <.255*(%>./)(-<./) 1 o.440*1 o.2p1*(i.%<:) RATE_pwav_

65536 wavplay&>({.,&.>}.);:'System Asterisk Exclamation Exit Hand Question'

0.25 0.25 0.5 1 wavnote 0 2 4 5
0.25 wavnote 0 2 4 5
joy=.     11 11 12 14  14 12 11 9  7 7 9 11  11 9 9 9
joy=. joy,11 11 12 14  14 12 11 9  7 7 9 11  9  7 7 7
0.35 wavnote joy
(0.35 wavmakenote joy) fwrite jpath'~addons/media/wav/joy.wav'

<.0.5+freq_pwav_+i.12  NB. main octave
load 'plot'
plot 1 o.1 2 4*/2p1*(i.%<:)101
)

Note 'Test reading waves'
  load 'plot'
  W=. fread jpath '~addons/media/wav/test1.wav'
  wavinfo W
  plot wavdata W

  W=. fread jpath '~addons/media/wav/test2.wav'
  wavinfo W
  plot wavdata W

  wavfile jpath '~addons/media/wav/test1.wav'
  wavfile jpath '~addons/media/wav/test2.wav'
  plot 2 wavfile jpath '~addons/media/wav/test1.wav'
)

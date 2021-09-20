:: test bmp unpack/pack
..\libex unpack -i fon_s.lib -o out_fon_s
..\libex pack -i out_fon_s -o fon_s_copy.lib -t bmp_v1 -v 1

:: test wav unpack/pack
..\libex unpack -i wav_ef.lib -o out_wav_ef
..\libex pack -i out_wav_ef -o wav_ef_copy.lib -t wav_v1 -v 1

:: test anm unpack/pack
..\libex unpack -i anm.lib -o out_anm
..\libex pack -i out_anm -o anm_copy.lib -t anm_v2 -v 1

:: test animation conversion
..\anm2anm -i anim_v1.anm -o anim_v2_converted.anm

:: check
fc /b fon_s.lib fon_s_copy.lib
fc /b wav_ef.lib wav_ef_copy.lib
fc /b anm.lib anm_copy.lib
fc /b anim_v2.anm anim_v2_converted.anm
pause

::clear
rd /s /q out_fon_s\
rd /s /q out_wav_ef\
rd /s /q out_anm\
del fon_s_copy.lib
del wav_ef_copy.lib
del anm_copy.lib
del anim_v2_converted.anm
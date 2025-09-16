echo "Unpack roms"
python ./unpack.py 1942.zip
python ./unpack.py digdug.zip
python ./unpack.py dkong.zip
python ./unpack.py eyes.zip
python ./unpack.py frogger.zip
python ./unpack.py galaga.zip
python ./unpack.py lizwiz.zip
python ./unpack.py mrtnt.zip
python ./unpack.py pacman.zip
python ./unpack.py theglobp.zip

echo "Z80"
python ./z80patch.py

REM converted logos are included
REM echo "Logos"
REM python ./logoconv.py ../logos/1942.png ../source/src/machines/1942/1942_logo.h
REM python ./logoconv.py ../logos/digdug.png ../source/src/machines/digdug/digdug_logo.h
REM python ./logoconv.py ../logos/dkong.png ../source/src/machines/dkong/dkong_logo.h
REM python ./logoconv.py ../logos/eyes.png ../source/src/machines/eyes/eyes_logo.h
REM python ./logoconv.py ../logos/frogger.png ../source/src/machines/frogger/frogger_logo.h
REM python ./logoconv.py ../logos/galaga.png ../source/src/machines/galaga/galaga_logo.h
REM python ./logoconv.py ../logos/lizwiz.png ../source/src/machines/lizwiz/lizwiz_logo.h
REM python ./logoconv.py ../logos/mrtnt.png ../source/src/machines/mrtnt/mrtnt_logo.h
REM python ./logoconv.py ../logos/pacman.png ../source/src/machines/pacman/pacman_logo.h
REM python ./logoconv.py ../logos/theglob.png ../source/src/machines/theglob/theglob_logo.h

echo "CPU code"
python ./romconv.py _1942_rom_cpu1 ./roms/srb-03.m3 ./roms/srb-04.m4 ../source/src/machines/1942/1942_rom1.h
python ./romconv.py _1942_rom_cpu1_b0 ./roms/srb-05.m5 ../source/src/machines/1942/1942_rom1_b0.h
python ./romconv.py _1942_rom_cpu1_b1 ./roms/srb-06.m6 ../source/src/machines/1942/1942_rom1_b1.h
python ./romconv.py _1942_rom_cpu1_b2 ./roms/srb-07.m7 ../source/src/machines/1942/1942_rom1_b2.h
python ./romconv.py _1942_rom_cpu2 ./roms/sr-01.c11 ../source/src/machines/1942/1942_rom2.h
python ./romconv.py digdug_rom_cpu1 ./roms/dd1a.1 ./roms/dd1a.2 ./roms/dd1a.3 ./roms/dd1a.4 ../source/src/machines/digdug/digdug_rom1.h
python ./romconv.py digdug_rom_cpu2 ./roms/dd1a.5 ./roms/dd1a.6 ../source/src/machines/digdug/digdug_rom2.h
python ./romconv.py digdug_rom_cpu3 ./roms/dd1.7 ../source/src/machines/digdug/digdug_rom3.h
python ./romconv.py digdug_playfield ./roms/dd1.10b ../source/src/machines/digdug/digdug_playfield.h
python ./romconv.py dkong_rom_cpu1 ./roms/c_5et_g.bin ./roms/c_5ct_g.bin ./roms/c_5bt_g.bin ./roms/c_5at_g.bin ../source/src/machines/dkong/dkong_rom1.h
python ./romconv.py dkong_rom_cpu2 ./roms/s_3i_b.bin ./roms/s_3j_b.bin ../source/src/machines/dkong/dkong_rom2.h
python ./romconv.py -d eyes_rom ./roms/d7 ./roms/e7 ./roms/f7 ./roms/h7 ../source/src/machines/eyes/eyes_rom.h
python ./romconv.py frogger_rom_cpu1 ./roms/frogger.26 ./roms/frogger.27 ./roms/frsm3.7 ../source/src/machines/frogger/frogger_rom1.h
python ./romconv.py frogger_rom_cpu2 ./roms/frogger.608 ./roms/frogger.609 ./roms/frogger.610 ../source/src/machines/frogger/frogger_rom2.h
python ./romconv.py -p galaga_rom_cpu1 ./roms/gg1_1b.3p ./roms/gg1_2b.3m ./roms/gg1_3.2m ./roms/gg1_4b.2l ../source/src/machines/galaga/galaga_rom1.h
python ./romconv.py galaga_rom_cpu2 ./roms/gg1_5b.3f ../source/src/machines/galaga/galaga_rom2.h
python ./romconv.py galaga_rom_cpu3 ./roms/gg1_7b.2c ../source/src/machines/galaga/galaga_rom3.h
python ./romconv.py lizwiz_rom ./roms/6e.cpu ./roms/6f.cpu ./roms/6h.cpu ./roms/6j.cpu ./roms/wiza ./roms/wizb ../source/src/machines/lizwiz/lizwiz_rom.h
python ./romconv.py -d mrtnt_rom ./roms/tnt.1 ./roms/tnt.2 ./roms/tnt.3 ./roms/tnt.4 ../source/src/machines/mrtnt/mrtnt_rom.h
python ./romconv.py pacman_rom ./roms/pacman.6e ./roms/pacman.6f ./roms/pacman.6h ./roms/pacman.6j ../source/src/machines/pacman/pacman_rom.h
python ./romconv.py theglob_rom ./roms/glob.u2 ./roms/glob.u3 ../source/src/machines/theglob/theglob_rom.h

echo "Tiles"
python ./tileconv.py _1942_charmap ./roms/sr-02.f2 ../source/src/machines/1942/1942_charmap.h
python ./tileconv.py _1942_tilemap ./roms/sr-08.a1 ./roms/sr-09.a2 ./roms/sr-10.a3 ./roms/sr-11.a4 ./roms/sr-12.a5 ./roms/sr-13.a6 ../source/src/machines/1942/1942_tilemap.h
python ./tileconv.py digdug_tilemap ./roms/dd1.9 ../source/src/machines/digdug/digdug_tilemap.h
python ./tileconv.py digdug_pftiles ./roms/dd1.11 ../source/src/machines/digdug/digdug_pftiles.h
python ./tileconv.py dkong_tilemap ./roms/v_5h_b.bin ./roms/v_3pt.bin ../source/src/machines/dkong/dkong_tilemap.h
python ./tileconv.py eyes_tilemap ./roms/d5 ../source/src/machines/eyes/eyes_tilemap.h
python ./tileconv.py frogger_tilemap ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_tilemap.h
python ./tileconv.py galaga_tilemap ./roms/gg1_9.4l ../source/src/machines/galaga/galaga_tilemap.h
python ./tileconv.py lizwiz_tilemap ./roms/5e.cpu ../source/src/machines/lizwiz/lizwiz_tilemap.h
python ./tileconv.py mrtnt_tilemap ./roms/tnt.5 ../source/src/machines/mrtnt/mrtnt_tilemap.h
python ./tileconv.py pacman_tilemap ./roms/pacman.5e ../source/src/machines/pacman/pacman_tilemap.h
python ./tileconv.py theglob_tilemap ./roms/glob.5e ../source/src/machines/theglob/theglob_tilemap.h

echo "Sprites"
python ./spriteconv.py _1942_sprites 1942 ./roms/sr-14.l1 ./roms/sr-15.l2 ./roms/sr-16.n1 ./roms/sr-17.n2 ../source/src/machines/1942/1942_spritemap.h
python ./spriteconv.py digdug_sprites digdug ./roms/dd1.15 ./roms/dd1.14 ./roms/dd1.13 ./roms/dd1.12 ../source/src/machines/digdug/digdug_spritemap.h
python ./spriteconv.py dkong_sprites dkong ./roms/l_4m_b.bin  ./roms/l_4n_b.bin  ./roms/l_4r_b.bin  ./roms/l_4s_b.bin ../source/src/machines/dkong/dkong_spritemap.h
python ./spriteconv.py eyes_sprites eyes ./roms/e5 ../source/src/machines/eyes/eyes_spritemap.h
python ./spriteconv.py frogger_sprites frogger ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_spritemap.h
python ./spriteconv.py galaga_sprites galaga ./roms/gg1_11.4d ./roms/gg1_10.4f ../source/src/machines/galaga/galaga_spritemap.h
python ./spriteconv.py lizwiz_sprites lizwiz ./roms/5f.cpu ../source/src/machines/lizwiz/lizwiz_spritemap.h
python ./spriteconv.py mrtnt_sprites mrtnt ./roms/tnt.6 ../source/src/machines/mrtnt/mrtnt_spritemap.h
python ./spriteconv.py pacman_sprites pacman ./roms/pacman.5f ../source/src/machines/pacman/pacman_spritemap.h
python ./spriteconv.py theglob_sprites pacman ./roms/glob.5f ../source/src/machines/theglob/theglob_spritemap.h

echo "Colormaps"
python ./cmapconv.py _1942_colormap_chars ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 128 ./roms/sb-0.f1 ../source/src/machines/1942/1942_character_cmap.h
python ./cmapconv.py _1942_colormap_tiles ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 -1 ./roms/sb-4.d6,./roms/sb-3.d2,./roms/sb-2.d1 ../source/src/machines/1942/1942_tile_cmap.h
python ./cmapconv.py _1942_colormap_sprites ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 64 ./roms/sb-8.k3 ../source/src/machines/1942/1942_sprite_cmap.h
python ./cmapconv.py digdug_colormap_tiles ./roms/136007.113 0 ./roms/136007.112 ../source/src/machines/digdug/digdug_cmap_tiles.h
python ./cmapconv.py digdug_colormap_sprites ./roms/136007.113 16 ./roms/136007.111 ../source/src/machines/digdug/digdug_cmap_sprites.h
python ./cmapconv.py digdug_colormaps ./roms/136007.113 ../source/src/machines/digdug/digdug_cmap.h
python ./cmapconv.py dkong_colormap ./roms/c-2k.bpr ./roms/c-2j.bpr 0 ./roms/v-5e.bpr ../source/src/machines/dkong/dkong_cmap.h
python ./cmapconv.py eyes_colormap ./roms/82s123.7f 0 ./roms/82s129.4a ../source/src/machines/eyes/eyes_cmap.h
python ./cmapconv.py frogger_colormap ./roms/pr-91.6l ../source/src/machines/frogger/frogger_cmap.h
python ./cmapconv.py galaga_colormap_sprites ./roms/prom-5.5n 0 ./roms/prom-3.1c ../source/src/machines/galaga/galaga_cmap_sprites.h
python ./cmapconv.py galaga_colormap_tiles ./roms/prom-5.5n 16 ./roms/prom-4.2n ../source/src/machines/galaga/galaga_cmap_tiles.h
python ./cmapconv.py lizwiz_colormap ./roms/7f.cpu 0 ./roms/4a.cpu ../source/src/machines/lizwiz/lizwiz_cmap.h
python ./cmapconv.py mrtnt_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/mrtnt/mrtnt_cmap.h
python ./cmapconv.py pacman_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/pacman/pacman_cmap.h
python ./cmapconv.py theglob_colormap ./roms/glob.7f 0 ./roms/glob.4a ../source/src/machines/theglob/theglob_cmap.h

echo "Audio"
python ./audioconv.py digdug_wavetable ./roms/136007.110 ../source/src/machines/digdug/digdug_wavetable.h
python ./audioconv.py eyes_wavetable ./roms/82s126.1m ../source/src/machines/eyes/eyes_wavetable.h
python ./audioconv.py galaga_wavetable ./roms/prom-1.1d ../source/src/machines/galaga/galaga_wavetable.h
python ./audioconv.py lizwiz_wavetable ./roms/82s126.1m ../source/src/machines/lizwiz/lizwiz_wavetable.h
python ./audioconv.py mrtnt_wavetable ./roms/82s126.1m ../source/src/machines/mrtnt/mrtnt_wavetable.h
python ./audioconv.py pacman_wavetable ./roms/82s126.1m ../source/src/machines/pacman/pacman_wavetable.h
python ./audioconv.py theglob_wavetable ./roms/82s126.1m ../source/src/machines/theglob/theglob_wavetable.h

Pause
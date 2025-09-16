echo "Unpack roms"
python ./unpack.py dkong.zip

echo "Logos"
python ./logoconv.py ../logos/dkong.png ../source/src/machines/dkong/dkong_logo.h

echo "CPU code"
python ./romconv.py dkong_rom_cpu1 ./roms/c_5et_g.bin ./roms/c_5ct_g.bin ./roms/c_5bt_g.bin ./roms/c_5at_g.bin ../source/src/machines/dkong/dkong_rom1.h
python ./romconv.py dkong_rom_cpu2 ./roms/s_3i_b.bin ./roms/s_3j_b.bin ../source/src/machines/dkong/dkong_rom2.h

echo "Tiles"
python ./tileconv.py dkong_tilemap ./roms/v_5h_b.bin ./roms/v_3pt.bin ../source/src/machines/dkong/dkong_tilemap.h

echo "Sprites"
python ./spriteconv.py dkong_sprites dkong ./roms/l_4m_b.bin  ./roms/l_4n_b.bin  ./roms/l_4r_b.bin  ./roms/l_4s_b.bin ../source/src/machines/dkong/dkong_spritemap.h

echo "Colormaps"
python ./cmapconv.py dkong_colormap ./roms/c-2k.bpr ./roms/c-2j.bpr 0 ./roms/v-5e.bpr ../source/src/machines/dkong/dkong_cmap.h

Pause
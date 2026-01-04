# romconv - ROM/bin file conversion

Arcade emulation involves dealing with the original ROM files of the
machines to be emulated. Sometimes the conversion is limited to the
transcription from binary to an equivalent C source file. But in many
cases the conversion includes further data processing. E.g. all color
tables are converted into the 16 bit color format used by the ILI9341
or ST7789 displays. Sprite and tile data is converted into a format
easier to process on the ESP32.

The conversion could be done on the ESP32 target at run time. But in
Galagino it's done beforehand. This offloads these tasks from the
ESP32.

It's possible to implement only one or two of all arcade
machines. In that case the related ROM conversion can be omitted and
the machine in question has to be disabled in the file
[config.h](../source/src/config.h).

The necessary ROM files need be placed in the [romszip
directory](../romszip) before these scripts can be run.

The logo conversion for the game selection menu might require the
seperate installation of the ```imageio python module``` which can
e.g. be done by the following command. This is usually not needed as
the logos are included pre-converted. This is only needed if you intend
the change the logos.

```pip3 install imageio```

## Do-it-all script

A [batch](conv__all.bat) is included that does all the conversion.
If you prefer to do everything manually, then use the instructions
below. Otherwise running the script is all you need to do.

## Do it step by step

Execute the following batch files:
conv_z80.bat

Execute depenend on wanted games:
conv_1942.bat
conv_digdug.bat
conv_dkong.bat
conv_eyes.bat
conv_frogger.bat
conv_galaga.bat
conv_lizwiz.bat
conv_mrtnt.bat
conv_pacman.bat
conv_theglob.bat
conv_crush.bat
conv_anteater.bat
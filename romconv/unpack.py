import zipfile
import shutil
import sys

ZIP="../romszip/" + sys.argv[1]
DEST="./roms/"

shutil.unpack_archive(ZIP, DEST)

import sys
from PIL import Image

BGCOLOR = (255, 0, 255)

FILE_OUT = "out"

if len(sys.argv) == 1:
    print("Usage: python font2c.py <imagename>")
    sys.exit(0)

im = Image.open(sys.argv[1])
px = im.load()

data = []

with open(FILE_OUT, "w") as f:
    f.write(".global GLYPHS\n")
    f.write("GLYPHS:\n")
    for j in range(16):
        for i in range(16):
            for y in range(8):
                n = 0
                for x in range(8):
                    if px[i*8+x, j*8+y] != BGCOLOR:
                        n |= (1 << (7 - x))
                f.write(".byte 0x%02x\n" % n)


# Run on the PC: converts a photo to a 1-bit MONO_HLSB bitmap for the 128x64 SSD1306 OLED
import sys
from PIL import Image, ImageOps

src = sys.argv[1] if len(sys.argv) > 1 else "44794609.png"
out = sys.argv[2] if len(sys.argv) > 2 else "photo.bin"
SIZE = 64  # square photo fits the full screen height
CROP = (165, 215, 305, 355)  # (left, top, right, bottom): Joel's head and shoulders
THRESHOLD = 70  # plain threshold; dithering turns the grey sky/road into noise

img = Image.open(src).convert("L")
if CROP:
    img = img.crop(CROP)
img = ImageOps.fit(img, (SIZE, SIZE), Image.LANCZOS)
img = ImageOps.autocontrast(img, cutoff=2)
img = img.point(lambda p: 255 if p > THRESHOLD else 0).convert("1")

with open(out, "wb") as f:
    f.write(img.tobytes())
print(f"Wrote {out}: {SIZE}x{SIZE}, {SIZE * SIZE // 8} bytes")

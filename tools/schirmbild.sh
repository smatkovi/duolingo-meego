#!/bin/sh
# Grabs the framebuffer of an N9/N950 and turns it into a PNG here.
#
# The panel stores its picture rotated: 16 bpp RGB565, 1712 bytes per row
# (856 pixels, of which 854 are visible), three frames of 512 rows each. The
# visible screen is 854x480 in that buffer and has to be turned a quarter
# turn to be seen the way it is held.
#
#   shot.sh out.png [host]
set -e
OUT=${1:-shot.png}
HOST=${2:-192.168.1.8}
SPRUNG=${N9_JUMP:+-J $N9_JUMP}
K="$SPRUNG -oHostKeyAlgorithms=+ssh-rsa -oPubkeyAcceptedAlgorithms=+ssh-rsa -i $HOME/.ssh/id_rsa_n9"
cat > /tmp/fbgrab-remote.sh <<'SH'
dd if=/dev/fb0 of=/home/user/fb.raw bs=65536 2>/dev/null
chmod 644 /home/user/fb.raw
SH
scp $K /tmp/fbgrab-remote.sh user@$HOST:/home/user/ >/dev/null
ssh $K user@$HOST 'sudo sh /home/user/fbgrab-remote.sh' >/dev/null 2>&1
scp $K user@$HOST:/home/user/fb.raw /tmp/fb-shot.raw >/dev/null 2>&1
python3 - "$OUT" <<'PY'
import sys
from PIL import Image
data = open('/tmp/fb-shot.raw', 'rb').read()
STRIDE, W, H = 1712, 854, 480
# Der Puffer haelt drei Bilder zu je 512 Zeilen; welches gerade angezeigt
# wird, wechselt. Also alle drei lesen und das mit dem meisten Inhalt
# nehmen -- ein nicht benutztes ist schwarz.
FRAME_ROWS = 512
best, besthue = None, -1
for frame in range(3):
    off = frame * STRIDE * FRAME_ROWS
    image = Image.new('RGB', (W, H))
    pixels = image.load()
    for y in range(H):
        row = data[off + y * STRIDE: off + y * STRIDE + W * 2]
        if len(row) < W * 2:
            break
        for x in range(W):
            value = row[2 * x] | (row[2 * x + 1] << 8)
            pixels[x, y] = (((value >> 11) & 31) * 255 // 31,
                            ((value >> 5) & 63) * 255 // 63,
                            (value & 31) * 255 // 31)
    farben = len(set(image.resize((80, 45)).getdata()))
    if farben > besthue:
        best, besthue = image, farben
print("Puffer mit %d Farben gewaehlt" % besthue)
best.transpose(Image.ROTATE_270).save(sys.argv[1])
print(sys.argv[1], "geschrieben")
PY

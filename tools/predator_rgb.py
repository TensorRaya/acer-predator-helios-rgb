#!/usr/bin/env python3
"""
predator_rgb.py - reference implementation for the Acer Predator Helios 16 (PH16-7x) lighting.

No dependencies beyond the Python standard library. Talks to /dev/hidraw* directly
(needs read/write permission, see udev/60-acer-predator-helios.rules).

Zones and the protocol each one speaks:

  keyboard   Jing-Mold/Sunrex 05AF:667B/667A (or Chicony 04F2:0117), interface 3  vendor protocol
  lightbar   Darfon "InfiniteRing" 0D62:A20A, interface 1                    HID LampArray (33 lamps:
             0-3 left edge, 4-17 front edge, 18-32 palm rest logo + stripes)
  logo       Darfon "Cover Logo"  0D62:A00A, interface 1                    HID LampArray (14 lamps)
  glass      Darfon "Cover Logo"  0D62:A00A, interface 2                    HID LampArray (16 lamps)

The InfiniteRing also has a vendor interface 0 that accepts the keyboard protocol, but once the
LampArray has been taken over by the host (autonomous mode off) that path no longer drives the
LEDs; use the LampArray.

Examples:
  predator_rgb.py keyboard static  00AEC7 --brightness 100
  predator_rgb.py keyboard breathing FF0000 --speed 9
  predator_rgb.py keyboard perkey  0000FF                # all 128 cells
  predator_rgb.py lightbar color 00AEC7
  predator_rgb.py lightbar color FF00FF --lamps 18-32     # palm rest logo only
  predator_rgb.py logo  color 00FF00
  predator_rgb.py glass color FF00FF --intensity 128
  predator_rgb.py glass firmware                          # hand back to built-in effect
  predator_rgb.py all off
"""
import argparse, array, fcntl, glob, os, struct, sys, time

# ---------------------------------------------------------------- hidraw helpers
HIDIOCSFEATURE = lambda n: (3 << 30) | (n << 16) | (ord('H') << 8) | 0x06
HIDIOCGFEATURE = lambda n: (3 << 30) | (n << 16) | (ord('H') << 8) | 0x07

def find_hidraw(vid, pid, interface):
    for h in sorted(glob.glob("/sys/class/hidraw/hidraw*")):
        uevent = open(h + "/device/uevent").read().upper()
        if f"{vid:04X}:0000{pid:04X}" not in uevent:
            continue
        dev = os.path.realpath(h + "/device")
        if interface is None:                      # I2C-HID (no USB interface number), e.g. the ENE top key
            return "/dev/" + os.path.basename(h)
        try:
            iface = int(open(dev + "/../bInterfaceNumber").read(), 16)
        except OSError:
            continue
        if iface == interface:
            return "/dev/" + os.path.basename(h)
    return None

class HidRaw:
    def __init__(self, path, pid=None):
        self.fd = os.open(path, os.O_RDWR)
        self.pid = pid
    def close(self):
        os.close(self.fd)
    def set_feature(self, data):
        buf = array.array('B', data)
        fcntl.ioctl(self.fd, HIDIOCSFEATURE(len(buf)), buf, True)
    def get_feature(self, report_id, length):
        buf = array.array('B', [report_id] + [0] * (length - 1))
        fcntl.ioctl(self.fd, HIDIOCGFEATURE(length), buf, True)
        return bytes(buf)
    def write(self, data):
        os.write(self.fd, bytes(data))

# ---------------------------------------------------------------- vendor protocol (keyboard)
# 8-byte feature report (report id 0): op, p1..p6, checksum = 0xFF - (sum(op..p6) & 0xFF)
OP_BEGIN, OP_MODE_ZONE, OP_MODE_PERKEY, OP_SET_COLOR, OP_APPLY = 0x88, 0xB1, 0x12, 0x14, 0x08
EFFECTS = {
    "static": 0x01, "breathing": 0x02, "rainbow": 0x03, "snake": 0x05, "ripple": 0x06,
    "neon": 0x08, "rain": 0x0A, "explosion": 0x12, "pulse": 0x25, "stars": 0x26,
    "meteor": 0x27, "aura": 0x28,
}
PERKEY_EFFECT, NUM_CELLS = 0x33, 128
CUSTOM_COLOUR_TAIL_PIDS = {0x667A}   # keyboards whose APPLY must end 00 00 to use a custom colour

def vendor_cmd(dev, op, *params):
    body = [op] + list(params) + [0] * (6 - len(params))
    body.append((0xFF - (sum(body) & 0xFF)) & 0xFF)
    dev.set_feature([0x00] + body)

def vendor_effect(dev, effect, rgb, brightness=100, speed=5):
    """speed: 1 = fastest .. 9 = slowest (firmware convention)."""
    r, g, b = rgb
    vendor_cmd(dev, OP_BEGIN)
    vendor_cmd(dev, OP_MODE_ZONE)
    if effect == "off":
        vendor_cmd(dev, OP_APPLY, 0x02, EFFECTS["static"], speed, 0, 0x01, 0x05)
        return
    vendor_cmd(dev, OP_SET_COLOR, 0x00, 0x00, r, g, b, 0x00)
    if dev.pid in CUSTOM_COLOUR_TAIL_PIDS:
        # Sunrex 05AF:667A (PH16-73): APPLY byte P5 selects the colour source, 0x01 = built-in
        # preset (P6 picks it, 0x05 = blue), 0x00 = the colour from SET_COLOR. Confirmed on
        # hardware: tail 01 05 / 01 00 -> always blue, tail 00 00 -> the requested colour.
        vendor_cmd(dev, OP_APPLY, 0x02, EFFECTS[effect], speed, brightness, 0x00, 0x00)
        time.sleep(0.05)
        vendor_cmd(dev, OP_SET_COLOR, 0x00, 0x00, r, g, b, 0x00)
    else:
        vendor_cmd(dev, OP_APPLY, 0x02, EFFECTS[effect], speed, brightness, 0x01, 0x05)

def vendor_perkey(dev, cells, brightness=100):
    """cells: list of up to 128 (r,g,b) tuples. Frame = 128 x {00 R G B}, sent as 8 x 64 byte OUT packets."""
    frame = bytearray(NUM_CELLS * 4)
    for i, (r, g, b) in enumerate(cells[:NUM_CELLS]):
        frame[i * 4 + 1:i * 4 + 4] = bytes((r, g, b))
    vendor_cmd(dev, OP_BEGIN)
    vendor_cmd(dev, OP_MODE_PERKEY, 0x00, 0x00, 0x08)
    for off in range(0, len(frame), 64):
        dev.write(b"\x00" + frame[off:off + 64])
    vendor_cmd(dev, OP_APPLY, 0x02, PERKEY_EFFECT, 0x05, brightness, 0x08, 0x01)

# ---------------------------------------------------------------- HID LampArray (lightbar, logo, glass)
LA_ATTRIBUTES, LA_RANGE_UPDATE, LA_CONTROL = 0x01, 0x05, 0x06

def lamparray_count(dev):
    # The Darfon firmware NAKs GET requests (EPROTO) for a short window after it has been
    # written to; back off and retry instead of failing.
    for attempt in range(8):
        try:
            return struct.unpack("<H", dev.get_feature(LA_ATTRIBUTES, 23)[1:3])[0]
        except OSError:
            if attempt == 7:
                raise
            time.sleep(0.05 * (attempt + 1))
    return 0

def lamparray_autonomous(dev, enabled):
    dev.set_feature([LA_CONTROL, 1 if enabled else 0])

def lamparray_fill(dev, rgb, intensity=255, first=0, last=None):
    n = lamparray_count(dev)
    if last is None or last >= n:
        last = n - 1
    lamparray_autonomous(dev, False)
    dev.set_feature([LA_RANGE_UPDATE, 0x01] + list(struct.pack("<HH", first, last)) + list(rgb) + [intensity])

# ---------------------------------------------------------------- CLI
ZONES = {
    "keyboard": [(0x05AF, 0x667B, 3), (0x05AF, 0x667A, 3), (0x04F2, 0x0117, 3)],
    "lightbar": [(0x0D62, 0xA20A, 1)],
    "logo":     [(0x0D62, 0xA00A, 1)],
    "glass":    [(0x0D62, 0xA00A, 2)],
    # Lit button above the keyboard (top left): ENE 0CF2:5130, I2C-HID, vendor usage page 0xFF5A.
    # Same 16-byte feature report OpenRGB's "Acer Nitro HID LED Device" controller uses.
    "topkey":   [(0x0CF2, 0x5130, None)],
}
ENE_DEVICE_ID = 0x65
# ENE modes (predator-sense hid_rgb.rs numbering). The PH16-73 target 0x65 advertises 1,2,4,5,6;
# mode 0x01 turns the LED off there, 0x02 is static.
ENE_MODE_OFF, ENE_MODE_STATIC, ENE_MODE_BREATHING, ENE_MODE_NEON = 0x01, 0x02, 0x04, 0x05
ENE_MODES = {"color": ENE_MODE_STATIC, "static": ENE_MODE_STATIC,
             "breathing": ENE_MODE_BREATHING, "neon": ENE_MODE_NEON}

def ene_set(dev, rgb, brightness, mode=ENE_MODE_STATIC, speed=0):
    """topkey: 0xA4 <target> <mode> <brightness 0-100> <speed> <flag> r g b <zone mask lo> <hi> … (16 bytes)."""
    pkt = [0xA4, ENE_DEVICE_ID, mode, min(brightness, 100), speed if mode != ENE_MODE_STATIC else 0, 0x00,
           *rgb, 0x01, 0x00]
    dev.set_feature(pkt + [0] * (16 - len(pkt)))

def open_zone(zone):
    for vid, pid, iface in ZONES[zone]:
        path = find_hidraw(vid, pid, iface)
        if path:
            return HidRaw(path, pid)
    sys.exit(f"{zone}: device not found (is it a PH16-7x, and do you have permission on /dev/hidraw*?)")

def parse_rgb(s):
    s = s.strip().lstrip("#")
    if s.lower().startswith("0x"):
        s = s[2:]
    if len(s) != 6:
        sys.exit(f"bad colour {s!r}: use RRGGBB, e.g. FF0000")
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("zone", choices=list(ZONES) + ["all"])
    ap.add_argument("mode", help="keyboard: " + "|".join(["color"] + list(EFFECTS)) + "|perkey|off ; "
                    "lightbar/logo/glass: color|firmware|off ; topkey: color|breathing|neon|off")
    ap.add_argument("rgb", nargs="?", default="FFFFFF", help="RRGGBB")
    ap.add_argument("--brightness", type=int, default=100, help="0-100 (vendor zones)")
    ap.add_argument("--speed", type=int, default=5, help="1 fast .. 9 slow (vendor zones)")
    ap.add_argument("--intensity", type=int, default=255, help="0-255 (lamparray zones)")
    ap.add_argument("--lamps", default=None, help="lamp range FIRST-LAST for lamparray zones, e.g. 18-32")
    a = ap.parse_args()
    rgb = parse_rgb(a.rgb)
    zones = list(ZONES) if a.zone == "all" else [a.zone]
    if a.zone == "all" and a.mode == "firmware":
        zones = ["lightbar", "logo", "glass"]       # only the LampArray zones have a firmware mode
    first, last = 0, None
    if a.lamps:
        first, last = (int(x) for x in a.lamps.split("-"))
    for z in zones:
        dev = open_zone(z)
        try:
            if z == "keyboard":
                mode = "static" if a.mode == "color" else a.mode
                if mode == "perkey":
                    vendor_perkey(dev, [rgb] * NUM_CELLS, a.brightness)
                elif mode in EFFECTS or mode == "off":
                    vendor_effect(dev, mode, rgb, a.brightness, a.speed)
                else:
                    sys.exit(f"{z}: unknown mode {a.mode}")
            elif z == "topkey":
                if a.mode == "off":
                    ene_set(dev, (0, 0, 0), 0, ENE_MODE_OFF)
                elif a.mode in ENE_MODES:
                    ene_set(dev, rgb, a.brightness, ENE_MODES[a.mode], min(a.speed, 9))
                else:
                    sys.exit(f"{z}: unknown mode {a.mode} (topkey: color|breathing|neon|off)")
            else:
                if a.mode == "firmware":
                    lamparray_autonomous(dev, True)
                elif a.mode == "off":
                    lamparray_fill(dev, (0, 0, 0), 0, first, last)
                elif a.mode in ("color", "static"):
                    lamparray_fill(dev, rgb, a.intensity, first, last)
                else:
                    sys.exit(f"{z}: unknown mode {a.mode}")
            print(f"{z}: {a.mode} {a.rgb}")
        finally:
            dev.close()

if __name__ == "__main__":
    main()

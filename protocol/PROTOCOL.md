# Acer Predator Helios 16 (PH16-7x) — RGB lighting protocol reference

Everything below was verified on a **Predator PH16-73** (board `Emira_ARX`, BIOS V1.52)
running CachyOS (Linux 6.17) on 2026-09-10. The keyboard protocol is identical to what
USBPcap captures of PredatorSense on a PH16-71 show (see [Sources](#sources)), so it
should hold for the whole PH16-71/72/73 family and very likely the Helios Neo (PHN16-7x).

The laptop has **five independently lit areas**: four served by **three USB HID devices**
speaking **two different protocols**, plus the button above the keyboard on an ENE I²C
chip that OpenRGB already supports. None of them is reachable through ACPI-WMI on the
PH16-73 — `acer-wmi`/`linuwu-sense` `four_zone_mode` writes are accepted by the firmware
but do nothing visible.

| Area | USB device | Interface | Protocol |
|---|---|---|---|
| Keyboard backlight (per-key capable) | `05AF:667B` or `05AF:667A` Jing-Mold/Sunrex "ACER Keyboard" (Chicony `04F2:0117` on some units) | 3 (usage page `0xFF02`) | Acer vendor protocol |
| Front light bar + palm rest logo ("InfiniteRing") | `0D62:A20A` Darfon | 1 (usage page `0x59`) | **HID LampArray**, 33 lamps |
| Lid logo | `0D62:A00A` Darfon "Cover Logo" | 1 (usage page `0x59`) | **HID LampArray**, 14 lamps |
| Rear glass bar (above the I/O ports) | `0D62:A00A` Darfon "Cover Logo" | 2 (usage page `0x59`) | **HID LampArray**, 16 lamps |

The InfiniteRing also has a vendor interface (0, usage page `0xFF01`) that accepts the
keyboard protocol below. It only drives part of the strip, brightness and effects behave
inconsistently, and once the host has taken over the LampArray (autonomous mode = 0) the
vendor path stops having any visible effect. Use the LampArray.

InfiniteRing lamp map (verified visually on a PH16-73, lamps are numbered left to right):

| Lamps | Location |
|---|---|
| 0–3 | left side edge |
| 4–17 | front edge, left → right |
| 18–32 | Predator logo and the four diagonal stripes on the right palm rest |

Raw report descriptors and USB attributes of every interface: [`hid-descriptors.txt`](hid-descriptors.txt),
decoded LampArray report layout: [`lamparray-reports.txt`](lamparray-reports.txt).

---

## 1. Acer vendor protocol (keyboard)

### Transport

* **Feature reports** (`SET_REPORT`, report type 3, report ID 0) of **8 bytes** on the
  vendor interface. On Linux: `HIDIOCSFEATURE` on the interface's `hidraw` node with a
  leading `0x00` report-ID byte (9 bytes total).
* Per-key frames additionally use the interface's **interrupt OUT endpoint** (EP 4 on the
  keyboard) in **64-byte packets**.

### Command format

```
byte  0   1   2   3   4   5   6   7
      OP  P1  P2  P3  P4  P5  P6  CK      CK = 0xFF - ((OP+P1+...+P6) & 0xFF)
```

Every command carries this inverted-sum checksum; the firmware silently ignores frames
with a wrong checksum.

### Commands

| OP | Name | Bytes | Notes |
|---|---|---|---|
| `0x88` | BEGIN | `88 00 00 00 00 00 00 77` | Always first. |
| `0xB1` | MODE_ZONE | `B1 00 00 00 00 00 00 4E` | Select single-colour/effect mode. |
| `0x12` | MODE_PERKEY | `12 00 00 08 00 00 00 E5` | Select per-key mode; a 512-byte frame follows on the OUT endpoint. |
| `0x14` | SET_COLOR | `14 00 00 R G B 00 CK` | Colour for the next APPLY. |
| `0x08` | APPLY | `08 02 EFF SPD BRT SCOPE 05 CK` | Commit. See below. |

APPLY parameters:

| Byte | Meaning | Values |
|---|---|---|
| `EFF` | effect id | see table |
| `SPD` | speed | `1` = fastest … `9` = slowest (PredatorSense default `5`; captures show `1`) |
| `BRT` | brightness | `0`–`100` (captures show `0x32` = 50). `0` = off |
| `SCOPE` | target | `0x01` whole zone (single colour), `0x08` per-key frame |
| last | | `0x05` for zone mode, `0x01` for per-key mode |

Effect ids (from the PredatorSense binary, `static`/`breathing` verified, speed verified):

| id | effect | colour used |
|---|---|---|
| `0x01` | Static | yes |
| `0x02` | Breathing | yes |
| `0x03` | Rainbow / Wave | no |
| `0x05` | Snake | yes |
| `0x06` | Ripple | yes |
| `0x08` | Neon | no |
| `0x0A` | Rain | yes |
| `0x12` | Explosion | yes |
| `0x25` | Pulse | yes |
| `0x26` | Stars | yes |
| `0x27` | Meteor | yes |
| `0x28` | Aura | yes |
| `0x33` | Per-key (with frame) | frame |

### Sequence: single colour, whole zone

```
88 00 00 00 00 00 00 77            BEGIN
B1 00 00 00 00 00 00 4E            MODE_ZONE
14 00 00 FF 00 00 00 EC            SET_COLOR red
08 02 01 01 32 01 05 BB            APPLY static, speed 1, brightness 50, scope zone
```

Off = APPLY with `BRT = 0` (the firmware has no dedicated off command).

### Sequence: per-key frame (keyboard only)

```
88 00 00 00 00 00 00 77            BEGIN
12 00 00 08 00 00 00 E5            MODE_PERKEY
<8 x 64-byte interrupt OUT packets>  512 bytes = 128 cells x {00 R G B}
08 02 33 05 19 08 01 9B            APPLY per-key, brightness 25, scope per-key
```

Cell → physical key mapping is **not yet known**. 128 cells cover the 4-zone keyboard;
filling all cells with one colour lights the whole keyboard. Contributions welcome.

### Per-key cell map (Sunrex `05AF:667B`, PH16-73, ISO/DE)

The MCU numbers the 128 frame cells **column-wise, top to bottom**, then left to right
(0 Esc, 1 `^`, 2 Tab, 3 Caps, 4 Shift, 5 Ctrl, 6 F1, 7 `1`, 8 Q, …). This differs from
the Chicony `04F2:0117` order documented by fuho for the PH16-71 (row-wise), so the two
MCUs need separate maps. Recorded by lighting one cell at a time and capturing the evdev
key event ([`keymap-ph16-73-sunrex.json`](keymap-ph16-73-sunrex.json) has the full table);
Fn, media keys, power, Predator and Copilot keys produce no keyboard event and were
identified visually. 104 of 128 cells have a key, 24 are unused.

| 0: ESC | 1: GRAVE | 2: TAB | 3: CAPSLOCK | 4: LEFTSHIFT | 5: LEFTCTRL | 6: F1 | 7: 1 |
| 8: Q | 9: A | 10: 102ND | 11: FN | 12: F2 | 13: 2 | 14: W | 15: S |
| 16: Z | 17: LEFTMETA | 18: F3 | 19: 3 | 20: E | 21: D | 22: X | 23: LEFTALT |
| 24: F4 | 25: 4 | 26: R | 27: F | 28: C | 29: - | 30: F5 | 31: 5 |
| 32: T | 33: G | 34: V | 35: - | 36: F6 | 37: 6 | 38: Y | 39: H |
| 40: B | 41: SPACE | 42: F7 | 43: 7 | 44: U | 45: J | 46: N | 47: - |
| 48: F8 | 49: 8 | 50: I | 51: K | 52: M | 53: - | 54: F9 | 55: 9 |
| 56: O | 57: L | 58: COMMA | 59: RIGHTALT | 60: F10 | 61: 0 | 62: P | 63: SEMICOLON |
| 64: DOT | 65: COMPOSE | 66: F11 | 67: MINUS | 68: LEFTBRACE | 69: APOSTROPHE | 70: SLASH | 71: - |
| 72: F12 | 73: EQUAL | 74: RIGHTBRACE | 75: - | 76: - | 77: - | 78: SYSRQ | 79: - |
| 80: - | 81: - | 82: - | 83: COPILOT | 84: INSERT | 85: - | 86: - | 87: BACKSLASH |
| 88: RIGHTSHIFT | 89: LEFT | 90: DELETE | 91: BACKSPACE | 92: - | 93: ENTER | 94: UP | 95: DOWN |
| 96: PREVIOUSSONG | 97: PREDATOR | 98: KP7 | 99: KP4 | 100: KP1 | 101: RIGHT | 102: PLAYPAUSE | 103: NUMLOCK |
| 104: KP8 | 105: KP5 | 106: KP2 | 107: KP0 | 108: NEXTSONG | 109: KPSLASH | 110: KP9 | 111: KP6 |
| 112: KP3 | 113: KPDOT | 114: POWER | 115: KPASTERISK | 116: KPMINUS | 117: KPPLUS | 118: - | 119: KPENTER |
| 120: - | 121: - | 122: - | 123: - | 124: - | 125: - | 126: - | 127: - |

The Predator key (cell 97) and the Copilot key (cell 83) are part of the keyboard array.
The **lit button above the keyboard, top left** is *not* — see section 3.

### Quirks

* **Idle-off is EC firmware and cannot be switched off from the host.** The keyboard
  backlight goes dark after ~30 s without input and wakes on the next key press. Tested on
  the PH16-73 without effect: `predator_sense/backlight_timeout` (linuwu-sense, WMI GUID
  `61EF69EA-…` arg `0x88402`/`0x1E0000088402`) in both positions, KDE PowerDevil (there is
  no `kbd_backlight` LED class device, so KDE does not even know the keyboard), and the WMI
  colour path. Colour commands are accepted while asleep and become visible on wake. The
  only working approach is a keepalive that re-applies the state every ≤30 s — Venator does
  the same (`start_keepalive`, 30 s). Our variant runs through the OpenRGB SDK server so
  there is exactly one writer, and only while the lid is open and the internal panel is on
  (`/proc/acpi/button/lid/*/state`, `/sys/class/drm/card*-eDP-*/dpms`).
* **Zone-mode STATIC did not take effect in one late-night session** while per-key frames
  did: after a WMI `four_zone_mode` write (00:11) every vendor `B1 … 08 02 01 …` sequence
  (USB), every `per_zone_mode`/`four_zone_mode` write and even a USB re-enumeration of the
  MCU left the keyboard on the previously set colour, whereas a 128-cell per-key frame was
  visible immediately. Not reproduced yet with a clean baseline (no prior WMI write); treat
  zone-mode static as *unverified after a WMI write* and prefer per-key fill for "static".
* `hid_write` on the vendor interface (interrupt OUT, report ID 0) is **not** a substitute
  for the feature report — commands sent that way are ignored.
* Interfaces 0–2 of the keyboard also accept feature reports (usage pages `0xFF89`,
  `0xFF00`, consumer) but do not drive the LEDs.

---

## 2. HID LampArray (front bar, lid logo, rear glass bar)

The Darfon "Cover Logo" device implements the **standard USB HID LampArray usage page
(0x59)** — the same interface Windows Dynamic Lighting uses. No vendor knowledge needed;
the device describes itself.

Report IDs and layout (from the descriptor, both interfaces identical):

| Report | Type | Payload | Content |
|---|---|---|---|
| 1 | Feature | 22 B | **LampArrayAttributes**: LampCount u16, BoundingBox W/H/D u32 (µm), LampArrayKind u32, MinUpdateInterval u32 (µs) |
| 2 | Feature | 2 B | LampAttributesRequest: LampId u16 |
| 3 | Feature | 28 B | LampAttributesResponse: LampId, position X/Y/Z, update latency, purposes, RGBI levels, is-programmable, input binding |
| 4 | Feature | 50 B | **LampMultiUpdate**: LampCount u8 (≤8), flags u8, 8 × LampId u16, 8 × {R,G,B,I} |
| 5 | Feature | 9 B | **LampRangeUpdate**: flags u8, LampIdStart u16, LampIdEnd u16, R, G, B, I |
| 6 | Feature | 1 B | **LampArrayControl**: AutonomousMode u8 (1 = firmware effect, 0 = host controls) |

Measured attributes:

| Interface | LampCount | Bounding box | Kind | Min update |
|---|---|---|---|---|
| Cover Logo if 1 (lid logo) | 14 | 60 × 68 × 1 mm | 5 | 33 ms |
| Cover Logo if 2 (glass bar) | 16 | 345 × 25 × 1 mm | 10 | 33 ms |
| InfiniteRing if 1 | 33 | 360 × 100 × 10 mm | 7 | 33 ms |

### Sequence: fill one array

```
06 00                               LampArrayControl: AutonomousMode = 0   (stops the built-in rainbow)
05 01 00 00 0D 00 R G B FF          LampRangeUpdate: flags=UpdateComplete, lamps 0..13, colour, intensity 255
```

Hand control back to the firmware effect: `06 01`.

### Quirks

* **Intensity matters.** `I = 1` is effectively dark; use `255` for full brightness.
* The firmware **NAKs GET_REPORT for a short window after a write** (`EPROTO` on Linux,
  observed on the attributes read). Retry with a small back-off (50 ms steps) — 0/40
  failures with 8 attempts vs. ~1/10 without.
* Sending `AutonomousMode = 0` alone turns the array off until the first colour update.

---

## 3. What does *not* work on the PH16-73

Recorded so that nobody repeats it:

| Path | Result |
|---|---|
| `acer-wmi` / `linuwu-sense` `four_zone_mode`, `per_zone_mode` (WMI GUID `7A4DDFE7-…`, method 20) | Firmware returns success, no LED changes. Method 21 (GET) returns error 1. |
| ENE `0CF2:5130` (OpenRGB "Acer Nitro HID" controller) | Does **not** drive the keyboard here — but it **does drive the lit button above the keyboard (top left)**. OpenRGB's existing controller (`openrgb -d "Acer Nitro HID LED Device" -m Static -c 00FF00`) hits it; a hand-rolled 11-byte report did not. So the PH16-73 has **five** lit areas, and the fifth needs no new code — just include the device in the profile. |
| Rust `predator-sense` `magic_rgb.rs` (decompiled DLL protocol, `0x41` mode byte layout) | Ignored by the keyboard MCU — layout differs from the captured one above. |
| Feature reports on keyboard interfaces 0/1/2 | Accepted, no effect. |
| Darfon vendor interface 0 for effects | Cover Logo: firmware effects (e.g. Breathing) **work** while `AutonomousMode = 1`, but only for the lid logo — no zone selector reaches the glass bar (tried colour byte P1 and APPLY byte P6 = 1/2/6). InfiniteRing: colour is accepted, effect ids are not, and the strip **flickers** in vendor mode; stable via LampArray. |
| `predator_sense/backlight_timeout` | Reads/writes fine, no influence on the ~30 s keyboard idle-off (see keyboard quirks). |

---

## Sources

* **USBPcap captures of PredatorSense (PH16-71)** by *roosterduck*, published in
  [OpenRGB issue #4158](https://gitlab.com/CalcProgrammer1/OpenRGB/-/work_items/4158):
  `just_global_R_G_B.pcap`, `short_white.pcap`, `global_to_area_and_back.pcap`. Every
  byte in section 1 comes from these files. Analysis notes in the same thread by *fuho*.
* [Exyons/Venator](https://github.com/Exyons/Venator) — kernel module + CLI for the
  PH16-71 that implements the same keyboard protocol; its source confirmed the per-key
  frame format and the effect catalogue.
* [lau-bin/acer_brightness](https://github.com/lau-bin/acer_brightness) — WMI method 20
  payload layout (tested on PHN16-73) that let us rule the WMI path out quickly.
* USB HID Usage Tables 1.5, section 25 "Lighting And Illumination Page (0x59)".

## Addendum: PH16-73 with keyboard `05AF:667A`

Found on a second PH16-73 (BIOS V1.53) whose keyboard enumerates as `05AF:667A` instead of `667B`.

* **APPLY colour source.** In `08 02 <effect> <speed> <brightness> P5 P6`, P5 selects where the colour
  comes from: `01` = a built-in preset picked by P6 (`05` = blue), `00` = the colour from SET_COLOR (`14`).
  With the `01 05` tail every effect came out blue whatever SET_COLOR said, and `01 00` was blue too;
  `00 00` (with SET_COLOR repeated after APPLY) gives the requested colour for static and breathing.
  The tool uses `00 00` only for `667A`; `667B` keeps `01 05`.
* **ENE `0CF2:5130` modes.** Report `A1` lists one target, `0x65`; `A2 65` then `A3` returns
  `a3 65 01 09 01 00 00 00 3b` (1 zone, mode mask `0x3b`). Using the mode numbering from
  [predator-sense](https://github.com/cleyton1986/predator-sense) (`hid_rgb.rs`), `A4 65 <mode> …` with
  mode `01` turns the LED **off**, `02` = static, `04` = breathing, `05` = neon (all confirmed by eye);
  mode `06` is advertised but untested. Zone mask `01 00` at bytes 9-10.

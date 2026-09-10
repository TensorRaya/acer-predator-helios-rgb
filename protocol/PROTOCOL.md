# Acer Predator Helios 16 (PH16-7x) — RGB lighting protocol reference

Everything below was verified on a **Predator PH16-73** (board `Emira_ARX`, BIOS V1.52)
running CachyOS (Linux 6.17) on 2026-09-10. The keyboard protocol is identical to what
USBPcap captures of PredatorSense on a PH16-71 show (see [Sources](#sources)), so it
should hold for the whole PH16-71/72/73 family and very likely the Helios Neo (PHN16-7x).

The laptop has **four independently lit areas**, served by **three USB HID devices**
speaking **two different protocols**. None of them is reachable through ACPI-WMI on the
PH16-73 — `acer-wmi`/`linuwu-sense` `four_zone_mode` writes are accepted by the firmware
but do nothing visible.

| Area | USB device | Interface | Protocol |
|---|---|---|---|
| Keyboard backlight (per-key capable) | `05AF:667B` Jing-Mold/Sunrex "ACER Keyboard" (Chicony `04F2:0117` on some units) | 3 (usage page `0xFF02`) | Acer vendor protocol |
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

### Quirks

* The keyboard backlight goes to sleep after ~30 s without input (firmware). Colour
  commands are accepted while asleep and become visible on the next key press.
* `hid_write` on the vendor interface (interrupt OUT, report ID 0) is **not** a substitute
  for the feature report — commands sent that way are ignored.
* Interfaces 0–2 of the keyboard also accept feature reports (usage pages `0xFF89`,
  `0xFF00`, consumer) but do not drive the LEDs.

---

## 2. HID LampArray (lid logo, rear glass bar)

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
| ENE `0CF2:5130` (OpenRGB "Acer Nitro HID" controller) | Device exists (I²C `ENEK5130`) and accepts reports, but drives no visible LED on this model. It **does** drive the keyboard on Nitro models. |
| Rust `predator-sense` `magic_rgb.rs` (decompiled DLL protocol, `0x41` mode byte layout) | Ignored by the keyboard MCU — layout differs from the captured one above. |
| Feature reports on keyboard interfaces 0/1/2 | Accepted, no effect. |

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

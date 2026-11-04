# Acer Predator Helios 16 RGB on Linux (PH16-71 / PH16-72 / PH16-73)

Full control of **all four lighting zones** of the Acer Predator Helios 16 under Linux —
keyboard, front light bar, lid logo and rear glass bar — without PredatorSense, without
Windows, without kernel patches.

* **Protocol documentation** for every zone, byte by byte: [`protocol/PROTOCOL.md`](protocol/PROTOCOL.md)
* **OpenRGB controller** (submitted upstream, see below): [`openrgb/`](openrgb/)
* **Stand-alone Python tool**, no dependencies: [`tools/predator_rgb.py`](tools/predator_rgb.py)
* **udev rule** so you don't need root: [`udev/60-acer-predator-helios.rules`](udev/60-acer-predator-helios.rules)

Verified on a Predator PH16-73 (RTX 5090 laptop, CachyOS, kernel 6.17). The keyboard
protocol matches USB captures from a PH16-71, so the whole PH16-7x family should work.
If you have a PH16-71/72 or a Helios Neo (PHN16-7x), please open an issue and tell me
what happens.

## Why this exists

Searching for "Predator Helios 16 RGB Linux" in 2026 gets you: a WMI driver
(`linuwu-sense`) whose keyboard RGB does nothing on this model, a Rust project with a
protocol decompiled from the Windows DLL that the hardware ignores, and an OpenRGB
controller for the Nitro line that talks to a chip which exists in the Helios but doesn't
drive any LED. It took captures from a PH16-71 owner, a kernel module written for that
model and an evening of "is it blue now?" to find out that the Helios uses **two
protocols on three USB devices**, and that two of the zones speak the standard
**HID LampArray** interface — no reverse engineering required at all.

This repository writes it all down so the next person needs ten minutes, not a day.

## The hardware

| Zone | USB ID | Interface | Protocol |
|---|---|---|---|
| Keyboard | `05AF:667B` / `05AF:667A` (or `04F2:0117`) | 3 | Acer vendor, feature reports + 512-byte per-key frames |
| Front light bar + palm rest logo ("InfiniteRing") | `0D62:A20A` | 1 | HID LampArray, 33 lamps (left edge 4, front edge 14, palm rest logo 15) |
| Lid logo | `0D62:A00A` | 1 | HID LampArray, 14 lamps |
| Rear glass bar | `0D62:A00A` | 2 | HID LampArray, 16 lamps |
| Button above the keyboard (top left) | `0CF2:5130` ENE, I²C | – | already supported by OpenRGB ("Acer Nitro HID LED Device") |

Details, byte tables, quirks and everything that does *not* work: [`protocol/PROTOCOL.md`](protocol/PROTOCOL.md).

## Quick start (Python tool)

```sh
sudo install -m 644 udev/60-acer-predator-helios.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger      # then re-plug or log out/in once

tools/predator_rgb.py keyboard static  00AEC7            # whole keyboard, one colour
tools/predator_rgb.py keyboard breathing FF0000 --speed 9
tools/predator_rgb.py keyboard perkey  0000FF            # per-key frame (all 128 cells)
tools/predator_rgb.py lightbar color 00AEC7
tools/predator_rgb.py lightbar color FF00FF --lamps 18-32   # palm rest logo only
tools/predator_rgb.py logo  color 00FF00
tools/predator_rgb.py glass color FF00FF --intensity 128
tools/predator_rgb.py glass firmware                     # back to the built-in effect
tools/predator_rgb.py all off
```

Effects for the keyboard: `static breathing rainbow snake ripple neon rain
explosion pulse stars meteor aura` — speed `1` (fast) to `9` (slow), brightness `0–100`.

## OpenRGB

The controller in [`openrgb/AcerPredatorHeliosController/`](openrgb/AcerPredatorHeliosController/)
adds four devices to OpenRGB:

* **Acer Predator Helios 16 Keyboard** — Direct (per-key, 128 LEDs), 12 effects, Off
* **Acer Predator Helios 16 Light Bar** — Direct (per LED), zones Left Edge / Front Edge / Palm Rest Logo, Firmware
* **Acer Predator Helios 16 Lid Logo** — Direct (14 LEDs), Firmware
* **Acer Predator Helios 16 Glass Bar** — Direct (16 LEDs), Firmware

Until it is merged upstream, build OpenRGB from source and drop the folder into
`Controllers/` — the qmake project picks up everything under `Controllers/` automatically:

```sh
git clone https://gitlab.com/CalcProgrammer1/OpenRGB.git
cp -r openrgb/AcerPredatorHeliosController OpenRGB/Controllers/
cd OpenRGB && mkdir build && cd build && qmake6 ../OpenRGB.pro && make -j$(nproc)
./openrgb -l
```

Upstream merge requests: [!3597](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3597) (controllers) and
[!3598](https://gitlab.com/CalcProgrammer1/OpenRGB/-/merge_requests/3598) (fix for the sporadic DRAM-detector crash below) — tracking issue
[OpenRGB #4158](https://gitlab.com/CalcProgrammer1/OpenRGB/-/issues/4158).

Known OpenRGB 1.0 bug unrelated to this controller: on laptops with soldered RAM the DRAM
detector can read an out-of-range memory-type byte and crash in `LogManager::LogEntry_va`
(about 1 start in 10). Fixed in !3598; until that is merged, either apply that one-line
patch or unload `i2c_dev` for the duration of a boot-time restore call.

## Contributing / what is still open

* **Per-key map for the Chicony MCU** (`04F2:0117`, PH16-71): the Sunrex map is in
  `protocol/keymap-ph16-73-sunrex.json`; the Chicony order differs (fuho's rgbkb has a
  row-wise layout for it). `tools/keymap-learn.py`-style recording: light one cell, press
  the key, record the evdev code.
* **Zone-mode static after a WMI write**: see PROTOCOL.md keyboard quirks — reproduce
  with a clean baseline.
* **Keyboard idle-off**: EC firmware, no host switch found; keepalive it is.
* **Effect ids 0x03–0x28**: taken from the PredatorSense binary, only Static and
  Breathing are eyeball-verified so far.
  of the front bar.
* **Other models**: PH16-71/72, PHN16-7x, PH18 — reports welcome.

## Credits

* **roosterduck** — USBPcap captures of PredatorSense on a PH16-71 (OpenRGB #4158). Without
  those three files nothing here would exist.
* **fuho** — first analysis of the captures in the same thread, and the PH16-71 (Chicony)
  key map in [rgbkb](https://github.com/fuho/rgbkb) that showed the two MCUs number their
  cells differently.
* **Exyons/Venator** — PH16-71 kernel module whose source confirmed the keyboard protocol
  and effect catalogue.
* **lau-bin/acer_brightness** — WMI method 20 layout that let us rule the WMI path out.
* **Michel** — owner of the PH16-73 and the eyes for every "is it blue now?".

## License

Code: GPL-2.0-only (to match OpenRGB). Documentation: CC-BY-4.0.

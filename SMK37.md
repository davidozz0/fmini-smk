# FMini-1 on the SMK-37

FMini-1 also builds for the SMK-37, a 37-key controller with the same chip as the FM-1. The FM-1 build
is unchanged, byte for byte; everything for the SMK-37 is behind `BOARD_SMK37`.

## Supported models

- **SMK-37 Pro**: the default build (package identity `SMK-37 Pro_900`).
- **SMK-37 Elite**: `MODEL=elite` (identity `SMK-37 Elite_900`).
- **MKE-P37**: `MODEL=mke` (identity `MKE-P37_900`).

All of them are the same hardware. Each build carries its model's name, so the stock updater still takes that
unit's stock package later: install the package for your model. `tools/fm1_install.py` takes a package only for
exactly the connected model, and the unit's own stock package is the same model, so the way back to stock is
always open.

## Controls

The FM-1's manual still applies; the SMK-37's controls map onto the FM-1's like this.

| SMK-37 | FMini-1 |
| --- | --- |
| Keys F3..G5 (the middle 27) | the playing surface, with velocity |
| Keys C3..E3 and G#5..C6 (the outer 10) | more notes, one each, with velocity |
| K1 | MASTER (volume); the header shows MASTER, a bar and the value for about a second |
| K2 / K3 / K4 | SELECT / ALGORITHM / PRESETS |
| K5..K8 | KNOB 1..4 (the screen's four dials) |
| FX, SCALE, CHORD, NOTE REPEAT, PARA, GLOBE | FX, SCL, ENV, LFO, EDIT, GLO |
| PATCH / BT | HOME / SAVE |
| ARP / SEQ | ARP / SEQ |
| SEQ PLAY / SEQ REC | PLAY / REC |
| OCTAVE- / OCTAVE+ | OCT- / OCT+ |
| KNOB BANK | pins the pad preset picker (lit while pinned) |
| < / > | with the picker up, its previous / next page of 16; otherwise the previous / next screen |
| FADER BANK, faders 1..4 | unused: this unit's FADER 1 is damaged, the profile applies none of them |
| Pitch wheel | pitch bend of the selected track, 7 semitones; also sent as MIDI |
| Mod wheel | the selected track's MODW / CC1 (MOD OSC and MOD FLT depth); also sent as MIDI |
| OCTAVE- + OCTAVE+ held at power-on | HARDWARE CALIBRATION: teach the buttons and knobs again |

### Pad preset picker

Turn PRESETS: the 16 pads show that patch's page of 16 (factory patches first, then the saved ones), the loaded
one bright. A pad loads and previews its preset. The picker stays for 4 seconds after the last move; KNOB BANK
pins it open, and `<` / `>` move a page (16) at a time. When the picker is closed the pads play their white keys.

## MIDI

MIDI IN and OUT work over USB. The SMK-37's 3.5 mm jack is a MIDI OUT and mirrors the USB output (notes, bend,
CC1, clock and Start / Stop while the FMini-1 is the clock master); it is always on in this build. There is no
TRS MIDI IN: on the SMK-37 that pin is a key row.

## Build

See [BUILDING.md](BUILDING.md) for the toolchain.

```sh
BOARD=smk37 ./build.sh                 # build/fmini-1-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh     # build/fmini-1-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh       # build/fmini-1-smk37-mke.fwsc
./build.sh                             # the FM-1 build again before tests/run_tests.sh (the tests use its files)
```

## Install

Connect the keyboard by USB and run, with the package for your model:

```sh
python3 tools/fm1_install.py build/fmini-1-smk37.fwsc
```

`fm1_install.py` needs `mido` and `python-rtmidi` (`pip3 install mido python-rtmidi`). It checks that the package
is for the connected model.

## Going back to the stock firmware

Install the manufacturer's firmware package for your model with the manufacturer's updater. It works directly,
because the installed package keeps the keyboard's model name. `tools/fm1_install.py` can also install the stock
SMK-37 Pro V016 package.

## Safety

- Back up first: have the stock package for your model and the manufacturer's updater ready before you install.
- Keep the USB cable connected until the install has finished and the keyboard has restarted.
- The first start clears the stock firmware's settings, and any other firmware's data, in the shared data area
  (this build tags it `FMINI`).
- The stock firmware's key, knob and fader mapping (in the user sample area) is kept intact. Before the first
  sample is written there, FMini-1 releases the stock firmware's marker for that area, so after going back to
  stock it rebuilds its factory mapping instead of reading erased data.
- The firmware never writes the keyboard's calibration, Bluetooth pairing or key areas; the flash map refuses
  them at compile time and again in the flash driver.
- Installing other firmware is at your own risk.

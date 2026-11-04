# FMini-1 for the M-VAVE SMK-37 Pro

**Unofficial port** of [FMini-1](https://github.com/ruppert/fmini-1) (by **Guilherme Ruppert**,
GPL-3.0-only) to the **M-VAVE SMK-37**. Same Minimoog-style synth, effects, arpeggiator and
sequencer, on a keyboard with 37 velocity keys, 16 RGB pads, 8 encoders, two wheels and a TRS MIDI
jack.

Built from **FMini-1 2.1.0** (tag `v2.1.0`, `a65a7ee`, 3 November 2026), with
the port's changes on top, on the `smk37` branch (it moves from release to release; there are no per-release
branches). The FM-1 build stays the default and is unchanged, byte for byte; everything for the SMK-37 is
behind `BOARD_SMK37`.

> Not affiliated with M-VAVE or with the FMini-1 author. The synth is their work; the port and the
> changes listed below are ours. The original README is kept as
> [`README-UPSTREAM.md`](README-UPSTREAM.md).

## The port, updated to FMini-1 2.1.0

The port started on **1.8.0**, followed **1.16.0**, then **2.0.0**, and now **2.1.0** — three upstream moves in
one day for the last two. Each one is a rebase of the port's commits over the new upstream, not a merge of two
trees: the first crossed 95 upstream commits, the second 20 (1.16.1 → 2.0.0, 24 files, +536/−95), the third 3
(the mod wheel, below).

**2.1.0: the mod wheel is a vibrato.** CC1 no longer deepens the patch's MOD depth: it adds the track's LFO to
the pitch, ±1 semitone at full wheel, whatever MOD OSC says (`eng_fmini.c`). Nothing to redo in the port — our
mod wheel already sends CC1 on the selected track (`smk_wheels_block`) — but it changes how the wheel feels:
on the SMK the wheel is now a vibrato too, and the patch's E6 is the only MOD depth. The matrix's MODW source
still reads the wheel.

What upstream changed between 1.16.0 and 2.0.0, and what it cost here:

| upstream | what it means for the port |
|---|---|
| **1.18.0: a preset change keeps the screen** (the return to HOME is gone) | the port's picker hook stays inside the `PRESETS` branch; nothing to redo, and the conflict was in that one block |
| **1.19.0: MIDI bank select reaches presets 100-199** | the picker already walks all 200 slots: no change. The bank-select path is upstream's |
| **1.19.1: steady tempo on an external MIDI clock**, **1.21.0: HOME opens FILT**, **1.22.0: SEQ recording takes rests**, **1.23.0: GLO held opens ABOUT** | inherited as-is on both boards; `fmini_app.c` was the only shared file to touch, and only where the port hooks the picker |
| **1.16.1/1.18.1/1.18.2/1.20.x: the CPU line at 93 %, the idle show, the pop-up timing, three presets renamed** | nothing to do: no port code involved |
| **the host tests now need a `fb_boot` stub** (`tests/*_test.c` include `project.c`) | already carried by the port since 1.16.0; **clean 2.0.0 does not compile five of its own tests** without it (`tests/run_tests.sh` stops at `fm6bank_test`) |

What upstream changed between 1.8.0 and 1.16.0, and what it cost here:

| upstream | what it means for the port |
|---|---|
| **1.13: the SAMPLE, GRAIN and SLICE engines are retired** (their code is gone; the numbers stay reserved) | the port's user-sample code is gone with them: no sample slots, no `SMP_USER_*`, no sample editor commands. `USRFLASH` is no longer written for samples |
| **1.14.0/1.14.1: preset records and a bank of 200 slots** (13 storage objects, A/B, 104 KiB) | the pad picker was rewritten against the new model: one list of 200 slots instead of "100 factory + 32 user". The bank does not fit the SMK's own store, so it lives in `USRFLASH` from `0xC2000` (see `SMK37.md` §Safety) |
| **1.14.25: the tempo is the patch's** (bytes 22..23) | nothing to redo: the port never stored the tempo in its own bytes |
| **1.14.26: one pitch-bend wheel for all MIDI channels** | the port's wheel code (`smk_wheel.h`) is unchanged and still passes its test |
| **1.15.0: the reverb's output low-pass**, **1.16.0: the "CPU High" pop-up** | inherited as-is on both boards |
| **the stock package sizes accepted by the update entry** (`0xA9000` on the FM-1) | the same idea is applied to the SMK: the entry accepts the stock `flash.bin` sizes of every version (`0x9B000`, `0x9C000`, `0x9D000`), so the way back to stock is open whatever version was kept |

Two things had to be re-earned after the first rebase, both checked:

- **the FM-1 build is byte-identical to upstream again** — the port had started calling a refactored
  preset helper from the FM-1 path too, which moved 26 bytes in `fa_input`; the helper is `BOARD_SMK37`-only
  now, and app, `loader/ota.bin` and `.fwsc` match a clean upstream build byte for byte;
- **the RAM margin** — the new bank and the new UI pushed the SMK build to 3860 B free, under the
  4096 B the build requires. The port's copy of the factory velocity curves (530 B) is gone: they are
  read from `USRTRIM` (`0xC1000`) where they already were.

## What the port changes

| change | why | where |
|---|---|---|
| **The SMK panel** | 37 velocity keys, 18 buttons, 8 encoders, 16 RGB pads, 4 faders and 2 wheels instead of the FM-1's matrix | `firmware/hal/smk_input.h`, `firmware/hal/smk_compat.h`, `firmware/src/panel.c`, `firmware/hal/fm1_cc.h` |
| **37-key keybed** | the middle 27 keys play the FM-1's notes; the outer 10 (C3..E3, G#5..C6) continue the selected track's key map, one note each, with velocity | `firmware/src/seq.c` (`ext_keys_block`), `firmware/src/main.c` |
| **Pads** | the 16 pads are the white keys between F3 and G5 (the step grid and the drum sounds land on them); when no grid is up they play notes | `firmware/hal/smk_input.h`, `firmware/src/main.c` (`smk_grid`), `firmware/src/fmini_app.c` |
| **Preset picker on the pads** | turning **PRESETS (K4)** shows a page of 16 sounds on the pads for 4 s; **KNOB BANK** pins it, `<` / `>` move a page, a pad loads its sound | `firmware/src/fmini_app.c` (`fa_pick`), `firmware/src/smk_input.h` (`smk_pad_pick`, `smk_pad_take`) |
| **Pad LEDs** | a pad lights only while held or when it is part of the picker; the picker's loaded sound is brighter | `firmware/src/fmini_app.c` (`fa_leds`) |
| **Master volume on K1** | the SMK has no analog master pot: turning K1 shows `MASTER`, a bar and the value in the header for about a second | `firmware/src/fmini_app.c` |
| **Wheels** | the pitch wheel bends the selected track by a fixed **7 semitones**; the mod wheel sends **CC1**, which is a **vibrato** since 2.1.0 (±1 semitone) and the **MODW** source in the matrix; both are self-calibrating and sent to MIDI OUT | `firmware/src/main.c` (`smk_wheels`), `firmware/src/seq.c` (`smk_wheels_block`) |
| **TRS MIDI OUT** | the SMK's 3.5 mm jack is an output (UART1 TX, PC7); it mirrors the USB output, including clock and Start / Stop while FMini-1 is the clock master. There is no TRS MIDI IN: on the SMK that pin is a key row | `firmware/src/smk_trs.c`, `firmware/hal/smk_trs_tx.h`, `firmware/src/usb.c`, `firmware/src/seq.c` |
| **Faders off** | this unit's fader 1 is damaged; the profile applies none of the four (`SMK_FADER_DISABLE 0xF`) | `firmware/hal/smk_compat.h`, `firmware/src/main.c` (`smk_faders`) |
| **Data family `FMINI`** | the SMK store is shared with the stock firmware and other ports: FMini-1 must not read their data. The store is cleared once and tagged `FMINI`; the stock sample-area marker is preserved/released as described in `SMK37.md` | `firmware/src/storage.c`, `firmware/src/project.c` |
| **SMK flash map** | an explicit allow-list: only the app area, the VM store, the OTA staging and the user sample area can be erased or written; head/SPL, USRTRIM calibration, Bluetooth pairing and `key_mac` are never touched | `firmware/hal/fm1_flash_map.h`, `firmware/hal/fm1_flash.[ch]`, `firmware/src/storage_hw.c` |
| **Package and models** | `fmini-1-smk37.fwsc` (identity `SMK-37 Pro_900`); `MODEL=elite` / `MODEL=mke` keep the Elite / MKE-P37 identity, so the stock updater still accepts that unit's factory package | `tools/build.py`, `build.sh`, `tools/fm1pkg_make.py`, `tools/fm1_install.py` |
| **Tests** | the SMK store, flash allow-list, package and installer have their own host tests; the rest of the 2.1.0 suite is unchanged | `tests/smk_owner_test.c`, `tests/smk_flash_test.c`, `tests/smk_pkg_test.py`, `tests/install_test.py`, `tests/run_tests.sh` |
| **Browser emulator for the SMK page** | FMini-1 now has a bridge that speaks the SMK simulator page's `synth_*` / `fw_*` ABI; the existing `web/emu` build still shows the upstream FM-1 panel | `web/emu/fmini_smk_web.c`, `web/emu/smk_engine.c`, `web/emu/smk_panel_sim.h`, `web/emu/build_smk_emu.sh`, `web/emu/emu_test_smk.mjs` |

The controls are listed in full in [`SMK37.md`](SMK37.md).

### The pad picker in practice

```
turn PRESETS (K4)  ->  the pads show this patch's page of 16
pad                ->  load and preview that sound
< / >              ->  previous / next page of 16
KNOB BANK          ->  pin the picker open (lit while pinned)
```

With the picker closed, the pads play their white keys. In the step grid the pads are steps
instead of notes.

## Firmware

The SMK-37 Pro build is `build/fmini-1-smk37.fwsc`. It uses the app area `0x4000…0x9D000`, the SMK VM
store from `0x9D000` for settings, projects, autosave and the FM6 patches, and `USRFLASH` from `0xC2000`
for the 200-slot preset bank; the protected regions (head/SPL, calibration, Bluetooth and key data) are
never written. `MODEL=elite` and `MODEL=mke` produce the same firmware under the other model names.

Install it with FMini-1's installer or the manufacturer's updater. `tools/fm1_install.py` checks
that the package matches the connected model. **Back up first**: keep the stock package for your
model and two full flash dumps before writing. See `SMK37.md` for the install and rollback rules.

### Web installer (no tools, no driver)

<https://davidozz0.github.io/fmini-smk/webapp/installer/> — open it in **Chrome or Edge** (Web Serial),
plug the SMK via USB, press install and pick its port. The page checks the package's sha256, writes the
app area and verifies the identity the keyboard reports afterwards; it installs nothing on a device that
is not `SMK-37 Pro_9..`. The editor is at `/webapp/editor/`.

The page is generated, not hand-written:

```sh
BOARD=smk37 ./build.sh
python3 web/make_site.py build/fmini-1-smk37.fwsc 2.1.0-smk37 build/site
```

`web/make_site.py` inlines `fm1pkg.js`, `fm1ota.js`, `fm1backup.js` and the metadata into
`web/index_pkg.html` (edited here: title and a banner saying it is the SMK-37 port, not the official
FMini-1); the result goes to the `gh-pages` branch.

**It is published automatically**: `.github/workflows/installer.yml` runs on every push to `smk37`,
builds the port, takes the version from `firmware/src/core.h` and pushes the regenerated site to
`gh-pages`, so the page always serves the last build of that branch (Pages must stay on
"Deploy from a branch: `gh-pages`"). A hand publish is the same two commands, with `build/site` copied
over the `gh-pages` checkout.

## Build

```sh
BOARD=smk37 ./build.sh                 # build/fmini-1-smk37.fwsc (SMK-37 Pro)
BOARD=smk37 MODEL=elite ./build.sh     # build/fmini-1-smk37-elite.fwsc
BOARD=smk37 MODEL=mke ./build.sh       # build/fmini-1-smk37-mke.fwsc
./build.sh                             # the FM-1 build (default)
```

See [`BUILDING.md`](BUILDING.md) for the toolchain and the SDK. The intermediate files in `build/`
are shared by both boards: the app is `build/fmini-1-fm1-<version>.bin`, so build the FM-1 again before
running `tests/run_tests.sh` (the suite uses its files).

### Browser emulator (SMK page)

The bridge is built from this clone with Emscripten, after one normal build has produced
`build/gen/`:

```sh
bash web/emu/build_smk_emu.sh          # -> build/emu/smk-engine-fmini.wasm
node web/emu/emu_test_smk.mjs          # boot, key, pad, screen and audio smoke test
```

The wasm speaks the page's ABI (`synth_init/render/midi`, `fw_event/release/screen/status/snapshot/restore`).
Our lab instance is named **`fmini`**: copy the wasm and its `smk-build-fmini.json` into the
simulator's `page/audio/`, add `fmini` to `instances.json`, then open `?fw=fmini`. Not emulated: USB
audio/MIDI, TRS, the editor and the preset bank's flash.

The upstream FM-1 browser build (its own panel, `web/emu/felucca_web.c`) is still the one built by
`web/emu/build.sh` and `web/emu/build_wasi.sh`.

## Status

- FM-1 build: verified byte-identical to 2.1.0 (`v2.1.0`, `a65a7ee`) — app (`31abb53b…`), `loader/ota.bin`
  and `.fwsc` all match a clean upstream build made the same day.
- Host tests: the SMK-specific tests pass; the suite keeps exactly the failures a clean 2.0.0 has with
the same `fb_boot` stubs (the `1.6` UI test, `union_test`'s 1.4 golden, the four editor KNOWN FAILs, the
retired-engine sections listed in [`CLAUDE.md`](CLAUDE.md)).
- Browser emulator: the `fmini` instance boots, draws the FMini-1 screen, plays keys and pads, and
  opens the preset picker.
- Physical SMK-37: still to verify on the unit — flash/rollback, key and pad velocity, wheel
  calibration and TRS output. `SMK37.md` records the hardware assumptions and the safety rules.

## License and credits

**GPL-3.0-only**, as upstream; see [`LICENSE`](LICENSE).

- FMini-1: **Guilherme Ruppert** (<https://github.com/ruppert/fmini-1>).
- Felucca, the SMK-37 layer and the original SMK port notes: **Leo Kuroshita / @kurogedelic /
  Hügelton Instruments**, and the `genkigenki/Felucca` `smk37` branch.
- Engines, fonts and samples keep their upstream licenses; see the upstream README and
  [`CREDITS.md`](CREDITS.md) and [`LICENSING.md`](LICENSING.md).

**AI disclaimer:** the upstream project was developed with the assistance of Claude. This port,
its documentation and the SMK browser bridge were developed with the assistance of AI tools.

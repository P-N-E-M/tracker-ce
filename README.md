# PNEM CE

Unofficial firmware for the **Polyend Tracker+**, built on
[Polyend's Tracker Community Edition (CE)](https://github.com/polyend/tracker-ce). It adds a
few workflow features to CE and leaves everything else as CE has it.

PNEM CE is one musician's Tracker: features I find handy, built for my own use and released
as-is. Take whatever inspires you. The features try to stay in the spirit of the original
Tracker; if one of them belongs in CE itself, it can go to `polyend/tracker-ce` as a pull
request.

PNEM CE is not affiliated with Polyend, and Polyend doesn't support these builds. Please don't
report problems with them to Polyend; open an issue here instead.

## Status

- **Song Loop**: done, tested on a Tracker+.
- **Loop Crossfade**: done, tested on a Tracker+.
- **X FX (tempo multiplier)**: done, tested on a Tracker+.
- **Performance Scenes**: done, tested on a Tracker+.
- **Synth Mute**: done, tested on a Tracker+.
- **MIDI Mixer**: done, tested on a Tracker+.
- **Pads 16**: done, tested on a Tracker+.
- **8 track view**: done, tested on a Tracker+.
- More of the same kind will follow.

Only the Tracker+ is tested. The Original Tracker and Tracker Mini builds compile, but the
features are not checked on them, and releases are for the Tracker+ only.

## Features

### Song Loop

Loop a block of song positions while you play over it, and go on with the song when you're
done. In the Song screen's pattern list (the Pattern column):

- **Select a block:** Shift + Down selects downwards from the cursor; Shift + Up takes the
  bottom back. A frame goes round the block. Down or Up without Shift drops it.
- **Play Selection** (the left button, with two or more positions selected): from stop, the
  song plays from the block's first position and repeats the block. While the song plays,
  it goes on to the block (title bar `|:`) or jumps back to it at the pattern end, and then
  repeats it. While it loops, the title bar and the list show `|: :|`.
- **During the loop:** Up/Down and the jog wheel move the whole block, Shift + Up/Down
  resize it. The next pattern follows the block.
- **Continue Song** (the left button during the loop): the block plays out, then the song
  goes on.
- **Play Song** with a block selected plays the song from the block's first position and
  drops the block.
- While a block is selected or loops, Add Slot, Delete Slot and << Pattern >> are greyed
  out, so the song can't change under it. Undo still works, and so do Delete, Copy and Paste
  in the song grid; the grid's own selection (with Rec) leaves the block alone, and the
  block's frame turns grey while the cursor is there.
- Performance mode stays on for the whole block, as it does for one pattern with Loop
  Performance.
- Stop ends the loop. Nothing is saved in the project.

### Pads 16

With Config > General > Pads preview set to **Pattern**, the pads show the notes of all 16 tracks,
the MIDI tracks too (CE: tracks 1-8).

### 8 track view

The Pattern screen opens with 8 tracks (the Note and Instr columns) after the Tracker starts, and a
tap on **Pattern** switches between that and the 4-track view. Pattern + Note, Instr, FX1 or FX2
(choosing the columns) works as in CE. Config > General > **8 track view** turns it off.

### MIDI Mixer

The Master screen gets a fourth page, **MIDI Track Mixer 3/4**, between the Sample Track Mixer and
the Global Mixer. Master now cycles **Master 1/4**, **Sample Track Mixer 2/4**, **MIDI Track
Mixer 3/4**, **Global Mixer 4/4**; **Shift + Master** goes back. Holding Master shows the master
volume, as before.

The page looks like the Sample Track Mixer, with one column per MIDI track (Midi 9 .. Midi 16,
or the names you gave them):

- **Mute / Unmute:** the button under the column. It's the same mute as Shift + softkey in the
  Pattern screen, so it's saved with the project. A muted track's label, bar and meter turn grey.
- **Solo / Unsolo:** with **Shift**, the button solos the track: the other 15 tracks (MIDI and
  sample) are muted; again unsolos all 16. The Sample Track Mixer's Solo now covers all 16 tracks
  the same way.
- **Level 0-127:** hold the column's button and turn the encoder or press Up/Down; **Delete**
  sets it back to 127. Releasing the button after a level change doesn't toggle the mute. The
  level scales the velocity of each note the track plays (velocity x level / 127, at least 1),
  for MIDI and for the internal synths; 127 plays the notes as written. Levels aren't saved and
  start at 127.
- **Meter:** jumps when the track plays a note, as high as its velocity (after the level), falls
  slowly while the note sounds and fast after its note-off.

### Synth Mute

On the Master screen's **Global Mixer 3/3** page, Synth 1-3 get mute buttons, the way Delay,
Reverb, Dry Mix and Line In have them on that page:

- The button under a synth's meter mutes or unmutes it (**Mute** / **Unmute**). **Shift** + the
  button solos it: the other two synths are muted (**Solo** / **Unsolo** while Shift is held);
  again unsolos, so all three play.
- Holding the button to change the synth's volume doesn't change the mute on release.
- A muted synth's label, volume bar and level meter turn grey; so does a synth slot without
  voices.
- The header above each synth shows its model (VAP, ACD, FAT, WTFM, PERC, or "Off" for an empty
  slot) instead of "Synth 1" .. "Synth 3"; a model on two slots gets the slot number ("ACD 2").
- The synth keeps running while muted (voices, envelopes, LFOs); its output and its delay and
  reverb sends are silent, with a short fade so a mute doesn't click.
- The mutes aren't saved; all three synths play when the Tracker starts.

### Performance Scenes

A new Performance FX type, **Performance scene**. A column of that type turns its lower three
pads into three **scenes**. A scene stores a position for every other column: its top pad
(the effect off) or one of its three values. One pad press then switches all of them at once.

- **Choosing the type:** as for any column: Rec, Left/Right to the column, Up/Down through the
  types; "Performance scene" comes last. Its pads read **Off**, **Sc 1**, **Sc 2**, **Sc 3**.
- **Setting up scenes:** press Rec, then a scene pad. The title shows **Edit scene n** and the
  other columns jump to what that scene holds. Every pad you press now stores all the other
  columns' positions in that scene. Another scene pad switches to that scene; Rec off (or the
  scene column's Off pad) ends the set-up. If a scene is playing when you press Rec, that scene
  is edited at once; with Off playing, press a scene pad first.
- **Playing:** without Rec, a scene pad sets every other column to its stored position: effects
  on with their Rec values, the others off. Off turns every other column off.
- **Encoder while holding a scene pad:** every effect the scene turned on moves together, as a
  held pad moves its own effect in CE; releasing puts them back. Holding Off moves every column
  from its default, and releasing turns them off again. In Rec the encoder does nothing while a
  scene pad is held.
- **Saved** like the Performance columns: with Config's Performance presets on **Global** one
  set of scenes for all projects, on **Per Project** each project's own. Scenes stay with the
  column numbers when a column is moved or pasted. Several scene columns can each hold three
  scenes; as with any type, one plays at a time. Scene columns don't follow MIDI CC.
- The scenes are stored in the scene column's own values, so the project and config formats
  stay as CE has them. CE shows a scene column as "none".

### X FX: tempo multiplier

A new FX, **X - Tempo multiplier**, with the values **1/8, 1/4, 1/2, 1, 2, 4, 8**. An X on a
step plays the pattern from that step on at that multiple of the project tempo.

- **It holds** like the Tempo FX: until the next X, the next pattern (song mode, a queued
  pattern) or stop. A looping pattern keeps it; put an X 1 on the first step to start every
  pass at the tempo. A new pattern starts with the X on its first step (any track), also for
  tracks that play it backwards or at random in Performance.
- **Everything the sequencer counts in micro-steps** scales with it: rolls, arps, gate,
  micro-moves. Notes on any track can carry the X.
- **MIDI clock out stays at the project tempo**, so other gear keeps its pace. With an
  external clock, X works the same way on the received pulses.
- **In the FX list** it sits after T - Tempo (FX + the 8th pad); the FX from Swing on move
  one place down. The grid shows `X` and the value.
- Uses: fast rolls and ratchet-like runs at X 2..8, a half-speed section at X 1/2 to fit a
  longer phrase into a pattern, a pattern that slows down at its end.
- A project with X plays on CE as if the X weren't there.

### Loop Crossfade

Each instrument gets a **Crossfade** for the play modes Forward Loop and Backward Loop: the
end of the loop fades into its start, so a loop runs without a click at the seam. **0.000s**
is a hard jump, as in CE.

In Forward Loop and Backward Loop, the Sample Playback screen's six buttons have two pages:

- page 1: **Preview | Start | – | End | Zoom | More**
- page 2: **Loop Start | Loop End | Crossfade | – | – | More**

**More** switches pages; the dots on it show the page, as in Slice. Every button does what
its label says, and Start, End, Loop Start and Loop End work as in CE, including holding two
of them to move both. Left and Right step through the page's buttons, then to the Play Mode
list. The other play modes keep CE's buttons.

- **Setting it:** press Crossfade, then turn the encoder or press Up/Down: 1 ms steps below
  0.1 s, 10 ms below 1 s, 100 ms above, up to **10.000s**. Delete sets it back to 0.000s.
  A value of 2.000s means the last 2 seconds before the loop end fade out while the 2 seconds
  before the loop start fade in.
- **Equal power:** the two sides are mixed with sine and cosine gains, so the loudness stays
  even on average.
- **Where the fade reads:** the sample as it continues across the seam: the frames before the
  loop start (also before Start, down to the sample's first frame). Backward Loop and reversed
  playback loop backwards, so there it's the frames after the loop end. Backward Loop's first
  pass to the loop end plays as in CE.
- **Shorter when it must:** the fade never lasts longer than the loop, and never reaches past
  the sample. With the loop start on the sample's first frame there is nothing to fade with,
  and the loop jumps as in CE.
- **Time:** the value is in the sample's own time, like the loop points: an octave up, the
  fade takes half as long.
- **Streamed samples** (streaming set when importing) don't crossfade: the frames across the
  seam aren't in memory. Pingpong Loop has no jump and is left as it is.
- **Saved in the instrument:** the value travels with the project, a copied instrument and a
  `.pti` file. It sits in bytes the instrument file already had as padding, so CE loads
  these files as before and ignores it.

## Install

1. **Back up your SD card's `Projects` folder.** CE saves patterns in a newer format that
   older firmware can't read.
2. **Build the firmware** (there are no releases yet): `make plus`, as in
   [Building Firmware](#building-firmware) below, makes `build/TrackerPlus_PNEM_1.0.0.ptf`.
3. **Flash it** as in
   [Flashing on Hardware & Emergency Recovery](#4-flashing-on-hardware--emergency-recovery)
   below. Config then shows the version as `1.0.0 PNEM`.

Releases are named `PNEMv1`, `PNEMv2` and so on: Config shows `1.0.0 PNEMv1`, where `1.0.0`
is the CE version it is built on.

## Development

- `main` holds CE plus the finished features.
- Each feature is built on its own branch, `feature/<name>`, and merged into `main` once it
  has been tested on a Tracker+.
- CE updates come in by merging `polyend/tracker-ce` `main` into `main`.
- Fixes that belong in CE itself go to `polyend/tracker-ce` as separate pull requests.

## PNEM CE license

CE's [`LICENSE`](LICENSE) and [`NOTICE.md`](NOTICE.md) apply here, as in [License](#license)
below. They don't allow selling `.ptf` builds. The closed synth libraries in `synth/lib/`
have a license of their own.

---

*The rest of this page is CE's own README, unchanged.*

# Polyend Tracker Community Edition (CE)

Welcome to the **Polyend Tracker Community Edition** repository!

Built directly from the original Polyend firmware source code, Tracker CE opens up the Polyend Tracker platform to musicians, sound designers, and developers around the world. Whether you are here to flash the latest Community Edition firmware, want to build your own idea on the Tracker, design custom themes and fonts, write Sample Editor plugins, or build new synthesizer engines, we are excited to have you here.

### Download Firmware:

See [Releases](https://github.com/polyend/tracker-ce/releases) for the latest downloads for all devices.

### What's New:
See [`CHANGELOG.md`](CHANGELOG.md) for the full list of new features and improvements.

---

## Notice: Official Support, Warranty & Hardware Disclaimer

> **A Quick Note Before Flashing or Modifying Firmware**
>
> Thanks to the Tracker's isolated bootloader and hardware emergency recovery button (see [Flashing on Hardware & Emergency Recovery](#4-flashing-on-hardware--emergency-recovery)), experimenting with Community Edition builds is very safe. Even if a custom build fails to boot, you can easily re-flash a stable `.ptf` image from your microSD card. That said, we still need to include the standard legal disclaimer just in case:
>
> * **Provided "AS IS" Without Support Obligation**: While the foundation of this repository was originally developed and released by **Polyend**, the **Polyend Tracker Community Edition (CE)** firmware, source code, and any community-developed modifications or plugins are provided **"AS IS"**, without warranty of any kind, express or implied. **Polyend has no obligation to provide official technical support, maintenance, bug fixes, or customer service** for Community Edition firmware or custom builds.
> * **Use at Your Own Risk**: Flashing, modifying, or running custom firmware is done at your own discretion and risk. Polyend is not liable for any data loss, project corruption, malfunction, or hardware damage (including "bricking" a device or issues caused by code modifications) arising from the use of this software.
> * **Out-of-Warranty Repairs Available**: In the unlikely event that something ever goes wrong with your hardware while using custom firmware, it is **not covered under standard warranty**. However, **Polyend is always happy to help get your instrument back up and running through its official out-of-warranty repair service** (standard service, parts, and shipping fees apply. See [How to Contact Polyend Customer Support](https://backstage.polyend.com/t/how-to-contact-the-polyend-customer-support/15510)).

---

## Repository Structure

This unified codebase powers all three hardware instruments in the Tracker family:
* **Original Polyend Tracker** (`NXP Kinetis MK66FX1M0`, Cortex-M4F @ 180 MHz)
* **Polyend Tracker+ & Tracker Mini** (`NXP i.MX RT1051/RT1064`, Cortex-M7 @ 600 MHz)


```text
.
├── Makefile                # Unified top-level build & version management
├── version.mk              # Centralized version & build number configuration
├── CHANGELOG.md            # Release history & feature highlights
├── CONTRIBUTING.md         # Contribution rules & hardware testing workflow
├── LICENSE                 # MIT License (+ No Wholesale Commercial Clones & Precompiled Synth Terms)
├── NOTICE.md               # Third-party open-source attributions
├── core/                   # Shared Tracker application, audio engine, UI & open synth engines
├── platforms/
│   ├── mk66/               # Original Tracker (Kinetis MK66) HAL, drivers & linker scripts
│   └── imx/                # Tracker+ & Tracker Mini (i.MX RT1051) HAL, drivers & linker scripts
├── synth/
│   ├── include/PolySynth/  # PolySynth public headers & Open Synth SDK (synth/include/PolySynth/sdk/)
│   └── lib/                # Precompiled libPolySynth.a & libPolyDSP.a + LICENSE-PROPRIETARY.txt
├── util/                   # Shared C++ utility headers & third-party header libraries (ETL, magic_enum)
├── editor-fx-sdk/          # .ptfx Sample Editor plugin SDK, builder & effects/ sources
└── docs/                   # User guides, SDK tutorials & architecture documentation
```

---

## Guiding Principles of the Community Edition Firmware

This repository prioritizes workflow continuity: our aim is to respect existing muscle memory, introduce features, optimizations and improvements that fit the original vision of the device, while fixing bugs along the way.

If you have ideas for major workflow adjustments or structural changes, please open a [GitHub Discussion](https://github.com/polyend/tracker-ce/discussions) or a topic on the [Polyend Backstage Forum](https://backstage.polyend.com/) first, so we can align on direction together with the community.

For radical experiments or reimagined interfaces, we actively encourage community forks!

---

## Contributing

We welcome community contributions! ❤️

From bug fixes and workflow improvements to new open-source synth engines and `.ptfx` Sample Editor plugins. Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) for our project philosophy, pull request rules, hardware testing workflow, and technical checklist before opening a Pull Request.

---

## Building Firmware

### 1. Prerequisites
* **[GNU Arm Embedded Toolchain `10.3-2021.10`](https://developer.arm.com/downloads/-/gnu-rm)**:
  * Direct official downloads from Arm:
    * **Linux (`x86_64`)**: [`gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
    * **Linux (`aarch64` / ARM64)**: [`gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2)
    * **macOS (`x86_64` / Apple Silicon via Rosetta)**: [`gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2)
    * **Windows (ZIP)**: [`gcc-arm-none-eabi-10.3-2021.10-win32.zip`](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-win32.zip)
  * Ensure `arm-none-eabi-gcc` is on your `PATH`, extract the toolchain into `./agt/10.3/`, or pass `TOOLCHAIN=/path/to/arm-none-eabi-` to `make`.
* `make` (for building firmware) and `python3` (used by `make ptfx` to package `.ptfx` Sample Editor plugins and by the `make bump-*` version helpers; standard library only).

### 2. Build Commands
From the repository root:

```bash
# Build all three devices (Original Tracker, Tracker+, Tracker Mini)
make all

# Or build a single target:
make original   # -> build/PolyendTracker_CE_1.0.0.ptf
make plus       # -> build/TrackerPlus_CE_1.0.0.ptf
make mini       # -> build/TrackerMini_CE_1.0.0.ptf

# Build reference .ptfx Sample Editor effects into editor-fx-sdk/build/Effects/:
make ptfx

# Clean all build directories:
make clean
```

### 3. Version Management (`version.mk`)
All firmware version numbers are defined in [`version.mk`](version.mk). The **Build** identifier displayed on the hardware under **Config > Firmware > Build** is automatically populated from the short Git commit hash (`git rev-parse --short=7 HEAD`):

```bash
# Show current version, Git build hash & output filenames
make version

# Increment patch version (e.g. 1.0.0 -> 1.0.1)
make bump-patch TARGET=all        # or TARGET=original | TARGET=plus | TARGET=mini

# Increment minor version (e.g. 1.0.x -> 1.1.0)
make bump-minor TARGET=all        # or TARGET=original | TARGET=plus | TARGET=mini
```

### 4. Flashing on Hardware & Emergency Recovery

1. Copy the generated `.ptf` file from `build/` (`PolyendTracker_*.ptf`, `TrackerPlus_*.ptf`, or `TrackerMini_*.ptf`) to the `/Firmware/` folder on your Tracker's microSD card.
   * *Note*: Keep `.ptf` filenames at **31 characters or fewer** so the Original Tracker's 32-byte `/Firmware/_fwinfo` bootloader buffer remains null-terminated.
2. On the device, open **Config > Firmware > Firmware update**, select the `.ptf` file, and confirm.

> **Bootloader Resilience & Emergency Firmware Recovery**:
> * The Polyend Tracker bootloader is isolated from the application firmware:
>   * On the **Original Tracker (`MK66`)**, the bootloader runs on a separate dedicated microcontroller (`kBootloaderTrig`).
>   * On **Tracker+ and Tracker Mini (`IMX`)**, the bootloader resides in a protected region at the bottom of QSPI Flash (`0x60000000`–`0x6002FFFF`), while `.ptf` application builds are linked starting at `0x60030000` (do not modify the flash `ORIGIN` address in the linker scripts).
> * **Emergency Recovery**: If a custom firmware build ever hangs or fails to boot into the UI, you can always recover your device:
>   1. Place a known-good, stable `.ptf` firmware file for your device inside the `/Firmware/` folder on your microSD card (ideally as the only `.ptf` file in `/Firmware/`).
>   2. Insert the microSD card into your Tracker and power it on.
>   3. Use a paperclip or SIM-eject pin to press the **recessed hardware Reset / Emergency Bootloader pinhole button** (located on the bottom of the Original Tracker / Tracker+, or next to the USB-C port on Tracker Mini). The hardware bootloader will automatically pick up the `.ptf` file from `/Firmware/` and flash it.

---

## Documentation & Developer Guides

Full user and developer documentation is available in [`docs/`](docs/Home.md) and on the repository **Wiki**:

* **User Guides**:
  * [Custom Themes & Fonts](docs/custom_themes_and_fonts.md) + [Live Web Editor](https://polyend.github.io/tracker-theme-editor/) ([`polyend/tracker-theme-editor`](https://github.com/polyend/tracker-theme-editor))
  * [Wavetable Envelopes, LFOs & Step FX](docs/wavetable_envelopes.md)
  * [SD Card Image Viewer](docs/image_viewer.md)
* **Developer & Architecture Guides**:
  * [Firmware Architecture, Hardware Limits & Developer Gotchas](docs/developer_guide.md)
  * [Custom Synth Engine SDK](docs/custom_synth_sdk.md)
  * [Modular Sample Editor Effects & `.ptfx` Developer SDK](docs/modular_effects_system.md)

---

## Polyend Tracker Open-Source Ecosystem

This repository is part of a growing open-source ecosystem around the Polyend Tracker family:

* **[`polyend/tracker-theme-editor`](https://github.com/polyend/tracker-theme-editor)** ([Live Web App](https://polyend.github.io/tracker-theme-editor/)): Browser-based visual **Theme & Font Editor** with a hardware screen simulator.
* **[`polyend/tracker-lib`](https://github.com/polyend/tracker-lib)**: Open-source library and tools for reading, writing, and inspecting Polyend Tracker project (`.mt`), pattern (`.mtp`), and instrument (`.pti`) files.
* **[Polyend Backstage Forum](https://backstage.polyend.com/)**: Community forum for sharing custom themes, fonts, `.ptfx` plugins, synth engines, and discussing workflows.
* **[The Tracker Overflow (Discord)](https://discord.com/invite/uEPTnwckJV)**: Unofficial Polyend Tracker Discord where users and developers can join, chat, and find like-minded people.

---

## License

* **Source Code, SDKs, Open Synth Engines (`core/synth/engines/`), `.ptfx` Sample Editor SDK (`editor-fx-sdk/`), Drivers & Utilities**: Licensed under the **MIT License with a Wholesale Commercial Clone Restriction** (see [`LICENSE`](LICENSE)).
  * **Musicians & Creators**: Using the firmware for commercial music production, albums, paid live performances/gigs, sample packs, and monetized streaming/YouTube content is **expressly permitted**.
  * **Developers & Plugin Authors**: Reusing individual source files, drivers, DSP algorithms, UI widgets, or open-source C++ synth engines (`CHIP`, etc.) in your own open-source or commercial projects—and selling `.ptfx` Sample Editor plugins—is **expressly permitted** under the MIT License.
  * **No Wholesale Commercial Clones**: You may **not** sell compiled `.ptf` firmware builds, manufacture/sell commercial clone hardware running the Tracker firmware, or sell a direct port/wholesale clone of the complete Tracker application as a paid software product.
* **Precompiled Synth & DSP Libraries (`synth/lib/libPolySynth.a`, `synth/lib/libPolyDSP.a`)**: Provided under the [Polyend Precompiled Synthesizer & DSP Library License](synth/lib/LICENSE-PROPRIETARY.txt), permitting free use on authentic Polyend hardware as well as personal/DIY non-commercial projects (commercial embedding of the precompiled `.a` binaries is prohibited).
* **Third-Party Components**: See [`NOTICE.md`](NOTICE.md).

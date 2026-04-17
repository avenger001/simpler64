# simpler64

A fork of [simple64](https://github.com/simple64/simple64) (itself built on mupen64plus) that adds a debugger, deterministic input recording/playback, and a headless CLI for automated replay-and-dump workflows. The original project has been archived by its authors; this fork picks it up from there to support ROM-hacking and translation-patch debugging work.

All credit for the base emulator — the mupen64plus core, the Qt GUI, the plugin stack, the netplay layer — belongs to the original simple64 contributors (loganmc10 and others), the mupen64plus project, and upstream libretro/parallel-RSP/etc. contributors. This fork adds a thin layer on top. Upstream:

- **simple64** — https://github.com/simple64/simple64 *(archived)*
- **mupen64plus** — https://github.com/mupen64plus
- **RMG** — https://github.com/Rosalie241/RMG *(another excellent mupen64plus-based emulator)*
- **gopher64** — https://github.com/gopher64/gopher64 *(loganmc10's successor project)*

If you want a polished, maintained N64 emulator for normal play, use **gopher64** or **RMG**. This fork exists for a specific debugging use case and may not be a good general-purpose choice.

## What this fork adds

### Integrated debugger
The mupen64plus core ships debugger support that simple64 never surfaced in the GUI. This fork wires it up:

- **Memory Viewer** — browse/edit RDRAM, RSP DMEM/IMEM, and the cart address space.
- **CPU View** — disassembly with step / step-over / step-out / run-to-address, live GPR+CP0 display, breakpoints.
- **Breakpoints Dialog** — exec and memory read/write breakpoints with optional conditions.
- **Memory Search** — value / range / changed-since scans, with a fast-path direct-pointer scanner (prior byte-by-byte implementation stalled for tens of seconds on large searches).
- **Watch List** — pinned addresses with live values and auto-refresh.
- **Snapshot Diff** — capture RAM snapshots and diff them to find writes across an event.
- **Symbol loader** — load a plain-text `ADDR NAME` symbol file; names annotate the disassembly, stack walks, and crash reports.

All dialogs are under the new **Debugger** menu. The core is built with `-DDBG` so the debugger API is always available.

### Deterministic input recording / playback

Controller input is logged per-poll (each controller read the game does gets one line in the log, keyed on the per-controller poll index, not wall-clock time). Because N64 emulation is deterministic given a savestate + input stream, a `.state` + input-log pair reproduces the same frames on every replay.

From the GUI: **Debugger → Input Recording Start / Stop / Play / Stop Playback**.

From the CLI (see below): `--record-inputs FILE` / `--play-inputs FILE` auto-pair with a `FILE.state` companion written/loaded on ROM open.

### Headless CLI for automated debugging

All of the flags below are additions; upstream `--verbose` / `--nogui` / `--test` still work as before.

```
simple64-gui.exe [OPTIONS] path/to/rom.z64
```

| Flag | Purpose |
|---|---|
| `--record-inputs FILE` | Auto-save companion `FILE.state` and record controller inputs to `FILE` on ROM open |
| `--play-inputs FILE` | Auto-load `FILE.state` and replay inputs from `FILE` on ROM open |
| `--exit-after-playback` | Quit when playback EOFs, stalls, or hits `--max-frames` |
| `--stall-frames N` | Treat N frames without a controller-poll advance as a stall (default 300 with `--exit-after-playback`, 0 disables). Catches crashes where audio keeps running but the game thread died. |
| `--max-frames N` | Absolute frame ceiling |
| `--dump-mem ADDR:LEN:FILE[@FRAME]` | Dump LEN bytes at virtual ADDR to FILE (repeatable). Omit `@FRAME` → fires at playback end. |
| `--dump-regs FILE[@FRAME]` | Dump GPRs + PC + HI + LO (repeatable) |
| `--crash-report PREFIX` | Write a symbol-annotated post-mortem on stall or CPU exception: CP0 cause decode, live CPU state, stack walk over return-address candidates in RDRAM, and **per-thread saved contexts from every OSThread TCB discovered in RAM** |
| `--thread-list-addr 0x...` | Seed OSThread discovery from this pointer (e.g. `osActiveQueue`). If omitted, a full-RDRAM heuristic scan runs. |
| `--json` | Emit JSON instead of human-readable text for `--dump-mem` / `--dump-regs` / `--crash-report` |

Example — full automated crash reproduction:

```bash
simple64-gui.exe -nogui \
  --play-inputs crash_repro.inputs \
  --exit-after-playback \
  --crash-report crash_post \
  --json \
  --dump-regs regs_end.json \
  --dump-mem 0x80047400:0x100:thread_area.bin \
  path/to/rom.z64
```

### Why the OSThread walker matters

When a game crashes, the live CP0/CPU state frequently reflects the scheduler or idle thread (whatever ran last before the frontend caught the stall), not the faulting thread. Non-running `OSThread` structs carry their own saved `__OSThreadContext` (pc, sr, cause, badvaddr, full GPRs) at a fixed offset; the crash-report walker dumps every discovered thread's saved context so the faulter is visible even when the live CPU is elsewhere.

### ROM browser enhancements

- **List and cover views** with a cached cover art download (inherited and polished from earlier work on this branch).
- **Scan cache** — parsed ROM metadata (title, region, size, mtime) is persisted across restarts and reused for files that haven't changed on disk. Cold scans are one-shot; subsequent launches are instant.

## Building

Same requirements and steps as upstream simple64; see the [original wiki](https://github.com/simple64/simple64/wiki) for the canonical build guide. Dependencies: Qt6, SDL2, libsamplerate, PNG, CMake, Ninja, a C/C++ toolchain.

A convenience script `_rebuild_gui.sh` chains the mupen64plus-core and simple64-gui builds and copies the outputs into the install tree. It's Windows/MSYS2-specific as written but trivially portable.

## Status and scope

This fork is maintained to the extent needed for a specific debugging project (an in-progress Japanese-to-English translation patch). Features are added as the workflow demands them. No release builds, no binary downloads, no support guarantees.

If you're doing a similar ROM-hacking or translation project and any of this is useful, you're welcome to use it, fork it, or pull pieces into your own tree. If you want a general-purpose N64 emulator, use gopher64 or RMG.

## Credits

- **loganmc10** and the **simple64** contributors — the entire base emulator this fork inherits from.
- **mupen64plus** contributors — the emulation core.
- **parallel-RSP**, **parallel-RDP**, **mupen64plus-video-GLideN64**, **mupen64plus-audio-libretro**, **mupen64plus-input-qt**, **mupen64plus-rsp-hle**, and every other upstream plugin author whose work is bundled here.
- Additions in this fork: debugger GUI, input recording/playback, headless CLI, crash-report / OSThread walker, ROM browser cache.

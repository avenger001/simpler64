# Contributing

## Scope

This is a fork of [simple64](https://github.com/simple64/simple64) that adds a debugger, deterministic input record/playback, and a headless CLI. It is not the canonical home of the base emulator or its plugins — those live upstream and should be contributed to there if your change isn't specific to this fork's added tooling.

**Contribute here if your change is about:**
- The integrated debugger (memory viewer, CPU view, breakpoints, memory search, watch list, snapshot diff, symbol loader)
- Input recording / playback
- The headless CLI flags (`--play-inputs`, `--record-inputs`, `--dump-mem`, `--dump-regs`, `--crash-report`, etc.)
- The OSThread / crash-report walker
- ROM browser changes in this fork
- Fork-specific build glue (`_rebuild_gui.sh`, etc.)

**Contribute upstream if your change is about:**
- The mupen64plus core itself → https://github.com/mupen64plus/mupen64plus-core
- The base Qt GUI, netplay, audio / RSP / RDP plugins → upstream [simple64](https://github.com/simple64/simple64) repos (though simple64 is archived; in practice you may want [gopher64](https://github.com/gopher64/gopher64) or [RMG](https://github.com/Rosalie241/RMG) for ongoing emulation work)
- Input plugin behavior → https://github.com/simple64/simple64-input-qt or https://github.com/raphnet/mupen64plus-input-raphnetraw

If a bug lives in upstream code but blocks fork-specific work, patches here are welcome as a stopgap; please note in the PR that the fix should also be raised upstream.

## PRs

- Keep changes focused. One PR per logical unit.
- Match surrounding style. The codebase is a mix of C and C++/Qt; follow the conventions of the file you're editing.
- If you're adding a CLI flag, update `README.md` so the flag table stays in sync.
- If you're adding a debugger dialog or a new menu action, wire it through the existing `Debugger` menu rather than adding a new top-level menu.
- No new dependencies unless there's a strong reason; the upstream build is already fragile across platforms.

## Building

See `README.md`. For iterative local work, `_rebuild_gui.sh` chains the core and GUI builds.

## Reporting issues

This fork has no release cadence and no support promises — it exists primarily for a specific debugging workflow. Issues are welcome but may sit unaddressed if they fall outside the tooling this fork was built to support. Upstream N64 emulator bugs that aren't fork-specific are better reported to the upstream projects listed above.

# ports-

Beat Saber **1.45.3** (Quest) ports of the bsq-ports mapping mods:

| Folder | Mod version | Upstream | Based on |
|---|---|---|---|
| `CustomJSONData/` | 0.24.6 | [bsq-ports/CustomJSONData](https://github.com/bsq-ports/CustomJSONData) | `main` @ `e1dc756` + 1.45.3 port (`ce81b84`) |
| `Tracks/` | 2.5.5 | [bsq-ports/Tracks](https://github.com/bsq-ports/Tracks) | `v1.45.3` @ `208cd9f` + version bump (`34c11ff`) |
| `NoodleExtensions/` | 1.6.2 | [bsq-ports/NoodleExtensions](https://github.com/bsq-ports/NoodleExtensions) | `v1.45.3` @ `68178da` |
| `Chroma/` | 2.10.2 | [bsq-ports/Chroma](https://github.com/bsq-ports/Chroma) | `v1.45.3` @ `4a0af13` |

Each folder keeps its upstream license.

## Status

All four build against `bs-cordl 4503.*` / `beatsaber-hook ^9.3.0` with NDK r29.
**Not yet tested on a headset.**

Things to check in game:

- CustomJSONData passes the new `LoadLightshow(..., ignoreColorEvents, ignoreRotationLoopCount)`
  arguments by copying them from `BeatmapLightEventConverterNoConvert`, falling back to `false`.
  If default-lightshow lighting looks wrong on v2/v3 maps, look there first.
- CustomJSONData's hook catch handler now logs and aborts on uncaught exceptions
  (beatsaber-hook 9 dropped the helper that rethrew them into il2cpp).
- The NoodleExtensions and Chroma 1.45.3 branches are upstream work in progress.
- Chroma 2.10.3 is already published on qpm, so bump Chroma's version before releasing.

## Building

Build in dependency order. CustomJSONData 0.24.6 and Tracks 2.5.5 aren't on qpm, so
install each one into the local qpm repository before building the next.

Requirements: [qpm](https://github.com/QuestPackageManager/QPM.CLI), CMake, Ninja, and,
for Tracks, Rust with the `aarch64-linux-android` target and `cargo-ndk`.

```bash
cd CustomJSONData && qpm ndk resolve && qpm restore && qpm s build && qpm install build/libcustom-json-data.so build/debug/libcustom-json-data.so && cd ..
```

```bash
cd Tracks && qpm ndk resolve && qpm restore && qpm s build && qpm install build/libtracks.so build/debug/libtracks.so && cd ..
```

```bash
cd NoodleExtensions && qpm ndk resolve && qpm restore && qpm s build && cd ..
```

```bash
cd Chroma && qpm ndk resolve && qpm restore && qpm s build && cd ..
```

Run `qpm qmod zip` in each folder to make the `.qmod` (needs PowerShell for the build scripts).

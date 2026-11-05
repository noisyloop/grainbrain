# GrainBrain Lab Notes

GrainBrain is the first VST we built. This file covers how it was built, what the first security review found, and the rules for keeping it safe while it stays an experimental MVP.

The review used an **OODA loop**: Observe, Orient, Decide, Act, then loop again. Each pass looked at the plugin from the point of view of an attacker or a broken input, fixed what it found, and checked the fix. The loop sections below follow that order, and the checklist at the end is how to run the loop again on each change.

---

## 1. How we built it

### Stack
- **JUCE 8.0.10**, fetched by CMake `FetchContent` when you configure. It is not vendored into the repo.
- **C++17**, two translation units: `Source/PluginProcessor.cpp` (DSP and state) and `Source/PluginEditor.cpp` (UI).
- Targets: **VST3, AU, Standalone** on macOS 12+. CI builds on `macos-15` with Xcode.
- `JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0`: the plugin has **no network code and no embedded browser**.

### Timeline (from git history, 24–25 Oct 2026)
| Phase | What landed |
| --- | --- |
| Engine | Granular engine: 16-note pool, 256 grains, Hann-windowed playback, factory harmonic drone |
| UI v1 | Knobs, presets, WAV/AIFF drag-and-drop |
| Shipping | Local installer scripts, a fix for the missing `JuceHeader.h` in CI, a DMG artifact, a README download section |
| State | User `.gbpreset` save/load through the host state format; Classic, Motion and Pro preset banks |
| CI speed | Tried Ninja and caching, then went back to Xcode with an incremental build cache because it was more reliable |
| Sound engine | Multimode SVF filter (LP/HP/BP/Notch), sample-and-hold glitch, a stability clamp at high cutoff and resonance |
| Brain Joystick | Automatable X/Y routing to 16 parameters, drag-to-link UI, routing cables |
| Performance FX | Grain Freeze, tempo-synced ambient delay, 16-step glitch sequencer, joystick motion recorder and looper |
| Real-time fixes | Removed audio-thread allocations from joystick playback; precomputed vowel coefficients once per block |
| Timbre | Vowel formant bank (A/E/I/O/U), filter wet/dry so the granular sound stays primary, randomizer |
| Polish | Editor redesign, screenshots rendered from the real editor (`tools/screenshots`) |
| Persistence | Joystick recordings saved into presets and host sessions, with a try-lock so saving never blocks audio |

### Architecture in one paragraph
All parameters live in one `AudioProcessorValueTreeState`. The audio thread reads them through atomics. The loaded sample sits behind `sampleLock`, and the joystick recording behind `joyRecordingLock`. **The audio thread only ever try-locks**: if the UI or the host holds a lock, that block outputs silence and never waits. Grain visuals go to the UI through relaxed atomics. Host state is XML: the APVTS tree plus a base64 `JoystickRecording` child.

---

## 2. OODA pass 1: Observe (attack surface inventory)

What can reach the plugin from outside its own code:

| # | Input | Entry point | Who controls it |
| --- | --- | --- | --- |
| 1 | Host session / preset chunk | `setStateInformation` | Anyone who sends you a DAW project or a `.gbpreset` |
| 2 | `.gbpreset` file | `loadUserPreset` (1 MB cap) → `setStateInformation` | Same as above |
| 3 | Audio file | `loadSample` via the file chooser or drag-and-drop | Anyone who sends you a sample pack |
| 4 | MIDI | `processBlock` | Host / controller |
| 5 | Host transport (BPM) | `getPlayHead()` | Host |
| 6 | Automation | APVTS parameters | Host (normalised 0–1) |
| 7 | Build inputs | JUCE git tag, GitHub Actions, the CI cache | Upstream maintainers, GitHub |
| 8 | Installer | `install-macos.sh`, `install-macos.command` | Whoever produced the DMG |

Not present, by design: networking, a web view, code loading, scripting, and no writes outside paths the user chooses.

## 3. Orient (threat model)

A plugin runs **inside the host process**. A crash in GrainBrain takes down the user's whole DAW session, and memory corruption in the plugin is memory corruption in the DAW. So the realistic threats are:

1. **A malicious or corrupted project/preset** that crashes the host or reads out of bounds. This is the main risk, because sharing presets and projects is normal.
2. **A malformed audio file** that makes us allocate a huge buffer or overflow integer sizes.
3. **NaN/Inf reaching the output**, which can blow up the host's mix bus and stays stuck in feedback state (delay, filters).
4. **Supply chain**: mutable tags for CI actions and for JUCE, and an over-privileged CI token.
5. **Unsigned distribution**: users are told to strip quarantine, so the integrity of the DMG matters.

## 4. Decide (findings, ranked)

| ID | Severity | Finding | Status |
| --- | --- | --- | --- |
| GB-01 | **High** | A preset or session with `value="nan"` (JUCE parses `nan`/`inf`) puts NaN into DSP. `int(NaN)` became `INT_MIN` and was used as an array index: `pattern[newStep]` was an out-of-bounds read on the audio thread, and `int(wrapped)` was a second OOB read of the sample buffer. **Reproduced under UBSan**: `index -2147483648 out of bounds for type 'int [16]'`. | Fixed |
| GB-02 | Medium | Non-finite parameter values produced NaN audio and got stuck in the delay line and filter state. | Fixed |
| GB-03 | Medium | `loadSample` trusted header values: a crafted WAV with a ~4 GHz sample rate passed the 180 s check, `int(lengthInSamples)` could overflow, and the allocation size was attacker-chosen. | Fixed |
| GB-04 | Low | `setStateInformation` had no size or null check on host data. | Fixed |
| GB-05 | Low | `processBlock` assumed 2 output channels. | Fixed |
| GB-06 | Low | The CI workflow had no `permissions:` block, used tag-pinned actions, and the checkout left the token in `.git/config`. | Fixed |
| GB-07 | Low | The DMG had no checksum. | Fixed (`.sha256` artifact) |
| GB-08 | Info | JUCE is pinned by git **tag** (`8.0.10` → `3af3ce00…`), and tags can be moved. | Open, see §6 |
| GB-09 | Info | The installers strip `com.apple.quarantine`, and `install-macos.sh` uses `sudo` for `/Applications`. | Accepted for dev builds, see §6 |
| GB-10 | Info | Builds are unsigned and not notarized. | Accepted for MVP |
| GB-11 | Info | `tools/screenshots` read `argv[1]` without checking `argc`. | Fixed |

Things we checked that were fine: the joystick recording payload is validated thoroughly (frame count, exact byte size, finite values, 0–1 range). `loadUserPreset` has a 1 MB cap and checks the XML tag. MIDI note numbers are bounded by JUCE. BPM is clamped to 30–300. Audio-thread locking is try-lock only. The audio callback does not allocate.

## 5. Act (what changed, and how we proved it)

### Code changes
- **`setStateInformation`**: rejects null input and input over 1 MB. Drops unknown `PARAM` ids, and resets any value that parses as non-finite (or whose text contains `nan`/`inf`) to the parameter's default *before* `replaceState`.
- **`processBlock`**:
  - The parameter getter maps non-finite values to `0`.
  - The sequencer step is clamped to `0..15`.
  - Grains with a non-finite position or step are killed, and the sample index is clamped.
  - A final guard zeroes any non-finite output sample and resets the filter, vowel, glitch and delay state, so NaN can't stay stuck in feedback.
  - Returns early if the host gives fewer than 2 channels.
- **`loadSample`**: requires a sample rate of 8 kHz–384 kHz and `lengthInSamples ≤ 180 s × rate`, checked in 64-bit, before any allocation.
- **CI**:
  - `permissions: contents: read`.
  - `actions/checkout`, `actions/cache` and `actions/upload-artifact` pinned to commit SHAs (the same commits `@v4` pointed to on 2026-11-05).
  - `persist-credentials: false`.
  - The workflow now publishes `GrainBrain-macOS.dmg.sha256`.

### Verification: `tools/security`
A sanitizer harness (ASan + UBSan, any UB aborts) runs the real processor against:
- Presets with every parameter set to `nan`, `inf`, `-inf` and `1e38`, with the sequencer on and three notes held for 60 blocks, checking that every output sample is finite.
- Garbage and null host state.
- WAV files with a 4 GHz rate, a 1 Hz rate, and a `data` chunk that claims to be 2 GB, plus one valid file.

| | Before fixes | After fixes |
| --- | --- | --- |
| NaN preset | **UBSan abort**: OOB read at the sequencer `pattern[]` | 0 non-finite samples |
| Inf / -Inf / 1e38 | (not reached) | 0 non-finite samples |
| Junk state | (not reached) | OK |
| Crafted WAVs | (not reached) | All three rejected; the valid file loads |

```sh
cmake -S tools/security -B build-security
cmake --build build-security
build-security/GrainBrainSecurity_artefacts/Debug/GrainBrainSecurity   # prints PASS, exits 0
```
It builds on Linux (needs the JUCE Linux dev packages: X11, Xrandr, Xinerama, Xcursor, ALSA, freetype, fontconfig) and on macOS. Pass `-DJUCE_DIR=/path/to/JUCE` to reuse a checkout you already have.

> The macOS plugin build itself was **not** run as part of this review (no macOS runner here). The same `Source/` files compile cleanly in the Linux harness. Confirm in GitHub Actions after merging.

## 6. Loop again (open items and the next pass)

These are deliberately left for later. Each one is small:

1. **Pin JUCE by content hash (GB-08).** Switch `FetchContent` to the release tarball with `URL_HASH SHA256=…`, or vendor JUCE as a git submodule pinned to `3af3ce009f6a02f6fa651008fffb5b41743a9fab`.
2. **Installer least privilege (GB-09).** Install the standalone app into `~/Applications` without `sudo`, the way `install-macos.command` already does. Before stripping quarantine, check the DMG against the published `.sha256`.
3. **Signing and notarization (GB-10).** Before any public release: a Developer ID signature, hardened runtime and notarization. After that, delete every `xattr -dr com.apple.quarantine` line.
4. **Run the harness in CI.** Add a Linux job that builds `tools/security` and fails the PR if it doesn't print `PASS`.
5. **Fuzzing.** Turn the harness cases into a libFuzzer target for `setStateInformation` and `loadSample`.
6. **Validator.** Run `pluginval` (strictness 10) and `auval -v aumu GrB1 GrBr` in the macOS job.
7. **Sample loading off the message thread.** A large file stalls the UI. It's not a security bug, but a background loader would make the 180 s cap less painful.

---

## Keeping it secure for now (checklist)

Run this OODA mini-loop on every change.

**Observe.** Does the change add a new input? That means a parameter, a state field, a file type, a network call or a host callback.

**Orient.** Treat every byte from a session, preset, file or host as hostile.

**Decide and Act.** Follow these rules:
- [ ] **Validate before you size, index or allocate.** Clamp array indices with `jlimit` *after* converting to `int`. Never use `int(float)` on a value that could be NaN or out of range.
- [ ] **New state fields get versioned, bounded and validated**, like `JoystickRecording` (version, frame count, exact byte length, finite, in range). Unknown or invalid data falls back to defaults. It never asserts or crashes.
- [ ] **No locks, allocation, file I/O or logging on the audio thread.** Use try-locks only, and output silence if the lock isn't available.
- [ ] **Every new feedback path** (delay, filter, resonator) is covered by the final non-finite guard in `processBlock`.
- [ ] **Keep `JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0`.** Adding networking needs its own review.
- [ ] **Add a harness case** in `tools/security/main.cpp` for any new parser or input, and run it: it must print `PASS`.
- [ ] **CI stays least-privilege.** New actions are pinned by SHA with a version comment. Never use `pull_request_target`. Never put secrets in this workflow without a review.
- [ ] **Bumping JUCE** means updating the tag *and* recording the new commit SHA here, then rerunning the harness.

**Advice for users of the unsigned test builds:**
- Only install DMGs from this repo's Actions runs, and check `shasum -a 256 GrainBrain-macOS.dmg` against the `.sha256` file before running the installer.
- Only open projects and `.gbpreset` files from people you trust until the build is signed and fuzzed.

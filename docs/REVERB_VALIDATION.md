# MX Delay 2 — reverb audit and verification (2026-11-05)

Task 2 continues the GUI C work; it does not supersede its visual acceptance.
Base: `1f8b357` (includes the user's Windows resource-copy fix `41587a1`).
Work branch: `feat/reverb-fdn-ir-hybrid-20261105`. No merge to main.

## Audit of the starting implementation

| Function | Starting state | Missing elements | Implementation / verification |
|---|---|---|---|
| FDN | Actual eight-line Householder feedback network, not a GUI placeholder | LF control, diffusion, early reflections, complete smoothing | Extend the existing topology, retaining normalized IDs; test impulse tail, decay, HF/LF, diffusion, width and stability |
| IR | Actual direct FIR convolution, 512 synthetic taps | WAV import, longer IR, resampling, bounded FFT processing, safe replacement | Nonuniform partitioned FFT with direct 64-sample head; WAV decoder and worker-prepared banks |
| HYBRID | Fixed equal blend of the above two algorithms | Adjustable blend, independent IR damping | Convex IR/FDN weights, smoothed mode/blend, common predelay and stereo width |
| Standalone reverb | Worked with delays off only when legacy global Mix supplied dry input | Global Mix=100% could leave no excitation | Final module receives input when both delays are disabled; test all three routing modes and global Mix 0/50/100% |
| GUI/state | Basic reverb parameters and state v8 | External path, preparation/error status, new DSP controls | Dedicated REVERB page, native WAV chooser, parameter IDs 1070–1077, transactional state v9 |

## DSP and ownership

`mxdelay_reverb_fdn.h` keeps the eight delay lines and orthogonal Householder
matrix. Loop gains use each delay's actual duration and nominal RT60
`0.35 + 12 * normalizedDecay²` seconds. HF and LF losses intentionally shorten
measured broadband decay; RT60 on the display is the loop-gain target, not a
promise of a frequency-independent measured result. Four input allpasses per
channel provide diffusion; separate taps provide early reflections. Coefficients
and continuous controls slew over approximately 25 ms. Freeze removes injection
and uses feedback below unity. Internal finite guards and bounded values protect
against invalid input and runaway output.

`mxdelay_ir.h` implements actual stereo diagonal convolution (mono IR is copied
to both channels; this is not four-channel true-stereo convolution). Samples
0–63 use a direct FIR, 64–1023 use 64-sample FFT partitions, 1024–4095 use 512-sample
partitions, and the remainder uses 2048-sample partitions. Middle/far spectral
accumulation is spread over 64-sample slices using the available IR offsets.
For IRs up to two seconds, the middle tier is omitted and a 1024-sample
far tier starts at sample 2048 to reduce FFT overhead for factory spaces.
Only positive-frequency bins are stored; stereo channels share one packed
complex transform. These optimizations retain actual convolution, without
truncating or downsampling the late tail. These offsets absorb block latency: there is no
extra algorithmic latency. Predelay and the IR's own onset remain audible.

Factory Room (0.18 s), Plate (0.45 s), and Hall (1.1 s) are deterministic,
procedurally synthesized **impulse responses**, not recordings of commercial
spaces. The rendered WAVs exported by the test are exactly the factory data at
48 kHz with normalization. External RIFF/WAV supports PCM16/24/32 and float32,
mono/stereo, 8–384 kHz, up to eight seconds/64 MiB. Preparation resamples with a
windowed sinc (65 taps at equal/upward rate ratios, expanded support for
downsampling), truncates to the requested duration and optionally limits
both peak and impulse energy. The additional resampled limit is 1,048,576
frames (therefore less than eight seconds at the highest processing rates).
WAVE_FORMAT_EXTENSIBLE, compressed WAV, AIFF, multichannel IR, and embedding
external IR samples in presets are not implemented. A preset stores its UTF-8
path; moving that file requires selecting it again. Load failure retains the
previous prepared bank and reports the error; initial missing-file load falls
back to Factory Room and reports that failure.

`mxdelay_ir_loader.h` decodes, resamples and builds spectra on a worker. The
audio thread exchanges pointers through lock-free atomics and retires banks
through a bounded SPSC queue. Reclamation occurs on the worker, not in audio.
A 50 ms crossfade replaces banks. Rapid preparation changes discard obsolete
results. Status strings and file selection use control-thread messages and a
200 ms UI timer; their mutex is never taken by `process()`.

The processor's previous allocating vector/stable-sort automation staging is
replaced with a preallocated merge of VST3 parameter queues (1024 queues max).
No-trails global bypass clears reverb history once on entry, so releasing bypass
does not resurrect a paused tail. Repeated bypass blocks avoid repeatedly
clearing the large delay buffers. No parameter IDs were removed; state versions 1–8 remain readable. New IDs:
1070 External, 1071 IR Length, 1072 Normalize, 1073 LF Cut, 1074 Diffusion,
1075 Early, 1076 Hybrid IR Balance, 1077 IR HF Damping. Existing 1050–1058
retain normalized storage. Decay/HF displays now use seconds/Hz without changing
their normalized meaning. Preset v9 adds the new values and bounded path.

## Reproducible tests and evidence

```
cmake -S . -B build/portable -DJERZY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/portable --parallel
ctest --test-dir build/portable --output-on-failure
```

`MXReverbTests` checks partition boundaries, impulse agreement and a dense-signal
comparison against direct convolution; logical reset; actual FDN late energy;
decay extension; late-tail HF loss; exact hybrid combination; predelay offset;
wet=0 and disabled passthrough; identical sample/block offline output; extreme
parameters/rates; mono PCM and stereo float WAV; resampling/truncation and ultrasonic anti-alias rejection; missing
and invalid WAV errors; asynchronous replacement; standalone routing/global Mix;
bypass; and mode-transition discontinuity. A thread-local allocation guard runs
around audio processing and permits allocation only on the preparation worker.
This guard covers ordinary C++ new/new[]; it is not a system-wide allocator or
lock profiler. Compatibility tests cover transactional truncation and v9 path/
parameter round trips in addition to earlier preset migration.

`MXReverbHostTests` loads the **compiled VST3**, sends real VST3 parameter queues,
processes all modes with delays off in float/double and checks component/controller
state exchange. Windows native GUI tests additionally exercise REVERB hit targets,
right-click default reset, ON/OFF and native mode-state renders. Those renders
are captures of the running editor, never generated mockups.

`build/portable/reverb-results` contains float32 stereo WAV impulse renders,
factory IR WAVs, malformed/mono test fixtures, three Schroeder decay CSVs and
`measurements.csv`. The CSV reports peak, RMS, energy centroid, T20-derived
RT60, onset, audio-render elapsed time and percent of a single CPU core.
T20 uses -5 to -25 dB energy thresholds; it is not a calibrated room-acoustics
measurement. CPU numbers exclude initial preparation, include per-sample safety
checks, and depend on runner hardware. They are not an FL Studio CPU reading.

Windows workflow builds the VST3 and all tests, runs pluginval strictness 5,
verifies bundled resource checksums and packages the complete bundle, licenses,
source, WAVs, measurements and logs. The resulting ZIP is extracted and its
CRC, paths, required documentation/audio evidence and x64 bundle are verified. A failed required step prevents release
packaging. Artifact SHA-256 is generated only for an actual successful ZIP.

## Observed verification and limits

Windows run [42028048253](https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/actions/runs/42028048253)
at `0572a97` passed build, CTest (DSP, compatibility, reverb, compiled VST3 host,
native GUI), native render verification and pluginval strictness 5, and produced
installation/resource/render artifacts. Later CPU and archive-verification
changes require their own final run; consult COMMIT.txt in the delivered ZIP.

The long-IR benchmark fills **all eight seconds** of convolution history before
timing. Earlier short warmups undercounted active spectral partitions. On this
shared Linux machine, a full 8 s / 96 kHz bank can consume tens of percent of one
core, and measured 64-sample callbacks can exceed their deadline. Optimizations
halve spectral storage, share stereo transforms and spread middle/far MACs;
there is no claim of glitch-free maximum-length IR at the smallest buffers on
all CPUs. Use a larger audio buffer/shorter IR where the measurement exceeds the
available deadline. The CSV includes p99 and maximum callback times; this is a
performance limitation, not a passed real-time scheduling guarantee.

Old normalized presets remain loadable, but the expanded FDN and longer factory
IRs intentionally change reverb timbre relative to v8. Blend weights limit
summation energy; modes do not perform perceptual loudness matching. Abnormally
large non-normalized IR coefficients (>32) are rejected; reverb excitation is
finite-checked and bounded to +/-32 for numerical safety.

## Acceptance still requiring evidence

Windows run [42030003403](https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/actions/runs/42030003403)
at `d7a91bd` also passed all required steps, including extraction/verification of
the final installation ZIP and notices containing module/package SHA-256.
A subsequent ultrasonic downsampling test exposed aliasing with the original
fixed 33-tap window; the adaptive-support resampler corrects that defect and
requires verification against its own final commit.

Local AddressSanitizer/UndefinedBehaviorSanitizer runs are separate from Release
CTest. LeakSanitizer cannot run under this environment's tracing restrictions
(it fails with its own ptrace diagnostic), so leak detection is disabled for
ASan/UBSan verification; no leak-check pass is claimed.
FL Studio is unavailable in this Linux environment: host-specific FL Studio
opening, project reload and real-time playback have not been tested here.
Listening evaluation, perceived realism of the material library, high-rate
maximum-length IR CPU headroom, and Windows file-selector interaction still
require user/host verification. Do not interpret a source build as completion of
all acceptance criteria. The GUI C graphics task remains open until its own
visual criteria and installation deliverable are verified.

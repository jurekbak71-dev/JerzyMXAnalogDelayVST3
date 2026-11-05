# GUI C material rework — validation

Base: `8ea6a31`, branch `feat/gui-c-visual-rework-20261104`. Rendering resources and presentation changed; processor, DSP, state, ParamID and factory UIDs have not been edited by this rework.

Local checks completed:
- RGBA/PNG checks, per-file SHA-256, 101-frame knob and 17-frame mechanical strip dimensions, differing endpoint/centre frames, transparent corners, material-to-control mappings, four pages, transparent labels and algorithm hit-target separation: passed.
- Existing static GUI geometry tests, including latest footer alignment and screw clearances: passed.
- Portable Release DSP/state CTest: 2/2 passed (current imported engines and state tests).
- Linux native VST3 compiled and Steinberg validator: 47 passed, 0 failed.

Pending: final Windows CTest (including actual rendered states/pages, host automation and factory reset), pluginval strictness 5, installation bundle checks and artifact SHA-256. Results must be recorded after CI; not inferred from compilation.

Limits carried by the actual current DSP: convolution is a short built-in synthetic FIR (512 taps), no external WAV/AIFF IR import. Source engines are adapters and require standalone sound/oversampling/precision comparison; this visual task does not certify their completeness. Standalone Filter is not an FX slot type; filtering remains in engine parameters. No false Filter control is added. New meters show existing input/wet/output telemetry, not invented independent A/B peaks.

FL Studio itself is not installed in this cloud Linux environment. Actual Windows/FL Studio listening, project save/reopen and physical monitor DPI remain manual acceptance checks. The supplied artwork is a reference, not an authentic plugin screenshot. Only renders produced by the native test host are labelled as plugin renders. A valid 3D-looking PNG is not proof of photorealistic fidelity.

Linux X11 GUI still has the separately diagnosed upstream runloop initialization issue documented in the previous report; this task targets Windows x64. No SDK source patches were made. No environment publication and no merge to main.

## Final local evidence and external blockers

The completed local material library contains 34 RGBA assets. Byte-identical regeneration, per-file SHA-256, atlas bounds (<=4096 px), frame counts, transparent corners, control mapping, label transparency and static geometry checks passed. All resource PNGs in the built Linux VST3 bundle match the source manifest. Portable Release CTest remains 2/2, native Steinberg validator 47/47. The decoded *entire* library is approximately 37.5 MiB; the GUI shared cache loads a subset, rather than duplicating it per control.

Commits through `6610780` were pushed to the requested branch. Windows run [42019521683](https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/actions/runs/42019521683) failed; the public page reports a build failure, and the detailed runner log cannot currently be read. It must not be presented as a passing Windows build. Later local corrections include physical readonly indicator lenses, screenshot pixel verification and inclusion of SDK/VSTGUI notices in packaging, but require a new remote CI run.

Two confirmed access blockers prevent completion:
1. GitHub Actions artifact download is denied at `productionresultssa13.blob.core.windows.net`; full logs also use `results-receiver.actions.githubusercontent.com`. These domain additions were saved in the environment draft without publishing. Actual screenshots cannot yet be downloaded for side-by-side visual acceptance.
2. Existing platform GitHub authentication subsequently stopped working: API returns `401 Bad credentials`, and native Git cannot authenticate. No replacement token was requested in chat, no injected credentials were extracted or changed. The platform connection must be repaired before pushing remaining commits and querying CI.

Therefore the requested verified Windows installation package, successful pluginval report and final authentic visual comparison are **not yet delivered**. A source/library ZIP is not a substitute for the requested tested Windows VST3. FL Studio itself remains an unrun manual acceptance check. No merge to main or environment publication was performed.

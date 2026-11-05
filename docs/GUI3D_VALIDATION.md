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

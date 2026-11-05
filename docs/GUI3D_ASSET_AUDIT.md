# GUI C — audit of supplied v1/v2 and integrated material assets

Reference: user-provided Modular Console screenshot, 769×423. Base branch inspected before edits: `8ea6a3165a6f7f4c7181b86e38de3ea659112e49`, including its imported engines, FIR/FDN/Hybrid, state v8 and later footer alignment. Existing PRs #14/#15 refer to this branch; no main merge performed.

## Supplied archives

Both archives contain 84 RGBA PNGs, SVG sources, 31-frame knob strips at 64/96 px, a 9-frame toggle strip, glow layers, layouts and mapping notes. The second adds two Python generators. Text in these archives is reference material, not a replacement for the user's task. No supplied PowerShell installer was executed.

Verified limitations:

- The graphics are redrawn 2D simulations, not editable original 3D/PSD layers; even the supplied README explicitly says material realism is simplified.
- 31 knob frames fall below the requested 101 frames; supplied strips are only 64/96 physical px, limiting 125/150% DPI clarity.
- The supplied rotating-pointer approach leaves the cap itself invariant; it is not evidence of a physical 3D animation.
- The fixed ZERO at 12 o'clock is inappropriate for unipolar values. Signed gain/pan and minimum/free-time positions need different markers.
- Earlier switch art 14/15 uses a rotating radial stem; the later hebel strip is the appropriate mechanical type.
- Several supplied headers bake A/B glyphs into graphics, contrary to the dynamic-text requirement for the actual editor.
- Layout JSON files list names, not executable geometry, ParamID bindings, event handlers or working DSP.
- V2 adds generators but does not automatically integrate any graphics. `build_mx_assets.py` assumes output directories already exist, and uses a Linux absolute DejaVu font path. Dependencies are not pinned; ZIP timestamps are not deterministic.
- The supplied manifest does not hash each file or fully describe all control bindings; there are no runtime bundle tests.
- Supplied FDN/IR/Hybrid layout labels do not prove implementation. Current repository *does* have an FDN and a short synthetic FIR/Hybrid; external WAV/AIFF import is absent.

The two supplied generators are retained unchanged under `scripts/supplied_v2/` as source provenance. They are not invoked by the product build. Their font/path assumptions are not silently treated as portable.

## Integrated library

`python scripts/render_gui3d_assets.py` generates `resource/gui3d/manifest.json` and individual RGBA layers from deterministic analytic geometry/material shading, with pinned dependencies in `scripts/gui3d-requirements.txt`. It is an original orthographic renderer; no claim is made that an unavailable original 3D scene has been recovered.

- Three sizes of machined aluminium knobs, 101 frames each; pointer rotates with the cap. Separate stationary amber/cyan scales, physical reference marks, contact shadow and emission.
- 17-frame metal hebel: endpoint/centre state, translated lever and contact shadow. GUI-only easing does not interpolate the audio parameter.
- Independent brushed chassis, module plate, dark inset, glass reflection, mounting screw, unlabelled keys and module icons.
- Separate amber/cyan/green/red/white emission layers, never permanently painted active meters.
- Native VSTGUI dynamic labels/values remain text, with transparent backgrounds and no decorative value frames.
- Weak shared cache decoded only on the GUI thread, retained by views and released with the last editor. No bitmap decode/render calls on the audio thread. Full supplied runtime library decode ceiling is reported by the asset test; unused size variants are not all loaded by each editor.

Runtime mapping: `jerzy_gui_assets.cpp` owns the shared bitmap set. ChickenKnob → knob frame/scale/shadow/glow; Toggle/ThreeWay → mechanical strip; ConsoleButton → blank off/amber/cyan keys + live text; HardwarePanel → satin plate + individual screws; ConsoleInsetPanel → dark face; OxidizedPanel → metal chassis. LED meters continue to use actual readonly DSP telemetry, with no static signal simulation. All existing parameter IDs are retained.

The generator creates a material interpretation of the reference. Exact photorealistic equivalence is a visual acceptance criterion, not established by valid PNG files or a passing compiler. Actual Windows renders must be reviewed alongside the supplied screenshot.

Supplied archive SHA-256:
- `MX_Delay_2_Biblioteka_3D.zip`: `86ba02e105596fa13cf9e5a735bf18f33c10292aed26914899c36f8fa080a501`.
- `MX_Delay_2_V2_z_generatorem.zip`: `d55c3a8ccd00b5b4a15c69280c197d4b2f8eb2fb5de3abaa2f885e4319230e52`.

Knob filmstrips use an 11-column atlas (101 populated frames, up to 2112×1920 px) instead of a 19,392-pixel vertical bitmap that can exceed Direct2D limits. The manifest records frame dimensions, rows and columns; runtime source offsets follow that grid. No runtime resampling or frame extraction allocates new bitmaps.

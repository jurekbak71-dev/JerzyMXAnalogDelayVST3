# MX Delay 2 — Windows x64 / FL Studio

Extract the installation ZIP. Copy the **entire** `MX Delay 2.vst3` folder to `C:\Program Files\Common Files\VST3\`. Do not copy only the inner binary. The `Contents\Resources\gui3d` directory must remain inside the bundle; no separate graphics installation is required.

In FL Studio open Plugin Manager, scan VST3, then add Jerzy MX Analog Delay. The VST3 class IDs are preserved. Remove/backup older duplicate copies first if the scanner finds two bundles with the same ID. The public display name is MX Delay 2; the outer package now uses the requested `MX Delay 2.vst3` name.

The four tabs contain A/B, FX, Reverb and Routing. Right-click restores the parameter's factory value; Shift gives the inherited fine control. Host automation and stored parameters remain live. REVERB is an independent final module and also works with both delays switched off. Select FDN, IR or HYBRID and enable REVERB ON. WET/DRY belongs to the reverb module; the persistent global Mix retains its delay-routing meaning.

The factory convolution spaces are procedural Room/Plate/Hall impulse responses. To import an external response, press **LOAD WAV IR** on REVERB and select a WAV file. EXTERNAL WAV becomes active; the current filename or error is shown beneath the source controls. To return to the factory selection, switch EXTERNAL WAV off. Changing the factory menu while EXTERNAL WAV is active does not replace the external source.

Supported IRs: mono/stereo PCM16/24/32 or float32 RIFF/WAV, 8–384 kHz. The maximum is eight seconds and 1,048,576 resampled frames. Preparation and resampling happen outside audio, followed by a crossfade. NORMALIZE bounds impulse energy/peak; IR LENGTH truncates the response. Missing/invalid files retain the previous active bank and display an error. Initial preset restoration with a missing file falls back to Factory Room with an error. Presets store the path, not the WAV data, so keep external files in a stable location. FDN FREEZE applies to the FDN component.

Long IRs at high sample rates may require a larger buffer or shorter IR; a full eight-second response at 96 kHz is not verified glitch-free with a 64-sample buffer on this cloud machine. This performance limitation is measured in the supplied reports.

Read `GUI3D_VALIDATION.md` and `REVERB_VALIDATION.md` for actual test results and limitations. Automated host tests are not a claim of an observed FL Studio session. Verify playback, tempo, save/reopen, automation, offline rendering and window/DPI on the intended Windows machine.

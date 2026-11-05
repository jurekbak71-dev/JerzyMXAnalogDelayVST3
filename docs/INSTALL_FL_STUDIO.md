# MX Delay 2 — Windows x64 / FL Studio

Extract the installation ZIP. Copy the **entire** `MX Delay 2.vst3` folder to `C:\Program Files\Common Files\VST3\`. Do not copy only the inner binary. The `Contents\Resources\gui3d` directory must remain inside the bundle; no separate graphics installation is required.

In FL Studio open Plugin Manager, scan VST3, then add Jerzy MX Analog Delay. The VST3 class IDs are preserved. Remove/backup older duplicate copies first if the scanner finds two bundles with the same ID. The public display name is MX Delay 2; the outer package now uses the requested `MX Delay 2.vst3` name.

The four tabs contain A/B, FX, Reverb and Routing. Right-click restores the parameter's factory value; Shift gives the inherited fine control. Host automation and stored parameters remain live. Built-in convolution uses short synthetic Room/Plate/Hall responses, not an external IR loader.

Read `GUI3D_VALIDATION.md` for actual test results and limitations. Automated host tests are not a claim of an observed FL Studio session. Verify playback, tempo, save/reopen, automation, offline rendering and window/DPI on the intended Windows machine.

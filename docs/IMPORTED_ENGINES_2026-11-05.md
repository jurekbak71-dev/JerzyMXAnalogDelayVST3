# Imported original delay engines — provenance and integration (2026-11-05)

These are source-code integrations, not sound-alike replacements.

| Engine | Original repository | Imported files | Local namespace |
|---|---|---|---|
| Memory Delay | https://github.com/jurekbak71-dev/JerzyMemoryDelay | `source/dsp.h`, `source/parameters.h` | `Jerzy` |
| Reel Echo | https://github.com/jurekbak71-dev/JerzyReelEchoVST3 | `Source/TapeEngine.{h,cpp}`, `Source/Parameters.h` | `jerzy_reel` |
| Oil Drum Echo | https://github.com/jurekbak71-dev/JerzyOilDrumEcho | `src/Engine.h`, `src/Parameters.h`, `src/Spring.h` | `jerzy_oil` |

Original Reel and Oil repositories are AGPL-3.0; license text is retained in `source/engines/AGPL-3.0-LICENSE.txt`. Check downstream distribution obligations before release. Memory repository has no explicit top-level license; its owner should confirm redistribution terms before public release.

Implementation notes:
- Original DSP sources are copied into `source/engines/` and isolated by namespace where required.
- `source/mxdelay_original_engines.h` adapts their existing parameter models and per-sample processing to independent engine A/B instances.
- VST3 algorithms 9–11: Memory Delay, Reel Echo, Oil Drum Echo. Existing algorithms 0–8 retain their order. State v7 remaps normalized selections from older nine-engine presets; v8 additionally stores reverb IR/hybrid settings.
- The built-in FIR convolution is real convolution with generated synthetic IRs. **External WAV/AIFF IR file import is not implemented**. Profiles labelled Room/Plate/Hall are short synthetic FIR textures, not measured impulse responses.
- Full real-time CPU profiling, audio comparison to original standalone plugins, and host listening tests remain necessary.
- Do not claim the GUI matches the approved hardware mockup: the current VSTGUI control surfaces are still a functional but visually incomplete interpretation.

# FL Studio integration — development branch v0.1
## Automation / host synchronization
- Test division clocks under 75, 90, 120, 140 BPM and with variable buffer sizes 32..1024 samples.
- Test both engines A/B in SERIES, PARALLEL, SPLIT L/R with external FL transport.
- Test discontinuous tempo automation while echoes are active; no NaN, runaway feedback, clicks from head changes or stuck buffers.
- Dedicated presets may be built using FL Automation Clips for time, wet, feedback and topology.
- Do not alter existing VST3 parameter IDs or GUI layout in this integration-only branch.
## Diagnostics
Use included `hardware/FL_ROUTING_PL.md` in Jerzy FL Bridge package to avoid MIDI clock feedback when external gear participates.
## Current branch
Test plan only, DSP unaffected; Windows VST3 build and FL host acceptance remain mandatory.
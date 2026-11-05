# GUI C validation — current status, 2026-11-05

The GUI C material rework and the independent reverb continuation are on separate
review branches. No environment publication or merge to main was performed.

Windows run [42028048253](https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/actions/runs/42028048253)
at `0572a97` passed x64 compilation, CTest (DSP, compatibility, reverb, compiled
VST3 host and native GUI), rendered-control checks and pluginval strictness 5.
It produced an installation ZIP, material library and authentic native renders
of all four pages and REVERB mode states. Later optimization/archive-validation
changes are tracked in the current workflow and `docs/REVERB_VALIDATION.md`;
consult `COMMIT.txt` in each downloaded installation package.

The material library contains 34 independently rendered RGBA assets, including
101-frame knob atlases, 17-state toggles, panels, shadows and separate light
layers. Byte-identical regeneration, SHA-256, atlas bounds <=4096 px, alpha,
frame counts, control mappings, transparent dynamic text and geometry checks
passed. The entire decoded library is approximately 37.5 MiB; runtime controls
share a cache rather than allocating copies per knob. The native GUI tests
capture VSTGUI itself, exercise host parameter updates, balanced right-click
reset gestures, hit targets, page switches and scaling. These captures are not
rendered mockups. Graphical realism and exact correspondence to the user's
reference still require side-by-side visual acceptance.

The original visual-only baseline had a 512-tap built-in FIR and no external IR
loader. Task 2 now supplies actual partitioned WAV convolution, a fuller FDN and
adjustable HYBRID. Read `REVERB_VALIDATION.md` for the audit, supported formats,
new IDs/state, audio evidence and measured long-IR performance limitations.
The three imported delay adapters remain preserved; these GUI/reverb tests do
not certify their standalone sound/oversampling/precision equivalence. Existing
input/wet/output meters display real telemetry. INPUT follows IN trim before
INPUT/PRE effects; WET follows the wet contribution after MIX and before OUT;
OUTPUT remains the actual final signal. The meter-tap regression checks both
precisions, mono/stereo, all routing modes and bit-identical audio with telemetry
enabled or disabled; no independent A/B peak data is
invented. Standalone Filter is not added as a fake FX slot type.

FL Studio is not installed in this Linux environment. Actual FL Studio project
save/reopen, playback, tempo, automation, Windows file selection and physical
monitor DPI are manual acceptance checks. Linux X11 has a separately diagnosed
upstream runloop initialization issue; this deliverable targets Windows x64.
No SDK source patches were made.

## Earlier failures and remaining access restriction

Windows runs through `6610780` failed before the user's quoted-resource-copy fix
`41587a1` was integrated. A later CI failure was a PowerShell invocation issue:
pluginval is a GUI-subsystem executable, so direct invocation could leave its
exit code unset. Waiting explicitly with Start-Process resolved that failure;
the successful run above includes actual pluginval validation.

The earlier GitHub authentication outage is resolved: commits, PR #16 and CI
requests now work. Artifact downloads remain denied by this environment's
network policy at `productionresultssa13.blob.core.windows.net`; full logs use
`results-receiver.actions.githubusercontent.com`. Host additions were saved in
the configuration draft without publishing. GitHub Actions artifact links are
available to the user, but local visual inspection of those Windows PNGs is not
claimed. Artifact existence, required checks and runner-side package verification
are recorded separately from visual acceptance.

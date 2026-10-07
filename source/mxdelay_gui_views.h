#pragma once
#include "jerzy_vst_gui_kit.h"

namespace JerzyAudio {

// Backward-compatible class names used by the existing MX2 .uidesc.
// The actual implementation lives in JerzyVSTGuiKit.
using ChickenKnob = JerzyKnob;
using AnalogMeter = JerzyAnalogMeter;
using ToggleSwitch = JerzyToggle;
using ThreeWaySwitch = JerzyThreeWay;
using LedToggleSwitch = JerzyLedToggle;
using LedThreeWaySwitch = JerzyLedThreeWay;
using BypassButton = JerzyBypassButton;
using HardwarePanel = JerzyHardwarePanel;
using OxidizedPanel = JerzyChassis;

void registerMXDelayViews();

} // namespace JerzyAudio

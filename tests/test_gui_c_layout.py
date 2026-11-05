#!/usr/bin/env python3
"""Static regression checks for the Modular Console VSTGUI resource."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = ET.parse(Path(__file__).resolve().parents[1] / "resource" / "mxdelay.uidesc").getroot()
templates = {t.attrib["name"]: t for t in root.findall("template")}
assert "view" in templates, "Missing main editor template"
assert all(f"{side}_Algo{i}" in templates for side in "AB" for i in range(12))
for side in "AB":
    for i in range(9):
        template = templates[f"{side}_Algo{i}"]
        assert template.find("view[@class='ConsoleInsetPanel']") is not None, (
            f"{side}_Algo{i}: missing recessed modular surface"
        )
main = templates["view"]
tags = {v.attrib.get("control-tag") for v in main.iter("view")}
for tag in ("AAlgo", "BAlgo", "ATime", "BTime", "AFeedback", "BFeedback",
            "InputTrim", "OutputTrim", "Mix", "GuiPage0", "GuiPage1",
            "GuiPage2", "GuiPage3", "ReverbMode", "ReverbIR", "ReverbEnable"):
    assert tag in tags, f"Lost binding: {tag}"
pages = [v for v in main.iter("view") if "gui-c-page" in v.attrib]
assert {p.attrib["gui-c-page"] for p in pages} == {"0", "1", "2", "3"}
assert all(p.attrib.get("visible") in ("true", "false") for p in pages)
print("GUI C XML valid; 24 engine surfaces, four pages and key parameter tags intact")

# Visual hierarchy regression: the two main decks must sit after their
# chassis backgrounds and before their parameter controls in paint order.
page = next(p for p in pages if p.attrib["gui-c-page"] == "0")
children = list(page)
for chassis_x, deck_x, time_tag in (("8, 0", "20, 50", "ATime"),
                                    ("648, 0", "660, 50", "BTime")):
    chassis = next(i for i, v in enumerate(children)
                   if v.attrib.get("class") == "HardwarePanel"
                   and v.attrib.get("origin") == chassis_x)
    deck = next(i for i, v in enumerate(children)
                if v.attrib.get("class") == "CViewContainer"
                and v.attrib.get("origin") == deck_x)
    knob = next(i for i, v in enumerate(children)
                if v.attrib.get("control-tag") == time_tag)
    assert chassis < deck < knob, f"{time_tag}: incorrect drawing order"
    assert children[knob].attrib["size"] == "84, 84", "Primary knob hierarchy regressed"
assert main.find("view[@gui-c-page='0']").attrib["origin"] == "0, 120"
print("GUI C primary knob sizing and paint order passed")

# Permanent three-stage metering strip: no analog meter may overlap output trim.
meter_tags=("InputPeak","WetPeak","OutputPeak")
meters=[v for v in main.iter("view") if v.attrib.get("control-tag") in meter_tags]
assert len(meters)==3 and all(v.attrib.get("class")=="LedBarMeter" for v in meters)
for v in meters:
    x,y=map(int,v.attrib["origin"].split(", "))
    w,h=map(int,v.attrib["size"].split(", "))
    assert 378<=x and x+w<=536, "LED bar escapes GLOBAL MIX panel or overlaps trim"
    assert 690<=y and y+h<=782, "LED bar escapes footer vertically"
assert [v.attrib["control-tag"] for v in meters]==list(meter_tags), "Meter order must be INPUT, WET, OUTPUT"
for tag in ("EngineAActive","EngineBActive","EngineBothActive"):
    leds=[v for v in main.iter("view") if v.attrib.get("control-tag")==tag]
    assert len(leds)==1 and leds[0].attrib.get("mouse-enabled")=="false", tag
print("Three horizontal LED bars inside Global Mix and three A/B activity LEDs validated")

# Footer visual regression: knob/value rectangles stay fully inside the
# hardware-panel inner area, clear of the bottom decorative screw line.
footer_panels=[(8,150),(166,370),(544,314),(866,214),(1088,184)]
for tag, panel_x, panel_w in (("InputTrim",8,150),("Mix",166,370),("OutputTrim",1088,184)):
    elements=[v for v in main.iter("view") if v.attrib.get("control-tag")==tag]
    assert len(elements)==2, f"{tag}: expected knob and numeric value"
    knob=next(v for v in elements if v.attrib.get("class")=="ChickenKnob")
    value=next(v for v in elements if v.attrib.get("class")=="CTextLabel")
    def bounds(v):
        x,y=map(int,v.attrib["origin"].split(", "))
        w,h=map(int,v.attrib["size"].split(", "))
        return x,y,x+w,y+h
    k=bounds(knob);v=bounds(value)
    assert panel_x+8<=k[0] and k[2]<=panel_x+panel_w-8
    assert panel_x+8<=v[0] and v[2]<=panel_x+panel_w-8
    assert k[1]>=708 and k[3]<=754
    assert v[1]>=752 and v[3]<=770, f"{tag}: value touches panel bottom screws"
    assert k[2]<=v[0] or v[2]<=k[0] or k[3]<=v[1], f"{tag}: knob overlaps numeric frame"
print("Footer knobs, numeric frames and panel screw clearance validated")

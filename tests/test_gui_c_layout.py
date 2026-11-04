#!/usr/bin/env python3
"""Static regression checks for the Modular Console VSTGUI resource."""
from pathlib import Path
import xml.etree.ElementTree as ET

root = ET.parse(Path(__file__).resolve().parents[1] / "resource" / "mxdelay.uidesc").getroot()
templates = {t.attrib["name"]: t for t in root.findall("template")}
assert "view" in templates, "Missing main editor template"
assert all(f"{side}_Algo{i}" in templates for side in "AB" for i in range(9))
for side in "AB":
    for i in range(9):
        template = templates[f"{side}_Algo{i}"]
        assert template.find("view[@class='HardwarePanel']") is not None, (
            f"{side}_Algo{i}: missing modular hardware surface"
        )
main = templates["view"]
tags = {v.attrib.get("control-tag") for v in main.iter("view")}
for tag in ("AAlgo", "BAlgo", "ATime", "BTime", "AFeedback", "BFeedback",
            "InputTrim", "OutputTrim", "Mix", "GuiPage0", "GuiPage1",
            "GuiPage2", "GuiPage3"):
    assert tag in tags, f"Lost binding: {tag}"
pages = [v for v in main.iter("view") if "gui-c-page" in v.attrib]
assert {p.attrib["gui-c-page"] for p in pages} == {"0", "1", "2", "3"}
assert all(p.attrib.get("visible") in ("true", "false") for p in pages)
print("GUI C XML valid; 18 engine surfaces, four pages and key parameter tags intact")

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
    assert children[knob].attrib["size"] == "74, 74", "Primary knob hierarchy regressed"
assert main.find("view[@gui-c-page='0']").attrib["origin"] == "0, 120"
print("GUI C primary knob sizing and paint order passed")

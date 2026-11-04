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

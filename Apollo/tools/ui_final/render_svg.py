"""Vector demonstrations of the production Apollo UI (not runtime screenshots).
Run: python Apollo/tools/ui_final/render_svg.py
Uses only Python's standard library. Coordinates mirror PluginEditor::resized.
"""
from pathlib import Path
from math import pi, sin, cos
from xml.etree.ElementTree import Element, SubElement, ElementTree

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs" / "ui-final"
NS = "http://www.w3.org/2000/svg"
IVORY, EDGE, PANEL = "#eee9dd", "#b9b4a8", "#222426"
LIGHT, DEEP, BORDER = "#343638", "#101214", "#4b4c4b"
TEXT, DIM, AMBER, RED = IVORY, "#b3afa5", "#efa34b", "#dc6554"


def add(parent, tag, **attrs):
    return SubElement(parent, tag, {k.replace("_", "-"): str(v) for k, v in attrs.items()})


def rect(parent, x, y, w, h, fill, radius=0, stroke=None):
    return add(parent, "rect", x=x, y=y, width=w, height=h, rx=radius, fill=fill,
               **({"stroke": stroke, "stroke-width": ".8"} if stroke else {}))


def line(parent, x1, y1, x2, y2, colour, width=1):
    return add(parent, "line", x1=x1, y1=y1, x2=x2, y2=y2, stroke=colour, stroke_width=width)


def text(parent, label, cx, cy, size=10, colour=TEXT, mono=False, bold=False, anchor="middle"):
    e = add(parent, "text", x=cx, y=cy, fill=colour, font_size=size,
            font_family="Consolas, monospace" if mono else "Arial, sans-serif",
            font_weight="700" if bold else "400", text_anchor=anchor,
            dominant_baseline="central", letter_spacing="0" if mono else str(size * (.12 if bold else .08)))
    e.text = label
    return e


def gradient(defs, name, top, bottom):
    g = add(defs, "linearGradient", id=name, x1="0", y1="0", x2="0", y2="1")
    add(g, "stop", offset="0", stop_color=top)
    add(g, "stop", offset="1", stop_color=bottom)


def bank(parent, x, y, width, height, labels, selected, dim=False):
    n, gap = len(labels), 4
    w = (width - gap * (n - 1)) / n
    for i, label in enumerate(labels):
        on = i == selected
        face = rect(parent, x + i * (w + gap) + 1, y + 1, w - 2, height - 2,
                    AMBER if on else LIGHT, 2, BORDER)
        if on and dim:
            face.set("fill-opacity", ".38")
        text(parent, label, x + i * (w + gap) + w / 2, y + height / 2, 9,
             DEEP if on and not dim else TEXT, mono=True)


def knob(parent, name, cx, y, diameter, value, readout, dim=False, bipolar=False):
    text(parent, name, cx, y - 13, 10)
    cy, radius = y + diameter / 2, diameter / 2
    start, end = 1.2 * pi, 2.8 * pi
    for i in range(9):
        a = start + i / 8 * (end - start)
        middle = bipolar and i == 4
        major = i in (0, 8) or middle
        inner, outer = radius * .80 - (2 if major else 0), radius - 2
        line(parent, cx + sin(a) * inner, cy - cos(a) * inner,
             cx + sin(a) * outer, cy - cos(a) * outer,
             AMBER if middle else DIM, 1.5 if middle else .8)
    r = radius * .68
    add(parent, "circle", cx=cx, cy=cy + 1.5, r=r, fill="#000", opacity=".25")
    add(parent, "circle", cx=cx, cy=cy, r=r, fill="url(#knob)")
    add(parent, "circle", cx=cx, cy=cy, r=r - .5, fill="none", stroke=BORDER, stroke_width=".8")
    add(parent, "circle", cx=cx, cy=cy, r=r - 2, fill="none", stroke=TEXT, stroke_width=".8", opacity=".06")
    a = start + value * (end - start)
    line(parent, cx + sin(a) * r * .32, cy - cos(a) * r * .32,
         cx + sin(a) * r * .86, cy - cos(a) * r * .86, DIM if dim else TEXT, 2.2)
    text(parent, readout, cx, y + diameter + 12, 10, DIM if dim else AMBER, mono=True)


def panel(parent, title, x, y, dim=False):
    text(parent, title, x + 24, y + 25, 12, DIM if dim else TEXT, bold=True, anchor="start")


def face(octave=0, action=0, perform=False, bypass=False, tone=.5, feed=True, size=2, diffusion=True):
    root = Element("svg", xmlns=NS, width="900", height="620", viewBox="0 0 900 620",
                   role="img", **{"aria-labelledby": "title description"})
    add(root, "title", id="title").text = "Apollo — 1968 aerospace interface"
    add(root, "desc", id="description").text = (
        "Vector demonstration of the implemented JUCE layout; not a runtime screenshot. "
        f"Octave {['Off', 'Up', 'Down', 'Both'][octave]}, "
        f"Perform {['Freeze', 'Drive', 'Octave'][action]} {'ON' if perform else 'OFF'}, "
        f"{'bypassed' if bypass else 'active'}, Tone {tone}.")
    defs = add(root, "defs")
    gradient(defs, "knob", LIGHT, DEEP)
    gradient(defs, "fader", IVORY, EDGE)
    gradient(defs, "perform", AMBER if perform else LIGHT, "#bd7d35" if perform else "#191b1d")

    rect(root, .5, .5, 899, 619, IVORY, stroke=EDGE)
    text(root, "APOLLO", 36, 43, 38, "#252729", bold=True, anchor="start")
    text(root, "STEREO SPACE PROCESSOR", 38, 77, 10, "#696960", anchor="start")
    rect(root, 712, 42, 162, 34, PANEL, 2)
    add(root, "circle", cx=726, cy=59, r=3, fill=RED if bypass else AMBER)
    text(root, "BYPASSED" if bypass else "ACTIVE", 800, 59, 11, RED if bypass else AMBER, mono=True)

    rect(root, 24, 112, 852, 484, PANEL, 3)
    line(root, 24, 392, 876, 392, BORDER)
    line(root, 636, 112, 636, 596, BORDER)
    off = octave == 0 or (action == 2 and not perform)
    panel(root, "SPACE", 24, 112, bypass)
    panel(root, "OUTPUT", 636, 112)
    panel(root, "OCTAVE", 24, 392, off or bypass)
    panel(root, "PERFORM", 636, 392)

    knob(root, "PRE-DELAY", 112, 182, 92, 0, "0 ms")
    knob(root, "DECAY", 258, 170, 116, .88, "88%")
    readout = "FLAT" if tone == .5 else f"{'HI' if tone < .5 else 'LO'} {round(abs(tone - .5) * 200)}%"
    knob(root, "TONE", 404, 182, 92, tone, readout, bipolar=True)
    text(root, "HIGH CUT", 356, 313, 7.5, DIM)
    text(root, "FLAT", 404, 313, 7.5)
    text(root, "LOW CUT", 452, 313, 7.5, DIM)
    knob(root, "MOD RATE", 560, 162, 68, .05, "1.05x")
    knob(root, "MOD DEPTH", 560, 286, 68, .06, "6%")
    text(root, "SIZE", 84, 354)
    bank(root, 112, 340, 248, 28, ["SMALL", "MEDIUM", "LARGE"], size)
    text(root, "DIFFUSION", 417, 354)
    bank(root, 464, 340, 38, 28, ["ON" if diffusion else "OFF"], 0 if diffusion else -1)

    text(root, "WET", 756, 161)
    rect(root, 753, 191, 6, 130, DEEP, 1, BORDER)
    rect(root, 755, 256, 2, 65, AMBER).set("opacity", ".35")
    for i in range(9):
        yy, length = 191 + 130 * i / 8, 12 if i % 4 == 0 else 6
        line(root, 742 - length, yy, 742, yy, DIM, .8)
        line(root, 770, yy, 770 + length, yy, DIM, .8)
    rect(root, 731, 246, 50, 24, "#000", 2).set("opacity", ".25")
    rect(root, 731, 244, 50, 24, "url(#fader)", 2)
    line(root, 737, 256, 775, 256, "#252729", 1.4)
    text(root, "DRY", 756, 360)

    text(root, "MODE", 180, 450)
    bank(root, 48, 466, 264, 34, ["OFF", "UP", "DOWN", "UP+DOWN"], octave)
    text(root, "REVERB FEED", 180, 525)
    bank(root, 48, 541, 264, 32, ["OCT", "OCT + DRY"], 1 if feed else 0, off or bypass)
    knob(root, "PRESENCE", 416, 464, 86, 13 / 48, "-11.0 dB", off or bypass)
    knob(root, "BODY", 556, 464, 86, 29 / 48, "5.0 dB", off or bypass)
    bank(root, 660, 444, 192, 32, ["FREEZE", "DRIVE", "OCTAVE"], action)
    rect(root, 663, 502, 186, 70, DEEP, 3)
    rect(root, 663, 500, 186, 70, "url(#perform)", 3, BORDER)
    text(root, "PERFORM", 756, 535, 17, DEEP if perform else TEXT, bold=True)
    return root


def write(name, **state):
    OUT.mkdir(parents=True, exist_ok=True)
    ElementTree(face(**state)).write(OUT / f"{name}.svg", encoding="utf-8", xml_declaration=True)


def main():
    write("apollo-default")
    write("apollo-octave-active", octave=3)
    write("apollo-perform-active", octave=3, action=0, perform=True)
    write("apollo-perform-drive", octave=3, action=1, perform=True)
    write("apollo-perform-octave", octave=1, action=2, perform=True)
    write("apollo-bypass", bypass=True)
    write("apollo-tone-high-cut", tone=.2)
    write("apollo-tone-low-cut", tone=.8)
    write("apollo-feed-oct", octave=3, feed=False)
    # A side-by-side comparison retains a full-size viewBox per face.
    comparison = Element("svg", xmlns=NS, width="2700", height="620", viewBox="0 0 2700 620")
    add(comparison, "title").text = "Apollo Tone: High Cut / Flat / Low Cut"
    for i, tone in enumerate((.2, .5, .8)):
        child = face(tone=tone)
        # Scope paint servers and accessible IDs to each nested SVG.
        for element in child.iter():
            if "id" in element.attrib:
                element.set("id", f"t{i}-" + element.get("id"))
            for key, value in list(element.attrib.items()):
                if value.startswith("url(#"):
                    element.set(key, value.replace("url(#", f"url(#t{i}-"))
        child.set("x", str(i * 900))
        child.set("aria-labelledby", f"t{i}-title t{i}-description")
        comparison.append(child)
    ElementTree(comparison).write(OUT / "apollo-tone-comparison.svg", encoding="utf-8", xml_declaration=True)
    print(f"Generated 10 SVG demonstrations in {OUT}")


if __name__ == "__main__":
    main()

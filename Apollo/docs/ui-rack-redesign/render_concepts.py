#!/usr/bin/env python3
"""
Apollo rack UI - concept renderer.

Generates CONCEPT RENDER SVGs (and PNGs via headless Chrome) that mirror the
geometry and materials implemented in Source/PluginEditor.cpp +
Source/ApolloLookAndFeel.cpp + Source/ApolloTheme.cpp at the 900x620 design
space (and the 1400x980 max size).

These are programmatic concept renders, NOT screenshots of the running plugin.
"""
import math
import os
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
DW, DH = 900, 620

CHROME_CANDIDATES = [
    r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
]

# --------------------------------------------------------------------------
# Palette (mirrors ApolloTheme.h)
# --------------------------------------------------------------------------
P = dict(
    chassisLight="#e3ddcd", chassisMid="#cdc5b2", chassisDark="#a79e8a",
    chassisEdge="#6d6656", chassisShadow="#3d3930",
    graphiteLight="#383d45", graphite="#262a30", graphiteDark="#15181c",
    graphiteDeep="#0b0d10", graphiteEdge="#050607",
    plateBlack="#0d0f11", plateBlackLit="#1a1210",
    textOnPanel="#eae4d6", textOnPanelDim="#8d897d",
    textOnChassis="#2c2b26", textOnChassisSoft="#5c574c",
    engraveHighlight="#fffdf6", engraveShadow="#2a2924",
    amber="#ff7d1a", amberDeep="#cf5c0f", amberGlow="#ffab63",
    red="#e35240", redDeep="#9c2a1c", cyan="#74c8d6", lampOff="#37342e",
    metalScrew="#b3ac9a", metalCap="#7d786c", metalHighlight="#f4f0e6",
    bakeliteLight="#3c372f", bakelite="#1d1a16", bakeliteDark="#0a0908",
    chromeLight="#d4cfc4", chromeMid="#8f897d", chromeDark="#514d45", knobPointer="#eae3d1",
)

SANS = "Segoe UI, Arial, sans-serif"
MONO = "Consolas, 'Courier New', monospace"


def lerp(a, b, t):
    return a + (b - a) * t


def pt(cx, cy, r, a):
    return (cx + r * math.sin(a), cy - r * math.cos(a))


def f(v):
    return ("%.2f" % v).rstrip("0").rstrip(".")


class Doc:
    def __init__(self):
        self.defs = []
        self.body = []
        self.n = 0

    def gid(self, prefix):
        self.n += 1
        return "%s%d" % (prefix, self.n)

    def grad(self, c1, c2, x1, y1, x2, y2, horizontal=False):
        i = self.gid("grad")
        if horizontal:
            coords = 'x1="0" y1="0" x2="1" y2="0"'
        else:
            coords = 'x1="%s" y1="%s" x2="%s" y2="%s"' % (f(x1), f(y1), f(x2), f(y2))
        stop1 = c1 if isinstance(c1, str) else "rgb(%d,%d,%d)" % c1
        stop2 = c2 if isinstance(c2, str) else "rgb(%d,%d,%d)" % c2
        self.defs.append(
            '<linearGradient id="%s" %s gradientUnits="userSpaceOnUse">'
            '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>'
            % (i, coords, stop1, stop2))
        return i

    def radial(self, inner, outer, cx, cy, r):
        i = self.gid("rad")
        self.defs.append(
            '<radialGradient id="%s" gradientUnits="userSpaceOnUse" cx="%s" cy="%s" r="%s">'
            '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></radialGradient>'
            % (i, f(cx), f(cy), f(r), inner, outer))
        return i

    def add(self, s):
        self.body.append(s)

    def svg(self, w, h, vbw=None, vbh=None):
        vbw = w if vbw is None else vbw
        vbh = h if vbh is None else vbh
        return ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'
                '<defs>%s</defs>%s</svg>' % (w, h, vbw, vbh, "".join(self.defs), "".join(self.body)))


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def text(x, y, s, size, fill, anchor="start", family=SANS, weight="normal", spacing=None, opacity=1.0):
    extra = ' letter-spacing="%s"' % spacing if spacing else ""
    op = ' opacity="%s"' % f(opacity) if opacity < 1 else ""
    return ('<text x="%s" y="%s" font-family="%s" font-size="%s" fill="%s" text-anchor="%s" '
            'font-weight="%s"%s%s>%s</text>' % (f(x), f(y), family, f(size), fill, anchor, weight, extra, op, esc(s)))


def ctext(rect, s, size, fill, family=SANS, weight="normal", spacing=None, opacity=1.0):
    x, y, w, h = rect
    return ('<text x="%s" y="%s" font-family="%s" font-size="%s" fill="%s" text-anchor="middle" '
            'dominant-baseline="middle" font-weight="%s"%s%s>%s</text>'
            % (f(x + w / 2.0), f(y + h / 2.0), family, f(size), fill, weight,
               (' letter-spacing="%s"' % spacing if spacing else ""),
               (' opacity="%s"' % f(opacity) if opacity < 1 else ""), esc(s)))


def engraved(x, y, s, size, face, highlight, anchor="start", weight="bold", spacing=None):
    return (text(x, y + 1, s, size, highlight, anchor, SANS, weight, spacing) +
            text(x, y, s, size, face, anchor, SANS, weight, spacing))


# --------------------------------------------------------------------------
# Components
# --------------------------------------------------------------------------
def screenshot_screw(d, cx, cy, r):
    d.add('<circle cx="%s" cy="%s" r="%s" fill="%s" opacity="0.5"/>' % (f(cx), f(cy + 1), f(r + 1), P["chassisShadow"]))
    g = d.grad(P["metalScrew"], "#6f6a5e", cx - r, cy - r, cx + r, cy + r)
    d.add('<circle cx="%s" cy="%s" r="%s" fill="url(#%s)"/>' % (f(cx), f(cy), f(r), g))
    d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="0.8" opacity="0.55"/>'
          % (f(cx), f(cy), f(r), P["metalHighlight"]))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="%s" opacity="0.55"/>'
          % (f(cx - r * 0.6), f(cy - r * 0.6), f(cx + r * 0.6), f(cy + r * 0.6), f(max(1.0, r * 0.3))))


def lamp(d, x, y, w, h, colour, intensity, corner=2.5):
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="%s" stroke="#000" stroke-width="1" opacity="0.85"/>'
          % (f(x), f(y), f(w), f(h), f(corner), P["graphiteDeep"]))
    gx, gy, gw, gh = x + 2, y + 2, w - 4, h - 4
    if intensity > 0.02:
        d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="%s" opacity="%s" filter="url(#softglow)"/>'
              % (f(gx - 1), f(gy - 1), f(gw + 2), f(gh + 2), f(corner), colour, f(0.5 * intensity)))
    base = colour if intensity > 0.02 else P["lampOff"]
    op = 0.35 + 0.65 * intensity if intensity > 0.02 else 1.0
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="%s" opacity="%s"/>'
          % (f(gx), f(gy), f(gw), f(gh), f(max(1.0, corner - 0.5)), base, f(op)))
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="#ffffff" opacity="%s"/>'
          % (f(gx + 1), f(gy + 1), f(gw - 2), f(gh * 0.4), f(max(1.0, corner - 1)), f(0.2 * max(intensity, 0.05))))


def module(d, x, y, w, h, title, subtitle, lamp_on, lamp_colour, dim):
    top = "#1b1d21" if dim else P["graphiteLight"]
    bottom = "#0a0b0d" if dim else P["graphiteDeep"]
    g = d.grad(top, bottom, x, y, x, y + h)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="url(#%s)"/>' % (f(x), f(y), f(w), f(h), g))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#fff" stroke-width="1" opacity="%s"/>'
          % (f(x + 5), f(y + 1), f(x + w - 5), f(y + 1), "0.03" if dim else "0.08"))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="1.6" opacity="0.45"/>'
          % (f(x + 5), f(y + h - 1), f(x + w - 5), f(y + h - 1)))
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="none" stroke="%s" stroke-width="1.2"/>'
          % (f(x), f(y), f(w), f(h), P["graphiteEdge"]))
    sr = max(2.4, min(3.6, h * 0.018))
    for px, py in [(x + 9, y + 9), (x + w - 9, y + 9), (x + 9, y + h - 9), (x + w - 9, y + h - 9)]:
        screenshot_screw(d, px, py, sr)
    hh = max(16.0, h * 0.11)
    title_size = max(10.0, min(14.0, h * 0.045))
    face = P["textOnPanelDim"] if dim else P["textOnPanel"]
    d.add(engraved(x + 16, y + 7 + hh * 0.72, title, title_size, face, "#000000", "start", "bold", "1.4"))
    lsize = min(16.0, hh)
    lamp_x = x + w - 16 - lsize
    if subtitle:
        st = max(7.0, min(9.5, h * 0.030))
        d.add(engraved(lamp_x - 10, y + 7 + hh * 0.70, subtitle, st, P["textOnPanelDim"], "#000000", "end", "normal", "1.0"))
    lamp(d, lamp_x, y + 7 + (hh - lsize) / 2.0, lsize, lsize, lamp_colour, 1.0 if lamp_on else 0.0, 2.5)


def knob(d, cx, cy, size, pos, dim=False, bipolar=False, focus=False):
    r = size / 2.0
    skirt_r = r * 0.68
    inner = skirt_r + 4.0
    outer = r - 1.5
    start = math.pi * 1.2
    end = math.pi * 2.8
    ticks = 10
    for i in range(ticks + 1):
        t = i / float(ticks)
        a = start + t * (end - start)
        major = (i % 5 == 0)
        centre_tick = bipolar and i == ticks // 2
        ti, to = inner, outer
        width = 1.7 if major else 1.0
        if major:
            ti = inner - 4.5
        if centre_tick:
            ti = inner - 7.0
            to = outer + 1.0
            width = 1.8
        col = "#5b584f" if dim else ("#8d897d" if major else "#6f6c63")
        if centre_tick:
            col = P["amberDeep"] if dim else P["amber"]
        p1 = pt(cx, cy, ti, a)
        p2 = pt(cx, cy, to, a)
        op = "0.35" if dim else ("0.9" if major else "0.55")
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="%s" opacity="%s" stroke-linecap="round"/>'
              % (f(p1[0]), f(p1[1]), f(p2[0]), f(p2[1]), col, f(width), op))
    # shadow
    d.add('<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="#000" opacity="%s"/>'
          % (f(cx), f(cy + 2.5), f(skirt_r), f(skirt_r), "0.25" if dim else "0.42"))
    # fluted bakelite skirt
    sg = d.grad(P["bakeliteLight"], P["bakeliteDark"], cx - skirt_r, cy - skirt_r, cx + skirt_r * 0.6, cy + skirt_r)
    d.add('<circle cx="%s" cy="%s" r="%s" fill="url(#%s)"/>' % (f(cx), f(cy), f(skirt_r), sg))
    flutes = 22
    gi = skirt_r - max(4.5, skirt_r * 0.20)
    go = skirt_r - 0.6
    for i in range(flutes):
        a = i / float(flutes) * math.tau
        p1 = pt(cx, cy, gi, a)
        p2 = pt(cx, cy, go, a)
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="1.9" opacity="%s"/>'
              % (f(p1[0]), f(p1[1]), f(p2[0]), f(p2[1]), "0.16" if dim else "0.34"))
        ar = a + 0.5 / float(flutes) * math.tau
        q1 = pt(cx, cy, gi + 0.5, ar)
        q2 = pt(cx, cy, go, ar)
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#fff" stroke-width="1.0" opacity="%s"/>'
              % (f(q1[0]), f(q1[1]), f(q2[0]), f(q2[1]), "0.03" if dim else "0.08"))
    d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="#000" stroke-width="1" opacity="0.45"/>'
          % (f(cx), f(cy), f(skirt_r)))
    # domed body
    body_r = skirt_r * 0.74
    bg = d.grad(P["bakeliteLight"], P["bakeliteDark"], cx - body_r * 0.42, cy - body_r * 0.52, cx + body_r * 0.55, cy + body_r * 0.65)
    d.add('<circle cx="%s" cy="%s" r="%s" fill="url(#%s)"/>' % (f(cx), f(cy), f(body_r), bg))
    d.add('<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="#fff" opacity="%s"/>'
          % (f(cx - body_r * 0.05), f(cy - body_r * 0.48), f(body_r * 0.52), f(body_r * 0.33), "0.03" if dim else "0.11"))
    d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="1.3" opacity="%s"/>'
          % (f(cx), f(cy), f(body_r), P["chromeDark"], "0.35" if dim else "0.85"))
    d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="0.8" opacity="%s"/>'
          % (f(cx), f(cy), f(body_r + 0.8), P["chromeLight"], "0.10" if dim else "0.30"))
    # ivory pointer across dome onto skirt
    angle = start + pos * (end - start)
    m1 = pt(cx, cy, skirt_r * 0.20, angle)
    m2 = pt(cx, cy, skirt_r * 0.90, angle)
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="2.6" opacity="0.45"/>'
          % (f(m1[0]), f(m1[1] + 1.0), f(m2[0]), f(m2[1] + 1.0)))
    mcol = "#6f6c63" if dim else P["knobPointer"]
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="2.3" stroke-linecap="round"/>'
          % (f(m1[0]), f(m1[1]), f(m2[0]), f(m2[1]), mcol))
    d.add('<circle cx="%s" cy="%s" r="1.6" fill="%s"/>' % (f(m2[0]), f(m2[1]), mcol))
    # chrome centre cap
    cap_r = body_r * 0.30
    cg = d.grad(P["chromeLight"], P["chromeDark"], cx - cap_r * 0.45, cy - cap_r * 0.55, cx + cap_r * 0.55, cy + cap_r * 0.60)
    d.add('<circle cx="%s" cy="%s" r="%s" fill="url(#%s)"/>' % (f(cx), f(cy), f(cap_r), cg))
    d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="0.8" opacity="0.7"/>'
          % (f(cx), f(cy), f(cap_r), P["chromeDark"]))
    d.add('<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="#fff" opacity="0.35"/>'
          % (f(cx - cap_r * 0.25), f(cy - cap_r * 0.30), f(cap_r * 0.30), f(cap_r * 0.20)))
    if focus:
        d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="3.2" opacity="0.18"/>'
              % (f(cx), f(cy), f(outer + 3), P["amber"]))
        d.add('<circle cx="%s" cy="%s" r="%s" fill="none" stroke="%s" stroke-width="1.6"/>'
              % (f(cx), f(cy), f(outer), P["amber"]))


def fader(d, x, y, w, h, value, dim=False):
    slot_w = 9.0
    slot_x = x + w * 0.30 - slot_w / 2.0
    top = y + 4.0
    bottom = y + h - 4.0
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
          % (f(slot_x - 2), f(top - 2), f(slot_w + 4), f(bottom - top + 4), P["graphiteDeep"]))
    thumb = bottom - value * (bottom - top)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="2.5" fill="%s" opacity="%s"/>'
          % (f(slot_x), f(thumb), f(slot_w), f(bottom - thumb), P["amberDeep"] if dim else P["amber"],
             "0.16" if dim else "0.30"))
    scale_x = x + w * 0.60
    for i in range(5):
        t = i / 4.0
        ty = bottom - t * (bottom - top)
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="1" opacity="0.75"/>'
              % (f(scale_x), f(ty), f(scale_x + 6), f(ty), P["textOnPanelDim"]))
        d.add(text(scale_x + 9, ty + 3, str(int(round(t * 100))), 8, P["textOnPanelDim"], "start", MONO))
    cap_w = w * 0.36
    cap_h = 26.0
    cap_x = slot_x + slot_w / 2.0 - cap_w / 2.0
    cap_y = thumb - cap_h / 2.0
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="#000" opacity="0.45"/>'
          % (f(cap_x), f(cap_y + 1.5), f(cap_w), f(cap_h)))
    cg = d.grad("#b3ada1", "#4f4a41", cap_x, cap_y, cap_x + cap_w, cap_y + cap_h)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="url(#%s)"/>'
          % (f(cap_x), f(cap_y), f(cap_w), f(cap_h), cg))
    for i in (1, 2, 3):
        yy = cap_y + cap_h * i / 4.0
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="0.8" opacity="0.3"/>'
              % (f(cap_x + 4), f(yy), f(cap_x + cap_w - 4), f(yy)))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="1.4"/>'
          % (f(cap_x + 4), f(cap_y + cap_h / 2), f(cap_x + cap_w - 4), f(cap_y + cap_h / 2),
             P["textOnPanelDim"] if dim else P["metalHighlight"]))


def selector(d, x, y, w, h, items, selected, vertical=False, dim=False):
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="4" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
          % (f(x), f(y), f(w), f(h), P["graphiteDeep"]))
    inner = (x + 2, y + 2, w - 4, h - 4)
    n = len(items)
    if vertical:
        usable = inner[3] - 10
        seg = usable / n
        for i, it in enumerate(items):
            sx, sy, sw, sh = inner[0], inner[1] + i * seg, inner[2], seg - 1
            on = (i == selected)
            if on:
                d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" opacity="%s"/>'
                      % (f(sx + 1), f(sy + 1), f(sw - 2), f(sh - 2), P["amberDeep"] if dim else P["amber"],
                         "0.32" if dim else "0.92"))
            d.add(ctext((sx, sy, sw, sh), it, 8.5, "#000" if (on and not dim) else P["textOnPanelDim"], MONO))
        d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="%s" stroke-width="1"/>'
              % (f(inner[0]), f(inner[1] + usable), f(inner[0] + inner[2]), f(inner[1] + usable), P["graphiteEdge"]))
        ax, ay = inner[0] + inner[2] - 7, inner[1] + inner[3] - 5
        d.add('<path d="M %s %s L %s %s L %s %s Z" fill="%s"/>'
              % (f(ax - 3), f(ay - 2), f(ax + 3), f(ay - 2), f(ax), f(ay + 2), P["textOnPanelDim"]))
    else:
        usable = inner[2] - 16
        seg = usable / n
        for i, it in enumerate(items):
            sx, sy, sw, sh = inner[0] + i * seg, inner[1], seg - 1, inner[3]
            on = (i == selected)
            if on:
                d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" opacity="%s"/>'
                      % (f(sx + 1), f(sy + 1), f(sw - 2), f(sh - 2), P["amberDeep"] if dim else P["amber"],
                         "0.32" if dim else "0.92"))
            d.add(ctext((sx, sy, sw, sh), it, 8.5, "#000" if on and not dim else P["textOnPanelDim"], MONO))
        for i in range(1, n):
            xx = inner[0] + i * seg
            d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="1" opacity="0.5"/>'
                  % (f(xx), f(inner[1] + 2), f(xx), f(inner[1] + inner[3] - 2)))
        ax, ay = inner[0] + inner[2] - 7, inner[1] + inner[3] / 2
        d.add('<path d="M %s %s L %s %s L %s %s Z" fill="%s"/>'
              % (f(ax - 3), f(ay - 2), f(ax + 3), f(ay - 2), f(ax), f(ay + 2), P["textOnPanelDim"]))


def push_bank(d, x, y, w, h, items, selected, dim=False, vertical=False):
    n = len(items)
    gap = 4.0
    bw, bh = w, h
    if vertical:
        bh = (h - gap * (n - 1)) / n
    else:
        bw = (w - gap * (n - 1)) / n
    for i, it in enumerate(items):
        if vertical:
            bx, by, bwi, bhi = x, y + i * (bh + gap), bw, bh
        else:
            bx, by, bwi, bhi = x + i * (bw + gap), y, bw, bh
        on = (i == selected)
        if on:
            d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="4" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
                  % (f(bx), f(by), f(bwi), f(bhi), P["graphiteDeep"]))
            face = (bx + 2.5, by + 2.5, bwi - 5, bhi - 5)
            fill = P["amberDeep"] if dim else P["amber"]
            d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" opacity="%s"/>'
                  % (f(face[0]), f(face[1]), f(face[2]), f(face[3]), fill, "0.35" if dim else "0.92"))
            d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="1.4" opacity="0.35"/>'
                  % (f(face[0] + 3), f(face[1] + 1), f(face[0] + face[2] - 3), f(face[1] + 1)))
            tcol = "#000" if not dim else P["textOnPanelDim"]
        else:
            g = d.grad(P["graphiteLight"], P["graphiteDeep"], bx, by, bx, by + bhi)
            d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="4" fill="url(#%s)"/>'
                  % (f(bx), f(by), f(bwi), f(bhi), g))
            d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#fff" stroke-width="1" opacity="%s"/>'
                  % (f(bx + 3), f(by + 1), f(bx + bwi - 3), f(by + 1), "0.04" if dim else "0.13"))
            d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#000" stroke-width="1.6" opacity="0.45"/>'
                  % (f(bx + 3), f(by + bhi - 1), f(bx + bwi - 3), f(by + bhi - 1)))
            d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="4" fill="none" stroke="%s" stroke-width="1"/>'
                  % (f(bx + 0.5), f(by + 0.5), f(bwi - 1), f(bhi - 1), P["graphiteEdge"]))
            tcol = P["textOnPanelDim"] if dim else P["textOnPanel"]
        d.add(ctext((bx, by, bwi, bhi), it, 9, tcol, MONO))


def rocker(d, x, y, w, h, on, dim=False):
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="4" fill="%s" stroke="#000" stroke-width="1" opacity="0.85"/>'
          % (f(x), f(y), f(w), f(h), P["graphiteDeep"]))
    px, py, pw, ph = x + 2.5, y + 2.5, w - 5, h - 5
    c1 = P["amberDeep"] if on else P["graphiteLight"]
    c2 = "#5a3c12" if on else P["graphiteDeep"]
    g = d.grad(c1, c2, px, py, px, py + ph)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="url(#%s)"/>' % (f(px), f(py), f(pw), f(ph), g))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#fff" stroke-width="1" opacity="%s"/>'
          % (f(px + 3), f(py + 1), f(px + pw - 3), f(py + 1), "0.28" if on else "0.10"))
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="none" stroke="#000" stroke-width="1" opacity="0.55"/>'
          % (f(px), f(py), f(pw), f(ph)))
    d.add(ctext((px, py, pw, ph), "ON" if on else "OFF", 10,
                "#000" if on and not dim else (P["textOnPanelDim"] if dim else P["textOnPanel"]), MONO))


def illuminated(d, x, y, w, h, active, label, lamp_colour, sub=None):
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
          % (f(x), f(y), f(w), f(h), P["graphiteDeep"]))
    cap = (x + 3, y + 3, w - 6, h - 6)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="%s"/>'
          % (f(cap[0]), f(cap[1] + 2), f(cap[2]), f(cap[3]), P["graphiteDeep"]))
    top = (P["amber"] if lamp_colour == "amber" else P["red"]) if active else P["graphiteLight"]
    bot = (P["amberDeep"] if lamp_colour == "amber" else P["redDeep"]) if active else P["graphiteDeep"]
    g = d.grad(top, bot, cap[0], cap[1], cap[0] + cap[2], cap[1] + cap[3])
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="url(#%s)"/>'
          % (f(cap[0]), f(cap[1]), f(cap[2]), f(cap[3]), g))
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="5" fill="none" stroke="#000" stroke-width="1" opacity="0.55"/>'
          % (f(cap[0]), f(cap[1]), f(cap[2]), f(cap[3])))
    ls = cap[3] - 10
    lamp(d, cap[0] + 8, cap[1] + (cap[3] - ls) / 2.0, ls, ls, lamp_colour if active else P["lampOff"], 1.0 if active else 0.0)
    tx = cap[0] + 8 + ls + 9
    d.add(text(tx, cap[1] + cap[3] * 0.44, label, 14, "#fff" if active else P["textOnPanel"], "start", SANS, "bold", "1.2"))
    if sub == "bypass":
        d.add(text(cap[0] + cap[2] - 9, cap[1] + cap[3] - 6, "BYPASSED" if active else "ACTIVE", 8,
                   "#fff" if active else P["textOnPanelDim"], "end", MONO, "normal", None, 0.8 if active else 1.0))
    elif active:
        d.add(text(cap[0] + cap[2] - 9, cap[1] + cap[3] - 6, "HOLD", 8, "#fff", "end", MONO, "normal", None, 0.75))


def annunciator(d, x, y, w, h, s, lamp_on, colour, dim=False):
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
          % (f(x), f(y), f(w), f(h), P["graphiteDeep"]))
    ls = min(h - 8, 12)
    lamp(d, x + 5, y + (h - ls) / 2.0, ls, ls, colour if lamp_on else P["lampOff"], 1.0 if lamp_on else 0.0, 2)
    tx = x + 5 + ls + 6
    col = colour if lamp_on else (P["textOnPanelDim"] if not dim else "#4c4944")
    d.add(text(tx, y + h / 2.0 + 3.5, s, max(8.0, min(12.0, h * 0.44)), col, "start", SANS, "normal", "0.6"))


# --------------------------------------------------------------------------
# Full editor
# --------------------------------------------------------------------------
def head_theme(dark):
    if dark:
        return dict(
            ch_top="#2b2e34", ch_bottom="#131519", ch_edge="#0a0b0d", ch_shadow="#05060a",
            plate_top="#3a3f47", plate_bottom="#1c1f24", plate_edge="#0a0b0d",
            word="#f2ecdd", word_hi="#0a0b0d", sub="#9a958a",
        )
    return dict(
        ch_top="#e3ddcd", ch_bottom="#b7af9c", ch_edge="#6d6656", ch_shadow="#3d3930",
        plate_top="#e7e1d2", plate_bottom="#c6beab", plate_edge="#8c8576",
        word="#2c2b26", word_hi="#fffdf6", sub="#5c574c",
    )


def render(state, W=DW, H=DH, dark=False):
    s = 1.0
    d = Doc()
    th = head_theme(dark)

    def X(v):
        return v

    # soft glow filter for SVG
    d.defs.append('<filter id="softglow" x="-60%%" y="-60%%" width="220%%" height="220%%">'
                  '<feGaussianBlur stdDeviation="%s"/></filter>' % f(max(1.5, 2.5 * s)))

    # chassis
    cg = d.grad(th["ch_top"], th["ch_bottom"], 0, 0, 0, H)
    d.add('<rect x="0" y="0" width="%s" height="%s" fill="url(#%s)"/>' % (W, H, cg))
    d.add('<rect x="0.5" y="0.5" width="%s" height="%s" fill="none" stroke="%s" stroke-width="1"/>'
          % (W - 1, H - 1, th["ch_edge"]))
    d.add('<line x1="0.5" y1="0.5" x2="%s" y2="0.5" stroke="#fff" stroke-width="1" opacity="0.22"/>' % f(W - 0.5))

    # header plate
    hx, hy, hw, hh = X(14), X(12), X(872), X(86)
    pg = d.grad(th["plate_top"], th["plate_bottom"], hx, hy, hx, hy + hh)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="url(#%s)"/>' % (f(hx), f(hy), f(hw), f(hh), f(X(5)), pg))
    d.add('<line x1="%s" y1="%s" x2="%s" y2="%s" stroke="#fff" stroke-width="1" opacity="0.12"/>'
          % (f(hx + 5), f(hy + 1), f(hx + hw - 5), f(hy + 1)))
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="%s" fill="none" stroke="%s" stroke-width="1"/>'
          % (f(hx), f(hy), f(hw), f(hh), f(X(5)), th["plate_edge"]))

    # model plate
    mx, my, mw, mh = X(470), X(28), X(190), X(56)
    d.add('<rect x="%s" y="%s" width="%s" height="%s" rx="3" fill="%s" stroke="#000" stroke-width="1" opacity="0.9"/>'
          % (f(mx), f(my), f(mw), f(mh), P["graphiteDeep"]))
    d.add(text(mx + 9, my + 22, "MOD. APOLLO-RA", 9.5, P["amber"], "start", MONO))
    d.add(text(mx + 9, my + 40, "STEREO SPACE PROCESSOR", 8, P["textOnPanelDim"], "start", MONO))

    d.add(engraved(X(36), X(52), "APOLLO", 34, th["word"], th["word_hi"], "start", "bold", "1.5"))
    d.add(engraved(X(41), X(72), "STEREO SPACE PROCESSOR", 11, th["sub"], th["word_hi"], "start", "normal", "1.0"))
    d.add(engraved(X(41), X(85), "PLATE + OCTAVE SYSTEM", 9, th["sub"], th["word_hi"], "start", "normal", "1.0"))

    # corner screws
    sr = max(3.0, 4.0 * s)
    for sx, sy in [(22, 20), (878, 20), (22, 600), (878, 600)]:
        screenshot_screw(d, X(sx), X(sy), sr)

    # ---- state ----
    em = state["effect_mode"]
    fs = state["footswitch"]
    perform = state["perform"]
    bypass = state["bypass"]
    octave_off = em == 0
    dim = bypass or octave_off
    focus = state.get("focus")

    # modules
    module(d, X(14), X(108), X(580), X(300), "REVERB", "SPACE GENERATOR", not bypass, P["amber"], bypass)
    module(d, X(604), X(108), X(282), X(360), "OUTPUT", "DRY / WET", not bypass, P["amber"], False)
    module(d, X(14), X(418), X(580), X(198), "OCTAVE", "SIGNAL GENERATOR", (not octave_off) and not bypass, P["amber"], dim)
    module(d, X(604), X(478), X(282), X(138), "PERFORMANCE", "OPERATIONAL CONTROL", perform,
           P["red"] if (perform and fs == 2 and octave_off) else P["amber"], False)

    # Header: internal bypass button replaces the old SYSTEM/ACTIVE indicator.
    illuminated(d, X(672), X(28), X(198), X(58), bypass, "BYPASS", "red", "bypass")

    # REVERB controls
    d.add(ctext((X(444), X(146), X(132), X(16)), "SIZE", 10, P["textOnPanel"]))
    push_bank(d, X(444), X(164), X(132), X(110), ["SMALL", "MEDIUM", "LARGE"], state["time_scale"], False, vertical=True)
    d.add(ctext((X(444), X(282), X(132), X(16)), "INPUT DIFFUSION", 9, P["textOnPanel"]))
    rocker(d, X(444), X(300), X(132), X(36), state["input_diffusion"], False)
    annunciator(d, X(300), X(314), X(130), X(56), "MOD LFO", state["moddepth"] > 0.005 and not bypass, P["cyan"], bypass)

    knob(d, X(97), X(207), X(90), state["predelay"], False, False, focus == "predelay")
    d.add(ctext((X(30), X(144), X(133), X(16)), "PRE-DELAY", 10, P["textOnPanel"]))
    d.add(ctext((X(30), X(254), X(133), X(16)), "%d ms" % int(round(state["predelay"] * 1000)), 10, P["amber"], MONO))

    knob(d, X(230), X(207), X(90), state["decay"], False, False, focus == "decay")
    d.add(ctext((X(163), X(144), X(133), X(16)), "DECAY", 10, P["textOnPanel"]))
    d.add(ctext((X(163), X(254), X(133), X(16)), "%d%%" % int(round(state["decay"] * 100)), 10, P["amber"], MONO))

    knob(d, X(363), X(207), X(90), state["damp"], False, True, focus == "damp")
    d.add(ctext((X(296), X(144), X(134), X(16)), "TONE", 10, P["textOnPanel"]))
    d.add(ctext((X(296), X(254), X(134), X(16)), "%d%%" % int(round(state["damp"] * 100)), 10, P["amber"], MONO))
    d.add(ctext((X(296), X(272), X(67), X(12)), "HI CUT", 7.5, P["textOnPanelDim"]))
    d.add(ctext((X(363), X(272), X(67), X(12)), "LO CUT", 7.5, P["textOnPanelDim"]))

    knob(d, X(97), X(343), X(90), state["modspeed"], False, False, focus == "modspeed")
    d.add(ctext((X(30), X(282), X(133), X(16)), "MOD RATE", 10, P["textOnPanel"]))
    d.add(ctext((X(30), X(390), X(133), X(16)), "%d%%" % int(round(state["modspeed"] * 100)), 10, P["amber"], MONO))

    knob(d, X(230), X(343), X(90), state["moddepth"], False, False, focus == "moddepth")
    d.add(ctext((X(163), X(282), X(133), X(16)), "MOD DEPTH", 10, P["textOnPanel"]))
    d.add(ctext((X(163), X(390), X(133), X(16)), "%d%%" % int(round(state["moddepth"] * 100)), 10, P["amber"], MONO))

    # OUTPUT (centred fader only; no MIX label)
    fader(d, X(670), X(156), X(150), X(288), state["mix"], False)
    d.add(ctext((X(636), X(154), X(54), X(16)), "WET", 10, P["textOnPanel"]))
    d.add(ctext((X(636), X(426), X(54), X(16)), "DRY", 10, P["textOnPanel"]))

    # OCTAVE (shelf knobs aligned with the mode bank)
    d.add(ctext((X(30), X(452), X(320), X(16)), "OCTAVE MODE", 10, P["textOnPanel"]))
    push_bank(d, X(30), X(470), X(320), X(42), ["OFF", "UP", "DOWN", "UP+DOWN"], em, False)
    d.add(ctext((X(30), X(522), X(320), X(16)), "DRY ROUTING", 9, P["textOnPanel"]))
    rocker(d, X(30), X(540), X(320), X(38), state["octave_dry_mix"], dim)

    knob(d, X(442), X(512), X(84), (state["eq1"] + 24) / 48.0, dim, False, focus == "eq1")
    d.add(ctext((X(400), X(452), X(84), X(16)), "OCT HI SHELF", 9, P["textOnPanel"]))
    d.add(ctext((X(400), X(556), X(84), X(16)), "%.1f dB" % state["eq1"], 10, P["amber"], MONO))

    knob(d, X(532), X(512), X(84), (state["eq2"] + 24) / 48.0, dim, False, focus == "eq2")
    d.add(ctext((X(490), X(452), X(84), X(16)), "OCT LO SHELF", 9, P["textOnPanel"]))
    d.add(ctext((X(490), X(556), X(84), X(16)), "%.1f dB" % state["eq2"], 10, P["amber"], MONO))

    # PERFORMANCE (three interlocked push buttons; state shown by the panel lamp)
    d.add(ctext((X(620), X(506), X(252), X(16)), "PERFORM ACTION", 10, P["textOnPanel"]))
    push_bank(d, X(620), X(524), X(252), X(44), ["FREEZE", "OVERDRIVE", "OCTAVE"], fs, False)
    d.add(ctext((X(620), X(578), X(252), X(16)), "TRIGGER: MIDI / AUTOMATION", 7.5, P["textOnPanelDim"]))

    return d.svg(W, H, DW, DH)


# --------------------------------------------------------------------------
# States
# --------------------------------------------------------------------------
BASE = dict(effect_mode=0, footswitch=0, perform=False, bypass=False, time_scale=2,
            predelay=0.0, decay=0.877, moddepth=0.0625, modspeed=0.0466, damp=0.5,
            eq1=-11.0, eq2=5.0, mix=0.5, input_diffusion=True, octave_dry_mix=True, focus=None)


def st(**kw):
    s = dict(BASE)
    s.update(kw)
    return s


STATES = {
    "01_default": st(),
    "02_octave_active": st(effect_mode=3),
    "03_freeze_active": st(perform=True, footswitch=0),
    "04_overdrive_active": st(perform=True, footswitch=1),
    "05_octave_perform_active": st(perform=True, footswitch=2, effect_mode=3),
    "06_bypassed": st(bypass=True),
    "07_keyboard_focus": st(focus="decay"),
    "08_max_size": st(effect_mode=1),
}


def write_svg(name, content):
    path = os.path.join(HERE, name)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(content)
    return path


def find_chrome():
    for c in CHROME_CANDIDATES:
        if os.path.exists(c):
            return c
    return None


def png_from_svg(chrome, svg_path, png_path, w, h, scale=2):
    url = "file:///" + svg_path.replace("\\", "/")
    cmd = [chrome, "--headless=new", "--disable-gpu", "--hide-scrollbars",
           "--force-device-scale-factor=%d" % scale,
           "--window-size=%d,%d" % (w, h),
           "--screenshot=" + png_path, url]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=120)


def build_contact_sheet():
    from PIL import Image, ImageDraw, ImageFont

    items = [
        ("01 DEFAULT", "01_default.png"),
        ("02 OCTAVE ACTIVE (UP+DOWN)", "02_octave_active.png"),
        ("03 FREEZE ACTIVE", "03_freeze_active.png"),
        ("04 OVERDRIVE ACTIVE", "04_overdrive_active.png"),
        ("05 OCTAVE PERFORM ACTIVE", "05_octave_perform_active.png"),
        ("06 BYPASSED", "06_bypassed.png"),
        ("07 KEYBOARD FOCUS (DECAY)", "07_keyboard_focus.png"),
        ("08 MAX SIZE 1400x980", "08_max_size.png"),
        ("CONCEPT A - DARK FLIGHT COMPUTER", "concept_a_dark_flight_computer.png"),
        ("CONCEPT B - APOLLO LAB EQUIPMENT", "concept_b_lab_equipment.png"),
    ]

    cols, pad, cap_h, cell_w = 3, 16, 30, 540
    cell_h = int(cell_w * 620.0 / 900.0) + cap_h
    rows = (len(items) + cols - 1) // cols
    sheet_w = cols * (cell_w + pad) + pad
    sheet_h = rows * (cell_h + pad) + pad

    sheet = Image.new("RGB", (sheet_w, sheet_h), (18, 19, 21))
    draw = ImageDraw.Draw(sheet)

    try:
        font = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 17)
    except Exception:
        font = ImageFont.load_default()

    for i, (caption, png) in enumerate(items):
        path = os.path.join(HERE, png)
        if not os.path.exists(path):
            continue
        img = Image.open(path).convert("RGB")
        img = img.resize((cell_w, int(img.height * cell_w / float(img.width))), Image.LANCZOS)
        r, c = divmod(i, cols)
        x = pad + c * (cell_w + pad)
        y = pad + r * (cell_h + pad)
        sheet.paste(img, (x, y + cap_h))
        draw.text((x + 4, y + 6), caption, fill=(233, 227, 214), font=font)

    out = os.path.join(HERE, "APOLLO_RACK_CONTACT_SHEET.png")
    sheet.save(out)
    print("contact sheet:", out)


def main():
    chrome = find_chrome()
    outputs = []
    for name, state in STATES.items():
        W, H = (1400, 980) if name == "08_max_size" else (DW, DH)
        svg = render(state, W, H)
        svg_path = write_svg(name + ".svg", svg)
        outputs.append((name, svg_path, W, H))
        if chrome:
            png_from_svg(chrome, svg_path, os.path.join(HERE, name + ".png"), W, H)
            print("rendered", name)

    for name, dark in [("concept_a_dark_flight_computer", True), ("concept_b_lab_equipment", False)]:
        svg = render(st(), DW, DH, dark=dark)
        svg_path = write_svg(name + ".svg", svg)
        outputs.append((name, svg_path, DW, DH))
        if chrome:
            png_from_svg(chrome, svg_path, os.path.join(HERE, name + ".png"), DW, DH)
            print("rendered", name)

    print("done", len(outputs), "concepts; chrome =", chrome)
    if chrome:
        build_contact_sheet()


if __name__ == "__main__":
    main()

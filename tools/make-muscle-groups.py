#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
# Writes the muscle-group pictures as SVG: one figure per group, front or back view as fits the
# group, plus template.svg with all groups in distinct colours. The geometry is original.
# Usage: make-muscle-groups.py <output directory>
# PNG files are rendered from the SVG files with: rsvg-convert -z 2 <name>.svg -o <name>.png
import sys, os
CX = 100.0

def L(h, y): return (CX - h, y)
def R(h, y): return (CX + h, y)
def mirror(pts): return [(2*CX - x, y) for x, y in pts]

def cr_open(pts, move=True):
    """Catmull-Rom through pts (open run). Returns path commands."""
    p = [pts[0]] + list(pts) + [pts[-1]]
    s = ("M%.2f %.2f " % pts[0]) if move else ""
    for i in range(1, len(p) - 2):
        p0, p1, p2, p3 = p[i-1], p[i], p[i+1], p[i+2]
        c1 = (p1[0] + (p2[0]-p0[0])/6.0, p1[1] + (p2[1]-p0[1])/6.0)
        c2 = (p2[0] - (p3[0]-p1[0])/6.0, p2[1] - (p3[1]-p1[1])/6.0)
        s += "C%.2f %.2f %.2f %.2f %.2f %.2f " % (c1+c2+p2)
    return s

def cr_closed(pts):
    n = len(pts)
    s = "M%.2f %.2f " % pts[0]
    for i in range(n):
        p0, p1, p2, p3 = pts[(i-1)%n], pts[i], pts[(i+1)%n], pts[(i+2)%n]
        c1 = (p1[0] + (p2[0]-p0[0])/6.0, p1[1] + (p2[1]-p0[1])/6.0)
        c2 = (p2[0] - (p3[0]-p1[0])/6.0, p2[1] - (p3[1]-p1[1])/6.0)
        s += "C%.2f %.2f %.2f %.2f %.2f %.2f " % (c1+c2+p2)
    return s + "Z"

# ---- silhouette (half-widths h from the centre line, y down) ----
outer = [(14.5,64),(14.5,80),(20,87),(30,91),(44,97),(53,100),(48,112),(43,132),(40,152),
         (38.5,184),(36.5,216),(38.5,240),(43,256),(46,270),(45,290),(43,318),(40,350),
         (38,377),(39.5,396),(40,414),(37,430),(32,446),(31,453),(33,460),(38,466),(36,471)]
inner = [(10,471),(7,467),(9,460),(12.8,452),(13,432),(13,414),(13.5,396),(13.5,377),
         (11,350),(8,322),(4.5,295),(2.5,262)]

def body_path():
    lo = [L(h, y) for h, y in outer]
    li = [L(h, y) for h, y in inner]
    ro = mirror(lo); ri = mirror(li)
    d = cr_open(lo)
    d += "L%.2f %.2f " % li[0]
    d += cr_open(li, move=False)
    d += "L%.2f %.2f " % ri[-1]
    d += cr_open(ri[::-1], move=False)
    d += "L%.2f %.2f " % ro[-1]
    d += cr_open(ro[::-1], move=False)
    return d + "Z"

arm_pts = [(50,96),(58,102),(61,114),(62,132),(64,160),(66,200),(67,236),(67.5,247),(69,258),
           (68,274),(64,288),(60,292),(57,288),(55,274),(54,260),(53.5,247),(49,215),(45,183),
           (41,156),(40,152),(44,132),(46,112),(48,100)]

def arm_path(side):
    pts = [L(h, y) for h, y in arm_pts]
    if side == 'R': pts = mirror(pts)
    return cr_closed(pts)

def head_path():
    # head: half-width ~21, top y=14, chin y=68
    hp = [(0,14),(11,15),(18,21),(21,34),(20,48),(16,60),(8,67),(0,69)]
    left = [L(h, y) for h, y in hp]
    right = mirror(left)[::-1][1:-1]
    pts = left + right
    return cr_closed(pts)

# ---- muscle regions: (view, clip, half-width polygon, mirrored) ----
REG = {
 'chest': dict(view='front', clip='torso', polys=[[(1,104),(18,103),(34,107),(44,114),(46,128),(42,142),(32,150),(16,153),(1,150)]]),
 'shoulders': dict(view='front', clip='arm', polys=[[(48,98),(57,102),(61,114),(61,130),(57,141),(50,146),(45,136),(44,118),(45,104)]]),
 'arms': dict(view='front', clip='arm', polys=[
        [(42,150),(58,146),(63,160),(64,185),(58,196),(48,192),(43,176),(41,160)],
        [(46,201),(60,199),(66,214),(67,238),(60,246),(54,244),(50,226)]]),
 'core': dict(view='front', clip='torso', polys=[
        [(1,154),(15,154),(23,163),(25,198),(23,232),(13,246),(1,250)],
        [(26,162),(41,158),(41,238),(31,245),(26,236),(27,200)]]),
 'quadriceps': dict(view='front', clip='torso', polys=[[(45,278),(34,268),(18,268),(6,276),(6,304),(10,334),(14,358),(22,370),(32,370),(38,358),(41,330),(45,300)]]),
 'back': dict(view='back', clip='torso', polys=[
        [(1,88),(12,86),(30,91),(42,98),(46,106),(42,128),(40,150),(38,186),(35,220),(28,238),(14,245),(4,247),(1,246),(1,200),(1,140),(1,100)]]),
 'glutes': dict(view='back', clip='torso', polys=[[(1,252),(20,247),(38,252),(46,266),(44,284),(34,295),(18,297),(1,294)]]),
 'hamstrings': dict(view='back', clip='torso', polys=[[(45,312),(44,300),(34,296),(18,298),(6,303),(7,326),(11,350),(17,366),(30,368),(37,356),(41,330)]]),
 'calves': dict(view='back', clip='torso', polys=[[(38,380),(30,377),(16,380),(13,395),(14,412),(19,430),(24,446),(29,446),(34,430),(41,412),(42,396)]]),
 'shoulders_b': dict(view='back', clip='arm', polys=[[(48,98),(57,102),(61,114),(61,130),(57,141),(50,146),(45,136),(44,118),(45,104)]]),
 'arms_b': dict(view='back', clip='arm', polys=[
        [(42,146),(60,144),(63,160),(63,190),(56,200),(47,194),(43,176)],
        [(46,201),(60,199),(66,214),(67,238),(60,246),(54,244),(50,226)]]),
}
GROUP_VIEW = {'chest':'front','shoulders':'front','arms':'front','core':'front','quadriceps':'front',
              'back':'back','glutes':'back','hamstrings':'back','calves':'back'}
ALIAS_BACK = {'shoulders':'shoulders_b','arms':'arms_b'}

def region_path(r, side):
    d = ""
    for poly in r['polys']:
        pts = [L(h, y) for h, y in poly]
        if side == 'R': pts = mirror(pts)
        d += cr_closed(pts) + " "
    return d

HINTS = {
 'front': [  # thin anatomical lines (half-width, y), drawn on both sides unless centre
   ('c', [(0,86),(0,252)]),
   ('s', [(14,88),(5,94),(0,93)]),
   ('s', [(14,176),(1,176)]), ('s', [(13,196),(1,196)]), ('s', [(12,216),(1,216)]),
   ('s', [(30,266),(26,296),(22,330),(21,356)]),
   ('s', [(30,374),(22,376),(15,374)]),
 ],
 'back': [
   ('c', [(0,80),(0,262)]),
   ('s', [(9,98),(24,112),(30,134),(16,138)]),
   ('s', [(30,266),(26,298)]),
   ('s', [(28,382),(26,440)]),
   ('s', [(21,298),(18,370)]),
 ],
}

def defs(uid):
    return f'''<defs>
<linearGradient id="body{uid}" x1="0" x2="1" y1="0" y2="0"><stop offset="0" stop-color="#d5d9de"/><stop offset=".5" stop-color="#f6f8fa"/><stop offset="1" stop-color="#d5d9de"/></linearGradient>
<linearGradient id="hot{uid}" x1="0" x2="0" y1="0" y2="1"><stop offset="0" stop-color="#ff9a45"/><stop offset="1" stop-color="#e5531a"/></linearGradient>
<clipPath id="torso{uid}"><path d="{body_path()}"/></clipPath>
<clipPath id="armL{uid}"><path d="{arm_path('L')}"/></clipPath>
<clipPath id="armR{uid}"><path d="{arm_path('R')}"/></clipPath>
</defs>'''

def figure(view, active, uid, colours=None, dx=0):
    """One figure. active: set of group names highlighted. colours: optional map group->fill."""
    out = [f'<g transform="translate({dx} 0)">']
    out.append(f'<path d="{body_path()}" fill="url(#body{uid})" stroke="#a9afb8" stroke-width="1.6" stroke-linejoin="round"/>')
    out.append(f'<path d="{arm_path("L")}" fill="url(#body{uid})" stroke="#a9afb8" stroke-width="1.6" stroke-linejoin="round"/>')
    out.append(f'<path d="{arm_path("R")}" fill="url(#body{uid})" stroke="#a9afb8" stroke-width="1.6" stroke-linejoin="round"/>')
    out.append(f'<path d="{head_path()}" fill="url(#body{uid})" stroke="#a9afb8" stroke-width="1.6" stroke-linejoin="round"/>')
    # inner hint lines
    for kind, pts in HINTS[view]:
        if kind == 'c':
            d = cr_open([L(h, y) for h, y in pts])
            out.append(f'<path d="{d}" fill="none" stroke="#c3c8cf" stroke-width=".8"/>')
        else:
            for side in ('L', 'R'):
                p = [L(h, y) for h, y in pts]
                if side == 'R': p = mirror(p)
                out.append(f'<path d="{cr_open(p)}" fill="none" stroke="#c3c8cf" stroke-width=".8"/>')
    # regions
    for name, r in REG.items():
        if r['view'] != view: continue
        group = name[:-2] if name.endswith('_b') else name
        on = group in active
        fill = (colours or {}).get(group, f'url(#hot{uid})') if on else 'none'
        stroke = '#c4410f' if on else '#c3c8cf'
        sw = '.9' if on else '.7'
        for side in ('L', 'R'):
            clip = f'torso{uid}' if r['clip'] == 'torso' else f'arm{side}{uid}'
            out.append(f'<path d="{region_path(r, side)}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}" stroke-linejoin="round" clip-path="url(#{clip})"/>')
    out.append('</g>')
    return "\n".join(o for o in out if o)

def single(group, uid='x'):
    view = GROUP_VIEW[group]
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 480" width="200" height="480">\n'
            + defs(uid) + "\n" + figure(view, {group}, uid) + "\n</svg>\n")

COL = {'chest':'#e6194b','shoulders':'#f58231','arms':'#ffe119','core':'#3cb44b','quadriceps':'#4363d8',
       'back':'#911eb4','glutes':'#f032e6','hamstrings':'#008080','calves':'#9a6324'}

def template():
    allg = set(COL)
    s = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 400 480" width="400" height="480">\n'
         + defs('t') + "\n")
    s += figure('front', allg, 't', colours=COL, dx=0)
    s += figure('back', allg, 't', colours=COL, dx=200)
    return s + "</svg>\n"

if __name__ == '__main__':
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for g in GROUP_VIEW:
        open(os.path.join(out, g + '.svg'), 'w').write(single(g))
    open(os.path.join(out, 'template.svg'), 'w').write(template())

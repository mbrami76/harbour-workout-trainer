#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""Generate the exercise pictograms (128x128 SVG, one colour, transparent background).

Style rules:
  * detached round head; torso = straight block with a flat top; separate pelvis block;
  * limbs are round-capped strokes, joints smooth; no feet or hands drawn;
  * every joint is gapped (2.2 units); a limb in front cuts a gap in what lies behind it;
  * one arm and one leg in side views (a second limb only when it is in a different position);
  * fixed lengths: thigh = shin = 19, upper arm = forearm = 16, torso 22, pelvis 7;
  * the floor line is always drawn (feet, hands and knees touch it at y = 108);
  * original work, no text.
Poses are built with figure(hip, torso_angle, back=, legs=, arms=); angles are degrees,
x to the right, y down (-90 = up). Contacts use ik() so limb lengths never change.

Usage:  make-exercise-icons.py OUTDIR [--png] [--sheet]
        (--png needs rsvg-convert, inkscape or convert; --sheet needs Pillow and writes OUTDIR/sheet.png)
"""
import math, sys, os
GAP = 2.2; BEND = 6.0
TW, LW, AW, HR = 16, 10, 7, 8.5
TH = SH = 19; UA = FA = 16
TORSO, PEL = 22, 7
def V(d): return (math.cos(math.radians(d)), math.sin(math.radians(d)))
def add(p, d, l): v = V(d) if not isinstance(d, tuple) else d; return (p[0]+v[0]*l, p[1]+v[1]*l)
def ik(o, t, hint, l1, l2):
    dx, dy = t[0]-o[0], t[1]-o[1]; d = math.hypot(dx, dy); mx = l1+l2-0.01
    if d > mx: t = (o[0]+dx/d*mx, o[1]+dy/d*mx); dx, dy = t[0]-o[0], t[1]-o[1]; d = mx
    a = (d*d+l1*l1-l2*l2)/(2*d); h = math.sqrt(max(l1*l1-a*a, 0)); m = (o[0]+dx/d*a, o[1]+dy/d*a); best = None
    for sg in (1, -1):
        k = (m[0]-sg*dy/d*h, m[1]+sg*dx/d*h); sc = (k[0]-m[0])*hint[0]+(k[1]-m[1])*hint[1]
        if best is None or sc > best[0]: best = (sc, k)
    return [o, best[1], t]
def chain(o, a1, a2, l1, l2): j = add(o, a1, l1); return [o, j, add(j, a2, l2)]
def leg(o, t, hint=(1, 0)): return ('limb', ik(o, t, hint, TH, SH), LW)
def legc(o, a1, a2): return ('limb', chain(o, a1, a2, TH, SH), LW)
def arm(o, t, hint=(0, 1)): return ('limb', ik(o, t, hint, UA, FA), AW)
def armc(o, a1, a2): return ('limb', chain(o, a1, a2, UA, FA), AW)

def path(pts):
    if len(pts) < 3: return "M" + " L".join("%.1f %.1f" % p for p in pts)
    d = "M%.1f %.1f" % pts[0]
    for i in range(1, len(pts)-1):
        p0, v, p1 = pts[i-1], pts[i], pts[i+1]
        l0 = math.hypot(v[0]-p0[0], v[1]-p0[1]); l1 = math.hypot(p1[0]-v[0], p1[1]-v[1])
        r0 = min(BEND, l0/2); r1 = min(BEND, l1/2)
        a = (v[0]+(p0[0]-v[0])/l0*r0, v[1]+(p0[1]-v[1])/l0*r0); b = (v[0]+(p1[0]-v[0])/l1*r1, v[1]+(p1[1]-v[1])/l1*r1)
        d += " L%.1f %.1f Q%.1f %.1f %.1f %.1f" % (a+v+b)
    return d + " L%.1f %.1f" % pts[-1]
def block_pts(p0, p1, w, r=2.5):
    dx, dy = p1[0]-p0[0], p1[1]-p0[1]; n = math.hypot(dx, dy); ux, uy = dx/n, dy/n; px, py = -uy, ux
    a = (p0[0]+ux*r, p0[1]+uy*r); b = (p1[0]-ux*r, p1[1]-uy*r); h = w/2-r
    return [(a[0]+px*h, a[1]+py*h), (b[0]+px*h, b[1]+py*h), (b[0]-px*h, b[1]-py*h), (a[0]-px*h, a[1]-py*h)]
def el(item, extra=0, colour="currentColor"):
    k = item[0]
    if k == 'limb':
        return f'<path d="{path(item[1])}" fill="none" stroke="{colour}" stroke-width="{item[2]+2*extra}" stroke-linecap="round" stroke-linejoin="round"/>'
    if k == 'block':
        pts = " ".join("%.1f,%.1f" % p for p in block_pts(item[1], item[2], item[3]))
        return f'<polygon points="{pts}" fill="{colour}" stroke="{colour}" stroke-width="{5+2*extra}" stroke-linejoin="round"/>'
    if k == 'circle':
        return f'<circle cx="{item[1]:.1f}" cy="{item[2]:.1f}" r="{item[3]+extra}" fill="{colour}"/>'
    if k == 'line':
        return f'<path d="M{item[1][0]} {item[1][1]} L{item[2][0]} {item[2][1]}" stroke="{colour}" stroke-width="{3.5+2*extra}" stroke-linecap="round"/>'
def render(layers, uid):
    defs, body = [], []
    for i, layer in enumerate(layers):
        cut = "".join(el(it, GAP, "black") for l in layers[i+1:] for it in l)
        shapes = "".join(el(it) for it in layer)
        if cut:
            defs.append(f'<mask id="m{uid}{i}" maskUnits="userSpaceOnUse" x="0" y="0" width="128" height="128"><rect width="128" height="128" fill="white"/>{cut}</mask>')
            body.append(f'<g mask="url(#m{uid}{i})">{shapes}</g>')
        else: body.append(f'<g>{shapes}</g>')
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="128" height="128" color="#3f4146">'
            f'<defs>{"".join(defs)}</defs>' + "".join(body) + '</svg>\n')

FLOOR = ('line', (6, 116), (122, 116))
def wall(x): return ('line', (x, 6), (x, 116))
ICONS = {}
def figure(H, ta, back=(), legs=(), arms=(), extra=(), head=None):
    u = V(ta); S = add(H, u, TORSO+2.2+PEL/2)
    body = [('block', add(H, u, PEL/2+2.2), S, TW), ('block', add(H, u, -PEL/2), add(H, u, PEL/2), TW),
            ('circle',) + (head or (add(S, u, 4+HR)+(HR,)))]
    return [[FLOOR]+list(extra), list(back), body, list(legs), list(arms)]
def shoulder(H, ta, below=6): return add(H, V(ta), TORSO+2.2+PEL/2-below)
def hipo(H, ta): return add(H, V(ta), -(PEL/2+2.2+LW/2))
FY = 108

# ---------- front views
def front(dy=0, arm_a=None, leg_a=0, legs_end=None):
    H = (64, 59.3+dy); S = shoulder(H, -90, 3.5-0)
    res = figure(H, -90)
    sx = S[1]
    O1, O2 = (64-6, 70+dy), (64+6, 70+dy)
    if leg_a == 0: lg = [legc(O1, 90, 90), legc(O2, 90, 90)]
    else: lg = [legc(O1, 90+leg_a, 90+leg_a), legc(O2, 90-leg_a, 90-leg_a)]
    if arm_a is None: ar = [armc((64-13.7, sx), 90, 90), armc((64+13.7, sx), 90, 90)]
    else: ar = [armc((64-13.7, sx), 180-arm_a, 180-arm_a), armc((64+13.7, sx), arm_a, arm_a)]
    res[3] = lg; res[4] = ar
    return res
ICONS['default'] = front()
ICONS['calf-raise'] = front(dy=-6)
ICONS['arm-circles'] = front(arm_a=0)
ICONS['jumping-jack'] = front(dy=-6, arm_a=-40, leg_a=22)
H = (64, 66.3)
ICONS['side-lunge'] = figure(H, -90, legs=[leg((58, 77), (36, FY), (-1, 0)), leg((70, 77), (84, FY), (1, 0))],
                             arms=[armc((64-13.7, 42), 125, 70), armc((64+13.7, 42), 55, 110)])

# ---------- plank family
def line_body(F, deg):
    u = (math.cos(math.radians(deg)), -math.sin(math.radians(deg))); ta = -deg
    O = (F[0]+u[0]*38, F[1]+u[1]*38); H = (O[0]+u[0]*(PEL/2+2.2+LW/2), O[1]+u[1]*(PEL/2+2.2+LW/2))
    return O, H, ta
O, H, ta = line_body((10, FY), 27); P = shoulder(H, ta)
ICONS['push-up'] = figure(H, ta, legs=[leg(O, (10, FY), (0, -1))], arms=[arm(P, (P[0], FY), (-1, 0))])
O, H, ta = line_body((10, FY), 13); P = shoulder(H, ta)
ICONS['plank'] = figure(H, ta, legs=[leg(O, (10, FY), (0, -1))], arms=[('limb', [P, (P[0], FY), (P[0]+UA, FY)], AW)])
K = (28, FY); u = V(-34); O = add(K, u, TH); H = add(O, u, PEL/2+2.2+LW/2); ta = -34; P = shoulder(H, ta)
ICONS['knee-push-up'] = figure(H, ta, legs=[('limb', [O, K, add(K, 200, SH)], LW)], arms=[arm(P, (P[0], FY), (-1, 0))])
F = (30, FY); O, H, ta = line_body(F, 44.7); P = shoulder(H, ta)
ICONS['wall-push-up'] = figure(H, ta, legs=[leg(O, F, (0, -1))], arms=[arm(P, (112, P[1]+2), (0, 1))], extra=[wall(118)])
O, H, ta = line_body((10, FY), 27); P = shoulder(H, ta)
ICONS['mountain-climber'] = figure(H, ta, back=[leg(O, (10, FY), (0, -1))], legs=[('limb', chain(O, 50, 170, TH, SH), LW)], arms=[arm(P, (P[0], FY), (-1, 0))])

# ---------- lying on the back / seated
def lying(Hx, ta, y=104.5): return (Hx, y), ta
H, ta = lying(66, 215); P = shoulder(H, ta); O = (76, 106)
ICONS['crunch'] = figure(H, ta, legs=[leg(O, (94, FY), (0, -1))], arms=[arm(P, (P[0]+28, P[1]-12), (0, -1))])
H, ta = lying(62, 252); P = shoulder(H, ta); O = (72.5, 106)
ICONS['sit-up'] = figure(H, ta, legs=[leg(O, (92, FY), (0, -1))], arms=[arm(P, (P[0]+30, P[1]-8), (0, -1))])
H, ta = lying(54, 232); P = shoulder(H, ta); O = (62, 106)
ICONS['russian-twist'] = figure(H, ta, legs=[leg(O, (90, FY), (0, -1))], arms=[arm(P, (P[0]+26, P[1]-10), (0, -1))])
H, ta = lying(66, 205); P = shoulder(H, ta); O = (76, 106)
ICONS['bicycle-crunch'] = figure(H, ta, back=[legc(O, -5, -5)], legs=[legc(O, -65, 15)], arms=[arm(P, (P[0]+24, P[1]-14), (0, -1))])
H, ta = (72, 105), 180; P = shoulder(H, ta); O = (82, 105)
ICONS['dead-bug'] = figure(H, ta, back=[legc(O, -3, -3)], legs=[legc(O, -90, 0)], arms=[arm(P, (P[0], P[1]-40), (1, 0))])
S0 = (26, 104); ta = 168; H = add(S0, V(-12), TORSO+2.2+PEL/2); P = shoulder(H, ta); O = add(H, V(-12), PEL/2+2.2+LW/2)
ICONS['glute-bridge'] = figure(H, ta, legs=[leg(O, (92, FY), (0, -1))], head=(7, 105.5, HR))
H, ta = (40, 105), -48; P = shoulder(H, ta); O = (50, 106)
ICONS['hamstring-stretch'] = figure(H, ta, legs=[legc(O, 0, 0)], arms=[arm(P, (84, 94), (0, -1))])
H, ta = (56, 102), -8; P = shoulder(H, ta); O = (45.5, 103.5)
ICONS['superman'] = figure(H, ta, back=[arm(P, (P[0]+28, P[1]-15), (0, -1))], legs=[legc(O, 183, 183)], head=(96, 103, HR))
H, ta = (64, 105), 180; P = shoulder(H, ta); O = (74.5, 105)
ICONS['side-leg-raise'] = figure(H, ta, back=[legc(O, 0, 0)], legs=[legc(O, -40, -40)])
K = (52, FY); O = (34, 96); H = (38, 92); ta = 20; P = shoulder(H, ta)
ICONS['child-pose'] = figure(H, ta, back=[arm(P, (P[0]+32, FY), (0, 1))], legs=[('limb', ik(O, (33, FY), (1, 0.2), TH, SH), LW)], head=(add(add(H, V(20), 27.7), V(20), 12.5)+(HR,)))
# quadruped
K = (48, FY); O = (48, 89); u = V(-12); H = add(O, u, PEL/2+2.2+LW/2); ta = -12; P = shoulder(H, ta)
ICONS['bird-dog'] = figure(H, ta, back=[armc(P, -15, -15)], legs=[('limb', [O, K, (K[0]-SH, FY)], LW), legc(O, 183, 183), arm(P, (P[0]+4, FY), (-1, 0))])
ICONS['donkey-kick'] = figure(H, ta, back=[('limb', [O, K, (K[0]-SH, FY)], LW)], legs=[legc(O, 185, 268), arm(P, (P[0]+4, FY), (-1, 0))])

# ---------- standing, side view
H = (50, 64); ta = -25; P = shoulder(H, ta); O = (48, 70)
ICONS['good-morning'] = figure(H, ta, legs=[legc(O, 90, 90)], arms=[arm(P, (P[0], P[1]+32), (1, 0))])
O = (45, 89); ta = -70; H = add(O, V(ta), PEL/2+2.2+LW/2); P = shoulder(H, ta)
ICONS['squat'] = figure(H, ta, legs=[leg(O, (64, FY), (1, 0))], arms=[arm(P, (P[0]+32, P[1]+1), (0, 1))])
O = (69, 89); H = (69, 78.3)
ICONS['lunge'] = figure(H, -90, back=[legc(O, 120, 200)], legs=[leg(O, (88, FY), (1, 0)), armc(shoulder(H, -90), 120, 50)])
O = (28, 89); H = (28, 78.3)
ICONS['wall-sit'] = figure(H, -90, legs=[leg(O, (47, FY), (1, 0))], arms=[arm(shoulder(H, -90), (60, 56.6), (0, 1))], extra=[wall(12)])
H = (60, 59.3); P = shoulder(H, -90); O = (60, 70)
ICONS['high-knees'] = figure(H, -90, back=[armc(P, 120, 220)], legs=[legc(O, 90, 90), legc(O, 0, 90), armc(P, 60, -40)])
H = (64, 59.3); P = shoulder(H, -90); O = (64, 70)
ICONS['standing-quad-stretch'] = figure(H, -90, back=[legc(O, 100, 255)], legs=[legc(O, 80, 80), arm(P, (55.8, 70.3), (0, 1))])
# ---------- more exercises of the starter library
# standing, side view
H = (60, 63); P = shoulder(H, -90); O = (60, 73.7)
ICONS['heel-walk'] = figure(H, -90, back=[armc(P, 105, 80)], legs=[leg(O, (46, FY), (-1, 0)), leg(O, (73, FY), (1, 0)), armc(P, 75, 60)])
H = (54, 59.3); P = shoulder(H, -90)
ICONS['wall-angel'] = figure(H, -90, legs=[legc((54, 70), 90, 90)], arms=[armc(P, -95, -90)], extra=[wall(43)])
res = front(); sy = shoulder((64, 59.3), -90, 3.5)[1]
res[4] = [armc((64-13.7, sy), 90, 90), armc((64+13.7, sy), 50, -110)]
ICONS['self-resisted-curl'] = res
O = (50, 70); ta = -10; H = add(O, V(ta), PEL/2+2.2+LW/2); P = shoulder(H, ta)
ICONS['single-leg-hinge'] = figure(H, ta, legs=[legc(O, 90, 90), legc(O, 190, 190)], arms=[arm(P, (P[0], P[1]+32), (1, 0))])
O = (45, 89); ta = -70; H = add(O, V(ta), PEL/2+2.2+LW/2); P = shoulder(H, ta)
ICONS['pistol-squat'] = figure(H, ta, legs=[leg(O, (64, FY), (1, 0)), legc(O, -3, -3), arm(P, (P[0]+32, P[1]+1), (0, 1))])
H = (60, 54.3); P = shoulder(H, 90); O = add(H, V(90), -(PEL/2+2.2+LW/2))
ICONS['wall-handstand-hold'] = figure(H, 90, legs=[legc(O, -77, -77)], arms=[arm(P, (68, FY), (1, 0))], extra=[wall(75)], head=(54, 93, HR))

# on hands and knees / on the floor
K = (52, FY); O = (52, 89); u = V(-12); H = add(O, u, PEL/2+2.2+LW/2); ta = -12; P = shoulder(H, ta)
ICONS['wrist-rock'] = figure(H, ta, legs=[('limb', [O, K, (K[0]-SH, FY)], LW), arm(P, (P[0]+4, FY), (-1, 0))])
O, H, ta = line_body((10, FY), 27); P = shoulder(H, ta)
ICONS['shoulder-tap'] = figure(H, ta, legs=[leg(O, (10, FY), (0, -1))], arms=[armc(P, 40, -150), arm(P, (P[0], FY), (-1, 0))])
O, H, ta = line_body((10, FY), 22); P = shoulder(H, ta)
ICONS['side-plank'] = figure(H, ta, legs=[leg(O, (10, FY), (0, -1)), leg((O[0]-2.6, O[1]-6.5), (7.4, FY-6.5), (0, -1))], arms=[('limb', [P, (P[0], FY), (P[0]+UA, FY)], AW)])

# prone
H = (56, 102); ta = -8; P = shoulder(H, ta); O = (45.5, 103.5)
ICONS['prone-y-raise'] = figure(H, ta, back=[armc(P, -35, -35)], legs=[legc(O, 181, 181), armc(P, -62, -62)], head=(96, 103, HR))
ICONS['prone-sweep'] = figure(H, ta, back=[armc(P, -158, -158)], legs=[legc(O, 181, 181), armc(P, -172, -172)], head=(96, 103, HR))

# on the back
H, ta = (72, 105), 180; P = shoulder(H, ta); O = (82, 105)
ICONS['leg-raise'] = figure(H, ta, legs=[legc(O, -90, -90)], arms=[armc(P, -130, -130)])
H, ta = (72, 105), 195; P = shoulder(H, ta); O = add(H, V(ta), -(PEL/2+2.2+LW/2))
ICONS['hollow-hold'] = figure(H, ta, legs=[legc(O, -15, -15)], arms=[armc(P, -30, -30)])
H, ta = (66, 96), -125; P = shoulder(H, ta); O = add(H, V(ta), -(PEL/2+2.2+LW/2))
ICONS['v-up'] = figure(H, ta, legs=[legc(O, -45, -45)], arms=[armc(P, -5, -5)])


# ---------- additional exercise icons (task 46; poses for the new starter exercises)
N = ICONS
def figw(H, ta, tw, back=(), legs=(), arms=(), head=None):
    u = V(ta); S = add(H, u, TORSO+2.2+PEL/2)
    body = [('block', add(H, u, PEL/2+2.2), S, tw), ('block', add(H, u, -PEL/2), add(H, u, PEL/2), tw),
            ('circle',) + (head or (add(S, u, 4+HR)+(HR,)))]
    return [[FLOOR], list(back), body, list(legs), list(arms)]
HO = PEL/2+2.2+LW/2
sxF = shoulder((64,59.3), -90, 3.5)[1]
# burpee: crouch, hands on the floor
O=(58,96); ta=-35; H=add(O,V(ta),HO); P=shoulder(H,ta)
N['burpee']=figure(H,ta,legs=[leg(O,(56,FY),(1,0))],arms=[arm(P,(P[0]+6,FY),(-1,0))])
# bear crawl
O=(40,84); ta=-15; H=add(O,V(ta),HO); P=shoulder(H,ta)
N['bear-crawl']=figure(H,ta,legs=[leg(O,(36,FY),(1,0))],arms=[arm(P,(P[0]+4,FY),(-1,0))])
# pike push-up
O=(44,74); ta=25; H=add(O,V(ta),HO); P=shoulder(H,ta)
N['pike-push-up']=figure(H,ta,legs=[leg(O,(26,FY),(0,-1))],arms=[arm(P,(P[0]+8,FY),(-1,0))])
# standing knee to elbow (front)
res=front(); H=(64,59.3)
res[3]=[legc((58,70),90,90), legc((70,70),-130,100)]
res[4]=[armc((64-13.7,sxF),95,60), armc((64+13.7,sxF),60,150)]
N['knee-to-elbow']=res
# oblique crunch (front): knee out to the side, elbow down
H=(62,59.3); ta=-84
res=figure(H,ta,legs=[legc((57,70),90,90), legc((69,70),0,95)],
           arms=[armc((64-13.7,sxF),-100,-170), armc((64+13.7-2,sxF),60,150)])
N['oblique-crunch']=res
# torso rotation: legs seen from the front, torso narrow (turned)
H=(64,59.3)
N['torso-rotation']=figw(H,-90,8,legs=[legc((58,70),90,90),legc((70,70),90,90)],
    arms=[armc((64,sxF),0,0), armc((64,sxF),180,170)])
# triceps stretch
res=front(); L=(64-13.7,sxF); R=(64+13.7,sxF)
res[4]=[armc(R,-100,150), arm(L,(76,36),(0,1))]
N['triceps-stretch']=res
# cross-body shoulder stretch
res=front(); res[4]=[armc(R,180,180), arm(L,(60,sxF+3),(0,1))]
N['cross-body-stretch']=res
# fire hydrant (side view)
K=(48,FY); O=(48,89); u=V(-12); H=add(O,u,HO); ta=-12; P=shoulder(H,ta)
N['fire-hydrant']=figure(H,ta,back=[('limb',[O,K,(K[0]-SH,FY)],LW)],legs=[legc(O,182,95), arm(P,(P[0]+4,FY),(-1,0))])
# clamshell: feet together, knees apart
H,ta=(60,105),180; O=(72,105); F=(O[0]+26,105)
N['clamshell']=figure(H,ta,back=[('limb',ik(O,F,(0,1),TH,SH),LW)],legs=[('limb',ik(O,F,(0,-1),TH,SH),LW)])
# cat (arched)
K=(48,FY); O=(48,86); ta=2; H=add(O,V(ta),HO); P=shoulder(H,ta); S=add(H,V(ta),TORSO+2.2+PEL/2)
N['cat-cow']=figure(H,ta,legs=[('limb',[O,K,(K[0]-SH,FY)],LW), arm(P,(P[0]-2,FY),(-1,0))],head=(S[0]+6,S[1]+15,HR))
# hip circles
H=(68,59.3); ta=-96; P=shoulder(H,ta)
N['hip-circles']=figure(H,ta,legs=[legc((62,70),90,90),legc((74,70),90,90)],
    arms=[arm((64-13.7+2,sxF),(58,67),(-1,0)), arm((64+13.7+2,sxF),(78,67),(1,0))])
# thoracic rotation
K=(48,FY); O=(48,89); u=V(-12); H=add(O,u,HO); ta=-12; P=shoulder(H,ta)
N['thoracic-rotation']=figure(H,ta,back=[armc(P,-100,-100)],legs=[('limb',[O,K,(K[0]-SH,FY)],LW), arm(P,(P[0]+4,FY),(-1,0))])
# world's greatest stretch
O=(54,92); ta=-50; H=add(O,V(ta),HO); P=shoulder(H,ta)
N['worlds-greatest-stretch']=figure(H,ta,back=[legc(O,120,200),armc(P,-120,-120)],legs=[leg(O,(78,FY),(1,0)),arm(P,(P[0]+4,FY),(0,1))])
# leg swings
H=(56,59.3); P=shoulder(H,-90); O=(56,70)
N['leg-swings']=figure(H,-90,back=[armc(P,120,200),legc(O,-28,-28)],legs=[legc(O,90,90),armc(P,50,-30)])
# kneeling hip flexor stretch
O=(62,89); H=(62,78.3)
N['hip-flexor-stretch']=figure(H,-90,back=[legc(O,90,180)],legs=[leg(O,(86,FY),(1,0)),arm(shoulder(H,-90),(O[0]+2,O[1]-9),(-1,0))])
# cobra
H=(76,103); ta=-65; P=shoulder(H,ta); O=add(H,V(ta),-HO)
N['cobra']=figure(H,ta,legs=[legc((H[0]-8,104),180,180),arm(P,(P[0]+6,FY),(-1,0))],head=(add(add(H,V(ta),27.7),V(ta),9)+(HR,)))
# figure four (standing)
H=(54,64); ta=-80; P=shoulder(H,ta); O=add(H,V(ta),-HO)
N['figure-four']=figure(H,ta,back=[legc(O,92,92)],legs=[('limb',ik(O,(O[0]+1,O[1]+13),(1,-0.4),TH,SH),LW),armc(P,60,70)])
# seated forward fold
H=(30,103); ta=-40; P=shoulder(H,ta); O=(40,104.5)
N['seated-forward-fold']=figure(H,ta,legs=[legc(O,0,0),arm(P,(P[0]+30,FY-6),(0,-1))])
# doorway chest stretch
H=(70,59.3); ta=-84; P=shoulder(H,ta); O=(70,70)
N['chest-stretch']=figure(H,ta,back=[armc(P,100,100)],legs=[legc(O,90,90),armc(P,0,-90)])
N['chest-stretch'][0]=[FLOOR,wall(P[0]+16+AW/2+3)]
# flutter kick
H,ta=(72,105),180; P=shoulder(H,ta); O=(82,105)
N['flutter-kick']=figure(H,ta,back=[legc(O,-32,-32)],legs=[legc(O,-10,-10),armc(P,-150,-150)])
# reverse crunch
H,ta=(66,102),180; P=shoulder(H,ta); O=(76,102)
N['reverse-crunch']=figure(H,ta,legs=[legc(O,-78,10),armc(P,-150,-150)])
# butt kicks
H=(60,59.3); P=shoulder(H,-90); O=(60,70)
N['butt-kicks']=figure(H,-90,back=[armc(P,120,200)],legs=[legc(O,90,90),legc(O,100,200),armc(P,50,-40)])
# neck rolls: standing, head tipped to one side
res=front(); c=res[2][2]
res[2][2]=('circle',c[1]+8,c[2]+2.5,c[3])
N['neck-rolls']=res
# ankle circles
H=(60,59.3); P=shoulder(H,-90); O=(60,70)
ring=[(math.cos(math.radians(a))*7+98,math.sin(math.radians(a))*7+98) for a in range(0,361,60)]
N['ankle-circles']=figure(H,-90,back=[armc(P,110,100)],legs=[legc(O,90,90),leg(O,(76,FY),(1,0)),armc(P,70,60)])
# frog jump
H=(60,60); ta=-28; P=shoulder(H,ta); O=add(H,V(ta),-HO)
N['frog-jump']=figure(H,ta,legs=[legc(O,160,165),armc(P,-12,-12)])

for n in ICONS: pass
def svg(name): return render(ICONS[name], name[:2])

def to_png(svg, png):
    """256 x 256 transparent PNG with the first renderer found: rsvg-convert, inkscape, ImageMagick."""
    import shutil, subprocess
    if shutil.which('rsvg-convert'): cmd = ['rsvg-convert', '-w', '256', '-h', '256', svg, '-o', png]
    elif shutil.which('inkscape'): cmd = ['inkscape', svg, '-w', '256', '-h', '256', '-o', png]
    elif shutil.which('convert'): cmd = ['convert', '-background', 'none', '-density', '192', svg, '-resize', '256x256', '-strip', png]
    else: raise SystemExit('no SVG renderer found (rsvg-convert, inkscape or convert)')
    subprocess.run(cmd, check=True)

def main():
    import argparse, subprocess
    ap = argparse.ArgumentParser(); ap.add_argument('out'); ap.add_argument('--png', action='store_true'); ap.add_argument('--sheet', action='store_true')
    a = ap.parse_args(); os.makedirs(a.out, exist_ok=True)
    for n in sorted(ICONS):
        p = os.path.join(a.out, n + '.svg'); open(p, 'w').write(svg(n))
        if a.png or a.sheet: to_png(p, os.path.join(a.out, n + '.png'))
    if a.sheet:
        from PIL import Image, ImageDraw
        names = sorted(ICONS); cols = 7; t = 160; rows = (len(names)+cols-1)//cols
        sh = Image.new('RGB', (cols*t, rows*t), 'white'); d = ImageDraw.Draw(sh)
        for i, n in enumerate(names):
            im = Image.open(os.path.join(a.out, n + '.png')).convert('RGBA').resize((t, t)); bg = Image.new('RGB', (t, t), 'white'); bg.paste(im, mask=im.split()[3])
            sh.paste(bg, ((i % cols)*t, (i//cols)*t)); d.text(((i % cols)*t+4, (i//cols)*t+2), n, fill=(200, 0, 0))
        sh.save(os.path.join(a.out, 'sheet.png'))

if __name__ == '__main__': main()

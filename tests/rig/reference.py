#!/usr/bin/env python3
"""Reference implementation of the part-space -> canvas mapping, straight from shared/rig.json.

Every port (shared/Rig.hpp for Windows + Linux, Sources/CatRenderer.swift, homepage/pet/renderer.js and the
KuroCat-Workshop renderer.js) is checked against the golden table this script writes:

  python3 tests/rig/reference.py --write   regenerate tests/rig/golden.tsv after changing rig.json
  python3 tests/rig/reference.py --check   exit 1 if golden.tsv is stale

golden.tsv, one case per line, tab-separated:
  head  x y skull dome headScale headYaw headPitch headTilt headX headY  ex ey
  body  x y bodyWidth bodySquash lieIntensity bodyTilt seatX seatY      ex ey
"""
import itertools
import json
import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
RIG = json.loads((HERE.parent.parent / 'shared/rig.json').read_text(encoding='utf-8'))
HT, HS, BD = RIG['headTurn'], RIG['headSurface'], RIG['bodyDeform']
GOLDEN = HERE / 'golden.tsv'


def head_depth(x, y, dome):
    if not dome:
        return HS['planeDepth']
    radius = max(0.0, 1 - x * x / HS['domeRadiusX'] ** 2 - y * y / HS['domeRadiusY'] ** 2)
    return HS['domeDepth'] * math.sqrt(radius)


def head_point(x, y, skull, dome, head_scale, yaw, pitch, tilt, hx, hy):
    if skull:
        qx, qy = x, y
    else:
        z = head_depth(x, y, dome)
        ya, pa = yaw * HT['yawPerUnit'], pitch * HT['pitchPerUnit']
        qx = x * math.cos(ya) + z * math.sin(ya)
        z1 = -x * math.sin(ya) + z * math.cos(ya)
        qy = y * math.cos(pa) + z1 * math.sin(pa)
    qx, qy = qx * head_scale, qy * head_scale
    c, s = math.cos(tilt), math.sin(tilt)
    return hx + qx * c - qy * s, hy + qx * s + qy * c


def body_point(x, y, body_width, squash, lie, tilt, sx0, sy0):
    lie = min(1.0, max(0.0, lie))
    lie_s = lie * lie * (3 - 2 * lie)
    sy = squash * (1 - lie_s * BD['lieSquash'])
    sx = (1 + (1 - squash) * BD['squashWiden']) * (1 + lie_s * BD['lieWiden'])
    c, s = math.cos(tilt), math.sin(tilt)
    return sx0 + x * body_width * sx * c - y * sy * s, sy0 + x * body_width * sx * s + y * sy * c


def fmt(v):
    return repr(float(v))


def cases():
    rows = []
    head_pts = [(0, 0), (17.2, -2), (-30, -25), (40, 10), (0, -38), (45, -5), (60, -30), (-80, 12), (20, 33)]
    poses = [(0, 0, 0), (1, 0, 0), (-1, 0, 0), (0.6, -0.65, -0.13), (-1, 0.65, 0.13), (0.3, 1, 0.2)]
    for (x, y), skull, dome, hs, (yaw, pitch, tilt) in itertools.product(head_pts, (0, 1), (1, 0), (1.0, 1.18), poses):
        hx, hy = 184 + yaw * HT['headShiftPerYaw'], 126.0
        ex, ey = head_point(x, y, skull, dome, hs, yaw, pitch, tilt, hx, hy)
        rows.append(['head', x, y, skull, dome, hs, yaw, pitch, tilt, hx, hy, ex, ey])
    body_pts = [(0, 0), (-52, -14), (40, -90), (30, 12), (-70, 20)]
    for (x, y), bw, (squash, lie), tilt in itertools.product(body_pts, (1.0, 1.3), [(1, 0), (0.85, 0), (1.12, 0), (1, 0.5), (0.9, 1)], (0, 0.08, -0.12)):
        sx0, sy0 = RIG['seatAnchor']['x'], RIG['seatAnchor']['y']
        ex, ey = body_point(x, y, bw, squash, lie, tilt, sx0, sy0)
        rows.append(['body', x, y, bw, squash, lie, tilt, sx0, sy0, ex, ey])
    return '\n'.join('\t'.join([r[0]] + [fmt(v) for v in r[1:]]) for r in rows) + '\n'


def main():
    text = cases()
    if '--write' in sys.argv:
        GOLDEN.write_text(text, encoding='utf-8')
        print(f'wrote {GOLDEN} ({text.count(chr(10))} cases)')
    elif '--check' in sys.argv:
        if not GOLDEN.exists() or GOLDEN.read_text(encoding='utf-8') != text:
            print('tests/rig/golden.tsv is stale: run python3 tests/rig/reference.py --write', file=sys.stderr)
            sys.exit(1)
    else:
        print(__doc__)


if __name__ == '__main__':
    main()

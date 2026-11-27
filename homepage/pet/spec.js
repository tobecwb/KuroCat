/* KuroCat .kurocat v1 specification helpers, validator, defaults, and preset generators */
(function (global) {
  'use strict';

  const VALID_SPACES = ['head', 'skull', 'body', 'tail', 'leftEar', 'rightEar', 'leftArm', 'rightArm', 'hindLeg'];
  const VALID_BRUSHES = ['round', 'soft', 'airbrush', 'smudge'];
  const PALETTE_KEYS = [
    'body', 'headBody', 'bodyShade', 'belly', 'legs', 'paws', 'pawPads',
    'hindLeg', 'hindPaw',
    'earOuter', 'earInner', 'nose', 'mouth',
    'iris', 'irisLeft', 'irisRight', 'pupil', 'pupilLeft', 'pupilRight',
    'tail', 'tailTip', 'stripes', 'whiskers', 'crease',
    'outline', 'points', 'pointsMid'
  ];

  const SHAPE_RANGES = {
    bodyWidth: [0.85, 1.35, 1.0],
    headScale: [0.90, 1.15, 1.0],
    cheekFullness: [0.85, 1.25, 1.0],
    neckWidth: [0.65, 1.40, 1.0],
    earSize: [0.80, 1.25, 1.0],
    earBaseWidth: [0.65, 1.40, 1.0],
    earPointiness: [0.50, 1.50, 1.0],
    earSpacing: [0.65, 1.35, 1.0],
    tailLength: [0.70, 1.30, 1.0],
    tailThickness: [0.70, 1.50, 1.0],
    tailRootY: [-14.0, 14.0, 0.0],
    armSpacing: [0.55, 1.60, 1.0],
    armRootY: [-18.0, 18.0, 0.0],
    armLength: [0.65, 1.45, 1.0],
    hindLegRootX: [-16.0, 16.0, 0.0],
    hindLegRootY: [-14.0, 14.0, 0.0],
    hindLegWidth: [0.70, 1.45, 1.0],
    hindLegReach: [0.75, 1.35, 1.0],
    pawStagger: [0.0, 1.0, 1.0]
  };

  // What the desktop parser clamps to (KuroCat/shared/SkinSpec.hpp, Sources/CatPhysics.swift). SHAPE_RANGES above is
  // the editor's slider range and may be narrower; parsing a file always clamps to these so the web pet and the
  // desktop draw the same cat from the same numbers.
  const SHAPE_LIMITS = {
    bodyWidth: [0.85, 1.35], headScale: [0.90, 1.15], cheekFullness: [0.85, 1.25], neckWidth: [0.65, 1.40],
    earSize: [0.80, 1.25], earBaseWidth: [0.65, 1.40], earPointiness: [0.50, 1.50], earSpacing: [0.60, 1.50],
    tailLength: [0.70, 1.30], tailThickness: [0.70, 1.50], tailRootY: [-30.0, 30.0],
    armSpacing: [0.50, 1.60], armRootY: [-30.0, 30.0], armLength: [0.60, 1.50],
    hindLegRootX: [-24.0, 24.0], hindLegRootY: [-24.0, 24.0], hindLegWidth: [0.60, 1.60], hindLegReach: [0.65, 1.50],
    pawStagger: [0.0, 1.0]
  };

  function clamp(v, lo, hi) {
    return Math.min(hi, Math.max(lo, v));
  }

  function round2(v) {
    return Math.round(v * 100) / 100;
  }

  function isValidHex(str) {
    return typeof str === 'string' && /^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/.test(str);
  }

  function parseHexColor(str) {
    if (!isValidHex(str)) return null;
    const r = parseInt(str.slice(1, 3), 16) / 255;
    const g = parseInt(str.slice(3, 5), 16) / 255;
    const b = parseInt(str.slice(5, 7), 16) / 255;
    const a = str.length === 9 ? parseInt(str.slice(7, 9), 16) / 255 : 1;
    return { r, g, b, a };
  }

  function rgbaToCss(c, alphaMul) {
    if (!c) return 'rgba(0,0,0,0)';
    const a = clamp((c.a === undefined ? 1 : c.a) * (alphaMul === undefined ? 1 : alphaMul), 0, 1);
    return `rgba(${Math.round(clamp(c.r, 0, 1) * 255)}, ${Math.round(clamp(c.g, 0, 1) * 255)}, ${Math.round(clamp(c.b, 0, 1) * 255)}, ${a.toFixed(4)})`;
  }

  function rgbToHex(r, g, b, a) {
    const h = (n) => clamp(Math.round(n), 0, 255).toString(16).padStart(2, '0').toUpperCase();
    if (a !== undefined && a < 0.996) {
      return '#' + h(r) + h(g) + h(b) + h(a * 255);
    }
    return '#' + h(r) + h(g) + h(b);
  }

  function luminance(c) {
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
  }

  function sameColor(a, b) {
    if (!a || !b) return false;
    return Math.abs(a.r - b.r) < 1e-4 && Math.abs(a.g - b.g) < 1e-4 &&
           Math.abs(a.b - b.b) < 1e-4 && Math.abs(a.a - b.a) < 1e-4;
  }

  function cubicPoint(a, b, t) {
    const u = 1 - t;
    const outX = a.outX !== undefined ? a.outX : a.x;
    const outY = a.outY !== undefined ? a.outY : a.y;
    const inX = b.inX !== undefined ? b.inX : b.x;
    const inY = b.inY !== undefined ? b.inY : b.y;
    return {
      x: u * u * u * a.x + 3 * u * u * t * outX + 3 * u * t * t * inX + t * t * t * b.x,
      y: u * u * u * a.y + 3 * u * u * t * outY + 3 * u * t * t * inY + t * t * t * b.y
    };
  }

  function flattenRegion(anchors) {
    const flat = [];
    const n = anchors.length;
    for (let i = 0; i < n; i++) {
      const a = anchors[i];
      const b = anchors[(i + 1) % n];
      for (let step = 0; step < 16; step++) {
        flat.push(cubicPoint(a, b, step / 16));
      }
    }
    return flat;
  }

  // Open path through the anchors (pen tool preview): 16 steps per cubic, endpoints included.
  function flattenOpen(anchors) {
    const flat = [];
    for (let i = 0; i + 1 < anchors.length; i++) {
      for (let step = 0; step < 16; step++) flat.push(cubicPoint(anchors[i], anchors[i + 1], step / 16));
    }
    if (anchors.length) flat.push({ x: anchors[anchors.length - 1].x, y: anchors[anchors.length - 1].y });
    return flat;
  }

  function pointInPolygon(pt, poly) {
    let inside = false;
    for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
      const a = poly[i], b = poly[j];
      if ((a.y > pt.y) !== (b.y > pt.y) && pt.x < (b.x - a.x) * (pt.y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
  }

  // Pen tool 「分割线」: an open curve drawn across a part splits it in two. Each side becomes an ordinary
  // closed region: the curve, straight extensions from both ends (along the end tangents) out to a box that
  // covers the whole part, and the box corners on that side. Regions are clipped to their part when drawn,
  // so the box only has to be big enough; the tail box stays small because tail space bends along the tail.
  const SPLIT_BOX = {
    head: { x0: -300, y0: -300, x1: 300, y1: 300 },
    skull: { x0: -300, y0: -300, x1: 300, y1: 300 },
    body: { x0: -300, y0: -300, x1: 300, y1: 300 },
    tail: { x0: -4, y0: -40, x1: 104, y1: 40 },
    leftEar: { x0: -200, y0: -260, x1: 200, y1: 60 },
    rightEar: { x0: -200, y0: -260, x1: 200, y1: 60 },
    leftArm: { x0: -4, y0: -24, x1: 104, y1: 24 },
    rightArm: { x0: -4, y0: -24, x1: 104, y1: 24 }
  };

  function unitVec(x, y) {
    const len = Math.hypot(x, y);
    return len > 1e-9 ? { x: x / len, y: y / len } : null;
  }

  function rayToBox(p, d, box) {
    let t = Infinity;
    if (d.x > 1e-9) t = Math.min(t, (box.x1 - p.x) / d.x);
    if (d.x < -1e-9) t = Math.min(t, (box.x0 - p.x) / d.x);
    if (d.y > 1e-9) t = Math.min(t, (box.y1 - p.y) / d.y);
    if (d.y < -1e-9) t = Math.min(t, (box.y0 - p.y) / d.y);
    if (!isFinite(t) || t < 0) t = 0;
    const q = { x: clamp(p.x + d.x * t, box.x0, box.x1), y: clamp(p.y + d.y * t, box.y0, box.y1) };
    // a point that started outside the box is pushed onto its nearest edge
    const dl = q.x - box.x0, dr = box.x1 - q.x, dt = q.y - box.y0, db = box.y1 - q.y;
    const m = Math.min(dl, dr, dt, db);
    if (m > 1e-6) {
      if (m === dl) q.x = box.x0; else if (m === dr) q.x = box.x1; else if (m === dt) q.y = box.y0; else q.y = box.y1;
    }
    return q;
  }

  // perimeter parameter in [0,4): top edge left->right, right edge down, bottom edge right->left, left edge up
  function boxParam(q, box) {
    const w = box.x1 - box.x0, h = box.y1 - box.y0, eps = 1e-6;
    if (Math.abs(q.y - box.y0) < eps && q.x < box.x1 - eps) return (q.x - box.x0) / w;
    if (Math.abs(q.x - box.x1) < eps && q.y < box.y1 - eps) return 1 + (q.y - box.y0) / h;
    if (Math.abs(q.y - box.y1) < eps && q.x > box.x0 + eps) return 2 + (box.x1 - q.x) / w;
    return 3 + (box.y1 - q.y) / h;
  }

  function boxCornersBetween(sFrom, sTo, forward, box) {
    const corners = [
      { s: 0, x: box.x0, y: box.y0 }, { s: 1, x: box.x1, y: box.y0 },
      { s: 2, x: box.x1, y: box.y1 }, { s: 3, x: box.x0, y: box.y1 }
    ];
    const span = forward ? (sTo - sFrom + 4) % 4 || 4 : (sFrom - sTo + 4) % 4 || 4;
    return corners
      .map((c) => ({ c, k: forward ? (c.s - sFrom + 4) % 4 : (sFrom - c.s + 4) % 4 }))
      .filter((e) => e.k > 1e-9 && e.k < span - 1e-9)
      .sort((a, b) => a.k - b.k)
      .map((e) => ({ x: e.c.x, y: e.c.y }));
  }

  // Returns both sides of an open pen curve as closed anchor lists (null when the curve is too short).
  function splitSides(anchors, space) {
    if (!anchors || anchors.length < 2) return null;
    const box = SPLIT_BOX[space] || SPLIT_BOX.body;
    const n = anchors.length;
    const A0 = anchors[0], An = anchors[n - 1];
    const chord = unitVec(An.x - A0.x, An.y - A0.y) || { x: 1, y: 0 };
    const startRef = A0.outX !== undefined && Math.hypot(A0.outX - A0.x, A0.outY - A0.y) > 0.05
      ? { x: A0.outX, y: A0.outY } : anchors[1];
    const endRef = An.inX !== undefined && Math.hypot(An.inX - An.x, An.inY - An.y) > 0.05
      ? { x: An.inX, y: An.inY } : anchors[n - 2];
    const d0 = unitVec(A0.x - startRef.x, A0.y - startRef.y) || { x: -chord.x, y: -chord.y };
    const d1 = unitVec(An.x - endRef.x, An.y - endRef.y) || chord;
    const E0 = rayToBox(A0, d0, box), E1 = rayToBox(An, d1, box);
    E0.x = round2(E0.x); E0.y = round2(E0.y); E1.x = round2(E1.x); E1.y = round2(E1.y);
    const s0 = boxParam(E0, box), s1 = boxParam(E1, box);
    const curve = anchors.map((a, i) => {
      const c = { x: a.x, y: a.y };
      if (i > 0 && a.inX !== undefined) { c.inX = a.inX; c.inY = a.inY; }
      if (i < n - 1 && a.outX !== undefined) { c.outX = a.outX; c.outY = a.outY; }
      return c;
    });
    const make = (forward) => curve.concat([{ x: E1.x, y: E1.y }], boxCornersBetween(s1, s0, forward, box), [{ x: E0.x, y: E0.y }]);
    return { sides: [make(true), make(false)], ends: [{ from: A0, to: E0 }, { from: An, to: E1 }] };
  }

  // The side of a pen curve that contains pt (part-space point), as closed region anchors.
  function pickSplitSide(anchors, space, pt) {
    const split = splitSides(anchors, space);
    if (!split) return null;
    for (const side of split.sides) if (pointInPolygon(pt, flattenRegion(side))) return side;
    return null;
  }

  // Exact match to shared/SkinSpec.hpp skinJawSide()
  function jawSide(headSide, cheekFullness) {
    const c = cheekFullness;
    const edgeY = headSide ? -400 : 400;
    const pts = [{ x: -400, y: edgeY }, { x: 400, y: edgeY }, { x: 400, y: 8 }, { x: 44 * c, y: 8 }];
    const cubicTo = (p0, p1, p2, p3) => {
      for (let i = 1; i <= 16; i++) {
        const t = i / 16, u = 1 - t;
        pts.push({
          x: u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x,
          y: u * u * u * p0.y + 3 * u * u * t * p1.y + 3 * u * t * t * p2.y + t * t * t * p3.y
        });
      }
    };
    cubicTo({ x: 44 * c, y: 8 }, { x: 44 * c, y: 22 }, { x: 24 * c, y: 34 }, { x: 0, y: 34 });
    cubicTo({ x: 0, y: 34 }, { x: -24 * c, y: 34 }, { x: -44 * c, y: 22 }, { x: -44 * c, y: 8 });
    pts.push({ x: -400, y: 8 });
    return pts;
  }

  // Validate a raw JSON object against formatVersion 1 rules (matching shared/SkinSpec.hpp)
  // --- Decorations (stickers): base64 PNG helpers shared by the editor and 喵包机 ---
  const DECO_ANCHORS = ['headTop', 'face', 'neck', 'tailTip'];

  function stripDataUrl(b64) {
    return typeof b64 === 'string' ? b64.replace(/^data:image\/png;base64,/, '').replace(/\s+/g, '') : '';
  }

  // { bytes, width, height } of a base64 PNG, or null (signature + IHDR only, like skinjson::pngSize)
  function pngInfo(b64) {
    const clean = stripDataUrl(b64);
    if (!clean || !/^[A-Za-z0-9+/]*={0,2}$/.test(clean)) return null;
    let head;
    try { head = atob(clean.slice(0, 48)); } catch (_) { return null; }
    const sig = '\x89PNG\r\n\x1a\n';
    if (head.length < 24 || head.slice(0, 8) !== sig || head.slice(12, 16) !== 'IHDR') return null;
    const u32 = (o) => ((head.charCodeAt(o) << 24) >>> 0) + (head.charCodeAt(o + 1) << 16) + (head.charCodeAt(o + 2) << 8) + head.charCodeAt(o + 3);
    const pad = clean.endsWith('==') ? 2 : clean.endsWith('=') ? 1 : 0;
    return { bytes: Math.floor(clean.length / 4) * 3 - pad, width: u32(16), height: u32(20) };
  }

  // --- Left/right mirroring (face and body centre line is x = 0; ears swap sides, tails have no mirror) ---
  const MIRROR_EAR = { leftEar: 'rightEar', rightEar: 'leftEar', leftArm: 'rightArm', rightArm: 'leftArm' };

  function canMirror(space) {
    return space !== 'tail';
  }

  function mirrorAnchors(anchors) {
    return anchors.map((a) => {
      const m = { x: round2(-a.x), y: a.y };
      if (a.inX !== undefined) { m.inX = round2(-a.inX); m.inY = a.inY; }
      if (a.outX !== undefined) { m.outX = round2(-a.outX); m.outY = a.outY; }
      return m;
    });
  }

  function mirrorRegion(r) {
    const copy = JSON.parse(JSON.stringify(r));
    if (MIRROR_EAR[r.space]) copy.space = MIRROR_EAR[r.space];
    else copy.anchors = mirrorAnchors(r.anchors);
    return copy;
  }

  function mirrorMark(m) {
    const copy = JSON.parse(JSON.stringify(m));
    if (MIRROR_EAR[m.space]) copy.space = MIRROR_EAR[m.space];
    else copy.points = m.points.map(([x, y]) => [round2(-x), y]);
    return copy;
  }

  function validateRawSkin(raw) {
    if (!raw || typeof raw !== 'object' || Array.isArray(raw)) {
      return { ok: false, error: '皮肤文件最外层必须是 JSON 对象' };
    }
    if (!Number.isInteger(raw.formatVersion) || raw.formatVersion < 1) {
      return { ok: false, error: 'formatVersion 必须是正整数' };
    }
    if (raw.formatVersion > FORMAT_VERSION_MAX) {
      return { ok: false, error: `这个皮肤使用了更新的格式（formatVersion ${raw.formatVersion}），请刷新工坊页面` };
    }
    if (raw.minAppVersion !== undefined && !isVersionString(raw.minAppVersion)) {
      return { ok: false, error: 'minAppVersion 必须是形如 "1.5.3" 的版本号' };
    }
    const name = typeof raw.name === 'string' ? raw.name.trim() : '';
    if (!name) return { ok: false, error: '皮肤名称 name 不能为空' };
    if (name.length > 40) return { ok: false, error: '皮肤名称不能超过 40 字' };
    if (raw.author !== undefined && raw.author !== null) {
      if (typeof raw.author !== 'string') return { ok: false, error: 'author 必须是字符串' };
    }
    if (!raw.palette || typeof raw.palette !== 'object') {
      return { ok: false, error: '缺少 palette 调色板' };
    }
    if (!isValidHex(raw.palette.body)) {
      return { ok: false, error: 'palette.body 必须是 #RRGGBB 或 #RRGGBBAA' };
    }
    for (const key of PALETTE_KEYS) {
      const val = raw.palette[key];
      if (val !== undefined && val !== null && !isValidHex(val)) {
        return { ok: false, error: `palette.${key} 颜色格式无效 (${val})` };
      }
    }
    if (raw.shape) {
      if (raw.shape.build && raw.shape.build !== 'standard' && raw.shape.build !== 'sleek') {
        return { ok: false, error: 'shape.build 必须是 standard 或 sleek' };
      }
      for (const [k, [lo, hi]] of Object.entries(SHAPE_RANGES)) {
        if (raw.shape[k] !== undefined && (typeof raw.shape[k] !== 'number' || !isFinite(raw.shape[k]))) {
          return { ok: false, error: `shape.${k} 必须是数字` };
        }
      }
    }
    if (raw.idleLegs != null && raw.idleLegs !== 'legs' && raw.idleLegs !== 'chest') {
      return { ok: false, error: 'idleLegs 必须是 legs 或 chest' };
    }
    if (raw.headSurface && raw.headSurface !== 'dome' && raw.headSurface !== 'plane') {
      return { ok: false, error: 'headSurface 必须是 dome 或 plane' };
    }
    if (raw.regions) {
      if (!Array.isArray(raw.regions)) return { ok: false, error: 'regions 必须是数组' };
      if (raw.regions.length > 64) return { ok: false, error: 'regions 最多 64 个分区' };
      for (let i = 0; i < raw.regions.length; i++) {
        const r = raw.regions[i];
        if (!VALID_SPACES.includes(r.space)) return { ok: false, error: `regions[${i}].space 无效` };
        if (!isValidHex(r.fill)) return { ok: false, error: `regions[${i}].fill 颜色无效` };
        if (!Array.isArray(r.anchors) || r.anchors.length < 2 || r.anchors.length > 128) {
          return { ok: false, error: `regions[${i}].anchors 需要 2-128 个锚点` };
        }
        for (let k = 0; k < r.anchors.length; k++) {
          const a = r.anchors[k];
          if (typeof a.x !== 'number' || typeof a.y !== 'number') {
            return { ok: false, error: `regions[${i}].anchors[${k}] 缺少 x/y` };
          }
        }
      }
    }
    if (raw.marks) {
      if (!Array.isArray(raw.marks)) return { ok: false, error: 'marks 必须是数组' };
      if (raw.marks.length > MAX_MARKS) return { ok: false, error: `marks 最多 ${MAX_MARKS} 条` };
      for (let i = 0; i < raw.marks.length; i++) {
        const m = raw.marks[i];
        if (!VALID_SPACES.includes(m.space)) return { ok: false, error: `marks[${i}].space 无效` };
        if (m.kind !== 'polygon' && m.kind !== 'stroke') return { ok: false, error: `marks[${i}].kind 无效` };
        if (m.color && !isValidHex(m.color)) return { ok: false, error: `marks[${i}].color 无效` };
        if (!m.color && !isValidHex(raw.palette.stripes)) {
          return { ok: false, error: `marks[${i}] 缺少 color 且无 palette.stripes` };
        }
        const minPts = m.kind === 'stroke' ? 2 : 3;
        if (!Array.isArray(m.points) || m.points.length < minPts || m.points.length > 512) {
          return { ok: false, error: `marks[${i}].points 数量无效` };
        }
      }
    }
    if (raw.decorations) {
      if (!Array.isArray(raw.decorations)) return { ok: false, error: 'decorations 必须是数组' };
      if (raw.decorations.length > 8) return { ok: false, error: 'decorations 最多 8 个贴纸' };
      for (let i = 0; i < raw.decorations.length; i++) {
        const d = raw.decorations[i];
        if (!d || typeof d !== 'object') return { ok: false, error: `decorations[${i}] 必须是对象` };
        const info = pngInfo(d.png);
        if (!info) return { ok: false, error: `decorations[${i}].png 必须是 base64 编码的 PNG` };
        if (info.bytes > 1024 * 1024) return { ok: false, error: `decorations[${i}].png 超过 1 MB` };
        if (info.width > 1024 || info.height > 1024) return { ok: false, error: `decorations[${i}].png 尺寸超过 1024×1024` };
        if (!DECO_ANCHORS.includes(d.anchor)) return { ok: false, error: `decorations[${i}].anchor 必须是 headTop / face / neck / tailTip 之一` };
        for (const k of ['offsetX', 'offsetY', 'scale', 'rotation']) {
          if (d[k] !== undefined && (typeof d[k] !== 'number' || !isFinite(d[k]))) return { ok: false, error: `decorations[${i}].${k} 必须是数字` };
        }
      }
    }
    return { ok: true, error: '' };
  }

  // Resolve defaults for runtime rendering (exact match to shared/SkinSpec.hpp lines 450-482)
  function resolveSkin(raw) {
    const pal = {};
    for (const k of PALETTE_KEYS) {
      pal[k] = raw.palette && raw.palette[k] ? parseHexColor(raw.palette[k]) : null;
    }
    if (!pal.body) pal.body = parseHexColor('#161618');
    if (!pal.legs) pal.legs = { ...pal.body };
    if (!pal.paws) pal.paws = pal.points ? { ...pal.points } : { ...pal.legs };
    if (!pal.hindLeg) pal.hindLeg = Object.assign({}, pal.legs);
    if (!pal.hindPaw) pal.hindPaw = Object.assign({}, pal.paws);
    if (!pal.irisLeft && pal.iris) pal.irisLeft = Object.assign({}, pal.iris);
    if (!pal.irisRight && pal.iris) pal.irisRight = Object.assign({}, pal.iris);
    if (!pal.pupilLeft && pal.pupil) pal.pupilLeft = Object.assign({}, pal.pupil);
    if (!pal.pupilRight && pal.pupil) pal.pupilRight = Object.assign({}, pal.pupil);
    if (!pal.earOuter) pal.earOuter = pal.points ? { ...pal.points } : { ...pal.legs };
    if (!pal.tail) pal.tail = { ...pal.earOuter };
    if (pal.points && !pal.pointsMid) {
      pal.pointsMid = {
        r: (pal.points.r + pal.body.r) * 0.5,
        g: (pal.points.g + pal.body.g) * 0.5,
        b: (pal.points.b + pal.body.b) * 0.5,
        a: 1
      };
    }
    const darkBody = luminance(pal.body) < 0.45;
    if (!pal.whiskers) {
      pal.whiskers = darkBody
        ? { r: 0.88, g: 0.90, b: 0.92, a: 0.68 }
        : { r: 0x8D / 255, g: 0x81 / 255, b: 0x70 / 255, a: 0.68 };
    }
    if (!pal.crease) {
      pal.crease = darkBody
        ? { r: 0.82, g: 0.92, b: 0.89, a: 0.76 }
        : { r: 0x8B / 255, g: 0x81 / 255, b: 0x71 / 255, a: 0.60 };
    }

    const shape = { build: (raw.shape && raw.shape.build === 'sleek') ? 'sleek' : 'standard' };
    for (const [k, [, , def]] of Object.entries(SHAPE_RANGES)) {
      const [lo, hi] = SHAPE_LIMITS[k];
      const v = raw.shape && typeof raw.shape[k] === 'number' ? raw.shape[k] : def;
      shape[k] = clamp(v, lo, hi);
    }

    const regions = (raw.regions || []).map((r) => {
      const fill = parseHexColor(r.fill) || { r: 1, g: 1, b: 1, a: 1 };
      const alpha = typeof r.alpha === 'number' ? clamp(r.alpha, 0, 1) : 1;
      const anchors = r.anchors.map((a) => ({
        x: clamp(a.x, -400, 400),
        y: clamp(a.y, -400, 400),
        inX: clamp(a.inX !== undefined ? a.inX : a.x, -400, 400),
        inY: clamp(a.inY !== undefined ? a.inY : a.y, -400, 400),
        outX: clamp(a.outX !== undefined ? a.outX : a.x, -400, 400),
        outY: clamp(a.outY !== undefined ? a.outY : a.y, -400, 400)
      }));
      return {
        space: r.space,
        fill: { ...fill, a: fill.a * alpha },
        anchors,
        flat: flattenRegion(anchors)
      };
    });

    const marks = (raw.marks || []).map((m) => {
      const baseCol = m.color ? parseHexColor(m.color) : pal.stripes;
      const col = baseCol ? Object.assign({}, baseCol) : { r: 0.2, g: 0.2, b: 0.2, a: 1 };
      const alpha = typeof m.alpha === 'number' ? clamp(m.alpha, 0, 1) : 1;
      col.a *= alpha;
      const brush = VALID_BRUSHES.includes(m.brush) ? m.brush : 'round';
      const softness = typeof m.softness === 'number' ? clamp(m.softness, 0, 1) : 0.65;
      return {
        space: m.space,
        stroke: m.kind === 'stroke',
        width: m.kind === 'stroke' ? clamp(typeof m.width === 'number' ? m.width : 2, 0.2, 24) : 0,
        brush,
        softness,
        color: col,
        points: m.points.map(([x, y]) => ({ x: clamp(x, -400, 400), y: clamp(y, -400, 400) }))
      };
    });

    const partGradients = {};
    if (raw.partGradients && typeof raw.partGradients === 'object') {
      for (const sp of VALID_SPACES) {
        const g = raw.partGradients[sp];
        if (g && isValidHex(g.from) && isValidHex(g.to)) {
          partGradients[sp] = {
            from: parseHexColor(g.from),
            to: parseHexColor(g.to),
            stop: typeof g.stop === 'number' ? clamp(g.stop, 0.1, 0.9) : 0.5
          };
        }
      }
    }

    const hasPartArt = (space) =>
      regions.some((r) => r.space === space) || marks.some((m) => m.space === space);

    return {
      name: (raw.name || '未命名皮肤').trim().slice(0, 40),
      author: (raw.author || '').trim().slice(0, 40),
      palette: pal,
      shape,
      partGradients,
      headSurface: raw.headSurface === 'plane' ? 'plane' : 'dome',
      renderStyle: (raw.renderStyle === 'douni' || raw.renderStyle === 'dazed' || raw.renderStyle === 'damiao' || raw.renderStyle === 'naimi' || raw.renderStyle === 'beagle') ? raw.renderStyle : 'standard',
      typingStyle: raw.typingStyle === 'douni' ? 'douni' : 'standard',
      // "legs" (default): sitting front legs tinted with palette.legs; "chest": built-in coat look
      idleLegsTinted: raw.idleLegs !== 'chest',
      regions,
      marks,
      decorations: (raw.decorations || []).filter((d) => d && DECO_ANCHORS.includes(d.anchor) && pngInfo(d.png)).slice(0, 8).map((d) => ({
        png: stripDataUrl(d.png),
        anchor: d.anchor,
        offsetX: clamp(typeof d.offsetX === 'number' ? d.offsetX : 0, -120, 120),
        offsetY: clamp(typeof d.offsetY === 'number' ? d.offsetY : 0, -120, 120),
        scale: clamp(typeof d.scale === 'number' ? d.scale : 1, 0.05, 4),
        rotation: clamp(typeof d.rotation === 'number' ? d.rotation : 0, -180, 180)
      })),
      colorpoint: Boolean(pal.points),
      hasPartArt
    };
  }

  // Produce a clean, minimal, spec-strict .kurocat v1 JSON object for export
  function sanitizeForExport(raw, previewBase64) {
    const clean = {
      formatVersion: 1,
      name: ((raw.name || '我的小猫').trim() || '我的小猫').slice(0, 40)
    };
    const author = (raw.author || '').trim().slice(0, 40);
    if (author) clean.author = author;

    clean.palette = {};
    for (const k of PALETTE_KEYS) {
      const v = raw.palette && raw.palette[k];
      if (isValidHex(v)) clean.palette[k] = v.toUpperCase();
    }
    if (!clean.palette.body) clean.palette.body = '#161618';

    const shape = {};
    if (raw.shape && raw.shape.build === 'sleek') shape.build = 'sleek';
    else shape.build = 'standard';
    for (const [k, [, , def]] of Object.entries(SHAPE_RANGES)) {
      const [lo, hi] = SHAPE_LIMITS[k];
      const v = raw.shape && typeof raw.shape[k] === 'number' ? clamp(raw.shape[k], lo, hi) : def;
      if (Math.abs(v - def) > 0.004) shape[k] = round2(v);
    }
    clean.shape = shape;
    clean.headSurface = raw.headSurface === 'plane' ? 'plane' : 'dome';
    if (raw.renderStyle === 'douni' || raw.renderStyle === 'dazed' || raw.renderStyle === 'damiao' || raw.renderStyle === 'naimi' || raw.renderStyle === 'beagle') clean.renderStyle = raw.renderStyle;
    if (raw.typingStyle === 'douni') clean.typingStyle = 'douni';
    if (raw.idleLegs === 'chest') clean.idleLegs = 'chest';

    if (raw.regions && raw.regions.length > 0) {
      // Unknown spaces are dropped, never re-labelled (an arm stripe must not land on the face).
      clean.regions = raw.regions.filter((r) => r && VALID_SPACES.includes(r.space)).slice(0, 64).map((r) => {
        const out = {
          space: r.space,
          fill: isValidHex(r.fill) ? r.fill.toUpperCase() : '#FAF6EE',
          anchors: r.anchors.slice(0, 128).map((a) => {
            const pt = { x: round2(clamp(a.x, -400, 400)), y: round2(clamp(a.y, -400, 400)) };
            if (a.inX !== undefined && (Math.abs(a.inX - a.x) > 0.05 || Math.abs(a.inY - a.y) > 0.05)) {
              pt.inX = round2(clamp(a.inX, -400, 400));
              pt.inY = round2(clamp(a.inY, -400, 400));
            }
            if (a.outX !== undefined && (Math.abs(a.outX - a.x) > 0.05 || Math.abs(a.outY - a.y) > 0.05)) {
              pt.outX = round2(clamp(a.outX, -400, 400));
              pt.outY = round2(clamp(a.outY, -400, 400));
            }
            return pt;
          })
        };
        if (typeof r.alpha === 'number' && r.alpha < 0.999) out.alpha = round2(clamp(r.alpha, 0, 1));
        return out;
      });
    }

    if (raw.marks && raw.marks.length > 0) {
      clean.marks = raw.marks.filter((m) => m && VALID_SPACES.includes(m.space)).slice(0, MAX_MARKS).map((m) => {
        const out = {
          space: m.space,
          kind: m.kind === 'stroke' ? 'stroke' : 'polygon',
          color: isValidHex(m.color) ? m.color.toUpperCase() : (clean.palette.stripes || '#3A3A3A'),
          points: m.points.slice(0, 512).map(([x, y]) => [round2(clamp(x, -400, 400)), round2(clamp(y, -400, 400))])
        };
        if (typeof m.alpha === 'number' && m.alpha < 0.999) out.alpha = round2(clamp(m.alpha, 0, 1));
        if (out.kind === 'stroke') {
          out.width = round2(clamp(typeof m.width === 'number' ? m.width : 2.2, 0.2, 24));
          if (VALID_BRUSHES.includes(m.brush) && m.brush !== 'round') {
            out.brush = m.brush;
            if (typeof m.softness === 'number') out.softness = round2(clamp(m.softness, 0, 1));
          }
        }
        return out;
      });
    }

    if (raw.partGradients && typeof raw.partGradients === 'object') {
      const pgOut = {};
      let pgCount = 0;
      for (const sp of VALID_SPACES) {
        const g = raw.partGradients[sp];
        if (g && isValidHex(g.from) && isValidHex(g.to)) {
          pgOut[sp] = {
            from: g.from.toUpperCase(),
            to: g.to.toUpperCase(),
            stop: round2(clamp(typeof g.stop === 'number' ? g.stop : 0.5, 0.1, 0.9))
          };
          pgCount++;
        }
      }
      if (pgCount > 0) clean.partGradients = pgOut;
    }

    if (raw.decorations && raw.decorations.length > 0) {
      const decos = raw.decorations.filter((d) => d && DECO_ANCHORS.includes(d.anchor) && pngInfo(d.png)).slice(0, 8);
      if (decos.length) {
        clean.decorations = decos.map((d) => ({
          png: stripDataUrl(d.png),
          anchor: d.anchor,
          offsetX: round2(clamp(typeof d.offsetX === 'number' ? d.offsetX : 0, -120, 120)),
          offsetY: round2(clamp(typeof d.offsetY === 'number' ? d.offsetY : 0, -120, 120)),
          scale: round2(clamp(typeof d.scale === 'number' ? d.scale : 1, 0.05, 4)),
          rotation: round2(clamp(typeof d.rotation === 'number' ? d.rotation : 0, -180, 180))
        }));
      }
    }

    if (previewBase64 && typeof previewBase64 === 'string') {
      clean.preview = previewBase64;
    }
    const need = requiredAppVersion(clean).version;
    clean.formatVersion = compareVersions(need, LAST_FORMAT1_APP_VERSION) > 0 ? 2 : 1;
    if (compareVersions(need, BASE_APP_VERSION) > 0) {
      // Keep it right after name/author in the file, where people look.
      const { formatVersion, name, author, ...rest } = clean;
      return Object.assign({ formatVersion, name }, author ? { author } : {}, { minAppVersion: need }, rest);
    }
    return clean;
  }

  // Reusable Bezier region & mark builders
  function makePresetRegion(kind, color) {
    const fill = color || '#FAF4EA';
    switch (kind) {
      case 'whiteBelly':
        return {
          _label: '白肚皮与围脖',
          space: 'body',
          fill,
          anchors: [
            { x: -34, y: -112, inX: -36, inY: -106, outX: -12, outY: -112 },
            { x: 34, y: -112, inX: 12, inY: -112, outX: 36, outY: -106 },
            { x: 38, y: -44, inX: 40, inY: -76, outX: 35, outY: -16 },
            { x: 0, y: 2, inX: 24, inY: 2, outX: -24, outY: 2 },
            { x: -38, y: -44, inX: -35, inY: -16, outX: -40, outY: -76 }
          ]
        };
      case 'blazeFace':
        return {
          _label: '八字白脸',
          space: 'head',
          fill,
          anchors: [
            { x: 0, y: -16, inX: -4, inY: -12, outX: 4, outY: -12 },
            { x: 14, y: 7, inX: 7, inY: -4, outX: 25, outY: 13 },
            { x: 40, y: 24, inX: 34, inY: 17, outX: 44, outY: 36 },
            { x: 0, y: 46, inX: 26, inY: 46, outX: -26, outY: 46 },
            { x: -40, y: 24, inX: -44, inY: 36, outX: -34, outY: 17 },
            { x: -14, y: 7, inX: -25, inY: 13, outX: -7, outY: -4 }
          ]
        };
      case 'muzzleWhite':
        return {
          _label: '圆润白嘴套',
          space: 'head',
          fill,
          anchors: [
            { x: 0, y: 3, inX: -15, inY: 3, outX: 15, outY: 3 },
            { x: 26, y: 18, inX: 26, inY: 8, outX: 27, outY: 30 },
            { x: 0, y: 44, inX: 18, inY: 44, outX: -18, outY: 44 },
            { x: -26, y: 18, inX: -27, inY: 30, outX: -26, outY: 8 }
          ]
        };
      case 'leftPatch':
        return {
          _label: '左额头花斑',
          space: 'head',
          fill: color || '#D9803B',
          anchors: [
            { x: -62, y: -56, inX: -62, inY: -36, outX: -32, outY: -56 },
            { x: 4, y: -54, inX: -14, inY: -54, outX: 8, outY: -32 },
            { x: -4, y: -12, inX: 2, inY: -22, outX: -14, outY: -2 },
            { x: -30, y: 8, inX: -18, inY: 6, outX: -46, outY: 12 },
            { x: -62, y: 4, inX: -54, inY: 10, outX: -62, outY: -24 }
          ]
        };
      case 'rightPatch':
        return {
          _label: '右额头花斑',
          space: 'head',
          fill: color || '#2C2725',
          anchors: [
            { x: -4, y: -54, inX: -8, inY: -32, outX: 18, outY: -56 },
            { x: 62, y: -56, inX: 34, inY: -56, outX: 62, outY: -24 },
            { x: 62, y: 6, inX: 62, inY: -18, outX: 52, outY: 12 },
            { x: 28, y: 10, inX: 44, inY: 12, outX: 16, outY: 4 },
            { x: 6, y: -14, inX: 14, inY: -4, outX: 0, outY: -26 }
          ]
        };
      case 'earTips':
        return [
          {
            _label: '左耳尖配色',
            space: 'leftEar',
            fill: color || '#B36324',
            anchors: [
              { x: -60, y: -42, inX: -60, inY: -42, outX: -20, outY: -38 },
              { x: 60, y: -42, inX: 20, inY: -38, outX: 60, outY: -42 },
              { x: 60, y: -118, inX: 60, inY: -118, outX: 60, outY: -118 },
              { x: -60, y: -118, inX: -60, inY: -118, outX: -60, outY: -118 }
            ]
          },
          {
            _label: '右耳尖配色',
            space: 'rightEar',
            fill: color || '#B36324',
            anchors: [
              { x: -60, y: -42, inX: -60, inY: -42, outX: -20, outY: -38 },
              { x: 60, y: -42, inX: 20, inY: -38, outX: 60, outY: -42 },
              { x: 60, y: -118, inX: 60, inY: -118, outX: 60, outY: -118 },
              { x: -60, y: -118, inX: -60, inY: -118, outX: -60, outY: -118 }
            ]
          }
        ];
      default:
        return null;
    }
  }

  function makeTabbyForeheadMarks(color, alpha) {
    const c = color || '#8C5222';
    const a = alpha === undefined ? 0.62 : alpha;
    return [
      { space: 'head', kind: 'stroke', color: c, alpha: a, width: 2.8, points: [[-11, -33], [-9, -23], [-7, -15]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a, width: 3.1, points: [[0, -34], [0, -23], [0, -14]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a, width: 2.8, points: [[11, -33], [9, -23], [7, -15]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a * 0.85, width: 2.2, points: [[-42, 2], [-35, 5], [-29, 6]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a * 0.85, width: 2.0, points: [[-42, 11], [-35, 13], [-30, 13]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a * 0.85, width: 2.2, points: [[42, 2], [35, 5], [29, 6]] },
      { space: 'head', kind: 'stroke', color: c, alpha: a * 0.85, width: 2.0, points: [[42, 11], [35, 13], [30, 13]] }
    ];
  }

  function makeTailRingMarks(color, alpha) {
    const c = color || '#8C5222';
    const a = alpha === undefined ? 0.65 : alpha;
    return [
      { space: 'tail', kind: 'stroke', color: c, alpha: a, width: 3.6, points: [[28, -9], [28, 9]] },
      { space: 'tail', kind: 'stroke', color: c, alpha: a, width: 3.8, points: [[52, -9], [52, 9]] },
      { space: 'tail', kind: 'stroke', color: c, alpha: a, width: 4.0, points: [[76, -9], [76, 9]] }
    ];
  }

  function makeArmRingMarks(color, alpha, count, width, sides) {
    const c = color || '#6E4C32';
    const a = alpha === undefined ? 0.78 : clamp(alpha, 0.1, 1);
    const n = clamp(Math.round(count === undefined ? 3 : count), 1, 6);
    const w = clamp(width === undefined ? 2.8 : width, 1.0, 6.0);
    const targets = sides === 'leftArm' ? ['leftArm'] : sides === 'rightArm' ? ['rightArm'] : ['leftArm', 'rightArm'];
    const out = [];
    for (const sp of targets) {
      for (let i = 0; i < n; i++) {
        const x = n === 1 ? 44 : round2(22 + (42 * i) / (n - 1));
        out.push({
          space: sp,
          kind: 'stroke',
          color: c,
          alpha: round2(a),
          width: round2(w),
          points: [[x, -11], [round2(x + 2.5), 0], [x, 11]]
        });
      }
    }
    return out;
  }

  // Built-in & Popular Breed Templates
  const OFFICIAL_TEMPLATES = [
    {
      id: 'black',
      file: './skins/black.kurocat',
      tag: '原版经典',
      fallback: {
        formatVersion: 1, name: '经典小黑猫', author: 'KuroCat',
        palette: { body: '#161618', legs: '#161618', paws: '#161618', earOuter: '#161618', tail: '#161618', whiskers: '#E0E6EBAD', crease: '#D1EBE3C2' },
        shape: { build: 'standard' }, headSurface: 'dome', idleLegs: 'chest', marks: []
      }
    },
    {
      id: 'siamese',
      file: './skins/siamese.kurocat',
      tag: '小豆泥风格',
      fallback: {
        formatVersion: 1, name: '挖煤暹罗猫', author: 'KuroCat',
        palette: {
          body: '#F6EFE4', bodyShade: '#EADBC8', points: '#35261F', pointsMid: '#6A5041',
          legs: '#35261F', paws: '#35261F', earOuter: '#35261F', tail: '#35261F',
          earInner: '#5D463A', outline: '#AD96806B', crease: '#8D78678A',
          whiskers: '#7A63539E', nose: '#241714F2', mouth: '#C7A88F8C'
        },
        shape: { build: 'sleek' }, headSurface: 'dome', idleLegs: 'chest', marks: []
      }
    },
    {
      id: 'shorthair',
      file: './skins/shorthair.kurocat',
      tag: '银虎斑加白',
      fallback: {
        formatVersion: 1, name: '美短（银虎斑加白）', author: 'KuroCat',
        palette: { body: '#C2C5C1', legs: '#F2EEE4', earOuter: '#C2C5C1', tail: '#C2C5C1', belly: '#F2EEE4', paws: '#F2EEE4', stripes: '#55595A', whiskers: '#8D8170AD', crease: '#8B817199' },
        shape: { build: 'standard', pawStagger: 0 }, headSurface: 'plane', idleLegs: 'chest',
        regions: [makePresetRegion('whiteBelly', '#F2EEE4'), makePresetRegion('blazeFace', '#F2EEE4')],
        marks: [...makeTabbyForeheadMarks('#363536', 0.8), ...makeTailRingMarks('#55595A', 0.55)]
      }
    },
    {
      id: 'orangeWhite',
      file: './skins/orangeWhite.kurocat',
      tag: '暖橘加白',
      fallback: {
        formatVersion: 1, name: '橘白猫', author: 'KuroCat',
        palette: { body: '#D8AA73', legs: '#F2EEE4', earOuter: '#D8AA73', tail: '#D8AA73', belly: '#F2EEE4', paws: '#F2EEE4', stripes: '#A57543', whiskers: '#8D8170AD', crease: '#8B817199' },
        shape: { build: 'standard', pawStagger: 0 }, headSurface: 'dome', idleLegs: 'chest',
        regions: [makePresetRegion('whiteBelly', '#F2EEE4'), makePresetRegion('blazeFace', '#F2EEE4')],
        marks: [...makeTabbyForeheadMarks('#A57543', 0.52), ...makeTailRingMarks('#A57543', 0.52)]
      }
    }
  ];

  const EXTRA_PRESETS = [
    {
      id: 'tuxedo',
      tag: '黑白礼服',
      spec: {
        formatVersion: 1, name: '燕尾服奶牛猫', author: 'KuroCat 皮肤工坊',
        palette: {
          body: '#1F2024', bodyShade: '#15161A', belly: '#FAF7F1', legs: '#1F2024', paws: '#FAF7F1',
          pawPads: '#E99CA6', earOuter: '#1F2024', earInner: '#4A3F42',
          nose: '#E58D99', mouth: '#8D7D76', iris: '#D6B74E', tail: '#1F2024', tailTip: '#FAF7F1',
          whiskers: '#E8ECF0B8', crease: '#B5AEA68F'
        },
        shape: { build: 'standard', bodyWidth: 1.06, cheekFullness: 1.08, pawStagger: 0 },
        headSurface: 'dome',
        regions: [makePresetRegion('whiteBelly', '#FAF7F1'), makePresetRegion('blazeFace', '#FAF7F1')],
        marks: []
      }
    },
    {
      id: 'calico',
      tag: '暖橘黑三花',
      spec: {
        formatVersion: 1, name: '软萌三花猫', author: 'KuroCat 皮肤工坊',
        palette: {
          body: '#FAF5EC', bodyShade: '#F1E8DA', belly: '#FAF5EC', legs: '#FAF5EC', paws: '#FAF5EC',
          pawPads: '#EAA1A7', earOuter: '#E08C45', earInner: '#F4BEB8',
          nose: '#E69196', mouth: '#A38375', iris: '#B5C954', tail: '#2E2724', tailTip: '#E08C45',
          outline: '#B8A6925C', whiskers: '#8D8170A8', crease: '#9C8D7E80'
        },
        shape: { build: 'standard', bodyWidth: 1.12, cheekFullness: 1.12, pawStagger: 0 },
        headSurface: 'dome',
        regions: [
          makePresetRegion('leftPatch', '#E08C45'),
          makePresetRegion('rightPatch', '#2E2724'),
          {
            _label: '右背黑斑', space: 'body', fill: '#2E2724',
            anchors: [
              { x: 24, y: -90, inX: 16, inY: -70, outX: 40, outY: -94 },
              { x: 66, y: -88, inX: 54, inY: -92, outX: 66, outY: -50 },
              { x: 62, y: -12, inX: 66, inY: -30, outX: 46, outY: -4 },
              { x: 26, y: -26, inX: 38, inY: -10, outX: 18, outY: -52 }
            ]
          },
          {
            _label: '左腰橘斑', space: 'body', fill: '#E08C45',
            anchors: [
              { x: -64, y: -64, inX: -64, inY: -40, outX: -46, outY: -72 },
              { x: -24, y: -52, inX: -34, inY: -66, outX: -20, outY: -30 },
              { x: -30, y: 2, inX: -22, inY: -12, outX: -46, outY: 6 },
              { x: -64, y: -6, inX: -56, inY: 4, outX: -64, outY: -34 }
            ]
          },
          {
            _label: '右耳黑色', space: 'rightEar', fill: '#2E2724',
            anchors: [
              { x: -60, y: 10 }, { x: 60, y: 10 }, { x: 60, y: -120 }, { x: -60, y: -120 }
            ]
          }
        ],
        marks: []
      }
    },
    {
      id: 'britishBlue',
      tag: '圆脸英短蓝',
      spec: {
        formatVersion: 1, name: '英短蓝猫', author: 'KuroCat 皮肤工坊',
        palette: {
          body: '#747D88', bodyShade: '#5E6670', belly: '#87909B', legs: '#747D88', paws: '#6C7580',
          pawPads: '#5C5458', earOuter: '#666E78', earInner: '#A3989D',
          nose: '#4D5259', mouth: '#4B5056', iris: '#E5A63B', tail: '#6A737D',
          whiskers: '#DCE2E8A8', crease: '#4C545E85'
        },
        shape: { build: 'standard', bodyWidth: 1.20, cheekFullness: 1.22, earSize: 0.90, earPointiness: 0.72, tailThickness: 1.22, pawStagger: 0 },
        headSurface: 'dome',
        regions: [],
        marks: []
      }
    },
    {
      id: 'ragdoll',
      tag: '海双布偶猫',
      spec: {
        formatVersion: 1, name: '海双布偶猫', author: 'KuroCat 皮肤工坊',
        palette: {
          body: '#F7F1E7', bodyShade: '#E9DFD0', belly: '#FCF9F3', legs: '#F7F1E7', paws: '#FCF9F3',
          pawPads: '#E9A7AE', earOuter: '#5D483C', earInner: '#D6B7AE',
          nose: '#E89DA3', mouth: '#A98A7E', iris: '#89C8F0', tail: '#695346', tailTip: '#4E3B30',
          outline: '#BFA99466', whiskers: '#8B796AA6', crease: '#9C8A7A75'
        },
        shape: { build: 'standard', bodyWidth: 1.14, cheekFullness: 1.10, earSize: 1.04, tailThickness: 1.32, pawStagger: 0 },
        headSurface: 'dome',
        regions: [
          {
            _label: '左眼海豹色面罩', space: 'head', fill: '#654F42',
            anchors: [
              { x: -64, y: -52, inX: -64, inY: -24, outX: -32, outY: -54 },
              { x: -5, y: -48, inX: -18, inY: -54, outX: -3, outY: -26 },
              { x: -12, y: 2, inX: -5, inY: -10, outX: -24, outY: 10 },
              { x: -46, y: 18, inX: -34, inY: 16, outX: -58, outY: 16 },
              { x: -64, y: 8, inX: -60, inY: 14, outX: -64, outY: -18 }
            ]
          },
          {
            _label: '右眼海豹色面罩', space: 'head', fill: '#654F42',
            anchors: [
              { x: 5, y: -48, inX: 3, inY: -26, outX: 18, outY: -54 },
              { x: 64, y: -52, inX: 32, inY: -54, outX: 64, outY: -24 },
              { x: 64, y: 8, inX: 64, inY: -18, outX: 60, outY: 14 },
              { x: 46, y: 18, inX: 58, inY: 16, outX: 34, outY: 16 },
              { x: 12, y: 2, inX: 24, inY: 10, outX: 5, outY: -10 }
            ]
          },
          {
            _label: '柔和奶咖马鞍背', space: 'body', fill: '#E3D3BE', alpha: 0.72,
            anchors: [
              { x: -66, y: -64 }, { x: -34, y: -54 }, { x: -30, y: 4 }, { x: -66, y: 4 }
            ]
          },
          {
            _label: '右侧奶咖背毛', space: 'body', fill: '#E3D3BE', alpha: 0.72,
            anchors: [
              { x: 34, y: -54 }, { x: 66, y: -64 }, { x: 66, y: 4 }, { x: 30, y: 4 }
            ]
          }
        ],
        marks: []
      }
    }
  ];

  const BLANK_WHITE_SKIN = {
    formatVersion: 1,
    name: '纯白素胚小猫',
    author: 'KuroCat 皮肤工坊',
    palette: {
      body: '#FAF7F1',
      belly: '#FFFFFF',
      legs: '#FAF7F1',
      paws: '#FFFFFF',
      pawPads: '#F3A6B2',
      earOuter: '#F4EFE6',
      earInner: '#F6C6CE',
      nose: '#EB9CA8',
      mouth: '#7B675E',
      iris: '#E8B84B',
      tail: '#F4EFE6',
      stripes: '#5A4A42',
      whiskers: '#9A8C80',
      crease: '#B7AA9D'
    },
    shape: { build: 'standard' },
    headSurface: 'dome',
    regions: [],
    marks: [],
    decorations: []
  };

  // --- Format version, minimum app version, limits ---
  // Every field below first worked in the desktop release named next to it (from the git history of
  // KuroCat/shared/SkinSpec.hpp). Export writes the highest one a skin uses as `minAppVersion`; apps from 1.5.3 on
  // refuse a skin that needs a newer app with an "upgrade" message instead of dropping fields silently.
  // formatVersion stays 1 while the skin works on 1.5.2 (the last release that only accepts 1) and becomes 2 above
  // that, so 1.5.2 and older also say "请先升级" rather than rejecting a field. Keep NEXT_APP_VERSION equal to the
  // version of the desktop release that ships formatVersion 2 + the 1024-mark limit.
  const FORMAT_VERSION_MAX = 2;
  const BASE_APP_VERSION = '1.4.0';       // first release with .kurocat skins
  const LAST_FORMAT1_APP_VERSION = '1.5.2';
  const NEXT_APP_VERSION = '1.5.3';
  const MARKS_LIMIT_V1 = 256;             // what 1.5.2 and older accept
  const MAX_MARKS = 1024;
  const WEB_ONLY_RENDER_STYLES = { damiao: '大喵', naimi: '奶咪' }; // drawn by the web renderer only

  function compareVersions(a, b) {
    const x = String(a).split('.').map((n) => parseInt(n, 10) || 0);
    const y = String(b).split('.').map((n) => parseInt(n, 10) || 0);
    for (let i = 0; i < Math.max(x.length, y.length); i++) {
      const l = x[i] || 0, r = y[i] || 0;
      if (l !== r) return l < r ? -1 : 1;
    }
    return 0;
  }

  const isVersionString = (v) => typeof v === 'string' && v.length <= 16 && /^[0-9]+(\.[0-9]+)*$/.test(v);

  // -> { version, reasons: [{ version, what }], webOnly: [label] } for a raw or exported skin object.
  function requiredAppVersion(raw) {
    const reasons = [];
    const need = (version, what) => reasons.push({ version, what });
    const shape = (raw && raw.shape) || {};
    const pal = (raw && raw.palette) || {};
    const items = [].concat((raw && raw.regions) || [], (raw && raw.marks) || []);
    const differs = (k) => typeof shape[k] === 'number' && SHAPE_RANGES[k] && Math.abs(shape[k] - SHAPE_RANGES[k][2]) > 1e-6;
    const palSet = (k, fallback) => isValidHex(pal[k]) && !(isValidHex(pal[fallback]) && sameColor(parseHexColor(pal[k]), parseHexColor(pal[fallback])));

    if (raw && raw.renderStyle === 'douni') need('1.4.1', '小豆泥造型');
    if (raw && raw.typingStyle) need('1.4.1', '打字动作 typingStyle');
    if (raw && raw.renderStyle === 'dazed') need('1.4.4', '呆猫造型');
    if (raw && raw.renderStyle === 'beagle') need('1.5.0', '比格造型');
    for (const k of ['earBaseWidth', 'neckWidth']) if (differs(k)) need('1.4.4', `体型 ${k}`);
    for (const k of ['armLength', 'armRootY', 'armSpacing', 'earSpacing', 'tailRootY']) if (differs(k)) need('1.4.5', `体型 ${k}`);
    for (const k of ['hindLegReach', 'hindLegRootX', 'hindLegRootY', 'hindLegWidth']) if (differs(k)) need('1.4.6', `体型 ${k}`);
    if (items.some((it) => it && (it.space === 'leftArm' || it.space === 'rightArm'))) need('1.4.4', '胳膊上的花纹');
    if (items.some((it) => it && it.space === 'hindLeg')) need('1.4.5', '后腿上的花纹');
    for (const [k, fb] of [['headBody', 'body'], ['hindLeg', 'legs'], ['hindPaw', 'paws'], ['irisLeft', 'iris'], ['irisRight', 'iris'],
      ['pupil', null], ['pupilLeft', 'pupil'], ['pupilRight', 'pupil']]) {
      if (palSet(k, fb)) need('1.4.5', `颜色 ${k}`);
    }
    const pg = raw && raw.partGradients;
    if (pg && typeof pg === 'object' && Object.keys(pg).length) {
      need('1.4.5', '部位渐变 partGradients');
      if (Object.values(pg).some((g) => g && typeof g.stop === 'number' && Math.abs(g.stop - 0.5) > 1e-6)) need('1.4.6', '渐变位置 stop');
    }
    if (((raw && raw.marks) || []).some((m) => m && m.brush && m.brush !== 'round')) need('1.4.8', '柔边 / 喷枪 / 涂抹笔刷');
    if (((raw && raw.marks) || []).length > MARKS_LIMIT_V1) need(NEXT_APP_VERSION, `超过 ${MARKS_LIMIT_V1} 条花纹`);

    let version = BASE_APP_VERSION;
    for (const r of reasons) if (compareVersions(r.version, version) > 0) version = r.version;
    const webOnly = raw && WEB_ONLY_RENDER_STYLES[raw.renderStyle] ? [WEB_ONLY_RENDER_STYLES[raw.renderStyle]] : [];
    return { version, reasons, webOnly };
  }

  // --- Legacy `skull` space → `head` ---
  // `skull` only follows head tilt and head movement, never the 3D turn, so a cap or forehead pattern drawn
  // there stays put while the eyes slide sideways (up to ~13 units at full yaw). At the neutral pose
  // (yaw = pitch = 0) both spaces map a point to the same place, so relabelling is invisible at rest.
  // Region anchors outside the dome rim (x/46, y/40 beyond the ellipse) sit outside the head silhouette at
  // rest; on a turn they shrink toward the centre by cos(yaw), so they are pushed out by 1/cos(max yaw) to keep
  // the far edge from pulling inside the silhouette. Marks are clipped to the whole cat (head + body), where a
  // point outside the rim can still be visible, so their points are only relabelled.
  const SKULL_RIM_PUSH = 1 / Math.cos(0.5); // renderer: yaw = headYaw * 0.50, headYaw ∈ [-1, 1]

  function isSkullItem(item) {
    return !!item && item.space === 'skull';
  }

  function countSkullItems(raw) {
    if (!raw) return 0;
    return (raw.regions || []).filter(isSkullItem).length + (raw.marks || []).filter(isSkullItem).length;
  }

  function migrateSkullRegion(r) {
    if (!isSkullItem(r)) return false;
    r.space = 'head';
    for (const a of (r.anchors || [])) {
      if (!a || Math.hypot(a.x / 46, a.y / 40) < 1) continue;
      for (const [kx, ky] of [['x', 'y'], ['inX', 'inY'], ['outX', 'outY']]) {
        if (typeof a[kx] === 'number') a[kx] = round2(a[kx] * SKULL_RIM_PUSH);
        if (typeof a[ky] === 'number') a[ky] = round2(a[ky] * SKULL_RIM_PUSH);
      }
    }
    return true;
  }

  function migrateSkullMark(m) {
    if (!isSkullItem(m)) return false;
    m.space = 'head';
    return true;
  }

  // Mutates `raw`; returns how many regions + marks were converted.
  function migrateSkullToHead(raw) {
    if (!raw) return 0;
    let n = 0;
    for (const r of (raw.regions || [])) if (migrateSkullRegion(r)) n++;
    for (const m of (raw.marks || [])) if (migrateSkullMark(m)) n++;
    return n;
  }

  global.KuroSpec = {
    VALID_SPACES,
    FORMAT_VERSION_MAX,
    MAX_MARKS,
    MARKS_LIMIT_V1,
    NEXT_APP_VERSION,
    compareVersions,
    requiredAppVersion,
    countSkullItems,
    migrateSkullRegion,
    migrateSkullMark,
    migrateSkullToHead,
    VALID_BRUSHES,
    PALETTE_KEYS,
    SHAPE_RANGES,
    SHAPE_LIMITS,
    BLANK_WHITE_SKIN,
    clamp,
    round2,
    isValidHex,
    parseHexColor,
    rgbaToCss,
    rgbToHex,
    luminance,
    sameColor,
    flattenRegion,
    flattenOpen,
    pointInPolygon,
    DECO_ANCHORS,
    stripDataUrl,
    pngInfo,
    canMirror,
    mirrorAnchors,
    mirrorRegion,
    mirrorMark,
    splitSides,
    pickSplitSide,
    jawSide,
    validateRawSkin,
    resolveSkin,
    sanitizeForExport,
    makePresetRegion,
    makeTabbyForeheadMarks,
    makeTailRingMarks,
    makeArmRingMarks,
    OFFICIAL_TEMPLATES,
    EXTRA_PRESETS
  };
})(window);

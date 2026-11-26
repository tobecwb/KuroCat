/* Pixel-faithful HTML5 Canvas port of KuroCat's native 2.5D CatRenderer (Sources/CatRenderer.swift) */
(function (global) {
  'use strict';

  const { clamp, rgbaToCss, sameColor, jawSide } = global.KuroSpec;

  const SEAT_ANCHOR = { x: 184, y: 226 };
  const LEFT_PAW_REST = { x: 164, y: 235 };
  const RIGHT_PAW_REST = { x: 204, y: 235 };
  const HIND_FOOT_REST = { x: 214, y: 234 };
  const MOUSE_BASE = { x: 78, y: 258 };
  const CAT_BLACK = 'rgb(22, 22, 24)';

  const STANDARD_BUILD = {
    headLift: 0,
    cheekHalfW: 42.5, crownCtrlW: 26.0, crownBaseY: -30.0, cheekY: 5.0, cheekCtrlTopY: -14.0, neckPinchX: 2.5, neckCtrlY: 24.0,
    hipHalfW: 49.0, waistCtrlW: 51.5, waistCtrlY: -48.0, botOuterW: 47.0, botInnerW: 36.0, botHalfW: 24.0,
    earFold: 0.06, earLen: 32.0, earInX: 9.0, earInY: -16.0, earOutX: 35.0, earOutY: -5.0, earTipX: 24.5, earTipY: -20.0, earTipSpread: 3.8,
    tailWidth: 11.0,
    hindLeftX: -35.0, hindW: 17.0, hindRightX: 18.0, hindLeftY: 3.0, hindLeftH: 11.0, hindStaggerY: 4.5, hindStaggerH: 12.5,
    idleTopDY: 30, idleTopX: 18.0, idleBotX: 16.0, shoulderX: 20.0, limbStartW: 19.0, limbEndW: 13.5,
    sleekEyes: false
  };

  const SLEEK_BUILD = {
    headLift: -3.0,
    cheekHalfW: 42.5, crownCtrlW: 25.0, crownBaseY: -29.5, cheekY: 6.5, cheekCtrlTopY: -13.0, neckPinchX: 6.5, neckCtrlY: 27.5,
    hipHalfW: 44.5, waistCtrlW: 41.0, waistCtrlY: -50.0, botOuterW: 42.0, botInnerW: 32.0, botHalfW: 22.0,
    earFold: 0.14, earLen: 29.5, earInX: 10.0, earInY: -16.5, earOutX: 35.5, earOutY: -4.5, earTipX: 25.0, earTipY: -18.5, earTipSpread: 4.2,
    tailWidth: 9.5,
    hindLeftX: -31.0, hindW: 15.5, hindRightX: 15.5, hindLeftY: 3.5, hindLeftH: 12.0, hindStaggerY: 4.5, hindStaggerH: 12.5,
    idleTopDY: 31, idleTopX: 16.5, idleBotX: 15.0, shoulderX: 18.5, limbStartW: 17.0, limbEndW: 12.5,
    sleekEyes: true
  };

  const EYE_THEMES = {
    neonChartreuse: { primary: [201, 242, 64], glow: [196, 242, 61] },
    siameseBlue: { primary: [176, 218, 242], glow: [173, 217, 242] },
    warmAmber: { primary: [250, 194, 56], glow: [250, 189, 51] },
    referenceHazel: { primary: [218, 178, 87], glow: [218, 178, 87] },
    softAmber: { primary: [217, 173, 85], glow: [217, 173, 85] },
    iceCyan: { primary: [115, 235, 245], glow: [107, 230, 250] }
  };

  const KB_THEMES = {
    classicMilk: {
      mouseBody: '#F4F2EC', mouseBorder: '#242429', wheel: '#F5B029',
      base: '#1A1A1F', deck: '#242429', key: '#F7F6F0', keyBorder: 'rgba(199,196,189,0.75)', accent: '#F7B329', darkText: true
    },
    midnightHacker: {
      mouseBody: '#33363D', mouseBorder: '#14141A', wheel: '#C7F240',
      base: '#0F0F14', deck: '#1A1C21', key: '#363840', keyBorder: 'rgba(28,28,33,0.95)', accent: '#C4F040', darkText: false
    },
    retroCream: {
      mouseBody: '#E3DED1', mouseBorder: '#33302E', wheel: '#E06B3D',
      base: '#403D38', deck: '#615C54', key: '#E6E0D1', keyBorder: 'rgba(173,166,153,0.85)', accent: '#E06B3D', darkText: true
    }
  };

  // Simplified 65% mechanical keyboard rows (15 units wide, 5 rows)
  const KEYBOARD_ROWS = [
    [
      { l: 'ESC', w: 1, accent: true }, { l: '1', w: 1 }, { l: '2', w: 1 }, { l: '3', w: 1 }, { l: '4', w: 1 },
      { l: '5', w: 1 }, { l: '6', w: 1 }, { l: '7', w: 1 }, { l: '8', w: 1 }, { l: '9', w: 1 },
      { l: '0', w: 1 }, { l: '-', w: 1 }, { l: '=', w: 1 }, { l: '⌫', w: 2 }
    ],
    [
      { l: '⇥', w: 1.5 }, { l: 'Q', w: 1 }, { l: 'W', w: 1 }, { l: 'E', w: 1 }, { l: 'R', w: 1 },
      { l: 'T', w: 1 }, { l: 'Y', w: 1 }, { l: 'U', w: 1 }, { l: 'I', w: 1 }, { l: 'O', w: 1 },
      { l: 'P', w: 1 }, { l: '[', w: 1 }, { l: ']', w: 1 }, { l: '\\', w: 1.5 }
    ],
    [
      { l: '⇪', w: 1.75 }, { l: 'A', w: 1 }, { l: 'S', w: 1 }, { l: 'D', w: 1 }, { l: 'F', w: 1 },
      { l: 'G', w: 1 }, { l: 'H', w: 1 }, { l: 'J', w: 1 }, { l: 'K', w: 1 }, { l: 'L', w: 1 },
      { l: ';', w: 1 }, { l: '\'', w: 1 }, { l: '↵', w: 2.25, accent: true }
    ],
    [
      { l: '⇧', w: 2.25 }, { l: 'Z', w: 1 }, { l: 'X', w: 1 }, { l: 'C', w: 1 }, { l: 'V', w: 1 },
      { l: 'B', w: 1 }, { l: 'N', w: 1 }, { l: 'M', w: 1 }, { l: ',', w: 1 }, { l: '.', w: 1 },
      { l: '/', w: 1 }, { l: '⇧', w: 1.75 }, { l: '↑', w: 1 }
    ],
    [
      { l: '⌃', w: 1.25 }, { l: '⌥', w: 1.25 }, { l: '⌘', w: 1.25 }, { l: 'space', w: 6.25, isSpace: true },
      { l: '⌘', w: 1 }, { l: '⌥', w: 1 }, { l: '←', w: 1 }, { l: '↓', w: 1 }, { l: '→', w: 1 }
    ]
  ];

  const KEYCAPS = [];
  KEYBOARD_ROWS.forEach((row, rIdx) => {
    const totalW = row.reduce((acc, k) => acc + k.w, 0);
    let x = 0;
    row.forEach((k, cIdx) => {
      KEYCAPS.push({
        id: `${rIdx}_${cIdx}`,
        row: rIdx,
        xUnits: x / totalW,
        widthUnits: k.w / totalW,
        legend: k.l,
        isSpace: !!k.isSpace,
        isAccent: !!k.accent
      });
      x += k.w;
    });
  });

  function screenQuadPoint(su, sv) {
    const tl = { x: 109, y: 238 }, tr = { x: 281, y: 247 };
    const bl = { x: 103, y: 282 }, br = { x: 286, y: 292 };
    const topX = tl.x + (tr.x - tl.x) * su;
    const topY = tl.y + (tr.y - tl.y) * su;
    const botX = bl.x + (br.x - bl.x) * su;
    const botY = bl.y + (br.y - bl.y) * su;
    return { x: topX + (botX - topX) * sv, y: topY + (botY - topY) * sv };
  }

  function around(v, c, k) {
    return k === 1 ? v : c + (v - c) * k;
  }

  function getBuild(skin) {
    return skin.shape.build === 'sleek' ? SLEEK_BUILD : STANDARD_BUILD;
  }

  function projectHead3D(x, y, z, yawRad, pitchRad) {
    const cy = Math.cos(yawRad), sy = Math.sin(yawRad);
    const x1 = x * cy + z * sy;
    const z1 = -x * sy + z * cy;
    const cp = Math.cos(pitchRad), sp = Math.sin(pitchRad);
    const y2 = y * cp + z1 * sp;
    const z2 = -y * sp + z1 * cp;
    return { x: x1, y: y2, z: z2 };
  }

  function headPoint(p, skull, skin, head, state) {
    const depth = skin.headSurface === 'dome'
      ? 29.6 * Math.sqrt(Math.max(0, 1 - (p.x / 46) ** 2 - (p.y / 40) ** 2))
      : 27.5;
    let proj = skull
      ? { x: p.x, y: p.y }
      : projectHead3D(p.x, p.y, depth, state.headYaw * 0.50, state.headPitch * 0.36);
    const hs = skin.shape.headScale;
    const px = proj.x * hs, py = proj.y * hs;
    const c = Math.cos(state.headTilt), s = Math.sin(state.headTilt);
    return { x: head.x + px * c - py * s, y: head.y + px * s + py * c };
  }

  function bodyPoint(p, skin, seatX, seatY, state) {
    const bw = skin.shape.bodyWidth;
    const sy = state.bodySquash, sx = 1 + (1 - sy) * 0.55;
    const c = Math.cos(state.bodyTilt), s = Math.sin(state.bodyTilt);
    return {
      x: seatX + p.x * bw * sx * c - p.y * sy * s,
      y: seatY + p.x * bw * sx * s + p.y * sy * c
    };
  }

  const WHITE_OUTLINE_W = 1.8;
  const WHITE_OUTLINE_CSS = 'rgba(255,255,255,0.96)';
  let stateWhiteOutline = true;

  function tailCurve(seatX, seatY, state, skin) {
    const p = state.tailPhase || 0;
    const hunt = clamp(state.huntIntensity || 0, 0, 1);
    const agility = clamp(state.tailAgility || 0, 0, 1);
    const lie = clamp(state.lieIntensity || 0, 0, 1);
    const lieS = lie * lie * (3.0 - 2.0 * lie);
    const wag1 = Math.sin(p) * (8.0 + hunt * 5.5 + agility * 4.0);
    const wag2 = Math.cos(p * 1.3) * (6.0 + hunt * 4.5 + agility * 3.5);
    const huntDrop = hunt * 14.0;
    const bw = skin.shape.bodyWidth, tl = skin.shape.tailLength;
    const shift = bw === 1 ? 0 : 34 * bw - 34;

    const baseRoot = { x: seatX + 34 + shift, y: seatY - 6 };
    const reach = clamp(state.tailTyping || 0, 0, 1);
    const target = state.tailTip || { x: 275, y: 196 };
    const flickScale = (1.0 - reach) * (1.0 - lieS);
    const fx = (state.tailFlickX || 0) * flickScale;
    const fy = (state.tailFlickY || 0) * flickScale;
    const curl = (state.tailCurl || 0) * flickScale;

    let baseC1 = {
      x: seatX + 64 + shift + 12 * reach + fx * 0.25,
      y: seatY + 2 + wag1 * 0.35 + huntDrop * 0.4 - 18 * reach + fy * 0.22 + curl * 0.35
    };
    let idleC2 = {
      x: seatX + 66 + wag2 * 0.4 + shift + fx * 0.65 - curl * 0.45,
      y: seatY - 26 + wag1 * 0.7 + huntDrop + fy * 0.60 - curl * 0.55
    };
    let idleTip = {
      x: seatX + 91 + wag2 * 0.5 + shift + fx,
      y: seatY - 30 + wag1 * 0.85 + huntDrop + fy
    };
    if (tl !== 1) {
      baseC1 = { x: around(baseC1.x, baseRoot.x, tl), y: around(baseC1.y, baseRoot.y, tl) };
      idleC2 = { x: around(idleC2.x, baseRoot.x, tl), y: around(idleC2.y, baseRoot.y, tl) };
      idleTip = { x: around(idleTip.x, baseRoot.x, tl), y: around(idleTip.y, baseRoot.y, tl) };
    }
    const baseC2 = {
      x: idleC2.x + (target.x + 24 - idleC2.x) * reach,
      y: idleC2.y + (target.y - 30 - idleC2.y) * reach
    };
    const baseTip = {
      x: idleTip.x + (target.x - idleTip.x) * reach,
      y: idleTip.y + (target.y - idleTip.y) * reach
    };

    const sleepWag = Math.sin(p * 0.6) * 2.0;
    const wrapRoot = { x: seatX + 40 + shift, y: seatY - 2 };
    const wrapC1 = { x: seatX + 54 + shift, y: seatY + 13 };
    const wrapC2 = { x: seatX + 14, y: seatY + 17 };
    const wrapTip = { x: seatX - 28, y: seatY + 8 + sleepWag };

    const root = { x: baseRoot.x + (wrapRoot.x - baseRoot.x) * lieS, y: baseRoot.y + (wrapRoot.y - baseRoot.y) * lieS };
    const c1 = { x: baseC1.x + (wrapC1.x - baseC1.x) * lieS, y: baseC1.y + (wrapC1.y - baseC1.y) * lieS };
    const c2 = { x: baseC2.x + (wrapC2.x - baseC2.x) * lieS, y: baseC2.y + (wrapC2.y - baseC2.y) * lieS };
    const tip = { x: baseTip.x + (wrapTip.x - baseTip.x) * lieS, y: baseTip.y + (wrapTip.y - baseTip.y) * lieS };
    return {
      root, c1, c2, tip, thickness: skin.shape.tailThickness,
      at(pt) {
        const t = clamp(pt.x / 100, 0, 1);
        const u = 1 - t;
        const qx = u * u * u * root.x + 3 * u * u * t * c1.x + 3 * u * t * t * c2.x + t * t * t * tip.x;
        const qy = u * u * u * root.y + 3 * u * u * t * c1.y + 3 * u * t * t * c2.y + t * t * t * tip.y;
        const dx = 3 * u * u * (c1.x - root.x) + 6 * u * t * (c2.x - c1.x) + 3 * t * t * (tip.x - c2.x);
        const dy = 3 * u * u * (c1.y - root.y) + 6 * u * t * (c2.y - c1.y) + 3 * t * t * (tip.y - c2.y);
        const len = Math.max(0.001, Math.hypot(dx, dy));
        const n = pt.y * skin.shape.tailThickness;
        return { x: qx - (dy / len) * n, y: qy + (dx / len) * n };
      }
    };
  }

  function tabletQuadPoint(u, v) {
    const tl = { x: 18, y: 235 }, tr = { x: 94, y: 240 };
    const bl = { x: 13, y: 284 }, br = { x: 91, y: 289 };
    const topX = tl.x + (tr.x - tl.x) * u, topY = tl.y + (tr.y - tl.y) * u;
    const botX = bl.x + (br.x - bl.x) * u, botY = bl.y + (br.y - bl.y) * u;
    return { x: topX + (botX - topX) * v, y: topY + (botY - topY) * v };
  }

  function roundedQuadPath(ctx, tl, tr, br, bl, radius) {
    ctx.beginPath();
    ctx.moveTo((tl.x + tr.x) * 0.5, (tl.y + tr.y) * 0.5);
    ctx.arcTo(tr.x, tr.y, br.x, br.y, radius);
    ctx.arcTo(br.x, br.y, bl.x, bl.y, radius);
    ctx.arcTo(bl.x, bl.y, tl.x, tl.y, radius);
    ctx.arcTo(tl.x, tl.y, tr.x, tr.y, radius);
    ctx.closePath();
  }

  function roundedRectPath(ctx, x, y, w, h, r) {
    const rr = Math.min(r, w * 0.5, h * 0.5);
    ctx.beginPath();
    ctx.moveTo(x + rr, y);
    ctx.arcTo(x + w, y, x + w, y + h, rr);
    ctx.arcTo(x + w, y + h, x, y + h, rr);
    ctx.arcTo(x, y + h, x, y, rr);
    ctx.arcTo(x, y, x + w, y, rr);
    ctx.closePath();
  }

  function drawPolyline(ctx, pts, closed, map) {
    ctx.beginPath();
    for (let i = 0; i < pts.length; i++) {
      const p = map(pts[i]);
      if (i === 0) ctx.moveTo(p.x, p.y);
      else ctx.lineTo(p.x, p.y);
    }
    if (closed) ctx.closePath();
  }

  function drawRegion(ctx, region, map) {
    ctx.fillStyle = rgbaToCss(region.fill);
    drawPolyline(ctx, region.flat, true, map);
    ctx.fill();
  }

  function drawMark(ctx, mark, cap, map) {
    ctx.save();
    drawPolyline(ctx, mark.points, !mark.stroke, map);
    if (!mark.stroke) {
      ctx.fillStyle = rgbaToCss(mark.color);
      ctx.fill();
    } else {
      ctx.strokeStyle = rgbaToCss(mark.color);
      ctx.lineWidth = mark.width;
      ctx.lineCap = cap || 'round';
      ctx.lineJoin = 'round';
      ctx.stroke();
    }
    ctx.restore();
  }

  function drawMouse(ctx, state, kbThemeName) {
    const th = KB_THEMES[kbThemeName] || KB_THEMES.classicMilk;
    const cx = MOUSE_BASE.x + state.mouseOffsetX;
    const cy = MOUSE_BASE.y + state.mouseOffsetY + state.mousePress * 1.6;

    ctx.save();
    ctx.translate(cx, cy);
    ctx.rotate(0.22);

    ctx.fillStyle = 'rgba(0,0,0,0.22)';
    ctx.beginPath();
    ctx.ellipse(0, 6.0, 13.5, 15.5, 0, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = th.mouseBody;
    ctx.beginPath();
    ctx.ellipse(0, 0, 13.5, 16.5, 0, 0, Math.PI * 2);
    ctx.fill();

    ctx.strokeStyle = 'rgba(36,36,41,0.32)';
    ctx.lineWidth = 1.05;
    ctx.beginPath();
    ctx.moveTo(0, 16);
    ctx.lineTo(0, 1.5);
    ctx.stroke();

    ctx.fillStyle = th.wheel;
    roundedRectPath(ctx, -1.9, 2.5, 3.8, 8.5, 1.9);
    ctx.fill();

    ctx.strokeStyle = th.mouseBorder;
    ctx.lineWidth = 1.7;
    ctx.beginPath();
    ctx.ellipse(0, 0, 13.5, 16.5, 0, 0, Math.PI * 2);
    ctx.stroke();

    ctx.restore();
  }

  function hsvRgbCss(h, s, v, a) {
    const hh = ((h % 1) + 1) % 1;
    const i = Math.floor(hh * 6);
    const f = hh * 6 - i;
    const p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    let r = v, g = t, b = p;
    switch (i % 6) {
      case 1: r = q; g = v; b = p; break;
      case 2: r = p; g = v; b = t; break;
      case 3: r = p; g = q; b = v; break;
      case 4: r = t; g = p; b = v; break;
      case 5: r = v; g = p; b = q; break;
    }
    return `rgba(${Math.round(r * 255)},${Math.round(g * 255)},${Math.round(b * 255)},${a})`;
  }

  function drawTablet(ctx, state, kbThemeName, eyeThemeName) {
    const eye = EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse;
    const eyeCss = (a) => `rgba(${eye.primary[0]},${eye.primary[1]},${eye.primary[2]},${a})`;
    const isDark = kbThemeName === 'midnightHacker';
    const isRetro = kbThemeName === 'retroCream';
    const baseCol = isDark ? '#0D0D12' : (isRetro ? '#383530' : '#1A1A1F');
    const bodyCol = isDark ? '#1C1F26' : (isRetro ? '#544F47' : '#2B2E36');
    const padCol  = isDark ? '#292B36' : (isRetro ? '#6B6359' : '#3B3D47');
    const keyCol  = isDark ? '#3D404D' : (isRetro ? '#8A8073' : '#4D525E');
    const accentCol = isDark ? '#C7F240' : (isRetro ? '#E06B3D' : '#F5B029');
    const tl = tabletQuadPoint(0, 0), tr = tabletQuadPoint(1, 0), bl = tabletQuadPoint(0, 1), br = tabletQuadPoint(1, 1);
    const depth = 3.6;

    ctx.save();
    roundedQuadPath(ctx, { x: tl.x, y: tl.y + depth + 2 }, { x: tr.x, y: tr.y + depth + 2 }, { x: br.x, y: br.y + depth + 2.6 }, { x: bl.x, y: bl.y + depth + 2.6 }, 5.5);
    ctx.fillStyle = 'rgba(0,0,0,0.22)';
    ctx.fill();

    roundedQuadPath(ctx, { x: tl.x, y: tl.y + depth }, { x: tr.x, y: tr.y + depth }, { x: br.x, y: br.y + depth }, { x: bl.x, y: bl.y + depth }, 5.0);
    ctx.fillStyle = baseCol;
    ctx.fill();

    roundedQuadPath(ctx, tl, tr, br, bl, 4.5);
    ctx.fillStyle = bodyCol;
    ctx.fill();
    ctx.strokeStyle = 'rgba(255,255,255,0.14)';
    ctx.lineWidth = 0.9;
    ctx.stroke();

    const pTL = tabletQuadPoint(0.21, 0.11), pTR = tabletQuadPoint(0.94, 0.11), pBR = tabletQuadPoint(0.94, 0.89), pBL = tabletQuadPoint(0.21, 0.89);
    roundedQuadPath(ctx, pTL, pTR, pBR, pBL, 2.6);
    ctx.fillStyle = padCol;
    ctx.fill();

    ctx.strokeStyle = 'rgba(255,255,255,0.34)';
    ctx.lineWidth = 0.95;
    ctx.lineCap = 'round';
    ctx.beginPath();
    for (const [cu, cv, du, dv] of [[0.24,0.15,0.07,0.11],[0.91,0.15,-0.07,0.11],[0.24,0.85,0.07,-0.11],[0.91,0.85,-0.07,-0.11]]) {
      const c = tabletQuadPoint(cu, cv), h = tabletQuadPoint(cu + du, cv), v = tabletQuadPoint(cu, cv + dv);
      ctx.moveTo(h.x, h.y); ctx.lineTo(c.x, c.y); ctx.lineTo(v.x, v.y);
    }
    ctx.stroke();

    const ledPt = tabletQuadPoint(0.105, 0.14);
    const penPress = clamp(state.tabletPenPress || 0, 0, 1);
    ctx.fillStyle = penPress > 0.15 ? eyeCss(1) : accentCol;
    ctx.beginPath(); ctx.arc(ledPt.x, ledPt.y, 1.35, 0, Math.PI * 2); ctx.fill();

    ctx.fillStyle = keyCol;
    for (const kv of [0.28, 0.44, 0.60, 0.76]) {
      roundedQuadPath(ctx, tabletQuadPoint(0.06, kv - 0.05), tabletQuadPoint(0.15, kv - 0.05), tabletQuadPoint(0.15, kv + 0.05), tabletQuadPoint(0.06, kv + 0.05), 1.2);
      ctx.fill();
    }

    const trail = state.tabletTrail || [];
    if (trail.length >= 2) {
      ctx.save();
      roundedQuadPath(ctx, pTL, pTR, pBR, pBL, 2.6);
      ctx.clip();
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      for (let i = 1; i < trail.length; i++) {
        const a = trail[i - 1], b = trail[i];
        if (Math.hypot(b.x - a.x, b.y - a.y) > 22) continue;
        const life = Math.max(0, 1 - (b.age || 0) / 0.72);
        if (life <= 0.01) continue;
        ctx.strokeStyle = eyeCss((life * 0.88).toFixed(3));
        ctx.lineWidth = 1.2 + life * 1.4;
        ctx.beginPath(); ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y); ctx.stroke();
      }
      ctx.restore();
    }
    ctx.restore();
  }

  function drawStylusBody(ctx, length, kbThemeName, eyeThemeName, pressed) {
    const eye = EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse;
    const isDark = kbThemeName === 'midnightHacker';
    const isRetro = kbThemeName === 'retroCream';
    const barrelCol = isDark ? '#21242B' : (isRetro ? '#47423D' : '#2E303B');
    const gripCol   = isDark ? '#383D4A' : (isRetro ? '#6B6359' : '#474D59');
    const ringCol   = isDark ? '#C7F240' : (isRetro ? '#E06B3D' : '#F5B029');
    const tracePen = () => {
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(7.5, -2.4);
      ctx.lineTo(length - 3, -2.6);
      ctx.quadraticCurveTo(length, -2.4, length, 0);
      ctx.quadraticCurveTo(length, 2.4, length - 3, 2.6);
      ctx.lineTo(7.5, 2.4);
      ctx.closePath();
    };
    ctx.save();
    tracePen();
    ctx.strokeStyle = 'rgba(245,245,245,0.80)';
    ctx.lineWidth = 1.6;
    ctx.lineJoin = 'round';
    ctx.stroke();
    ctx.fillStyle = barrelCol;
    ctx.fill();
    ctx.save();
    tracePen();
    ctx.clip();
    ctx.fillStyle = gripCol;
    ctx.fillRect(7.5, -3, 11.5, 6);
    ctx.fillStyle = ringCol;
    ctx.fillRect(19, -3, 2.4, 6);
    ctx.fillStyle = pressed > 0.15 ? `rgb(${eye.primary[0]},${eye.primary[1]},${eye.primary[2]})` : '#F0F0F0';
    ctx.fillRect(-0.5, -1.5, 3.4, 3);
    ctx.fillStyle = 'rgba(255,255,255,0.24)';
    ctx.fillRect(8, -1.9, length - 12, 1.1);
    ctx.restore();
    ctx.fillStyle = 'rgba(200,200,200,0.9)';
    roundedRectPath(ctx, 11, -3.1, 6, 1.4, 0.7);
    ctx.fill();
    ctx.restore();
  }

  function drawParkedStylus(ctx, kbThemeName, eyeThemeName) {
    const nib = tabletQuadPoint(0.88, -0.02), tail = tabletQuadPoint(0.18, -0.02);
    const ang = Math.atan2(tail.y - nib.y, tail.x - nib.x);
    const len = Math.hypot(tail.x - nib.x, tail.y - nib.y);
    ctx.save();
    ctx.translate(nib.x, nib.y);
    ctx.rotate(ang);
    drawStylusBody(ctx, len, kbThemeName, eyeThemeName, 0);
    ctx.restore();
  }

  function drawHeldStylus(ctx, state, kbThemeName, eyeThemeName) {
    const lPos = state.leftPaw || LEFT_PAW_REST;
    const press = clamp(state.tabletPenPress || 0, 0, 1);
    ctx.save();
    ctx.translate(lPos.x - 5.5, lPos.y + 7.0 + press * 1.4);
    ctx.rotate(-2.18 + (state.tabletPenTilt || 0));
    drawStylusBody(ctx, 35.0, kbThemeName, eyeThemeName, press);
    ctx.restore();
  }

  function drawKeyboard(ctx, state, kbThemeName, eyeThemeName) {
    const th = KB_THEMES[kbThemeName] || KB_THEMES.classicMilk;
    const eye = EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse;
    const tl = screenQuadPoint(-0.012, -0.04);
    const tr = screenQuadPoint(1.012, -0.04);
    const bl = screenQuadPoint(-0.016, 1.04);
    const br = screenQuadPoint(1.016, 1.04);
    const depth = 4.5;

    ctx.save();
    roundedQuadPath(ctx,
      { x: tl.x, y: tl.y + depth + 2 }, { x: tr.x, y: tr.y + depth + 2 },
      { x: br.x, y: br.y + depth + 3 }, { x: bl.x, y: bl.y + depth + 3 }, 6);
    ctx.fillStyle = 'rgba(0,0,0,0.20)';
    ctx.fill();

    roundedQuadPath(ctx,
      { x: tl.x, y: tl.y + depth }, { x: tr.x, y: tr.y + depth },
      { x: br.x, y: br.y + depth }, { x: bl.x, y: bl.y + depth }, 5.5);
    ctx.fillStyle = th.base;
    ctx.fill();

    roundedQuadPath(ctx, tl, tr, br, bl, 5.0);
    ctx.fillStyle = th.deck;
    ctx.fill();

    const padU = 0.0028, padV = 0.014;
    for (const cap of KEYCAPS) {
      const glow = state.keyGlow[cap.id] || 0;
      const su0 = (1.0 - (cap.xUnits + cap.widthUnits)) + padU;
      const su1 = (1.0 - cap.xUnits) - padU;
      const sv0 = (1.0 - (cap.row + 1) / 5.0) + padV;
      const sv1 = (1.0 - cap.row / 5.0) - padV;
      const dy = glow * 1.9;
      const kTL = screenQuadPoint(su0, sv0); kTL.y += dy;
      const kTR = screenQuadPoint(su1, sv0); kTR.y += dy;
      const kBR = screenQuadPoint(su1, sv1); kBR.y += dy;
      const kBL = screenQuadPoint(su0, sv1); kBL.y += dy;

      roundedQuadPath(ctx, kTL, kTR, kBR, kBL, 1.5);
      ctx.fillStyle = cap.isAccent ? th.accent : th.key;
      ctx.fill();
      if (state.marqueeEnabled !== false) {
        const phase = ((state.marqueePhase || 0) % 1 + 1) % 1;
        const ku = (su0 + su1) * 0.5;
        let dist = Math.abs(ku - phase);
        if (dist > 0.5) dist = 1.0 - dist;
        if (dist < 0.24) {
          const w = Math.cos((dist / 0.24) * Math.PI * 0.5);
          ctx.fillStyle = hsvRgbCss(phase + ku * 0.18, 0.72, 1.0, (w * w * 0.24).toFixed(3));
          ctx.fill();
        }
      }
      if (glow > 0.03) {
        ctx.fillStyle = `rgba(${eye.glow[0]},${eye.glow[1]},${eye.glow[2]},${(glow * 0.55).toFixed(3)})`;
        ctx.fill();
      }
      ctx.strokeStyle = th.keyBorder;
      ctx.lineWidth = 0.65;
      ctx.stroke();

      let cx = (kTL.x + kTR.x + kBR.x + kBL.x) * 0.25;
      let cy = (kTL.y + kTR.y + kBR.y + kBL.y) * 0.25;
      if (cap.isSpace) {
        const sp = screenQuadPoint(su0 + (su1 - su0) * 0.82, (sv0 + sv1) * 0.5);
        cx = sp.x; cy = sp.y + dy;
      }
      ctx.save();
      ctx.translate(cx, cy);
      ctx.rotate(Math.PI + 0.052);
      ctx.font = `600 ${cap.legend.length <= 1 ? 5.1 : 4.1}px -apple-system, sans-serif`;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillStyle = (cap.isAccent || th.darkText) ? 'rgba(36,36,41,0.78)' : 'rgba(224,230,235,0.82)';
      ctx.fillText(cap.legend, 0, 0);
      ctx.restore();
    }
    ctx.restore();
  }

  function isDazedSkin(skin) {
    return Boolean(skin && skin.renderStyle === 'dazed');
  }

  function drawDazedPaw(ctx, x, y, w, h) {
    ctx.save();
    const grad = ctx.createLinearGradient(x, y, x, y + h);
    grad.addColorStop(0, '#FFF1CD');
    grad.addColorStop(1, '#CCAC7F');
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.ellipse(x + w * 0.5, y + h * 0.5, w * 0.5, h * 0.5, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = 'rgba(126, 99, 74, 0.67)';
    ctx.lineWidth = 0.9;
    ctx.stroke();
    ctx.restore();
  }

  function drawDazedSword(ctx, seatX, seatY, state) {
    ctx.save();
    ctx.translate(seatX, seatY + (1.0 - (state.bodySquash || 1.0)) * 36.0);
    ctx.rotate(state.bodyTilt || 0);

    // Steel blade
    ctx.beginPath();
    ctx.moveTo(38.0, -96.0);
    ctx.bezierCurveTo(7.0, -61.0, -31.0, -33.0, -65.0, -38.0);
    ctx.bezierCurveTo(-62.0, -24.0, -51.0, -21.0, -37.0, -25.0);
    ctx.bezierCurveTo(-5.0, -33.0, 27.0, -60.0, 50.0, -84.0);
    ctx.closePath();
    const steel = ctx.createLinearGradient(-46.0, -51.0, -29.0, -22.0);
    steel.addColorStop(0, '#8395B0');
    steel.addColorStop(1, '#DDE7F2');
    ctx.fillStyle = steel;
    ctx.fill();
    ctx.strokeStyle = '#232638';
    ctx.lineWidth = 3.6;
    ctx.lineJoin = 'round';
    ctx.stroke();

    // Blade bevel highlight
    ctx.beginPath();
    ctx.moveTo(-59.0, -33.5);
    ctx.bezierCurveTo(-34.0, -24.0, 12.0, -53.0, 43.0, -87.0);
    ctx.strokeStyle = '#F3F7FB';
    ctx.lineWidth = 2.1;
    ctx.lineCap = 'round';
    ctx.stroke();

    // Hilt & brass crossguard
    ctx.translate(44.0, -90.0);
    ctx.rotate(44.0 * Math.PI / 180.0);

    roundedRectPath(ctx, -7.5, -32.0, 15.0, 32.0, 3.6);
    ctx.fillStyle = '#58617D';
    ctx.fill();
    ctx.save();
    ctx.clip();
    ctx.fillStyle = '#B5BED0';
    for (const y of [-27.0, -19.0, -11.0]) {
      ctx.beginPath();
      ctx.moveTo(-7.5, y - 2.0);
      ctx.lineTo(7.5, y + 1.0);
      ctx.lineTo(7.5, y + 4.3);
      ctx.lineTo(-7.5, y + 1.3);
      ctx.closePath();
      ctx.fill();
    }
    ctx.restore();
    roundedRectPath(ctx, -7.5, -32.0, 15.0, 32.0, 3.6);
    ctx.strokeStyle = '#232638';
    ctx.lineWidth = 3.6;
    ctx.lineJoin = 'round';
    ctx.stroke();

    roundedRectPath(ctx, -13.5, -3.8, 27.0, 7.6, 2.5);
    ctx.fillStyle = '#E2C58B';
    ctx.fill();
    ctx.strokeStyle = '#232638';
    ctx.lineWidth = 3.6;
    ctx.stroke();
    ctx.restore();
  }

  function drawDazedEar(ctx, sign, yawRad, pitchRad, fold, twitch) {
    const foldAngle = sign * fold * 0.34 + twitch;
    const tipDx = Math.sin(foldAngle) * 37.0;
    const tipDy = (1.0 - Math.cos(foldAngle)) * 37.0;
    const proj = (x, y, tipW) => projectHead3D(sign * x + tipDx * tipW, y + tipDy * tipW, -3.0 * tipW, yawRad, pitchRad);

    const innerBase = proj(5.0, -25.0, 0.0);
    const tip = proj(40.0, -63.0, 1.0);
    const outerBase = proj(42.5, -5.0, 0.0);
    const c1 = proj(13.0, -46.0, 0.30);
    const c2 = proj(31.0, -62.0, 0.90);
    const c3 = proj(54.0, -66.0, 0.95);
    const c4 = proj(57.0, -34.0, 0.40);

    ctx.save();
    ctx.beginPath();
    ctx.moveTo(innerBase.x, innerBase.y);
    ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, tip.x, tip.y);
    ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, outerBase.x, outerBase.y);
    ctx.closePath();
    const grad = ctx.createLinearGradient(tip.x, tip.y, outerBase.x, outerBase.y);
    grad.addColorStop(0, '#777FBD');
    grad.addColorStop(1, '#4D5892');
    ctx.fillStyle = grad;
    ctx.fill();
    ctx.restore();
  }

  function drawDazedSilhouette(ctx, seatX, seatY, headCenter, state) {
    ctx.save();
    ctx.translate(seatX, seatY);
    ctx.rotate(state.bodyTilt || 0);
    const squash = state.bodySquash || 1.0;
    ctx.scale(1.0 + (1.0 - squash) * 0.55, squash);

    drawDazedPaw(ctx, -34.0, -2.0, 19.0, 18.0);
    if ((state.hindFootActive || 0) < 0.35) {
      drawDazedPaw(ctx, 15.0, -2.0, 19.0, 18.0);
    }

    // Warm cream belly
    ctx.beginPath();
    ctx.moveTo(-42.0, -19.0);
    ctx.bezierCurveTo(-46.0, -5.0, -41.0, 11.5, 0.0, 11.5);
    ctx.bezierCurveTo(41.0, 11.5, 46.0, -5.0, 42.0, -19.0);
    ctx.closePath();
    const bellyGrad = ctx.createLinearGradient(0.0, -16.0, 0.0, 12.0);
    bellyGrad.addColorStop(0, '#F4DFB8');
    bellyGrad.addColorStop(1, '#C8A77A');
    ctx.fillStyle = bellyGrad;
    ctx.fill();

    // Indigo Palico tunic
    ctx.beginPath();
    ctx.moveTo(-21.0, -65.0);
    ctx.bezierCurveTo(-34.0, -56.0, -44.0, -35.0, -44.0, -17.0);
    ctx.bezierCurveTo(-37.0, -2.0, 37.0, -2.0, 44.0, -17.0);
    ctx.bezierCurveTo(44.0, -35.0, 34.0, -56.0, 21.0, -65.0);
    ctx.closePath();
    const clothGrad = ctx.createLinearGradient(-16.0, -65.0, 12.0, -5.0);
    clothGrad.addColorStop(0, '#555F9B');
    clothGrad.addColorStop(1, '#252E58');
    ctx.fillStyle = clothGrad;
    ctx.fill();

    ctx.beginPath();
    ctx.moveTo(-39.0, -15.0);
    ctx.bezierCurveTo(-28.0, -6.0, 28.0, -6.0, 39.0, -15.0);
    ctx.strokeStyle = 'rgba(124, 134, 188, 0.61)';
    ctx.lineWidth = 1.1;
    ctx.stroke();
    ctx.restore();

    // Hood around head
    ctx.save();
    ctx.translate(headCenter.x, headCenter.y);
    ctx.rotate(state.headTilt || 0);
    ctx.beginPath();
    ctx.moveTo(0.0, -33.0);
    ctx.bezierCurveTo(28.0, -35.0, 48.0, -16.0, 48.0, 9.0);
    ctx.bezierCurveTo(48.0, 32.0, 25.0, 41.0, 0.0, 41.0);
    ctx.bezierCurveTo(-25.0, 41.0, -48.0, 32.0, -48.0, 9.0);
    ctx.bezierCurveTo(-48.0, -16.0, -28.0, -35.0, 0.0, -33.0);
    ctx.closePath();
    const hoodGrad = ctx.createLinearGradient(-16.0, -35.0, 15.0, 42.0);
    hoodGrad.addColorStop(0, '#6871B4');
    hoodGrad.addColorStop(1, '#2F386D');
    ctx.fillStyle = hoodGrad;
    ctx.fill();
    ctx.restore();
  }

  function drawDazedFace(ctx, state, eyeThemeName, yawRad, pitchRad, scruffStartle, scruffWide) {
    const face = projectHead3D(0.0, 4.0, 22.0, yawRad * 0.72, pitchRad * 0.78);
    ctx.save();
    ctx.translate(face.x, face.y);
    ctx.scale(Math.cos(yawRad), Math.cos(pitchRad));

    // Cream face opening inside hood
    ctx.beginPath();
    ctx.moveTo(0.0, -22.0);
    ctx.bezierCurveTo(17.0, -23.0, 32.0, -9.0, 32.0, 8.0);
    ctx.bezierCurveTo(32.0, 22.0, 17.0, 27.5, 0.0, 27.5);
    ctx.bezierCurveTo(-17.0, 27.5, -32.0, 22.0, -32.0, 8.0);
    ctx.bezierCurveTo(-32.0, -9.0, -17.0, -23.0, 0.0, -22.0);
    ctx.closePath();
    const furGrad = ctx.createLinearGradient(-7.0, -23.0, 8.0, 29.0);
    furGrad.addColorStop(0, '#FFF4D4');
    furGrad.addColorStop(1, '#DDBF91');
    ctx.fillStyle = furGrad;
    ctx.fill();
    ctx.strokeStyle = '#34355D';
    ctx.lineWidth = 1.9;
    ctx.stroke();

    // Soft Siamese-brown nose/muzzle colorpoint
    ctx.save();
    ctx.translate(0.0, 11.0);
    ctx.scale(1.0, 34.0 / 42.0);
    const ptGrad = ctx.createRadialGradient(0, 0, 0, 0, 0, 21.0);
    ptGrad.addColorStop(0.0, 'rgba(141, 91, 68, 0.86)');
    ptGrad.addColorStop(0.62, 'rgba(141, 91, 68, 0.61)');
    ptGrad.addColorStop(0.83, 'rgba(141, 91, 68, 0.02)');
    ptGrad.addColorStop(1.0, 'rgba(141, 91, 68, 0.0)');
    ctx.fillStyle = ptGrad;
    ctx.beginPath();
    ctx.arc(0, 0, 21.0, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();

    // Rosy cheeks
    ctx.fillStyle = 'rgba(207, 129, 115, 0.37)';
    ctx.beginPath();
    ctx.ellipse(-18.5, 12.0, 4.5, 2.0, 0, 0, Math.PI * 2);
    ctx.ellipse(18.5, 12.0, 4.5, 2.0, 0, 0, Math.PI * 2);
    ctx.fill();

    // Mouth & nose
    const ink = '#282333';
    ctx.strokeStyle = ink;
    ctx.lineWidth = 1.7;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    ctx.moveTo(-6.5, 12.0);
    ctx.bezierCurveTo(-5.9, 17.0, -1.8, 17.5, 0.0, 13.4);
    ctx.bezierCurveTo(1.8, 17.5, 5.9, 17.0, 6.5, 12.0);
    ctx.stroke();

    ctx.fillStyle = ink;
    ctx.beginPath();
    ctx.ellipse(0.0, 11.5, 1.9, 1.4, 0, 0, Math.PI * 2);
    ctx.fill();

    // Eyes
    const happy = (state.happyIntensity || 0) > 0.35;
    const squint = (state.pettingIntensity || 0) > 0.35;
    if (happy || squint) {
      for (const x of [-11.8, 11.8]) {
        ctx.beginPath();
        ctx.moveTo(x - 7.2, 2.0);
        ctx.quadraticCurveTo(x, happy ? 7.0 : 4.0, x + 7.2, 2.0);
        ctx.stroke();
      }
      ctx.restore();
      return;
    }

    const closure = Math.max(state.eyeBlink || 0, (state.sleepIntensity || 0) * 0.92, (scruffStartle || 0) * 0.44);
    const open = 1.0 - clamp(closure, 0.0, 1.0);
    if (open <= 0.025) {
      ctx.restore();
      return;
    }

    const hunt = clamp(Math.max(state.huntIntensity || 0, (scruffWide || 0) * 0.94), 0.0, 1.0);
    const gx = clamp(state.gazeX || 0, -1.0, 1.0) * (1.35 - hunt * 0.65);
    const gy = clamp(state.gazeY || 0, -1.0, 1.0) * (1.1 - hunt * 0.55);
    const radius = 8.5 + hunt * 1.0;
    const pupilRadius = 6.25 + hunt * 1.35;
    const ep = (EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse).primary;
    const irisCol = {
      r: (255 * 0.80 + ep[0] * 0.20) / 255,
      g: (248 * 0.80 + ep[1] * 0.20) / 255,
      b: (229 * 0.80 + ep[2] * 0.20) / 255,
      a: 1
    };

    for (const x of [-11.8, 11.8]) {
      ctx.save();
      ctx.translate(x, 1.0);
      ctx.scale(1.0, open);
      ctx.fillStyle = ink;
      ctx.beginPath();
      ctx.arc(0, 0, radius, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = rgbaToCss(irisCol);
      ctx.beginPath();
      ctx.arc(0, 0, radius - 1.25, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#191825';
      ctx.beginPath();
      ctx.arc(gx, gy, pupilRadius, 0, Math.PI * 2);
      ctx.fill();
      if (hunt > 0.25) {
        const shine = 1.1 + hunt * 0.6;
        ctx.fillStyle = '#FFF9EA';
        ctx.beginPath();
        ctx.arc(gx - 3.4, gy - 4.2, shine, 0, Math.PI * 2);
        ctx.fill();
      }
      ctx.restore();
    }
    ctx.restore();
  }

  function drawDazedArm(ctx, traceLimb, leftEdge, rightEdge, pawRect) {
    ctx.save();
    traceLimb();
    ctx.fillStyle = '#465189';
    ctx.fill();

    const sleeveLength = 15.0;
    let distance = 0.0;
    for (let i = 1; i < leftEdge.length; i++) {
      const dx = (leftEdge[i].x + rightEdge[i].x - leftEdge[i - 1].x - rightEdge[i - 1].x) * 0.5;
      const dy = (leftEdge[i].y + rightEdge[i].y - leftEdge[i - 1].y - rightEdge[i - 1].y) * 0.5;
      const segment = Math.hypot(dx, dy);
      if (distance + segment < sleeveLength && i < leftEdge.length - 1) {
        distance += segment;
        continue;
      }

      const t = clamp((sleeveLength - distance) / Math.max(segment, 0.001), 0.0, 1.0);
      const interp = (a, b, f) => ({ x: a.x + (b.x - a.x) * f, y: a.y + (b.y - a.y) * f });
      const leftCuff = interp(leftEdge[i - 1], leftEdge[i], t);
      const rightCuff = interp(rightEdge[i - 1], rightEdge[i], t);
      const leftLip = interp(rightCuff, leftCuff, 1.23);
      const rightLip = interp(leftCuff, rightCuff, 1.23);
      const offsetAlong = (pt, off) => {
        const sc = off / Math.max(segment, 0.001);
        return { x: pt.x + dx * sc, y: pt.y + dy * sc };
      };
      const leftFront = offsetAlong(leftLip, 4.0);
      const rightFront = offsetAlong(rightLip, 4.0);
      const leftBack = offsetAlong(leftCuff, -8.0);
      const rightBack = offsetAlong(rightCuff, -8.0);

      // Flared indigo sleeve cuff
      ctx.beginPath();
      ctx.moveTo(leftBack.x, leftBack.y);
      ctx.lineTo(leftLip.x, leftLip.y);
      ctx.bezierCurveTo(leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
      ctx.lineTo(rightBack.x, rightBack.y);
      ctx.closePath();
      ctx.fillStyle = '#465189';
      ctx.fill();

      // Dark sleeve opening lining
      const rIn = offsetAlong(rightLip, -3.5);
      const lIn = offsetAlong(leftLip, -3.5);
      ctx.beginPath();
      ctx.moveTo(leftLip.x, leftLip.y);
      ctx.bezierCurveTo(leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
      ctx.bezierCurveTo(rIn.x, rIn.y, lIn.x, lIn.y, leftLip.x, leftLip.y);
      ctx.closePath();
      ctx.fillStyle = '#252D54';
      ctx.fill();

      // Cream fur forearm emerging from cuff
      ctx.beginPath();
      ctx.moveTo(leftCuff.x, leftCuff.y);
      for (let k = i; k < leftEdge.length; k++) ctx.lineTo(leftEdge[k].x, leftEdge[k].y);
      for (let k = rightEdge.length - 1; k >= i; k--) ctx.lineTo(rightEdge[k].x, rightEdge[k].y);
      ctx.lineTo(rightCuff.x, rightCuff.y);
      ctx.closePath();
      ctx.fillStyle = '#E3C99F';
      ctx.fill();

      drawDazedPaw(ctx, pawRect.x, pawRect.y, pawRect.w, pawRect.h);

      // Cuff seam highlight
      ctx.beginPath();
      ctx.moveTo(leftLip.x, leftLip.y);
      ctx.bezierCurveTo(leftFront.x, leftFront.y, rightFront.x, rightFront.y, rightLip.x, rightLip.y);
      ctx.strokeStyle = '#8790C8';
      ctx.lineWidth = 1.25;
      ctx.lineCap = 'round';
      ctx.stroke();
      break;
    }
    ctx.restore();
  }

  function drawTail(ctx, seatX, seatY, state, skin, outlineOnly) {
    const curve = tailCurve(seatX, seatY, state, skin);
    const width = isDazedSkin(skin) ? 13.0 : (getBuild(skin).tailWidth * skin.shape.tailThickness);
    const lie = clamp(state.lieIntensity || 0, 0, 1);
    const lieS = lie * lie * (3.0 - 2.0 * lie);

    ctx.save();
    if (outlineOnly) {
      ctx.strokeStyle = WHITE_OUTLINE_CSS;
      ctx.lineWidth = width + WHITE_OUTLINE_W * 2.0;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      ctx.beginPath();
      ctx.moveTo(curve.root.x, curve.root.y);
      ctx.bezierCurveTo(curve.c1.x, curve.c1.y, curve.c2.x, curve.c2.y, curve.tip.x, curve.tip.y);
      ctx.stroke();
      ctx.restore();
      return;
    }
    ctx.strokeStyle = rgbaToCss(skin.palette.tail);
    ctx.lineWidth = width;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    ctx.moveTo(curve.root.x, curve.root.y);
    ctx.bezierCurveTo(curve.c1.x, curve.c1.y, curve.c2.x, curve.c2.y, curve.tip.x, curve.tip.y);
    ctx.stroke();

    if (lieS > 0.35) {
      const rAlpha = Math.min(1.0, (lieS - 0.35) / 0.45) * 0.72;
      ctx.save();
      ctx.strokeStyle = rgbaToCss(skin.palette.crease, rAlpha);
      ctx.lineWidth = 1.45;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(curve.root.x - 4, curve.root.y - 3);
      ctx.bezierCurveTo(curve.c1.x - 6, curve.c1.y - 5.5, curve.c2.x, curve.c2.y - 5.5, curve.tip.x + 2, curve.tip.y - 4.5);
      ctx.stroke();
      ctx.restore();
    }

    if (skin.hasPartArt('tail') || skin.palette.tailTip) {
      // Build stroked clip capsule along tail curve
      ctx.save();
      const leftPts = [], rightPts = [];
      const half = width * 0.5 + 0.3;
      const steps = 24;
      for (let i = 0; i <= steps; i++) {
        const x = (i / steps) * 100;
        leftPts.push(curve.at({ x, y: half / skin.shape.tailThickness }));
        rightPts.push(curve.at({ x, y: -half / skin.shape.tailThickness }));
      }
      ctx.beginPath();
      ctx.moveTo(leftPts[0].x, leftPts[0].y);
      for (let i = 1; i < leftPts.length; i++) ctx.lineTo(leftPts[i].x, leftPts[i].y);
      // Rounded tip cap
      const tipAngle = Math.atan2(curve.tip.y - curve.c2.y, curve.tip.x - curve.c2.x);
      ctx.arc(curve.tip.x, curve.tip.y, half, tipAngle + Math.PI * 0.5, tipAngle - Math.PI * 0.5, true);
      for (let i = rightPts.length - 1; i >= 0; i--) ctx.lineTo(rightPts[i].x, rightPts[i].y);
      ctx.closePath();
      ctx.clip();

      const map = (pt) => curve.at(pt);
      for (const r of skin.regions) {
        if (r.space === 'tail') drawRegion(ctx, r, map);
      }
      if (skin.palette.tailTip) {
        const t = 0.78, u = 1 - t;
        const lerp = (a, b) => ({ x: u * a.x + t * b.x, y: u * a.y + t * b.y });
        const a = lerp(curve.root, curve.c1), b = lerp(curve.c1, curve.c2), q = lerp(curve.c2, curve.tip);
        const d = lerp(a, b), e = lerp(b, q), s = lerp(d, e);
        ctx.strokeStyle = rgbaToCss(skin.palette.tailTip);
        ctx.lineWidth = width + 2;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.moveTo(s.x, s.y);
        ctx.bezierCurveTo(e.x, e.y, q.x, q.y, curve.tip.x, curve.tip.y);
        ctx.stroke();
      }
      for (const m of skin.marks) {
        if (m.space === 'tail') drawMark(ctx, m, 'butt', map);
      }
      ctx.restore();
    }
    ctx.restore();
  }

  function drawEar3D(ctx, isLeft, yawRad, pitchRad, fold, twitch, skin, outlineOnly) {
    if (isDazedSkin(skin)) {
      if (!outlineOnly) drawDazedEar(ctx, isLeft ? -1 : 1, yawRad, pitchRad, fold, twitch);
      return;
    }
    const bd = getBuild(skin);
    const sign = isLeft ? -1 : 1;
    const foldAngle = sign * (bd.earFold + fold * 0.34) + twitch;
    const earLen = bd.earLen;
    const tipDx = Math.sin(foldAngle) * earLen;
    const tipDy = -Math.cos(foldAngle) * earLen;

    let innerBase = projectHead3D(sign * bd.earInX, bd.earInY, 0, yawRad, pitchRad);
    let outerBase = projectHead3D(sign * bd.earOutX, bd.earOutY, -2, yawRad, pitchRad);
    let tip = projectHead3D(sign * bd.earTipX + tipDx, bd.earTipY + tipDy, -3, yawRad, pitchRad);

    const es = skin.shape.earSize;
    if (es !== 1) {
      const mx = (innerBase.x + outerBase.x) * 0.5;
      const my = (innerBase.y + outerBase.y) * 0.5;
      innerBase = { x: around(innerBase.x, mx, es), y: around(innerBase.y, my, es) };
      outerBase = { x: around(outerBase.x, mx, es), y: around(outerBase.y, my, es) };
      tip = { x: around(tip.x, mx, es), y: around(tip.y, my, es) };
    }

    const ebw = skin.shape.earBaseWidth || 1;
    if (ebw !== 1) {
      const mx = (innerBase.x + outerBase.x) * 0.5;
      const my = (innerBase.y + outerBase.y) * 0.5;
      innerBase = { x: around(innerBase.x, mx, ebw), y: around(innerBase.y, my, ebw) };
      outerBase = { x: around(outerBase.x, mx, ebw), y: around(outerBase.y, my, ebw) };
    }

    const ep = skin.shape.earPointiness;
    const tipSpread = bd.earTipSpread / ep;
    const tipLeft = { x: tip.x - sign * tipSpread, y: tip.y + 2.8 };
    const tipRight = { x: tip.x + sign * tipSpread, y: tip.y + 3.2 };

    const traceEar = () => {
      ctx.beginPath();
      ctx.moveTo(innerBase.x, innerBase.y);
      ctx.quadraticCurveTo(
        (innerBase.x + tipLeft.x) * 0.5 - sign * 1.5,
        (innerBase.y + tipLeft.y) * 0.5 - 2.0,
        tipLeft.x, tipLeft.y
      );
      ctx.quadraticCurveTo(tip.x, tip.y - 2.2 * ep, tipRight.x, tipRight.y);
      ctx.quadraticCurveTo(
        (outerBase.x + tipRight.x) * 0.5 + sign * 2.5,
        (outerBase.y + tipRight.y) * 0.5 - 2.0,
        outerBase.x, outerBase.y
      );
      ctx.closePath();
    };

    ctx.save();
    traceEar();
    if (outlineOnly) {
      ctx.strokeStyle = 'rgba(255,255,255,0.94)';
      ctx.lineWidth = 3.2;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.stroke();
      ctx.restore();
      return;
    }
    ctx.fillStyle = rgbaToCss(skin.palette.earOuter);
    ctx.fill();

    if (skin.palette.earInner) {
      const inI = { x: innerBase.x + (tip.x - innerBase.x) * 0.24 + sign * 3.0, y: innerBase.y + (tip.y - innerBase.y) * 0.16 };
      const outI = { x: outerBase.x + (tip.x - outerBase.x) * 0.20 - sign * 3.8, y: outerBase.y + (tip.y - outerBase.y) * 0.18 };
      const tipI = { x: (innerBase.x + outerBase.x) * 0.16 + tip.x * 0.68, y: (innerBase.y + outerBase.y) * 0.14 + tip.y * 0.72 + 2.5 };
      ctx.beginPath();
      ctx.moveTo(inI.x, inI.y);
      ctx.quadraticCurveTo((inI.x + tipI.x) * 0.5 - sign * 0.8, (inI.y + tipI.y) * 0.5, tipI.x, tipI.y);
      ctx.quadraticCurveTo((outI.x + tipI.x) * 0.5 + sign * 1.2, (outI.y + tipI.y) * 0.5, outI.x, outI.y);
      ctx.closePath();
      ctx.fillStyle = rgbaToCss(skin.palette.earInner);
      ctx.fill();
    }

    const space = isLeft ? 'leftEar' : 'rightEar';
    if (skin.hasPartArt(space)) {
      const m = { x: (innerBase.x + outerBase.x) * 0.5, y: (innerBase.y + outerBase.y) * 0.5 };
      const map = (p) => {
        const u = p.x / 100, v = -p.y / 100;
        return {
          x: m.x + u * (outerBase.x - innerBase.x) + v * (tip.x - m.x),
          y: m.y + u * (outerBase.y - innerBase.y) + v * (tip.y - m.y)
        };
      };
      ctx.save();
      traceEar();
      ctx.clip();
      for (const r of skin.regions) if (r.space === space) drawRegion(ctx, r, map);
      for (const mk of skin.marks) if (mk.space === space) drawMark(ctx, mk, 'round', map);
      ctx.restore();
    }
    ctx.restore();
  }

  function drawUnifiedSilhouette(ctx, seatX, seatY, headCenter, breath, state, skin, outlineOnly) {
    if (isDazedSkin(skin)) {
      if (!outlineOnly) drawDazedSilhouette(ctx, seatX, seatY, headCenter, state);
      return;
    }
    const bd = getBuild(skin);
    const pal = skin.palette;
    const yaw = state.headYaw, pitch = state.headPitch;
    const hTilt = state.headTilt, bTilt = state.bodyTilt;
    const hs = skin.shape.headScale, bw = skin.shape.bodyWidth;
    const lie = clamp(state.lieIntensity || 0, 0, 1);
    const lieS = lie * lie * (3.0 - 2.0 * lie);

    const cH = Math.cos(hTilt), sH = Math.sin(hTilt);
    const H = (lx, ly) => {
      const x = lx * hs, y = ly * hs;
      return { x: headCenter.x + x * cH - y * sH, y: headCenter.y + x * sH + y * cH };
    };

    const cB = Math.cos(bTilt), sB = Math.sin(bTilt);
    const sy = state.bodySquash * (1.0 - lieS * 0.42);
    const sx = (1 + (1 - state.bodySquash) * 0.55) * (1.0 + lieS * 0.03);
    const B = (lx, ly) => {
      const rx = lx * bw * sx, ry = ly * sy;
      return { x: seatX + rx * cB - ry * sB, y: seatY + rx * sB + ry * cB };
    };

    const cheekShift = yaw * 3.0;
    const crownDrop = pitch * 2.0;
    const cheekHalfW = bd.cheekHalfW * skin.shape.cheekFullness;
    const leftCheekX = -cheekHalfW + cheekShift * 0.4;
    const rightCheekX = cheekHalfW + cheekShift * 0.4;

    const crown = H(cheekShift * 0.2, bd.crownBaseY + crownDrop);
    const rCheek = H(rightCheekX, bd.cheekY);
    const rHip = B(bd.hipHalfW, -14);
    const rBot = B(bd.botHalfW, 12);
    const lBot = B(-bd.botHalfW, 12);
    const lHip = B(-bd.hipHalfW, -14);
    const lCheek = H(leftCheekX, bd.cheekY);

    const traceSilhouette = () => {
      const nw = skin.shape.neckWidth || 1;
      const neckCenter = cheekShift * 0.4;
      const rNeckX = neckCenter + (rightCheekX - bd.neckPinchX - neckCenter) * nw;
      const lNeckX = neckCenter + (leftCheekX + bd.neckPinchX - neckCenter) * nw;
      const neckWaistW = bd.waistCtrlW * (1 + (nw - 1) * 0.32);

      ctx.beginPath();
      ctx.moveTo(crown.x, crown.y);
      const c1a = H(bd.crownCtrlW + cheekShift * 0.3, bd.crownBaseY + crownDrop);
      const c1b = H(rightCheekX + 1.5, bd.cheekCtrlTopY);
      ctx.bezierCurveTo(c1a.x, c1a.y, c1b.x, c1b.y, rCheek.x, rCheek.y);

      const c2a = H(rNeckX, bd.neckCtrlY);
      const c2b = B(neckWaistW, bd.waistCtrlY + breath * 0.5);
      ctx.bezierCurveTo(c2a.x, c2a.y, c2b.x, c2b.y, rHip.x, rHip.y);

      const c3a = B(bd.botOuterW, 5), c3b = B(bd.botInnerW, 12);
      ctx.bezierCurveTo(c3a.x, c3a.y, c3b.x, c3b.y, rBot.x, rBot.y);
      ctx.lineTo(lBot.x, lBot.y);

      const c4a = B(-bd.botInnerW, 12), c4b = B(-bd.botOuterW, 5);
      ctx.bezierCurveTo(c4a.x, c4a.y, c4b.x, c4b.y, lHip.x, lHip.y);

      const c5a = B(-neckWaistW, bd.waistCtrlY + breath * 0.5);
      const c5b = H(lNeckX, bd.neckCtrlY);
      ctx.bezierCurveTo(c5a.x, c5a.y, c5b.x, c5b.y, lCheek.x, lCheek.y);

      const c6a = H(leftCheekX - 1.5, bd.cheekCtrlTopY);
      const c6b = H(-bd.crownCtrlW + cheekShift * 0.3, bd.crownBaseY + crownDrop);
      ctx.bezierCurveTo(c6a.x, c6a.y, c6b.x, c6b.y, crown.x, crown.y);
      ctx.closePath();
    };

    if (outlineOnly) {
      ctx.save();
      traceSilhouette();
      ctx.strokeStyle = WHITE_OUTLINE_CSS;
      ctx.lineWidth = WHITE_OUTLINE_W * 2.0;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.stroke();
      ctx.restore();
      return;
    }

    const headMap = (p) => headPoint(p, false, skin, headCenter, state);
    const skullMap = (p) => headPoint(p, true, skin, headCenter, state);
    const bodyMap = (p) => bodyPoint(p, skin, seatX, seatY, state);

    const hasPartRegions = skin.regions.some((r) => r.space === 'head' || r.space === 'skull' || r.space === 'body');
    const hasPartMarks = skin.marks.some((m) => m.space === 'head' || m.space === 'skull' || m.space === 'body');

    ctx.save();
    if (!pal.bodyShade) {
      traceSilhouette();
      ctx.fillStyle = rgbaToCss(pal.body);
      ctx.fill();
    }

    if (pal.bodyShade || skin.colorpoint || hasPartRegions || hasPartMarks) {
      ctx.save();
      traceSilhouette();
      ctx.clip();

      if (pal.bodyShade) {
        const grad = ctx.createLinearGradient(seatX, headCenter.y - 34, seatX, seatY + 18);
        grad.addColorStop(0.15, rgbaToCss(pal.body));
        grad.addColorStop(1.0, rgbaToCss(pal.bodyShade));
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, 360, 320);
      }

      // Curve regions with exact jaw-line clipping
      for (const r of skin.regions) {
        if (r.space !== 'head' && r.space !== 'skull' && r.space !== 'body') continue;
        ctx.save();
        const side = jawSide(r.space !== 'body', skin.shape.cheekFullness);
        drawPolyline(ctx, side, true, headMap);
        ctx.clip();
        const map = r.space === 'body' ? bodyMap : (r.space === 'skull' ? skullMap : headMap);
        drawRegion(ctx, r, map);
        ctx.restore();
      }

      // 2.5D Colorpoint face mask
      if (pal.points) {
        const mid = pal.pointsMid || pal.points;
        const yawRad = yaw * 0.50, pitchRad = pitch * 0.36;
        ctx.save();
        ctx.translate(headCenter.x, headCenter.y);
        ctx.rotate(hTilt);
        if (hs !== 1) ctx.scale(hs, hs);

        const maskCenter = projectHead3D(0, 3.0, 27.0, yawRad, pitchRad);
        const maskScaleX = Math.max(0.76, Math.cos(yawRad * 0.7));
        const maskScaleY = Math.max(0.82, Math.cos(pitchRad * 0.7));

        ctx.save();
        ctx.translate(maskCenter.x, maskCenter.y);
        ctx.scale(maskScaleX * 1.24, maskScaleY * 0.88);
        const g1 = ctx.createRadialGradient(0, 0, 0, 0, 0, 31.5);
        g1.addColorStop(0.0, rgbaToCss(pal.points, 1.0));
        g1.addColorStop(0.42, rgbaToCss(pal.points, 0.92));
        g1.addColorStop(0.74, rgbaToCss(mid, 0.62));
        g1.addColorStop(1.0, rgbaToCss(mid, 0.0));
        ctx.fillStyle = g1;
        ctx.beginPath();
        ctx.arc(0, 0, 32, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();

        const muzCenter = projectHead3D(0, 7.5, 28.0, yawRad, pitchRad);
        ctx.save();
        ctx.translate(muzCenter.x, muzCenter.y);
        ctx.scale(maskScaleX * 1.04, maskScaleY * 0.76);
        const g2 = ctx.createRadialGradient(0, 0, 0, 0, 0, 19.5);
        g2.addColorStop(0.0, rgbaToCss(pal.points, 1.0));
        g2.addColorStop(0.50, rgbaToCss(pal.points, 0.85));
        g2.addColorStop(1.0, rgbaToCss(pal.points, 0.0));
        ctx.fillStyle = g2;
        ctx.beginPath();
        ctx.arc(0, 0, 20, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();

        ctx.restore();
      }

      // Marks (stripes/polygons)
      const bodyJawSide = jawSide(false, skin.shape.cheekFullness);
      const headJawSide = jawSide(true, skin.shape.cheekFullness);
      for (const mk of skin.marks) {
        if (mk.space === 'body') {
          ctx.save();
          drawPolyline(ctx, bodyJawSide, true, headMap);
          ctx.clip();
          drawMark(ctx, mk, 'round', bodyMap);
          ctx.restore();
        } else if (mk.space === 'skull' || mk.space === 'head') {
          const skipJawClip = mk.clipToBib || (mk.space === 'head' && mk.kind !== 'stroke' && mk.color && Math.abs(mk.color.r - 242 / 255) < 0.01 && Math.abs(mk.color.g - 238 / 255) < 0.01 && Math.abs(mk.color.b - 228 / 255) < 0.01 && mk.points.some((pt) => Math.abs(pt[0]) > 46));
          const mapFn = mk.space === 'skull' ? skullMap : headMap;
          if (skipJawClip) {
            drawMark(ctx, mk, 'round', mapFn);
          } else {
            ctx.save();
            drawPolyline(ctx, headJawSide, true, headMap);
            ctx.clip();
            drawMark(ctx, mk, 'round', mapFn);
            ctx.restore();
          }
        }
      }

      ctx.restore();
    }
    ctx.restore();

    if (pal.outline) {
      ctx.save();
      traceSilhouette();
      ctx.strokeStyle = rgbaToCss(pal.outline);
      ctx.lineWidth = 1.0;
      ctx.stroke();
      ctx.restore();
    }

    // Tucked hind paws
    ctx.save();
    ctx.translate(seatX, seatY);
    ctx.rotate(bTilt);
    ctx.scale(sx, sy);

    const stagger = skin.shape.pawStagger;
    const hindW = bd.hindW;
    const leftHindX = bw === 1 ? bd.hindLeftX : (bd.hindLeftX + hindW * 0.5) * bw - hindW * 0.5;
    const rightHindX = bw === 1 ? bd.hindRightX : (bd.hindRightX + hindW * 0.5) * bw - hindW * 0.5;
    const leftHindY = bd.hindLeftY, leftHindH = bd.hindLeftH;
    const rightHindY = bd.hindLeftY + (bd.hindStaggerY - bd.hindLeftY) * stagger;
    const rightHindH = bd.hindLeftH + (bd.hindStaggerH - bd.hindLeftH) * stagger;
    const rightTucked = state.hindFootActive < 0.35;

    ctx.fillStyle = rgbaToCss(pal.paws);
    ctx.beginPath();
    ctx.ellipse(leftHindX + hindW * 0.5, leftHindY + leftHindH * 0.5, hindW * 0.5, leftHindH * 0.5, 0, 0, Math.PI * 2);
    ctx.fill();
    if (rightTucked) {
      ctx.beginPath();
      ctx.ellipse(rightHindX + hindW * 0.5, rightHindY + rightHindH * 0.5, hindW * 0.5, rightHindH * 0.5, 0, 0, Math.PI * 2);
      ctx.fill();
    }

    ctx.strokeStyle = skin.colorpoint ? rgbaToCss(pal.body, 0.55) : rgbaToCss(pal.crease, 0.58);
    ctx.lineWidth = 1.3;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.arc(leftHindX + hindW * 0.5, leftHindY + leftHindH * 0.5, 7.5, 0.25, Math.PI - 0.25);
    ctx.stroke();
    if (rightTucked) {
      ctx.beginPath();
      ctx.arc(rightHindX + hindW * 0.5, rightHindY + rightHindH * 0.5, 7.5, 0.25, Math.PI - 0.25);
      ctx.stroke();
    }

    if (pal.pawPads) {
      ctx.fillStyle = rgbaToCss(pal.pawPads);
      const count = rightTucked ? 2 : 1;
      for (let side = 0; side < count; side++) {
        const cx = (side === 1 ? rightHindX : leftHindX) + hindW * 0.5;
        const cy = side === 1 ? rightHindY + rightHindH * 0.5 : leftHindY + leftHindH * 0.5;
        ctx.beginPath();
        ctx.ellipse(cx, cy + 1.6, 2.7, 1.9, 0, 0, Math.PI * 2);
        ctx.fill();
        for (const tx of [-3.4, 0, 3.4]) {
          const ty = cy - 1.7 - (tx === 0 ? 0.8 : 0);
          ctx.beginPath();
          ctx.ellipse(cx + tx, ty, 1.25, 1.1, 0, 0, Math.PI * 2);
          ctx.fill();
        }
      }
    }
    ctx.restore();
  }

  function drawEye3D(ctx, isLeft, yawRad, pitchRad, state, eyeThemeName, skin, scruffStartle, scruffWide) {
    const sStartle = scruffStartle || 0;
    const sWide = scruffWide || 0;
    const sign = isLeft ? -1 : 1;
    const customIris = skin.palette.iris;
    const eyeTheme = EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse;
    const eyeColorCss = customIris
      ? rgbaToCss(customIris)
      : `rgb(${eyeTheme.primary[0]},${eyeTheme.primary[1]},${eyeTheme.primary[2]})`;
    const siameseBlue = !customIris && eyeThemeName === 'siameseBlue';

    const proj = projectHead3D(sign * 17.2, -2.0, 27.5, yawRad, pitchRad);
    const eyeNormalAngle = yawRad + sign * 0.34;
    const fx = clamp(Math.cos(eyeNormalAngle) / Math.cos(0.34), 0.64, 1.06);
    const fy = clamp(Math.cos(pitchRad * 0.85), 0.78, 1.03);

    ctx.save();
    ctx.translate(proj.x, proj.y);
    ctx.scale(fx, fy);

    // Mode 0: Dizzy spinning spiral eyes (@ @) — appear AFTER initial golden stars warning (>0.28)
    const dizzy = clamp(state.dizzyIntensity || 0, 0, 1);
    if (dizzy > 0.28) {
      ctx.fillStyle = 'rgba(250,250,245,0.95)';
      ctx.beginPath();
      ctx.ellipse(0, 0, 11.5, 11.0, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.strokeStyle = 'rgba(36,36,46,0.95)';
      ctx.lineWidth = 1.85;
      ctx.lineCap = 'round';
      ctx.beginPath();
      const spin = (state.dizzyPhase || 0) * (isLeft ? 1.0 : -1.0);
      for (let step = 0; step <= 42; step++) {
        const t = step / 42.0;
        const theta = spin + t * Math.PI * 5.2;
        const rad = 1.0 + t * 8.6;
        const sx = Math.cos(theta) * rad;
        const sy = Math.sin(theta) * rad;
        if (step === 0) ctx.moveTo(sx, sy);
        else ctx.lineTo(sx, sy);
      }
      ctx.stroke();
      ctx.restore();
      return;
    }

    const bodyLuma = 0.299 * skin.palette.body.r + 0.587 * skin.palette.body.g + 0.114 * skin.palette.body.b;
    const pointsLuma = skin.palette.points
      ? (0.299 * skin.palette.points.r + 0.587 * skin.palette.points.g + 0.114 * skin.palette.points.b)
      : 1.0;
    const outlineLuma = skin.palette.outline
      ? (0.299 * skin.palette.outline.r + 0.587 * skin.palette.outline.g + 0.114 * skin.palette.outline.b)
      : 1.0;
    const isDarkEyeArea = bodyLuma < 0.28 || (skin.colorpoint && pointsLuma < 0.38);
    const closedLidCss = isDarkEyeArea
      ? eyeColorCss
      : (skin.palette.outline && outlineLuma < 0.42 ? rgbaToCss(skin.palette.outline, 0.90) : 'rgba(60, 50, 44, 0.88)');

    if (state.petHead > 0.45) {
      ctx.strokeStyle = closedLidCss;
      ctx.lineWidth = 2.6;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(-11.0, 1.2);
      ctx.quadraticCurveTo(sign * 1.5, 4.0, 11.0, 1.2);
      ctx.stroke();
      ctx.restore();
      return;
    }

    if (state.sleepiness > 0.62 && sStartle < 0.15) {
      // Peaceful closed curved sleeping eyelids (always clearly visible on both dark & light fur!)
      ctx.strokeStyle = closedLidCss;
      ctx.lineWidth = 2.5;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(-10.5, 0.4);
      ctx.quadraticCurveTo(0, 5.2, 10.5, 0.4);
      ctx.stroke();
      ctx.restore();
      return;
    }

    if (state.petBody > 0.35) {
      ctx.fillStyle = eyeColorCss;
      ctx.beginPath();
      ctx.moveTo(-11.5, -0.5);
      ctx.quadraticCurveTo(0, 0.6, 11.5, -0.5);
      ctx.quadraticCurveTo(0, 4.8, -11.5, -0.5);
      ctx.closePath();
      ctx.fill();
      if (!isDarkEyeArea) {
        ctx.strokeStyle = closedLidCss;
        ctx.lineWidth = 1.6;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.moveTo(-11.2, -0.5);
        ctx.quadraticCurveTo(0, 0.6, 11.2, -0.5);
        ctx.stroke();
      }
      ctx.restore();
      return;
    }

    const hunt = clamp(Math.max(state.huntIntensity, sWide * 0.94), 0, 1);
    ctx.rotate(sign * (0.06 * (1 - hunt * 0.6)));
    const isSiameseEye = getBuild(skin).sleekEyes && siameseBlue;
    const rx = isSiameseEye ? 11.4 + hunt * 1.3 : 12.0 + hunt * 1.1;
    const ry = isSiameseEye ? 9.8 + hunt * 2.0 : 10.8 + hunt * 1.8;

    const closure = clamp(Math.max(state.blinkProgress, state.sleepiness * 0.92, sStartle * 0.44), 0, 1);
    if (closure >= 0.99) {
      ctx.restore();
      return;
    }

    ctx.save();
    ctx.beginPath();
    ctx.ellipse(0, 0, rx, ry, 0, 0, Math.PI * 2);
    ctx.clip();

    if (closure > 0) {
      ctx.beginPath();
      ctx.rect(-rx - 2, -ry + 2 * ry * closure, 2 * rx + 4, 2 * ry * (1 - closure));
      ctx.clip();
    }

    // Angry furrowed V-brow slant clipped cleanly inside the eye opening (Siamese-safe!)
    const anger = clamp(state.angerIntensity || 0, 0, 1);
    if (anger > 0.15) {
      const drop = 7.5 * anger;
      ctx.beginPath();
      ctx.moveTo(sign * (rx + 4), -ry + 1.5);
      ctx.lineTo(-sign * (rx + 4), -ry + 2.5 + drop);
      ctx.lineTo(-sign * (rx + 4), ry + 4);
      ctx.lineTo(sign * (rx + 4), ry + 4);
      ctx.closePath();
      ctx.clip();
    }

    if (siameseBlue) {
      const g = ctx.createLinearGradient(0, -ry, 0, ry);
      g.addColorStop(0.0, '#8FC4E6');
      g.addColorStop(0.55, '#B5DEF5');
      g.addColorStop(1.0, '#DBF1FC');
      ctx.fillStyle = g;
    } else {
      ctx.fillStyle = eyeColorCss;
    }
    ctx.fillRect(-rx - 1, -ry - 1, rx * 2 + 2, ry * 2 + 2);

    const px = state.eyeGazeX * (4.2 - hunt * 1.2);
    const py = state.eyeGazeY * (3.6 - hunt * 1.0);
    const normalW = 5.0 * state.pupilScale;
    const normalH = 13.5 * Math.min(1.12, state.pupilScale);
    const pupilW = normalW + (18.8 - normalW) * hunt;
    const pupilH = normalH + (18.8 - normalH) * hunt;

    if (!customIris && eyeThemeName === 'referenceHazel') {
      for (let i = 0; i <= 16; i++) {
        const t = i / 16;
        const mix = t * t * (3 - 2 * t);
        const rim = (1 - t) * 2.2;
        ctx.fillStyle = `rgb(${Math.round(218 - 77 * mix)},${Math.round(178 - 33 * mix)},${Math.round(87 + 41 * mix)})`;
        ctx.beginPath();
        ctx.ellipse(px, py, pupilW * 0.5 + rim, pupilH * 0.5 + rim, 0, 0, Math.PI * 2);
        ctx.fill();
      }
    }

    ctx.fillStyle = CAT_BLACK;
    ctx.beginPath();
    ctx.ellipse(px, py, pupilW * 0.5, pupilH * 0.5, 0, 0, Math.PI * 2);
    ctx.fill();

    if (hunt > 0.25) {
      const alpha = Math.min(1, (hunt - 0.25) / 0.55) * 0.92;
      const gr = 2.1 + hunt * 0.9;
      ctx.fillStyle = `rgba(255,255,255,${alpha.toFixed(3)})`;
      ctx.beginPath();
      ctx.arc(px - pupilW * 0.22 + gr * 0.5, py - pupilH * 0.22 + gr * 0.5, gr, 0, Math.PI * 2);
      ctx.fill();
      if (sWide > 0.25) {
        const sAlpha = Math.min(1, (sWide - 0.25) / 0.45) * 0.88;
        ctx.fillStyle = `rgba(255,255,255,${sAlpha.toFixed(3)})`;
        ctx.beginPath();
        ctx.arc(px + pupilW * 0.16, py + pupilH * 0.18, 1.45, 0, Math.PI * 2);
        ctx.fill();
      }
    } else if (siameseBlue) {
      ctx.fillStyle = 'rgba(255,255,255,0.86)';
      ctx.beginPath();
      ctx.arc(px - 2.9, py - 3.6, 1.65, 0, Math.PI * 2);
      ctx.fill();
    }

    ctx.restore();
    ctx.restore();
  }

  function drawHeadFeatures(ctx, center, state, eyeThemeName, skin, scruffStartle, scruffWide) {
    ctx.save();
    ctx.translate(center.x, center.y);
    ctx.rotate(state.headTilt);
    if (skin.shape.headScale !== 1) ctx.scale(skin.shape.headScale, skin.shape.headScale);

    const yawRad = state.headYaw * 0.50;
    const pitchRad = state.headPitch * 0.36;

    if (isDazedSkin(skin)) {
      drawDazedFace(ctx, state, eyeThemeName, yawRad, pitchRad, scruffStartle, scruffWide);
      ctx.restore();
      return;
    }

    drawEye3D(ctx, true, yawRad, pitchRad, state, eyeThemeName, skin, scruffStartle, scruffWide);
    drawEye3D(ctx, false, yawRad, pitchRad, state, eyeThemeName, skin, scruffStartle, scruffWide);

    if (skin.palette.nose || skin.palette.mouth) {
      ctx.save();
      if (skin.palette.nose) {
        const n = projectHead3D(0, 8.2, 29.0, yawRad, pitchRad);
        ctx.fillStyle = rgbaToCss(skin.palette.nose);
        ctx.beginPath();
        ctx.moveTo(n.x - 2.3, n.y - 1.1);
        ctx.quadraticCurveTo(n.x, n.y - 1.6, n.x + 2.3, n.y - 1.1);
        ctx.quadraticCurveTo(n.x + 1.8, n.y + 0.6, n.x, n.y + 1.5);
        ctx.quadraticCurveTo(n.x - 1.8, n.y + 0.6, n.x - 2.3, n.y - 1.1);
        ctx.closePath();
        ctx.fill();
      }
      if (skin.palette.mouth && (state.yawnAmount || 0) < 0.15) {
        const mL = projectHead3D(-4.2, 12.2, 28.4, yawRad, pitchRad);
        const mM = projectHead3D(0, 11.2, 28.8, yawRad, pitchRad);
        const mR = projectHead3D(4.2, 12.2, 28.4, yawRad, pitchRad);
        ctx.strokeStyle = rgbaToCss(skin.palette.mouth);
        ctx.lineWidth = 1.05;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.moveTo(mL.x, mL.y);
        ctx.quadraticCurveTo((mL.x + mM.x) * 0.5, mL.y + 1.6, mM.x, mM.y);
        ctx.quadraticCurveTo((mM.x + mR.x) * 0.5, mR.y + 1.6, mR.x, mR.y);
        ctx.stroke();
      }
      ctx.restore();
    }

    // Cute pink open yawn mouth (with tongue & tiny white fangs!) right before falling asleep
    if ((state.yawnAmount || 0) > 0.03) {
      const y = clamp(state.yawnAmount, 0, 1);
      const mc = projectHead3D(0, 16.2 + y * 2.2, 30.2, yawRad, pitchRad);
      const rx = 3.2 + y * 5.8, ry = 1.6 + y * 7.4;
      ctx.save();
      ctx.translate(mc.x, mc.y);
      const traceMouth = () => {
        ctx.beginPath();
        ctx.moveTo(0, -ry * 0.92);
        ctx.bezierCurveTo(rx * 0.68, -ry * 0.92, rx * 1.05, -ry * 0.42, rx, ry * 0.08);
        ctx.bezierCurveTo(rx * 0.95, ry * 0.72, rx * 0.50, ry, 0, ry);
        ctx.bezierCurveTo(-rx * 0.50, ry, -rx * 0.95, ry * 0.72, -rx, ry * 0.08);
        ctx.bezierCurveTo(-rx * 1.05, -ry * 0.42, -rx * 0.68, -ry * 0.92, 0, -ry * 0.92);
        ctx.closePath();
      };
      traceMouth();
      ctx.fillStyle = 'rgba(199,48,64,0.98)';
      ctx.fill();
      ctx.save();
      traceMouth();
      ctx.clip();
      ctx.fillStyle = 'rgba(140,26,41,0.55)';
      ctx.beginPath(); ctx.ellipse(0, -ry * 0.26, rx * 0.65, ry * 0.46, 0, 0, Math.PI * 2); ctx.fill();
      if (y > 0.22) {
        const ta = Math.min(1, (y - 0.22) / 0.35) * 0.92;
        ctx.fillStyle = `rgba(242,122,140,${ta.toFixed(3)})`;
        ctx.beginPath();
        ctx.moveTo(-rx * 0.62, ry * 0.55);
        ctx.quadraticCurveTo(-rx * 0.32, ry * 0.16, 0, ry * 0.36);
        ctx.quadraticCurveTo(rx * 0.32, ry * 0.16, rx * 0.62, ry * 0.55);
        ctx.quadraticCurveTo(0, ry * 1.25, -rx * 0.62, ry * 0.55);
        ctx.closePath();
        ctx.fill();
      }
      if (y > 0.28) {
        const fs = Math.min(1, (y - 0.28) / 0.32);
        ctx.fillStyle = 'rgba(255,252,245,0.98)';
        const ubY = -ry * 0.80, uLen = (2.6 + y * 2.2) * fs;
        for (const s of [-1, 1]) {
          ctx.beginPath();
          ctx.moveTo(s * rx * 0.38, ubY);
          ctx.lineTo(s * rx * 0.52, ubY + uLen);
          ctx.lineTo(s * rx * 0.64, ubY);
          ctx.closePath();
          ctx.fill();
        }
      }
      ctx.restore();
      ctx.restore();
    }

    // 3D whiskers
    const wiggle = Math.sin(state.time * 4.5) * Math.max(state.petHead, state.petBody) * 1.2;
    ctx.strokeStyle = rgbaToCss(skin.palette.whiskers);
    ctx.lineWidth = 1.0;
    ctx.lineCap = 'round';
    ctx.beginPath();
    for (const sign of [-1, 1]) {
      const rows = [
        [10.5, 36.5, 8.8 + wiggle],
        [14.2, 37.8, 14.2 + wiggle * 0.5],
        [18.0, 35.5, 19.6 - wiggle * 0.4]
      ];
      for (const [y0, xEnd, yEnd] of rows) {
        const p0 = projectHead3D(sign * 24.0, y0, 23.0, yawRad, pitchRad);
        const p2 = projectHead3D(sign * xEnd, yEnd, 14.0, yawRad, pitchRad);
        ctx.moveTo(p0.x, p0.y);
        ctx.quadraticCurveTo((p0.x + p2.x) * 0.5, (p0.y + p2.y) * 0.5 - 0.8, p2.x, p2.y);
      }
    }
    ctx.stroke();

    // Pulsing red 4-corner comic anger vein (💢) when typing super fast
    const anger = clamp(state.angerIntensity || 0, 0, 1);
    if (anger > 0.25) {
      const aAlpha = Math.min(1.0, (anger - 0.25) / 0.35);
      const pulse = 1.0 + Math.sin(state.time * 14.0) * 0.10;
      const pt = projectHead3D(28.0, -22.0, 24.0, yawRad, pitchRad);
      ctx.save();
      ctx.translate(pt.x, pt.y);
      ctx.scale(pulse, pulse);
      ctx.strokeStyle = `rgba(245,56,71,${(0.95 * aAlpha).toFixed(3)})`;
      ctx.lineWidth = 2.0;
      ctx.lineCap = 'round';
      const d = 2.4, r = 6.2;
      ctx.beginPath();
      for (const [sx, sy] of [[-1,-1],[1,-1],[-1,1],[1,1]]) {
        ctx.moveTo(sx * r, sy * d);
        ctx.quadraticCurveTo(sx * d, sy * d, sx * d, sy * r);
      }
      ctx.stroke();
      ctx.restore();
    }
    ctx.restore();
  }

  function quadPoint(p0, p1, p2, t) {
    const mt = 1 - t;
    return {
      x: mt * mt * p0.x + 2 * mt * t * p1.x + t * t * p2.x,
      y: mt * mt * p0.y + 2 * mt * t * p1.y + t * t * p2.y
    };
  }

  function quadTangent(p0, p1, p2, t) {
    const mt = 1 - t;
    return {
      x: 2 * mt * (p1.x - p0.x) + 2 * t * (p2.x - p1.x),
      y: 2 * mt * (p1.y - p0.y) + 2 * t * (p2.y - p1.y)
    };
  }

  function drawLimb(ctx, start, control, end, startW, endW, outlineOnLeft, outlineAlpha, skin, frontPaw = false, outlineOnly = false) {
    const pal = skin.palette;
    const steps = 14;
    const leftEdge = [], rightEdge = [];
    for (let i = 0; i <= steps; i++) {
      const t = i / steps;
      const p = quadPoint(start, control, end, t);
      const tan = quadTangent(start, control, end, t);
      const len = Math.max(0.001, Math.hypot(tan.x, tan.y));
      const nx = -tan.y / len, ny = tan.x / len;
      const hw = (startW + (endW - startW) * t) * 0.5;
      leftEdge.push({ x: p.x + nx * hw, y: p.y + ny * hw });
      rightEdge.push({ x: p.x - nx * hw, y: p.y - ny * hw });
    }

    const traceLimb = () => {
      ctx.beginPath();
      ctx.moveTo(leftEdge[0].x, leftEdge[0].y);
      for (let i = 1; i < leftEdge.length; i++) ctx.lineTo(leftEdge[i].x, leftEdge[i].y);
      for (let i = rightEdge.length - 1; i >= 0; i--) ctx.lineTo(rightEdge[i].x, rightEdge[i].y);
      const tan0 = quadTangent(start, control, end, 0);
      const len0 = Math.max(0.001, Math.hypot(tan0.x, tan0.y));
      ctx.quadraticCurveTo(
        start.x - (tan0.x / len0) * startW * 0.5,
        start.y - (tan0.y / len0) * startW * 0.5,
        leftEdge[0].x, leftEdge[0].y
      );
      ctx.closePath();
    };

    const pawR = endW * 0.58;
    const lineColor = pal.outline || pal.crease;

    if (outlineOnly) {
      if (!stateWhiteOutline) return;
      ctx.save();
      ctx.strokeStyle = WHITE_OUTLINE_CSS;
      ctx.lineWidth = WHITE_OUTLINE_W * 2.0;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      ctx.beginPath();
      ctx.moveTo(leftEdge[0].x, leftEdge[0].y);
      for (let i = 1; i < leftEdge.length; i++) ctx.lineTo(leftEdge[i].x, leftEdge[i].y);
      ctx.moveTo(rightEdge[0].x, rightEdge[0].y);
      for (let i = 1; i < rightEdge.length; i++) ctx.lineTo(rightEdge[i].x, rightEdge[i].y);
      ctx.stroke();
      ctx.beginPath();
      if (isDazedSkin(skin)) {
        ctx.ellipse(end.x, end.y, pawR * 1.28, pawR * (0.85 + 0.46), 0, 0, Math.PI * 2);
      } else {
        ctx.ellipse(end.x, end.y, pawR, pawR * 0.85, 0, 0, Math.PI * 2);
      }
      ctx.stroke();
      ctx.restore();
      return;
    }

    if (isDazedSkin(skin)) {
      const pawRect = {
        x: end.x - pawR * 1.28,
        y: end.y - pawR * (0.85 + 0.46),
        w: pawR * 2.56,
        h: pawR * (1.70 + 0.92)
      };
      if (frontPaw) {
        drawDazedArm(ctx, traceLimb, leftEdge, rightEdge, pawRect);
      } else {
        ctx.save();
        traceLimb();
        ctx.fillStyle = '#E3C99F';
        ctx.fill();
        drawDazedPaw(ctx, pawRect.x, pawRect.y, pawRect.w, pawRect.h);
        ctx.restore();
      }
      return;
    }

    ctx.save();
    if (pal.points) {
      const mid = pal.pointsMid || pal.points;
      ctx.save();
      traceLimb();
      ctx.clip();
      const g = ctx.createLinearGradient(start.x, start.y, end.x, end.y);
      g.addColorStop(0.0, rgbaToCss(pal.body, 0));
      g.addColorStop(0.16, rgbaToCss(pal.body, 1));
      g.addColorStop(0.48, rgbaToCss(mid, 1));
      g.addColorStop(0.78, rgbaToCss(pal.points, 1));
      ctx.fillStyle = g;
      ctx.fillRect(0, 0, 360, 320);
      ctx.restore();

      ctx.fillStyle = rgbaToCss(pal.paws);
      ctx.beginPath();
      ctx.ellipse(end.x, end.y, pawR, pawR * 0.85, 0, 0, Math.PI * 2);
      ctx.fill();

      const outerEdge = outlineOnLeft ? rightEdge : leftEdge;
      ctx.strokeStyle = rgbaToCss(lineColor, 0.72);
      ctx.lineWidth = 1.25;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(outerEdge[3].x, outerEdge[3].y);
      for (let i = 4; i < outerEdge.length - 1; i++) ctx.lineTo(outerEdge[i].x, outerEdge[i].y);
      ctx.stroke();
    } else {
      traceLimb();
      if (pal.belly && !sameColor(pal.belly, pal.legs)) {
        ctx.save();
        ctx.clip();
        const g = ctx.createLinearGradient(start.x, start.y, end.x, end.y);
        g.addColorStop(0, rgbaToCss(pal.belly));
        g.addColorStop(0.30, rgbaToCss(pal.legs));
        g.addColorStop(1, rgbaToCss(pal.legs));
        ctx.fillStyle = g;
        ctx.fillRect(0, 0, 360, 320);
        ctx.restore();
      } else {
        ctx.fillStyle = rgbaToCss(pal.legs);
        ctx.fill();
      }

      if (sameColor(pal.paws, pal.legs)) {
        ctx.fillStyle = rgbaToCss(pal.legs);
        ctx.beginPath();
        ctx.ellipse(end.x, end.y, pawR, pawR * 0.85, 0, 0, Math.PI * 2);
        ctx.fill();
      } else {
        const first = Math.floor(steps * 0.72);
        ctx.fillStyle = rgbaToCss(pal.paws);
        ctx.beginPath();
        ctx.moveTo(leftEdge[first].x, leftEdge[first].y);
        for (let i = first + 1; i < leftEdge.length; i++) ctx.lineTo(leftEdge[i].x, leftEdge[i].y);
        for (let i = rightEdge.length - 1; i >= first; i--) ctx.lineTo(rightEdge[i].x, rightEdge[i].y);
        ctx.closePath();
        ctx.fill();
        ctx.beginPath();
        ctx.ellipse(end.x, end.y, pawR, pawR * 0.85, 0, 0, Math.PI * 2);
        ctx.fill();
      }
    }

    const highlightEdge = outlineOnLeft ? leftEdge : rightEdge;
    ctx.strokeStyle = skin.colorpoint ? rgbaToCss(lineColor, outlineAlpha * 0.78) : rgbaToCss(pal.crease, outlineAlpha * 0.9);
    ctx.lineWidth = 1.35;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(highlightEdge[2].x, highlightEdge[2].y);
    const endIdx = Math.floor((highlightEdge.length - 1) * 0.68);
    for (let i = 3; i <= endIdx; i++) ctx.lineTo(highlightEdge[i].x, highlightEdge[i].y);
    ctx.stroke();
    ctx.restore();
  }

  function drawIdleFrontLeg(ctx, top, bottom, isLeft, skin, outlineOnly = false) {
    if (isDazedSkin(skin)) {
      const sign = isLeft ? -1 : 1;
      const shoulder = { x: top.x + sign * 4.0, y: top.y - 10.0 };
      const elbow = { x: top.x + sign * 11.0, y: top.y + 1.0 };
      const paw = { x: top.x + sign * 10.0, y: top.y + 15.0 };
      drawLimb(ctx, shoulder, elbow, paw, 19.0, 14.0, !isLeft, 0.78, skin, true, outlineOnly);
      return;
    }
    if (outlineOnly) return;
    const pal = skin.palette;
    const sign = isLeft ? -1 : 1;
    ctx.save();

    const traceLeg = () => {
      const oTop = { x: top.x + sign * 8.5, y: top.y };
      const oCtrl = { x: top.x + sign * 9.5, y: (top.y + bottom.y) * 0.5 };
      const oBot = { x: bottom.x + sign * 7.5, y: bottom.y - 1 };
      const iBot = { x: bottom.x - sign * 6.0, y: bottom.y - 1 };
      const iCtrl = { x: top.x - sign * 6.5, y: (top.y + bottom.y) * 0.5 };
      const iTop = { x: top.x - sign * 7.0, y: top.y + 4 };
      ctx.beginPath();
      ctx.moveTo(oTop.x, oTop.y);
      ctx.quadraticCurveTo(oCtrl.x, oCtrl.y, oBot.x, oBot.y);
      ctx.quadraticCurveTo(bottom.x + sign * 7.0, bottom.y + 5.2, bottom.x + sign * 0.8, bottom.y + 5.2);
      ctx.quadraticCurveTo(bottom.x - sign * 6.2, bottom.y + 5.2, iBot.x, iBot.y);
      ctx.quadraticCurveTo(iCtrl.x, iCtrl.y, iTop.x, iTop.y);
      ctx.closePath();
    };

    if (pal.points) {
      const mid = pal.pointsMid || pal.points;
      ctx.save();
      roundedRectPath(ctx, bottom.x - 7.5, top.y, 15.0, bottom.y - top.y + 5.0, 6.5);
      ctx.clip();
      const g = ctx.createLinearGradient(top.x, top.y, bottom.x, bottom.y + 5.0);
      g.addColorStop(0.05, rgbaToCss(pal.body, 0));
      g.addColorStop(0.50, rgbaToCss(mid, 0.75));
      g.addColorStop(0.82, rgbaToCss(pal.points, 1));
      ctx.fillStyle = g;
      ctx.fillRect(bottom.x - 9, top.y - 1, 18, bottom.y - top.y + 8);
      ctx.restore();
    } else {
      // Smooth shoulder-fade leg fill so palette.legs shows cleanly on idle sitting legs too
      // ("idleLegs": "legs"; the built-in coats use "chest" and keep their approved look)
      if (skin.idleLegsTinted !== false) {
        ctx.save();
        traceLeg();
        ctx.clip();
        const g = ctx.createLinearGradient(top.x, top.y, bottom.x, bottom.y + 5.0);
        g.addColorStop(0.04, rgbaToCss(pal.legs, 0));
        g.addColorStop(0.32, rgbaToCss(pal.legs, 1));
        g.addColorStop(1.00, rgbaToCss(pal.legs, 1));
        ctx.fillStyle = g;
        ctx.fillRect(bottom.x - 14, top.y - 1, 28, bottom.y - top.y + 8);
        ctx.restore();
      }

      if (stateWhiteOutline) {
        ctx.save();
        ctx.strokeStyle = WHITE_OUTLINE_CSS;
        ctx.lineWidth = WHITE_OUTLINE_W * 1.8;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.arc(bottom.x, bottom.y - 1.5, 7.2, 0.15, Math.PI - 0.15);
        ctx.stroke();
        ctx.restore();
      }
      ctx.fillStyle = rgbaToCss(pal.paws);
      roundedRectPath(ctx, bottom.x - 7.5, bottom.y - 13, 15, 18, 6.5);
      ctx.fill();
    }

    ctx.strokeStyle = isClassicBlackRim(skin) ? 'rgba(255,255,255,0.75)' : rgbaToCss(pal.crease);
    ctx.lineWidth = 1.45;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(top.x + sign * 8.5, top.y);
    ctx.quadraticCurveTo(top.x + sign * 9.5, (top.y + bottom.y) * 0.5, bottom.x + sign * 7.5, bottom.y - 1);
    ctx.moveTo(top.x - sign * 7.0, top.y + 4);
    ctx.quadraticCurveTo(top.x - sign * 6.5, (top.y + bottom.y) * 0.5, bottom.x - sign * 6.0, bottom.y - 1);
    ctx.stroke();
    ctx.restore();
  }

  function drawFrontPaws(ctx, seatX, seatY, breath, state, skin, outlineOnly = false) {
    const bd = getBuild(skin);
    const bw = skin.shape.bodyWidth;
    const lie = clamp(state.lieIntensity || 0, 0, 1);
    const lieS = lie * lie * (3.0 - 2.0 * lie);
    const idleTopY = (seatY - bd.idleTopDY) + lieS * 24.0;
    const idleTopX = bd.idleTopX * bw - lieS * 5.0;
    const idleBotX = bd.idleBotX * bw - lieS * 4.5;
    const idleBotY = seatY + 12.0 - lieS * 4.0;
    const shX = bd.shoulderX * bw;

    const approved = !skin.colorpoint && skin.palette.belly && sameColor(skin.palette.belly, skin.palette.legs) && !skin.idleLegsTinted && skin.marks && skin.marks.length > 0;
    const shoulderDepth = approved ? 43 : 58;
    const faceClearance = approved ? 52 : 42;
    const sy = state.bodySquash * (1.0 - lieS * 0.42);
    const sx = (1 + (1 - state.bodySquash) * 0.55) * (1.0 + lieS * 0.03);
    const c = Math.cos(state.bodyTilt), s = Math.sin(state.bodyTilt);
    const faceBottom = 126 + bd.headLift + state.headOffsetY + state.headPitch * 4.8 + breath * 1.1 + faceClearance;
    const shoulder = (x) => ({
      x: seatX + x * sx * c + shoulderDepth * sy * s,
      y: Math.max(seatY + x * sx * s - shoulderDepth * sy * c, faceBottom)
    });

    const lPos = state.leftPaw, rPos = state.rightPaw;
    const lDist = Math.hypot(lPos.x - LEFT_PAW_REST.x, lPos.y - LEFT_PAW_REST.y);
    const rDist = Math.hypot(rPos.x - RIGHT_PAW_REST.x, rPos.y - RIGHT_PAW_REST.y);

    if (lDist < 6.0 && !state.leftPawActive) {
      if (lieS < 0.78) drawIdleFrontLeg(ctx, { x: seatX - idleTopX, y: idleTopY }, { x: seatX - idleBotX, y: idleBotY }, true, skin, outlineOnly);
    } else {
      const ls = shoulder(-shX);
      const mid = { x: (ls.x + lPos.x) * 0.5 - 6, y: (ls.y + lPos.y) * 0.5 - 6 };
      drawLimb(ctx, ls, mid, lPos, bd.limbStartW, bd.limbEndW, false, 0.78, skin, true, outlineOnly);
    }

    if (rDist < 6.0 && !state.rightPawActive) {
      if (lieS < 0.78) drawIdleFrontLeg(ctx, { x: seatX + idleTopX, y: idleTopY }, { x: seatX + idleBotX, y: idleBotY }, false, skin, outlineOnly);
    } else {
      const rs = shoulder(shX);
      const mid = { x: (rs.x + rPos.x) * 0.5 + 4, y: (rs.y + rPos.y) * 0.5 - 6 };
      drawLimb(ctx, rs, mid, rPos, bd.limbStartW, bd.limbEndW, true, 0.78, skin, true, outlineOnly);
    }
  }

  // --- Pixel-Exact Xiao Douni (Video 1 & Video 2) + Native-Face Scruff Drag Helpers ---
  const KEYCAP_BY_ID = new Map(KEYCAPS.map((c) => [c.id, c]));

  const DOUNI_INK = { r: 0.149, g: 0.118, b: 0.094, a: 1 };      // #261E18
  const DOUNI_CREAM = { r: 0.965, g: 0.937, b: 0.902, a: 1 };    // #F6EFE6
  const DOUNI_TAUPE = { r: 0.659, g: 0.576, b: 0.482, a: 1 };    // #A8937B
  const DOUNI_ICE_IRIS = { r: 0.847, g: 0.937, b: 0.918, a: 1 }; // #D8EFEA
  const DOUNI_V2_SCLERA = { r: 0.918, g: 0.922, b: 0.906, a: 1 };// #EAEBE7

  function lerpNum(a, b, t) {
    return a + (b - a) * t;
  }

  function lerpPt(a, b, t) {
    return { x: a.x + (b.x - a.x) * t, y: a.y + (b.y - a.y) * t };
  }

  function smoothstep(edge0, edge1, x) {
    const t = clamp((x - edge0) / Math.max(0.0001, edge1 - edge0), 0, 1);
    return t * t * (3.0 - 2.0 * t);
  }

  function blendRgba(a, b, t) {
    const k = clamp(t, 0, 1);
    return {
      r: a.r + (b.r - a.r) * k,
      g: a.g + (b.g - a.g) * k,
      b: a.b + (b.b - a.b) * k,
      a: (a.a === undefined ? 1 : a.a) + ((b.a === undefined ? 1 : b.a) - (a.a === undefined ? 1 : a.a)) * k
    };
  }

  function cssColorToRgba(str, fallback) {
    if (!str) return fallback || { r: 0.7, g: 0.7, b: 0.7, a: 1 };
    if (typeof str === 'object' && typeof str.r === 'number') return str;
    const s = String(str).trim();
    if (s[0] === '#') {
      const hex = s.slice(1);
      if (hex.length === 6 || hex.length === 8) {
        return {
          r: parseInt(hex.slice(0, 2), 16) / 255,
          g: parseInt(hex.slice(2, 4), 16) / 255,
          b: parseInt(hex.slice(4, 6), 16) / 255,
          a: hex.length === 8 ? parseInt(hex.slice(6, 8), 16) / 255 : 1
        };
      }
    }
    const m = s.match(/rgba?\(\s*([\d.]+)\s*,\s*([\d.]+)\s*,\s*([\d.]+)(?:\s*,\s*([\d.]+))?\s*\)/i);
    if (m) {
      return {
        r: clamp(parseFloat(m[1]) / 255, 0, 1),
        g: clamp(parseFloat(m[2]) / 255, 0, 1),
        b: clamp(parseFloat(m[3]) / 255, 0, 1),
        a: m[4] !== undefined ? clamp(parseFloat(m[4]), 0, 1) : 1
      };
    }
    return fallback || { r: 0.7, g: 0.7, b: 0.7, a: 1 };
  }

  function colorBrightness(c) {
    return Math.max(c.r, c.g, c.b);
  }

  function isClassicBlackRim(skin) {
    const b = skin.palette.body;
    const lum = 0.2126 * b.r + 0.7152 * b.g + 0.0722 * b.b;
    return !skin.colorpoint && !skin.palette.outline && skin.idleLegsTinted === false &&
      (!skin.marks || skin.marks.length === 0) && (!skin.regions || skin.regions.length === 0) && lum < 0.12;
  }

  function v2KbPoint(u, v) {
    const tl = { x: 242, y: 336 };
    const tr = { x: 782, y: 446 };
    const br = { x: 584, y: 586 };
    const bl = { x: 28, y: 440 };
    const topX = tl.x + (tr.x - tl.x) * u;
    const topY = tl.y + (tr.y - tl.y) * u;
    const botX = bl.x + (br.x - bl.x) * u;
    const botY = bl.y + (br.y - bl.y) * u;
    return { x: topX + (botX - topX) * v, y: topY + (botY - topY) * v };
  }

  function v2KeyCellInfo(capOrId) {
    const wellU0 = 0.31, wellU1 = 0.95;
    const wellV0 = 0.11, wellV1 = 0.87;
    const cap = typeof capOrId === 'string' ? KEYCAP_BY_ID.get(capOrId) : capOrId;
    if (cap && cap.isSpace) {
      const su = wellU0 + (wellU1 - wellU0) * (4.0 / 8.0);
      const sv = wellV0 + (wellV1 - wellV0) * (3.5 / 4.0);
      return { row: 3, col: 2, center: v2KbPoint(su, sv), prefersLeftPaw: false };
    }
    if (!cap) {
      const su = wellU0 + (wellU1 - wellU0) * (3.5 / 8.0);
      const sv = wellV0 + (wellV1 - wellV0) * (1.5 / 4.0);
      return { row: 1, col: 3, center: v2KbPoint(su, sv), prefersLeftPaw: true };
    }
    const normU = clamp(1.0 - (cap.xUnits + cap.widthUnits * 0.5), 0, 1);
    const col = clamp(Math.floor(normU * 7.99), 0, 7);
    let row = clamp(cap.row >= 3 ? 3 : cap.row, 0, 3);
    if (row === 3 && col >= 2 && col <= 5) row = 2;
    const su = wellU0 + (wellU1 - wellU0) * ((col + 0.5) / 8.0);
    const sv = wellV0 + (wellV1 - wellV0) * ((row + 0.5) / 4.0);
    return { row, col, center: v2KbPoint(su, sv), prefersLeftPaw: normU < 0.54 };
  }

  function drawRadialGlow(ctx, center, radius, rgbaColor, alphaMul) {
    ctx.save();
    const grad = ctx.createRadialGradient(center.x, center.y, 0, center.x, center.y, radius);
    grad.addColorStop(0, rgbaToCss(rgbaColor, alphaMul === undefined ? 1 : alphaMul));
    grad.addColorStop(1, rgbaToCss(rgbaColor, 0));
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(center.x, center.y, radius, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  }

  function drawSoftTaupePatch(ctx, center, rx, ry, color, solidStop) {
    const stop = solidStop === undefined ? 0.72 : solidStop;
    ctx.save();
    ctx.translate(center.x, center.y);
    ctx.scale(rx, ry);
    const grad = ctx.createRadialGradient(0, 0, 0, 0, 0, 1);
    grad.addColorStop(0, rgbaToCss(color, 1));
    grad.addColorStop(stop, rgbaToCss(color, 0.96));
    grad.addColorStop(1, rgbaToCss(color, 0));
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(0, 0, 1.05, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  }

  function drawVerticalFurGradient(ctx, topY, botY, topColor, botColor) {
    const grad = ctx.createLinearGradient(0, topY, 0, botY);
    grad.addColorStop(0, rgbaToCss(topColor));
    grad.addColorStop(1, rgbaToCss(botColor));
    ctx.fillStyle = grad;
    ctx.fillRect(-160, -160, 1320, 1080);
  }

  function v1RotatePoint(pt, pivot, angle) {
    const dx = pt.x - pivot.x, dy = pt.y - pivot.y;
    const c = Math.cos(angle), s = Math.sin(angle);
    return { x: pivot.x + dx * c - dy * s, y: pivot.y + dx * s + dy * c };
  }

  // ONE continuous closed path pixel-traced directly from Video 1's original head + ears contour (maxErr ~ 1.2px on 1000x750)
  function traceV1UnifiedHeadWithEars(ctx, headCX, headCY, headRX, headRY, earTipY, shiftX) {
    const headTopY = headCY - headRY;
    const leftEarOut = { x: headCX - headRX * 0.709, y: headCY - headRY * 0.903 };
    const leftEarTip = { x: 284 + shiftX, y: earTipY };
    const leftEarIn = { x: 339 + shiftX, y: headTopY };
    const rightEarIn = { x: 399 + shiftX, y: headTopY };
    const rightEarTip = { x: 460 + shiftX, y: earTipY };
    const rightEarOut = { x: headCX + headRX * 0.640, y: headCY - headRY * 0.945 };
    const cheekY = headCY - headRY * 0.021;
    const rightCheek = { x: headCX + headRX * 1.000, y: cheekY };
    const chin = { x: headCX, y: headCY + headRY * 1.007 };
    const leftCheek = { x: headCX - headRX * 1.002, y: cheekY };

    ctx.beginPath();
    ctx.moveTo(leftEarOut.x, leftEarOut.y);
    // Left ear outer -> tip -> inner (directly traced to v1_01 ink centerline)
    ctx.bezierCurveTo(
      202 + shiftX, lerpNum(leftEarOut.y, earTipY, 0.345),
      214 + shiftX, lerpNum(leftEarOut.y, earTipY, 0.853),
      leftEarTip.x, leftEarTip.y
    );
    ctx.bezierCurveTo(
      350 + shiftX, lerpNum(leftEarIn.y, earTipY, 0.897),
      329 + shiftX, lerpNum(leftEarIn.y, earTipY, 0.320),
      leftEarIn.x, leftEarIn.y
    );
    // Smooth crown bridge between ears
    ctx.quadraticCurveTo(369 + shiftX, headTopY - 1, rightEarIn.x, rightEarIn.y);
    // Right ear inner -> tip -> outer
    ctx.bezierCurveTo(
      416 + shiftX, lerpNum(rightEarIn.y, earTipY, 0.308),
      403 + shiftX, lerpNum(rightEarIn.y, earTipY, 0.856),
      rightEarTip.x, rightEarTip.y
    );
    ctx.bezierCurveTo(
      529 + shiftX, lerpNum(rightEarOut.y, earTipY, 0.876),
      539 + shiftX, lerpNum(rightEarOut.y, earTipY, 0.344),
      rightEarOut.x, rightEarOut.y
    );
    // Full soft plump mochi cheeks directly traced from Video 1 ink centerline
    ctx.bezierCurveTo(
      headCX + headRX * 0.838, headCY - headRY * 0.818,
      headCX + headRX * 1.020, headCY - headRY * 0.470,
      rightCheek.x, rightCheek.y
    );
    ctx.bezierCurveTo(
      headCX + headRX * 0.976, headCY + headRY * 0.924,
      headCX + headRX * 0.352, headCY + headRY * 1.054,
      chin.x, chin.y
    );
    ctx.bezierCurveTo(
      headCX - headRX * 0.346, headCY + headRY * 1.009,
      headCX - headRX * 0.972, headCY + headRY * 0.927,
      leftCheek.x, leftCheek.y
    );
    ctx.bezierCurveTo(
      headCX - headRX * 1.014, headCY - headRY * 0.419,
      headCX - headRX * 0.866, headCY - headRY * 0.720,
      leftEarOut.x, leftEarOut.y
    );
    ctx.closePath();
  }

  function drawV1EarTufts(ctx, isLeft, baseY, shiftX, ink, lw) {
    ctx.save();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.85;
    ctx.lineCap = 'round';
    ctx.beginPath();
    if (isLeft) {
      ctx.moveTo(256 + shiftX, baseY - 6); ctx.lineTo(248 + shiftX, baseY - 34);
      ctx.moveTo(282 + shiftX, baseY - 7); ctx.lineTo(274 + shiftX, baseY - 48);
      ctx.moveTo(306 + shiftX, baseY - 6); ctx.lineTo(300 + shiftX, baseY - 66);
    } else {
      ctx.moveTo(434 + shiftX, baseY - 6); ctx.lineTo(441 + shiftX, baseY - 66);
      ctx.moveTo(460 + shiftX, baseY - 7); ctx.lineTo(469 + shiftX, baseY - 48);
      ctx.moveTo(486 + shiftX, baseY - 6); ctx.lineTo(500 + shiftX, baseY - 34);
    }
    ctx.stroke();
    ctx.restore();
  }

  function drawV1UnifiedHeadAndEars(ctx, headCX, headCY, headRX, headRY, earTipY, shiftX, cream, taupe, ink, lw, muzzleCenter, muzzleRX, muzzleRY, petBlushAlpha) {
    const headTopY = headCY - headRY;
    // 1. Cream base fill across unified head+ears
    traceV1UnifiedHeadWithEars(ctx, headCX, headCY, headRX, headRY, earTipY, shiftX);
    ctx.fillStyle = rgbaToCss(cream);
    ctx.fill();

    // 2. Clipped interior shading: vertical taupe-to-cream ear gradients + muzzle + optional blush
    ctx.save();
    traceV1UnifiedHeadWithEars(ctx, headCX, headCY, headRX, headRY, earTipY, shiftX);
    ctx.clip();

    const makeEarGrad = (x0) => {
      const g = ctx.createLinearGradient(x0, earTipY, x0, headTopY + 28);
      g.addColorStop(0.0, rgbaToCss(taupe, 1));
      g.addColorStop(0.78, rgbaToCss(taupe, 1));
      g.addColorStop(1.0, rgbaToCss(cream, 0));
      return g;
    };
    ctx.save();
    ctx.beginPath();
    ctx.rect(145 + shiftX, earTipY - 12, 205, (headTopY + 34) - (earTipY - 12));
    ctx.clip();
    ctx.fillStyle = makeEarGrad(260 + shiftX);
    ctx.fillRect(145 + shiftX, earTipY - 12, 205, (headTopY + 34) - (earTipY - 12));
    ctx.restore();

    ctx.save();
    ctx.beginPath();
    ctx.rect(390 + shiftX, earTipY - 12, 205, (headTopY + 34) - (earTipY - 12));
    ctx.clip();
    ctx.fillStyle = makeEarGrad(470 + shiftX);
    ctx.fillRect(390 + shiftX, earTipY - 12, 205, (headTopY + 34) - (earTipY - 12));
    ctx.restore();

    drawSoftTaupePatch(ctx, muzzleCenter, muzzleRX, muzzleRY, taupe, 0.78);

    if (petBlushAlpha > 0.01) {
      ctx.fillStyle = `rgba(255, 143, 153, ${petBlushAlpha.toFixed(3)})`;
      ctx.beginPath();
      ctx.ellipse(158 + shiftX + 37, headCY + 14 + 17, 37, 17, 0, 0, Math.PI * 2);
      ctx.ellipse(514 + shiftX + 37, headCY + 14 + 17, 37, 17, 0, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.restore();

    // 3. Single unbroken bold ink contour around fused head + ears
    traceV1UnifiedHeadWithEars(ctx, headCX, headCY, headRX, headRY, earTipY, shiftX);
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    ctx.stroke();

    // 4. Signature 3-stroke inner ear tufts (|||)
    drawV1EarTufts(ctx, true, headTopY + 12, shiftX, ink, lw);
    drawV1EarTufts(ctx, false, headTopY + 12, shiftX, ink, lw);
  }

  function drawComicHeart(ctx, center, size, alpha, ink) {
    if (alpha <= 0.02) return;
    ctx.save();
    ctx.globalAlpha *= alpha;
    ctx.translate(center.x, center.y);
    ctx.beginPath();
    ctx.moveTo(0, size * 0.65);
    ctx.bezierCurveTo(-size * 0.65, size * 0.35, -size, size * 0.15, -size, -size * 0.15);
    ctx.bezierCurveTo(-size, -size * 0.78, -size * 0.25, -size * 0.82, 0, -size * 0.38);
    ctx.bezierCurveTo(size * 0.25, -size * 0.82, size, -size * 0.78, size, -size * 0.15);
    ctx.bezierCurveTo(size, size * 0.15, size * 0.65, size * 0.35, 0, size * 0.65);
    ctx.closePath();
    ctx.fillStyle = '#FF708F';
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = 6.5;
    ctx.lineJoin = 'round';
    ctx.stroke();
    ctx.restore();
  }

  function drawComicZ(ctx, center, size, alpha, ink) {
    if (alpha <= 0.02) return;
    ctx.save();
    ctx.globalAlpha *= alpha;
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = 7.5;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    const r = size * 0.55;
    ctx.beginPath();
    ctx.moveTo(center.x - r, center.y - r);
    ctx.lineTo(center.x + r, center.y - r);
    ctx.lineTo(center.x - r, center.y + r);
    ctx.lineTo(center.x + r, center.y + r);
    ctx.stroke();
    ctx.restore();
  }

  // V1Space (1000 x 750) Pixel-Exact Video 1 Rest / Pet / Sleep Flat Mochi Pancake Loaf
  function drawDouniV1LoafInSpace(ctx, state, skin) {
    const ink = skin.palette.outline || DOUNI_INK;
    const cream = skin.palette.body || DOUNI_CREAM;
    const taupe = skin.palette.points || skin.palette.paws || DOUNI_TAUPE;
    const lw = 13.2;

    const isPet = state.petHead > 0.12 || state.petBody > 0.22;
    const isSleep = state.sleepiness > 0.45 && !isPet;
    const petAmt = isPet ? Math.max(state.petHead, state.petBody) : 0;
    const breathWave = Math.sin(state.time * (isSleep ? 1.7 : 2.3));
    const squishY = 1.0 - petAmt * 0.065 + breathWave * (isSleep ? 0.028 : 0.015);
    const squishX = 1.0 + petAmt * 0.032 - breathWave * (isSleep ? 0.014 : 0.008);
    const purrShiftX = isPet ? Math.sin(state.time * 13.0) * 4.5 * petAmt : 0;

    const loafSpacePerCanvas = 1 / 0.345;
    const loafSpaceAnchors = {
      tailTip: { origin: { x: 845, y: 525 }, rot: 0, kx: 1.0, ky: 1.0, offsetScaleX: 1, offsetScaleY: 1 },
      neck: { origin: { x: 380, y: 616 }, rot: 0, kx: 1.32, ky: 1.05, offsetScaleX: 1.2, offsetScaleY: 1.0 },
      headTop: { origin: { x: 370, y: 328 }, rot: 0, kx: 1.25, ky: 1.18, offsetScaleX: 1.25, offsetScaleY: 1.18 },
      face: { origin: { x: 374, y: 468 + 6 * loafSpacePerCanvas }, rot: 0, kx: 1.48, ky: 1.18, offsetScaleX: 1.4, offsetScaleY: 1.15 }
    };

    ctx.save();
    ctx.translate(440 + purrShiftX, 616);
    ctx.scale(squishX, squishY);
    ctx.translate(-440, -616);
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    // 0. Tail-tip decorations behind loaf body
    drawDecorationsInSpace(ctx, skin, ['tailTip'], loafSpaceAnchors, loafSpacePerCanvas);

    // 1. Far back paw peeking right
    ctx.beginPath();
    ctx.moveTo(786, 492);
    ctx.bezierCurveTo(852, 472, 898, 506, 896, 554);
    ctx.bezierCurveTo(894, 590, 866, 602, 830, 598);
    ctx.bezierCurveTo(802, 596, 784, 588, 778, 576);
    ctx.closePath();
    ctx.fillStyle = rgbaToCss(taupe);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();
    ctx.lineWidth = lw * 0.88;
    ctx.beginPath();
    ctx.moveTo(844, 574); ctx.lineTo(855, 594);
    ctx.moveTo(872, 562); ctx.lineTo(884, 580);
    ctx.stroke();

    // 2. Cream loaf back dome behind head
    ctx.beginPath();
    ctx.moveTo(338, 318);
    ctx.bezierCurveTo(400, 282, 486, 272, 562, 274);
    ctx.bezierCurveTo(702, 278, 836, 368, 838, 492);
    ctx.bezierCurveTo(840, 562, 790, 594, 708, 594);
    ctx.lineTo(340, 590);
    ctx.closePath();
    ctx.fillStyle = rgbaToCss(cream);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 3. Left front paw peeking under left cheek
    ctx.beginPath();
    ctx.moveTo(118, 478);
    ctx.bezierCurveTo(54, 474, 8, 506, 8, 554);
    ctx.bezierCurveTo(8, 590, 38, 600, 86, 598);
    ctx.lineTo(158, 594);
    ctx.closePath();
    ctx.fillStyle = rgbaToCss(taupe);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();
    ctx.lineWidth = lw * 0.90;
    ctx.beginPath();
    ctx.moveTo(50, 564); ctx.lineTo(50, 596);
    ctx.moveTo(90, 562); ctx.lineTo(90, 596);
    ctx.stroke();

    // 3b. Neck decorations behind head & right paw
    drawDecorationsInSpace(ctx, skin, ['neck'], loafSpaceAnchors, loafSpacePerCanvas);

    // 4-6. Unified Seamless Head + Ears Silhouette
    drawV1UnifiedHeadAndEars(
      ctx, 380, 472, 292, 144, 165, 0,
      cream, taupe, ink, lw,
      { x: 376, y: 474 }, 98, 78,
      isPet ? 0.34 * petAmt : 0
    );

    // 7. Bold dark curved closed-eye lines (︶ ︶)
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 1.10;
    ctx.beginPath();
    ctx.moveTo(174, 452);
    ctx.quadraticCurveTo(264, 453, 334, 488);
    ctx.moveTo(410, 490);
    ctx.quadraticCurveTo(486, 456, 576, 462);
    ctx.stroke();

    // 7b. Face & head-top decorations above eyes, behind front right paw
    drawDecorationsInSpace(ctx, skin, ['face', 'headTop'], loafSpaceAnchors, loafSpacePerCanvas);

    // 8. Right front paw IN FRONT of lower-right cheek
    ctx.beginPath();
    ctx.moveTo(542, 586);
    ctx.bezierCurveTo(540, 534, 576, 506, 632, 508);
    ctx.bezierCurveTo(688, 508, 724, 538, 724, 588);
    ctx.bezierCurveTo(724, 626, 682, 636, 632, 635);
    ctx.bezierCurveTo(578, 634, 544, 622, 542, 586);
    ctx.closePath();
    ctx.fillStyle = rgbaToCss(taupe);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();
    ctx.lineWidth = lw * 0.90;
    ctx.beginPath();
    ctx.moveTo(591, 596); ctx.lineTo(591, 632);
    ctx.moveTo(636, 598); ctx.lineTo(636, 633);
    ctx.stroke();

    // 9. Whiskers
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.95;
    ctx.beginPath();
    ctx.moveTo(69, 259); ctx.lineTo(110, 326);
    ctx.moveTo(46, 307); ctx.lineTo(89, 341);
    ctx.moveTo(614, 318); ctx.lineTo(636, 286);
    ctx.moveTo(642, 328); ctx.lineTo(665, 298);
    ctx.stroke();

    ctx.restore();

    // 10. Floating comic petting heart or sleeping Zzz
    if (isPet) {
      const phase = (state.time * 1.4) % 1;
      const hy = 152 - phase * 62;
      const hx = 684 + Math.sin(state.time * 3.2) * 16;
      const alpha = Math.min(1, (1 - phase) * 1.6) * Math.min(1, petAmt * 1.4);
      drawComicHeart(ctx, { x: hx, y: hy }, 32, alpha, ink);
    } else if (isSleep) {
      for (let i = 0; i < 2; i++) {
        const p = (state.time * 0.45 + i * 0.5) % 1;
        const zx = 668 + p * 74 + Math.sin(p * 5.0) * 12;
        const zy = 218 - p * 112;
        const alpha = Math.sin(p * Math.PI) * Math.min(1, state.sleepiness * 1.3);
        drawComicZ(ctx, { x: zx, y: zy }, 24 + p * 18, alpha, ink);
      }
    }
    return { loafSpaceAnchors, squishX, squishY, purrShiftX };
  }

  function drawDouniV1LoafInCanvas(ctx, state, skin, loafMorph) {
    const m = clamp(loafMorph === undefined ? 1.0 : loafMorph, 0, 1);
    const alpha = smoothstep(0.36, 0.56, m);
    if (alpha <= 0.01) return null;
    const arrive = smoothstep(0.30, 0.64, m);
    const settlePhase = clamp((m - 0.52) / 0.48, 0, 1);
    const bounce = Math.sin(settlePhase * Math.PI) * 0.08 * (1.0 - settlePhase * 0.35);

    ctx.save();
    ctx.globalAlpha *= alpha;
    const scale = 0.345;
    const tx = 20.0 + state.bodyShiftX + (1.0 - arrive) * 26.0;
    const ty = 46.0 - (1.0 - arrive) * 14.0;
    const pivotX = tx + 440 * scale;
    const pivotY = ty + 616 * scale;
    const sx = 0.94 + 0.06 * arrive + bounce;
    const sy = 1.08 - 0.08 * arrive - bounce * 1.15;
    ctx.translate(pivotX, pivotY);
    ctx.scale(sx, sy);
    ctx.translate(-pivotX, -pivotY);
    ctx.translate(tx, ty);
    ctx.scale(scale, scale);
    const meta = drawDouniV1LoafInSpace(ctx, state, skin);
    ctx.restore();

    const decoAnchors = {};
    for (const key of Object.keys(meta.loafSpaceAnchors)) {
      const sp = meta.loafSpaceAnchors[key];
      const vx = 440 + meta.purrShiftX + (sp.origin.x - 440) * meta.squishX;
      const vy = 616 + (sp.origin.y - 616) * meta.squishY;
      const cx0 = tx + vx * scale;
      const cy0 = ty + vy * scale;
      decoAnchors[key] = {
        origin: {
          x: pivotX + (cx0 - pivotX) * sx,
          y: pivotY + (cy0 - pivotY) * sy
        },
        rot: sp.rot,
        kx: sp.kx * meta.squishX * sx,
        ky: sp.ky * meta.squishY * sy,
        offsetScaleX: sp.offsetScaleX * meta.squishX * sx,
        offsetScaleY: sp.offsetScaleY * meta.squishY * sy
      };
    }
    return decoAnchors;
  }

  function drawV1WateryEye(ctx, center, rx, ry, isLeft, iris, ink, lw, startle) {
    const s = startle;
    const lidY = center.y - ry - 6 + s * (ry * 0.80);

    ctx.save();
    if (s > 0.02) {
      ctx.beginPath();
      ctx.rect(center.x - rx - 14, lidY, rx * 2 + 28, ry * 2 + 28);
      ctx.clip();
    }

    // 1. Watery ice-blue iris
    ctx.fillStyle = rgbaToCss(iris);
    ctx.beginPath();
    ctx.ellipse(center.x, center.y, rx, ry, 0, 0, Math.PI * 2);
    ctx.fill();

    // 2. Giant glossy black pupil + 2 pure-white catchlights
    ctx.save();
    ctx.beginPath();
    ctx.ellipse(center.x, center.y, rx, ry, 0, 0, Math.PI * 2);
    ctx.clip();
    const pRX = rx * 0.86;
    const pRY = ry * 0.81;
    const pCX = center.x + (isLeft ? 4 : -2);
    const pCY = center.y - ry * 0.14 + s * 10;
    ctx.fillStyle = '#000000';
    ctx.beginPath();
    ctx.ellipse(pCX, pCY, pRX, pRY, 0, 0, Math.PI * 2);
    ctx.fill();

    ctx.fillStyle = '#FFFFFF';
    const bigDX = isLeft ? 19 : -21;
    const bigDY = lerpNum(-34, -4, s);
    const bigRX = isLeft ? 26 : 28;
    const bigRY = isLeft ? 21 : 22;
    ctx.beginPath();
    ctx.ellipse(center.x + bigDX, center.y + bigDY, bigRX, bigRY, 0, 0, Math.PI * 2);
    ctx.fill();

    const smDX = isLeft ? 28 : -8;
    const smDY = lerpNum(24, 34, s);
    ctx.beginPath();
    ctx.arc(center.x + smDX, center.y + smDY, 11, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();

    // 3. Outer eye ink stroke
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.94;
    ctx.beginPath();
    ctx.ellipse(center.x, center.y, rx, ry, 0, 0, Math.PI * 2);
    ctx.stroke();
    ctx.restore();

    // 4. Flat horizontal upper eyelid line when startled
    if (s > 0.04) {
      const dy = lidY - center.y;
      const ratio = clamp(dy / ry, -0.98, 0.98);
      const halfSpan = rx * Math.sqrt(Math.max(0.04, 1 - ratio * ratio)) + 3;
      ctx.save();
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = lw * 0.96;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(center.x - halfSpan, lidY);
      ctx.lineTo(center.x + halfSpan, lidY);
      ctx.stroke();
      ctx.restore();
    }
  }

  function drawV1DragLimb(ctx, traceOpen, topColor, pawColor, gradTopY, gradBotY, ink, lw, toes) {
    ctx.save();
    traceOpen();
    ctx.closePath();
    ctx.clip();
    drawVerticalFurGradient(ctx, gradTopY, gradBotY, topColor, pawColor);
    ctx.restore();

    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    traceOpen();
    ctx.stroke();

    if (toes && toes.length) {
      ctx.lineWidth = lw * 0.88;
      ctx.beginPath();
      for (const [a, b] of toes) {
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
      }
      ctx.stroke();
    }
  }

  // V1Space (1000 x 750) Xiao Douni Lifted Drag Pose (startle 1 = half-lidded <-> 0 = wide watery eyes + 4 dangling legs + limp tail)
  function drawDouniV1DragInSpace(ctx, skin, startle, swing) {
    const s = clamp(startle, 0, 1);
    const bodyColor = skin.palette.body || DOUNI_CREAM;
    const pointColor = skin.palette.points || skin.palette.paws || DOUNI_TAUPE;
    const legColor = skin.palette.paws || pointColor;
    const tailColor = skin.palette.tail || pointColor;
    const inkColor = skin.palette.outline || DOUNI_INK;
    const irisColor = skin.palette.iris || DOUNI_ICE_IRIS;
    const lw = 13.5;

    const headCX = lerpNum(400, 380, s);
    const headCY = lerpNum(236, 333, s);
    const headRX = 292;
    const headRY = lerpNum(148, 149, s);
    const headTopY = headCY - headRY;
    const earShiftX = headCX - 380;
    const earTipY = headTopY - 164;

    ctx.save();
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    const dragSpacePerCanvas = 1 / 0.330;
    const tPivot = { x: 754, y: 222 };
    const tAng = swing * 1.18 * (1 - s);
    const tPt = (x, y) => v1RotatePoint({ x, y }, tPivot, tAng);
    const dragTailAlpha = s < 0.85 ? (1 - smoothstep(0.45, 0.85, s)) : 0;
    const leftEyePos = { x: lerpNum(273, 268, s), y: lerpNum(223, 324, s) };
    const rightEyePos = { x: lerpNum(510, 501, s), y: lerpNum(222, 324, s) };
    const dragSpaceAnchors = {
      tailTip: {
        origin: tPt(764, 566),
        rot: tAng,
        kx: 1.0,
        ky: 1.0,
        offsetScaleX: 1,
        offsetScaleY: 1,
        alpha: dragTailAlpha
      },
      neck: {
        origin: { x: headCX, y: headCY + headRY + 22 },
        rot: swing * 0.16,
        kx: 1.32,
        ky: 1.12,
        offsetScaleX: 1.25,
        offsetScaleY: 1.1
      },
      headTop: {
        origin: { x: headCX - 10, y: headTopY - 18 },
        rot: 0,
        kx: 1.28,
        ky: 1.22,
        offsetScaleX: 1.28,
        offsetScaleY: 1.22
      },
      face: {
        origin: {
          x: (leftEyePos.x + rightEyePos.x) * 0.5,
          y: (leftEyePos.y + rightEyePos.y) * 0.5 + 6 * dragSpacePerCanvas
        },
        rot: 0,
        kx: 1.80,
        ky: 1.48,
        offsetScaleX: 1.6,
        offsetScaleY: 1.35
      }
    };

    // 1. Limp hanging tail behind right hip
    if (s < 0.85) {
      const traceTailOpen = () => {
        const p0 = tPt(726, 224), c1 = tPt(730, 326), c2 = tPt(722, 444), p1 = tPt(724, 520);
        const c3 = tPt(726, 554), c4 = tPt(744, 568), p2 = tPt(764, 566);
        const c5 = tPt(790, 564), c6 = tPt(804, 540), p3 = tPt(804, 504);
        const c7 = tPt(806, 390), c8 = tPt(796, 286), p4 = tPt(766, 212);
        ctx.beginPath();
        ctx.moveTo(p0.x, p0.y);
        ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
        ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
        ctx.bezierCurveTo(c5.x, c5.y, c6.x, c6.y, p3.x, p3.y);
        ctx.bezierCurveTo(c7.x, c7.y, c8.x, c8.y, p4.x, p4.y);
      };
      ctx.save();
      ctx.globalAlpha *= dragTailAlpha;
      ctx.save();
      traceTailOpen();
      ctx.closePath();
      ctx.clip();
      drawVerticalFurGradient(ctx, 225, 325, bodyColor, tailColor);
      ctx.restore();
      ctx.strokeStyle = rgbaToCss(inkColor);
      ctx.lineWidth = lw;
      traceTailOpen();
      ctx.stroke();
      ctx.restore();
    }

    // 1b. Tail-tip decorations on drooping tail, behind torso & limbs
    drawDecorationsInSpace(ctx, skin, ['tailTip'], dragSpaceAnchors, dragSpacePerCanvas);

    // 2. Back-left inner leg peeking under belly
    const blPivot = { x: lerpNum(590, 368, s), y: lerpNum(404, 524, s) };
    const blAng = swing * 0.9 * (1 - s);
    const blPt = (ax, ay, bx, by) => v1RotatePoint(lerpPt({ x: ax, y: ay }, { x: bx, y: by }, s), blPivot, blAng);
    const blToes = s > 0.85
      ? [[{ x: lerpNum(576, 338, s), y: lerpNum(456, 578, s) }, { x: lerpNum(574, 332, s), y: lerpNum(472, 598, s) }],
         [{ x: lerpNum(594, 360, s), y: lerpNum(458, 580, s) }, { x: lerpNum(592, 354, s), y: lerpNum(472, 600, s) }]]
      : [];
    drawV1DragLimb(ctx, () => {
      const m = blPt(568, 404, 318, 524);
      const c1 = blPt(564, 446, 304, 576), c2 = blPt(572, 472, 318, 602), p1 = blPt(586, 474, 350, 602);
      const c3 = blPt(606, 476, 398, 600), c4 = blPt(616, 448, 428, 564), p2 = blPt(616, 404, 434, 526);
      ctx.beginPath();
      ctx.moveTo(m.x, m.y);
      ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
      ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
    }, bodyColor, legColor, lerpNum(404, 515, s), lerpNum(440, 560, s), inkColor, lw, blToes);

    // 3. Arched back & torso dome
    ctx.beginPath();
    ctx.moveTo(lerpNum(346, 352, s), lerpNum(88, 178, s));
    ctx.bezierCurveTo(lerpNum(418, 420, s), lerpNum(14, 134, s), lerpNum(488, 514, s), lerpNum(-10, 120, s), lerpNum(548, 580, s), lerpNum(-6, 124, s));
    ctx.bezierCurveTo(lerpNum(672, 744, s), lerpNum(6, 172, s), lerpNum(768, 838, s), lerpNum(112, 304, s), lerpNum(765, 836, s), lerpNum(236, 442, s));
    ctx.bezierCurveTo(lerpNum(764, 834, s), lerpNum(348, 526, s), lerpNum(708, 782, s), lerpNum(412, 542, s), lerpNum(624, 720, s), lerpNum(414, 540, s));
    ctx.bezierCurveTo(lerpNum(512, 558, s), lerpNum(418, 536, s), lerpNum(378, 362, s), lerpNum(404, 528, s), lerpNum(282, 276, s), lerpNum(388, 524, s));
    ctx.closePath();
    ctx.fillStyle = rgbaToCss(bodyColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(inkColor);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 4. Back-right chubby hind leg
    const brPivot = { x: lerpNum(710, 810, s), y: lerpNum(345, 470, s) };
    const brAng = swing * 1.0 * (1 - s);
    const brPt = (ax, ay, bx, by) => v1RotatePoint(lerpPt({ x: ax, y: ay }, { x: bx, y: by }, s), brPivot, brAng);
    const brToes = [
      [v1RotatePoint({ x: lerpNum(684, 808, s), y: lerpNum(452, 574, s) }, brPivot, brAng),
       v1RotatePoint({ x: lerpNum(682, 818, s), y: lerpNum(485, 600, s) }, brPivot, brAng)],
      [v1RotatePoint({ x: lerpNum(712, 838, s), y: lerpNum(455, 566, s) }, brPivot, brAng),
       v1RotatePoint({ x: lerpNum(710, 850, s), y: lerpNum(486, 592, s) }, brPivot, brAng)]
    ];
    drawV1DragLimb(ctx, () => {
      const m = brPt(654, 398, 758, 526);
      const c1 = brPt(642, 454, 760, 582), c2 = brPt(668, 490, 786, 602), p1 = brPt(706, 489, 822, 601);
      const c3 = brPt(756, 486, 870, 598), c4 = brPt(780, 424, 882, 568), p2 = brPt(774, 332, 876, 534);
      const c5 = brPt(770, 312, 866, 492), c6 = brPt(762, 302, 848, 474), p3 = brPt(756, 298, 836, 468);
      ctx.beginPath();
      ctx.moveTo(m.x, m.y);
      ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
      ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
      ctx.bezierCurveTo(c5.x, c5.y, c6.x, c6.y, p3.x, p3.y);
    }, bodyColor, legColor, lerpNum(330, 468, s), lerpNum(405, 540, s), inkColor, lw, brToes);

    // 5. Front-left & Front-right dangling legs
    const flPivot = { x: lerpNum(276, 198, s), y: lerpNum(352, 430, s) };
    const flAng = swing * 1.05 * (1 - s);
    const flPt = (ax, ay, bx, by) => v1RotatePoint(lerpPt({ x: ax, y: ay }, { x: bx, y: by }, s), flPivot, flAng);
    const flToes = [
      [v1RotatePoint({ x: lerpNum(272, 108, s), y: lerpNum(516, 566, s) }, flPivot, swing * (1 - s)),
       v1RotatePoint({ x: lerpNum(272, 102, s), y: lerpNum(552, 598, s) }, flPivot, swing * (1 - s))],
      [v1RotatePoint({ x: lerpNum(304, 146, s), y: lerpNum(514, 570, s) }, flPivot, swing * (1 - s)),
       v1RotatePoint({ x: lerpNum(304, 142, s), y: lerpNum(548, 601, s) }, flPivot, swing * (1 - s))]
    ];
    drawV1DragLimb(ctx, () => {
      const m = flPt(208, 356, 146, 428);
      const c1 = flPt(204, 472, 66, 520), c2 = flPt(222, 554, 74, 602), p1 = flPt(288, 554, 138, 601);
      const c3 = flPt(356, 554, 226, 600), c4 = flPt(358, 478, 274, 544), p2 = flPt(348, 376, 282, 484);
      ctx.beginPath();
      ctx.moveTo(m.x, m.y);
      ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
      ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
    }, bodyColor, legColor, lerpNum(354, 432, s), lerpNum(420, 498, s), inkColor, lw, flToes);

    const frPivot = { x: lerpNum(514, 586, s), y: lerpNum(362, 442, s) };
    const frAng = swing * 1.05 * (1 - s);
    const frPt = (ax, ay, bx, by) => v1RotatePoint(lerpPt({ x: ax, y: ay }, { x: bx, y: by }, s), frPivot, frAng);
    const frToes = [
      [v1RotatePoint({ x: lerpNum(478, 598, s), y: lerpNum(532, 592, s) }, frPivot, swing * (1 - s)),
       v1RotatePoint({ x: lerpNum(478, 598, s), y: lerpNum(568, 626, s) }, frPivot, swing * (1 - s))],
      [v1RotatePoint({ x: lerpNum(510, 636, s), y: lerpNum(534, 590, s) }, frPivot, swing * (1 - s)),
       v1RotatePoint({ x: lerpNum(510, 638, s), y: lerpNum(568, 624, s) }, frPivot, swing * (1 - s))]
    ];
    drawV1DragLimb(ctx, () => {
      const m = frPt(458, 382, 498, 472);
      const c1 = frPt(454, 472, 494, 590), c2 = frPt(432, 570, 558, 628), p1 = frPt(498, 571, 616, 626);
      const c3 = frPt(564, 570, 696, 622), c4 = frPt(586, 482, 690, 536), p2 = frPt(578, 372, 656, 448);
      ctx.beginPath();
      ctx.moveTo(m.x, m.y);
      ctx.bezierCurveTo(c1.x, c1.y, c2.x, c2.y, p1.x, p1.y);
      ctx.bezierCurveTo(c3.x, c3.y, c4.x, c4.y, p2.x, p2.y);
    }, bodyColor, legColor, lerpNum(366, 448, s), lerpNum(436, 516, s), inkColor, lw, frToes);

    // 5b. Neck decorations over chest/legs, tucked behind head & chin
    drawDecorationsInSpace(ctx, skin, ['neck'], dragSpaceAnchors, dragSpacePerCanvas);

    // 6-7. Unified Seamless Head + Ears Silhouette
    drawV1UnifiedHeadAndEars(
      ctx, headCX, headCY, headRX, headRY, earTipY, earShiftX,
      bodyColor, pointColor, inkColor, lw,
      { x: headCX - 8, y: headCY + lerpNum(-12, 0, s) }, 88, 78, 0
    );

    // 8. Huge Watery Ice-Blue Anime Boba Eyes (double catchlights <-> flat half-lidded startle)
    const leftEye = leftEyePos;
    const rightEye = rightEyePos;
    const eyeRX = lerpNum(87, 89, s);
    const eyeRY = lerpNum(91, 84, s);
    drawV1WateryEye(ctx, leftEye, eyeRX, eyeRY, true, irisColor, inkColor, lw, s);
    drawV1WateryEye(ctx, rightEye, eyeRX + 2, eyeRY, false, irisColor, inkColor, lw, s);

    // 9. Upper-left whiskers
    ctx.strokeStyle = rgbaToCss(inkColor);
    ctx.lineWidth = lw * 0.92;
    ctx.beginPath();
    if (s < 0.5) {
      ctx.moveTo(84, 18); ctx.lineTo(132, 92);
      ctx.moveTo(62, 74); ctx.lineTo(108, 108);
    } else {
      ctx.moveTo(56, 168); ctx.lineTo(118, 184);
      ctx.moveTo(62, 222); ctx.lineTo(106, 214);
    }
    ctx.stroke();

    // 10. Face & head-top decorations above head & watery eyes
    drawDecorationsInSpace(ctx, skin, ['face', 'headTop'], dragSpaceAnchors, dragSpacePerCanvas);

    ctx.restore();
    return { dragSpaceAnchors, headCX, headCY };
  }

  function drawDouniV1DragInCanvas(ctx, state, skin, lift) {
    const liftS = smoothstep(0, 1, lift);
    let startleTarget;
    if (state.isScruffDragging) {
      const hold = state.scruffHoldTime || 0;
      startleTarget = hold < 0.14 ? 1.0 : Math.max(0, 1.0 - (hold - 0.14) / 0.07);
    } else {
      const drop = state.scruffDropTimer || 0;
      startleTarget = drop > 0.01 ? Math.min(1.0, drop / 0.12) : 1.0;
    }
    const swing = (state.scruffSwing || 0) * 1.15 + Math.sin(state.time * 4.6) * 0.055 * (1 - startleTarget);

    ctx.save();
    ctx.globalAlpha *= smoothstep(0, 0.22, lift);
    const tx = 24.0 + state.bodyShiftX + (state.scruffSwing || 0) * 18.0;
    const ty = 68.0 - liftS * 22.0 + Math.sin(state.time * 3.8) * 2.5 * (1 - startleTarget);
    ctx.translate(tx, ty);
    ctx.scale(0.330, 0.330);
    const meta = drawDouniV1DragInSpace(ctx, skin, startleTarget, swing);
    ctx.restore();

    const decoAnchors = {};
    for (const key of Object.keys(meta.dragSpaceAnchors)) {
      const sp = meta.dragSpaceAnchors[key];
      decoAnchors[key] = {
        origin: { x: tx + sp.origin.x * 0.330, y: ty + sp.origin.y * 0.330 },
        rot: sp.rot,
        kx: sp.kx,
        ky: sp.ky,
        offsetScaleX: sp.offsetScaleX,
        offsetScaleY: sp.offsetScaleY,
        alpha: sp.alpha
      };
    }
    return {
      headCenter: { x: tx + meta.headCX * 0.330, y: ty + meta.headCY * 0.330 },
      decoAnchors
    };
  }

  // V2Space (832 x 720) Pixel-Exact Video 2 Typing / Idle / Mouse Scene
  function drawV2EarTufts(ctx, isLeft, ink, lw) {
    ctx.save();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.72;
    ctx.lineCap = 'round';
    ctx.beginPath();
    if (isLeft) {
      ctx.moveTo(550, 176); ctx.lineTo(544, 158);
      ctx.moveTo(564, 176); ctx.lineTo(558, 146);
      ctx.moveTo(578, 175); ctx.lineTo(572, 134);
    } else {
      ctx.moveTo(632, 182); ctx.lineTo(644, 144);
      ctx.moveTo(646, 184); ctx.lineTo(660, 158);
      ctx.moveTo(660, 186); ctx.lineTo(674, 172);
    }
    ctx.stroke();
    ctx.restore();
  }

  // Unified outer contour pixel-traced directly from Video 2's original head + ears (maxErr ~ 0.8px on 832x720)
  function traceV2UnifiedHeadStroke(ctx, earLagX, earFlare) {
    const flare = earFlare || 0;
    const dxL = earLagX - flare;
    const dxR = earLagX + flare;
    const dyPin = Math.max(0, -flare) * 0.9;
    const neckLeft = { x: 576.0, y: 365.0 };
    const leftCheek = { x: 421.0, y: 275.0 };
    const leftEarOut = { x: 483.5, y: 180.0 };
    const leftEarTip = { x: 573.0 + dxL, y: 62.0 + dyPin };
    const leftEarIn = { x: 596.0, y: 174.0 };
    const rightEarIn = { x: 618.0, y: 178.0 };
    const rightEarTip = { x: 679.5 + dxR, y: 76.0 + dyPin };
    const rightEarOut = { x: 714.0, y: 196.0 };
    const rightCheek = { x: 757.0, y: 280.0 };
    const neckRight = { x: 722.0, y: 350.0 };

    ctx.beginPath();
    ctx.moveTo(neckLeft.x, neckLeft.y);
    ctx.bezierCurveTo(520.3, 369.5, 416.7, 351.4, leftCheek.x, leftCheek.y);
    ctx.bezierCurveTo(423.2, 235.2, 442.5, 192.8, leftEarOut.x, leftEarOut.y);
    ctx.bezierCurveTo(519.3, 147.1, 529.2 + dxL * 0.6, 90.3 + dyPin * 0.6, leftEarTip.x, leftEarTip.y);
    ctx.bezierCurveTo(596.8 + dxL * 0.4, 94.0 + dyPin * 0.5, 592.7, 136.4, leftEarIn.x, leftEarIn.y);
    ctx.quadraticCurveTo(607.0, 176.0, rightEarIn.x, rightEarIn.y);
    ctx.bezierCurveTo(640.1, 146.2, 642.6 + dxR * 0.6, 97.9 + dyPin * 0.6, rightEarTip.x, rightEarTip.y);
    ctx.bezierCurveTo(717.7 + dxR * 0.4, 102.3 + dyPin * 0.5, 709.7, 155.8, rightEarOut.x, rightEarOut.y);
    ctx.bezierCurveTo(738.0, 217.2, 757.4, 246.8, rightCheek.x, rightCheek.y);
    ctx.bezierCurveTo(756.3, 307.2, 745.0, 334.2, neckRight.x, neckRight.y);
  }

  function drawV2UnifiedHeadAndEars(ctx, earLagX, earFlare, cream, taupe, ink, lw, angerBlush) {
    const blush = angerBlush || 0;
    const chin = { x: 589.0, y: 366.0 };
    const neckLeft = { x: 576.0, y: 365.0 };

    const traceClosedFill = () => {
      traceV2UnifiedHeadStroke(ctx, earLagX, earFlare);
      ctx.quadraticCurveTo(660, 366, chin.x, chin.y);
      ctx.lineTo(neckLeft.x, neckLeft.y);
      ctx.closePath();
    };

    // 1. Cream base fill across unified head + ears
    traceClosedFill();
    ctx.fillStyle = rgbaToCss(cream);
    ctx.fill();

    // 2. Clipped ear gradients + nose-bridge taupe smudge + optional furious rosy blush
    ctx.save();
    traceClosedFill();
    ctx.clip();

    const makeGrad = (x0, y0, y1) => {
      const g = ctx.createLinearGradient(x0, y0, x0, y1);
      g.addColorStop(0.0, rgbaToCss(taupe, 1));
      g.addColorStop(0.72, rgbaToCss(taupe, 1));
      g.addColorStop(1.0, rgbaToCss(cream, 0));
      return g;
    };
    ctx.save();
    ctx.beginPath(); ctx.rect(472, 52, 130, 158); ctx.clip();
    ctx.fillStyle = makeGrad(545, 62, 204);
    ctx.fillRect(472, 52, 130, 158);
    ctx.restore();

    ctx.save();
    ctx.beginPath(); ctx.rect(612, 64, 118, 158); ctx.clip();
    ctx.fillStyle = makeGrad(672, 76, 214);
    ctx.fillRect(612, 64, 118, 158);
    ctx.restore();

    drawSoftTaupePatch(ctx, { x: 586, y: 270 }, 52, 48, taupe, 0.72);

    if (blush > 0.08) {
      ctx.fillStyle = `rgba(255, 112, 122, ${Math.min(0.42, blush * 0.42).toFixed(3)})`;
      ctx.beginPath();
      ctx.ellipse(432 + 28, 294 + 15, 28, 15, 0, 0, Math.PI * 2);
      ctx.ellipse(680 + 28, 304 + 15, 28, 15, 0, 0, Math.PI * 2);
      ctx.fill();
    }
    ctx.restore();

    // 3. Single unbroken bold ink contour around fused head + ears
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    traceV2UnifiedHeadStroke(ctx, earLagX, earFlare);
    ctx.stroke();

    // 4. Inner ear tufts (|||) cleanly anchored inside fused ears
    drawV2EarTufts(ctx, true, ink, lw);
    drawV2EarTufts(ctx, false, ink, lw);
  }

  function drawV2BobaEye(ctx, center, r, isLeft, gazeX, gazeY, blink, hunt, dizzy, dizzyPhase, anger, cream, ink, lw) {
    const h = hunt || 0;
    const dz = dizzy || 0;
    const dzPhase = dizzyPhase || 0;
    const ang = anger || 0;

    if (blink > 0.72 && dz < 0.25) {
      ctx.save();
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = lw * 1.05;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(center.x - r * 0.88, center.y + 4);
      ctx.quadraticCurveTo(center.x, center.y + 14, center.x + r * 0.88, center.y + 4);
      ctx.stroke();
      ctx.restore();
      return;
    }
    const openScaleY = dz > 0.28 ? 1.0 : Math.max(0.18, 1.0 - blink * 0.85);
    ctx.save();
    ctx.translate(center.x, center.y);
    ctx.scale(1.0, openScaleY);

    // 1. Soft pale off-white sclera disc
    ctx.fillStyle = rgbaToCss(DOUNI_V2_SCLERA);
    ctx.beginPath();
    ctx.arc(0, 0, r, 0, Math.PI * 2);
    ctx.fill();

    // 2. Interior: Dizzy spinning spiral (@ @) OR Video 2 Boba Pupil (+ Hunting 4-point star glint / Anger fierce dot)
    ctx.save();
    ctx.beginPath();
    ctx.arc(0, 0, r, 0, Math.PI * 2);
    ctx.clip();
    if (dz > 0.28) {
      const dir = isLeft ? 1.0 : -1.0;
      const phase = dzPhase * dir;
      const steps = 44;
      const maxTheta = Math.PI * 4.4;
      const maxR = r * 0.78;
      ctx.beginPath();
      for (let i = 0; i <= steps; i++) {
        const t = i / steps;
        const theta = t * maxTheta + phase;
        const rad = t * maxR;
        const sx = Math.cos(theta) * rad;
        const sy = Math.sin(theta) * rad;
        if (i === 0) ctx.moveTo(sx, sy);
        else ctx.lineTo(sx, sy);
      }
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = lw * 0.72;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      ctx.stroke();
    } else {
      const pr = r * (0.80 + h * 0.11);
      const baseX = lerpNum(-5.5, 0.0, h);
      const baseY = lerpNum(7.5, 0.0, h);
      const px = baseX + gazeX * (7.5 + h * 10.5);
      const py = baseY + gazeY * (5.5 + h * 8.5);
      ctx.fillStyle = '#000000';
      ctx.beginPath();
      ctx.arc(px, py, pr, 0, Math.PI * 2);
      ctx.fill();

      if (h > 0.12) {
        const ha = smoothstep(0.12, 0.65, h);
        ctx.fillStyle = `rgba(255,255,255,${ha.toFixed(3)})`;
        const sx = px - pr * 0.34, sy = py - pr * 0.32;
        const sr = r * (0.28 + h * 0.08);
        ctx.beginPath();
        ctx.moveTo(sx, sy - sr);
        ctx.quadraticCurveTo(sx + sr * 0.18, sy - sr * 0.18, sx + sr, sy);
        ctx.quadraticCurveTo(sx + sr * 0.18, sy + sr * 0.18, sx, sy + sr);
        ctx.quadraticCurveTo(sx - sr * 0.18, sy + sr * 0.18, sx - sr, sy);
        ctx.quadraticCurveTo(sx - sr * 0.18, sy - sr * 0.18, sx, sy - sr);
        ctx.closePath();
        ctx.fill();
        const dotR = r * 0.12;
        ctx.beginPath();
        ctx.arc(px + pr * 0.28, py + pr * 0.26, dotR, 0, Math.PI * 2);
        ctx.fill();
      } else if (ang > 0.18) {
        const fa = smoothstep(0.18, 0.60, ang);
        ctx.fillStyle = `rgba(255,255,255,${(0.85 * fa).toFixed(3)})`;
        const dotR = r * 0.11;
        ctx.beginPath();
        ctx.arc(px + (isLeft ? -11 : 11), py + 6, dotR, 0, Math.PI * 2);
        ctx.fill();
      }
    }
    ctx.restore();

    // 3. Bold ink border around eye circle
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.94;
    ctx.beginPath();
    ctx.arc(0, 0, r, 0, Math.PI * 2);
    ctx.stroke();

    // 4. Cleanly clipped comic slanted upper eyelid + bold ink V-brow when angry
    if (ang > 0.06 && dz < 0.25) {
      const a = smoothstep(0.06, 0.72, ang);
      const tiltAngle = (isLeft ? 1.0 : -1.0) * (0.32 * a);
      const lidY = -r + a * (r * 0.62);
      ctx.save();
      ctx.rotate(tiltAngle);
      ctx.fillStyle = rgbaToCss(cream);
      ctx.fillRect(-r - 16, -r - 22, r * 2 + 32, lidY - (-r - 22));
      const span = r * 1.12;
      const outerLift = -6.0 * a;
      const pLx = -span, pLy = lidY + (isLeft ? outerLift : 2.0);
      const pRx = span, pRy = lidY + (isLeft ? 2.0 : outerLift);
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = lw * 1.12;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(pLx, pLy);
      ctx.quadraticCurveTo(0, lidY + 4.0 * a, pRx, pRy);
      ctx.stroke();
      ctx.restore();
    }

    ctx.restore();
  }

  function drawDouniAngerVein(ctx, center, scale, alpha) {
    if (alpha <= 0.02) return;
    ctx.save();
    ctx.globalAlpha *= alpha;
    ctx.translate(center.x, center.y);
    ctx.scale(scale, scale);
    ctx.strokeStyle = '#F03847';
    ctx.lineWidth = 8.6;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    const d = 9.0, r = 24.0;
    ctx.beginPath();
    for (const [sx, sy] of [[-1, -1], [1, -1], [-1, 1], [1, 1]]) {
      ctx.moveTo(sx * r, sy * d);
      ctx.quadraticCurveTo(sx * d, sy * d, sx * d, sy * r);
    }
    ctx.stroke();
    ctx.restore();
  }

  function drawDouniDizzyStars(ctx, center, phase, dizzy, ink) {
    const a = smoothstep(0.08, 0.40, dizzy);
    if (a <= 0.01) return;
    const rx = 136.0, ry = 32.0;
    ctx.save();
    ctx.globalAlpha *= a;
    for (let i = 0; i < 3; i++) {
      const ang = phase * 0.72 + i * ((Math.PI * 2.0) / 3.0);
      const sx = center.x + Math.cos(ang) * rx;
      const sy = center.y + Math.sin(ang) * ry;
      const sz = 20.0 + Math.sin(ang) * 4.5;
      ctx.beginPath();
      for (let k = 0; k < 8; k++) {
        const ka = k * (Math.PI / 4.0) + phase * 0.4;
        const rad = (k % 2 === 0) ? sz : sz * 0.42;
        const px = sx + Math.cos(ka) * rad;
        const py = sy + Math.sin(ka) * rad;
        if (k === 0) ctx.moveTo(px, py);
        else ctx.lineTo(px, py);
      }
      ctx.closePath();
      ctx.fillStyle = '#FFD63D';
      ctx.fill();
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = 6.5;
      ctx.lineJoin = 'round';
      ctx.stroke();
    }
    ctx.restore();
  }

  function v2TabletPoint(u, v) {
    const tl = { x: 84, y: 184 };
    const tr = { x: 268, y: 222 };
    const br = { x: 192, y: 316 };
    const bl = { x: 14, y: 274 };
    const topX = tl.x + (tr.x - tl.x) * u;
    const topY = tl.y + (tr.y - tl.y) * u;
    const botX = bl.x + (br.x - bl.x) * u;
    const botY = bl.y + (br.y - bl.y) * u;
    return { x: topX + (botX - topX) * v, y: topY + (botY - topY) * v };
  }

  function douniTabletUV(canvasPt) {
    const dx = canvasPt.x - 18.0;
    const dy = canvasPt.y - 235.0;
    const det = 76.0 * 49.0 - (-5.0) * 5.0;
    const u = clamp((dx * 49.0 + dy * 5.0) / det, 0.06, 0.94);
    const v = clamp((-dx * 5.0 + dy * 76.0) / det, 0.06, 0.94);
    return { u, v };
  }

  function drawDouniComicStylus(ctx, length, accentColor, eyeColor, ink, lw, pressed) {
    const halfW = 10.5, coneX = 26.0;
    const tracePen = () => {
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(coneX, -halfW);
      ctx.lineTo(length - 12, -halfW);
      ctx.quadraticCurveTo(length, -halfW, length, 0);
      ctx.quadraticCurveTo(length, halfW, length - 12, halfW);
      ctx.lineTo(coneX, halfW);
      ctx.closePath();
    };
    tracePen();
    ctx.fillStyle = 'rgb(64,61,64)';
    ctx.fill();

    ctx.save();
    tracePen();
    ctx.clip();
    ctx.fillStyle = 'rgb(92,89,94)';
    ctx.fillRect(coneX, -halfW - 2, 34, (halfW + 2) * 2);
    ctx.fillStyle = rgbaToCss(accentColor);
    ctx.fillRect(coneX + 34, -halfW - 2, 10, (halfW + 2) * 2);
    ctx.fillStyle = pressed > 0.12 ? rgbaToCss(eyeColor) : 'rgb(240,240,240)';
    ctx.fillRect(-2, -halfW, 14, halfW * 2);
    ctx.fillStyle = 'rgba(255,255,255,0.26)';
    ctx.fillRect(coneX + 2, -halfW + 2.5, length - coneX - 18, 3.5);
    ctx.restore();

    tracePen();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.72;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    ctx.stroke();

    roundedRectPath(ctx, coneX + 8, -halfW - 3.5, 18, 5.5, 2.5);
    ctx.fillStyle = 'rgb(214,214,214)';
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.45;
    ctx.stroke();
  }

  function drawDouniComicTablet(ctx, state, deckColor, bevelColor, padColor, accentColor, glowColor, eyeColor, ink, lw, showParkedStylus) {
    const tl = v2TabletPoint(0, 0), tr = v2TabletPoint(1, 0), br = v2TabletPoint(1, 1), bl = v2TabletPoint(0, 1);
    const depth = 16.0;
    const penPress = clamp(state.tabletPenPress || 0, 0, 1);

    ctx.save();
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    // 1. Short wavy comic USB cord
    const cordStart = v2TabletPoint(0.36, 0.0);
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.85;
    ctx.beginPath();
    ctx.moveTo(cordStart.x, cordStart.y);
    ctx.bezierCurveTo(cordStart.x - 16, cordStart.y - 42, cordStart.x - 50, cordStart.y - 54, cordStart.x - 74, cordStart.y - 42);
    ctx.stroke();

    // 2. 3D Underside bevel lip
    roundedQuadPath(ctx,
      { x: tl.x, y: tl.y + depth * 0.45 }, { x: tr.x, y: tr.y + depth },
      { x: br.x, y: br.y + depth }, { x: bl.x, y: bl.y + depth * 0.85 }, 18
    );
    ctx.fillStyle = rgbaToCss(bevelColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 3. Top tablet hull
    roundedQuadPath(ctx, tl, tr, br, bl, 18);
    ctx.fillStyle = rgbaToCss(deckColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 4. Recessed active drawing pad
    const tracePad = () => roundedQuadPath(ctx,
      v2TabletPoint(0.23, 0.12), v2TabletPoint(0.93, 0.12),
      v2TabletPoint(0.93, 0.88), v2TabletPoint(0.23, 0.88), 12
    );
    tracePad();
    ctx.fillStyle = rgbaToCss(padColor);
    ctx.fill();

    ctx.save();
    tracePad();
    ctx.clip();
    const padIsDark = colorBrightness(padColor) < 0.30;
    ctx.strokeStyle = padIsDark ? 'rgba(219,219,219,0.48)' : rgbaToCss(ink, 0.45);
    ctx.lineWidth = lw * 0.45;
    ctx.beginPath();
    for (const [cu, cv, du, dv] of [
      [0.28, 0.20, 0.09, 0.14], [0.88, 0.20, -0.09, 0.14],
      [0.28, 0.80, 0.09, -0.14], [0.88, 0.80, -0.09, -0.14]
    ]) {
      const c = v2TabletPoint(cu, cv), h = v2TabletPoint(cu + du, cv), v = v2TabletPoint(cu, cv + dv);
      ctx.moveTo(h.x, h.y); ctx.lineTo(c.x, c.y); ctx.lineTo(v.x, v.y);
    }
    ctx.stroke();

    const trail = state.tabletTrail || [];
    if (trail.length >= 2) {
      for (let i = 1; i < trail.length; i++) {
        const a = trail[i - 1], b = trail[i];
        if (Math.hypot(b.x - a.x, b.y - a.y) > 22.0) continue;
        const life = Math.max(0, 1.0 - b.age / 0.72);
        if (life <= 0.01) continue;
        const uvA = douniTabletUV(a), uvB = douniTabletUV(b);
        const pA = v2TabletPoint(uvA.u, uvA.v), pB = v2TabletPoint(uvB.u, uvB.v);
        ctx.strokeStyle = rgbaToCss(eyeColor, life * 0.92);
        ctx.lineWidth = 4.5 + life * 5.0;
        ctx.beginPath();
        ctx.moveTo(pA.x, pA.y); ctx.lineTo(pB.x, pB.y);
        ctx.stroke();
      }
    }

    if (!showParkedStylus && penPress > 0.08) {
      const uv = douniTabletUV(state.tabletNib || { x: 56, y: 260 });
      const nib = v2TabletPoint(uv.u, uv.v);
      drawRadialGlow(ctx, nib, 24.0 + penPress * 14.0, eyeColor, 0.55 * penPress);
    }
    ctx.restore();

    tracePad();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.72;
    ctx.stroke();

    // 5. Left bezel ExpressKeys + glowing LED dot
    const ledPt = v2TabletPoint(0.125, 0.20);
    const ledCol = penPress > 0.12 ? eyeColor : glowColor;
    drawRadialGlow(ctx, ledPt, 14.0, ledCol, 0.65);
    ctx.fillStyle = rgbaToCss(ledCol);
    ctx.beginPath(); ctx.arc(ledPt.x, ledPt.y, 5.5, 0, Math.PI * 2); ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.42;
    ctx.stroke();

    for (const kv of [0.40, 0.58, 0.76]) {
      roundedQuadPath(ctx,
        v2TabletPoint(0.07, kv - 0.06), v2TabletPoint(0.175, kv - 0.06),
        v2TabletPoint(0.175, kv + 0.06), v2TabletPoint(0.07, kv + 0.06), 4.5
      );
      ctx.fillStyle = rgbaToCss(accentColor);
      ctx.fill();
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = lw * 0.48;
      ctx.stroke();
    }

    // 6. Parked stylus along top bezel
    if (showParkedStylus) {
      const nib = v2TabletPoint(0.88, -0.06), tail = v2TabletPoint(0.14, -0.06);
      const ang = Math.atan2(tail.y - nib.y, tail.x - nib.x);
      const len = Math.hypot(tail.x - nib.x, tail.y - nib.y);
      ctx.save();
      ctx.translate(nib.x, nib.y);
      ctx.rotate(ang);
      drawDouniComicStylus(ctx, len, glowColor, eyeColor, ink, lw, 0);
      ctx.restore();
    }
    ctx.restore();
  }

  function drawDouniComicMouse(ctx, center, pressLeft, pressRight, bodyColor, bevelColor, wheelColor, glowColor, ink, lw) {
    const pressShift = Math.max(pressLeft, pressRight) * 5.0;
    ctx.save();
    // Cute short wavy comic mouse cord trailing up-left
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.88;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(center.x - 42, center.y - 10 + pressShift);
    ctx.bezierCurveTo(center.x - 66, center.y - 36, center.x - 86, center.y - 42, center.x - 102, center.y - 16);
    ctx.stroke();

    ctx.translate(center.x, center.y + pressShift);
    ctx.rotate(-0.34);

    const rx = 46, ry = 33;
    // 3D comic bottom lip
    ctx.fillStyle = rgbaToCss(bevelColor);
    ctx.beginPath();
    ctx.ellipse(0, 10, rx, ry, 0, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // Top mouse shell
    ctx.fillStyle = rgbaToCss(bodyColor);
    ctx.beginPath();
    ctx.ellipse(0, 0, rx, ry, 0, 0, Math.PI * 2);
    ctx.fill();

    if (pressLeft > 0.04 || pressRight > 0.04) {
      ctx.save();
      ctx.beginPath();
      ctx.ellipse(0, 0, rx, ry, 0, 0, Math.PI * 2);
      ctx.clip();
      if (pressLeft > 0.04) {
        ctx.fillStyle = rgbaToCss(glowColor, 0.55 * pressLeft);
        ctx.fillRect(-rx, 0, rx * 0.96, ry);
      }
      if (pressRight > 0.04) {
        ctx.fillStyle = rgbaToCss(glowColor, 0.55 * pressRight);
        ctx.fillRect(-rx, -ry, rx * 0.96, ry);
      }
      ctx.restore();
    }

    // Button seam & scroll wheel
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.72;
    ctx.beginPath();
    ctx.moveTo(-4, -ry + 4);
    ctx.quadraticCurveTo(4, 0, -4, ry - 4);
    ctx.moveTo(-rx + 5, 0);
    ctx.lineTo(-4, 0);
    ctx.stroke();

    roundedRectPath(ctx, -29, -5.5, 17, 11, 5);
    ctx.fillStyle = rgbaToCss(wheelColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.62;
    ctx.stroke();

    // Outer shell outline
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.beginPath();
    ctx.ellipse(0, 0, rx, ry, 0, 0, Math.PI * 2);
    ctx.stroke();
    ctx.restore();
  }

  function drawDouniV2Keyboard(ctx, state, deckColor, bevelColor, keyColor, accentColor, glowColor, ink, lw, impactY, impactTilt) {
    const tl = v2KbPoint(0, 0), tr = v2KbPoint(1, 0), br = v2KbPoint(1, 1), bl = v2KbPoint(0, 1);
    const depth = 28.0;

    // 1. Whipping USB cord
    const cordWhipX = impactTilt * 24.0;
    const cordWhipY = impactY * 12.0;
    ctx.save();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 1.05;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(268 + impactTilt * 8.0, 522 + impactY * 8.0);
    ctx.bezierCurveTo(
      220 + cordWhipX * 0.4, 536 + cordWhipY * 0.5,
      218 + cordWhipX * 0.9, 572 + cordWhipY * 0.8,
      224 + cordWhipX * 0.7, 598 + cordWhipY * 0.6
    );
    ctx.bezierCurveTo(
      230 + cordWhipX * 0.6, 632 + cordWhipY * 0.4,
      186 + cordWhipX * 0.3, 648,
      132 + cordWhipX * 0.35, 646 + cordWhipY * 0.25
    );
    ctx.stroke();
    ctx.restore();

    // Keyboard squash/stretch & impact recoil transform
    ctx.save();
    const kbPivotX = 410, kbPivotY = 584;
    const sx = 1.0 + Math.max(0, impactY) * 0.026;
    const sy = 1.0 - Math.max(0, impactY) * 0.052;
    ctx.translate(kbPivotX + impactTilt * 11.0, kbPivotY + impactY * 9.5);
    ctx.rotate(impactTilt * 0.024);
    ctx.scale(sx, sy);
    ctx.translate(-kbPivotX, -kbPivotY);

    // 2. 3D Underside bevel hull
    roundedQuadPath(ctx,
      { x: tl.x, y: tl.y + depth * 0.45 },
      { x: tr.x, y: tr.y + depth },
      { x: br.x, y: br.y + depth },
      { x: bl.x, y: bl.y + depth * 0.85 },
      28
    );
    ctx.fillStyle = rgbaToCss(bevelColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 3. Top deck hull
    roundedQuadPath(ctx, tl, tr, br, bl, 28);
    ctx.fillStyle = rgbaToCss(deckColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.stroke();

    // 4. Left smooth rectangular pad
    roundedQuadPath(ctx,
      v2KbPoint(0.06, 0.14), v2KbPoint(0.28, 0.14),
      v2KbPoint(0.28, 0.84), v2KbPoint(0.06, 0.84), 14
    );
    ctx.fillStyle = rgbaToCss(keyColor);
    ctx.fill();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.82;
    ctx.stroke();

    // 5. Right 4x8 keycap well with vivid backlit glow & press travel
    const wellU0 = 0.31, wellU1 = 0.95, wellV0 = 0.11, wellV1 = 0.87;
    const traceWell = () => roundedQuadPath(ctx,
      v2KbPoint(wellU0, wellV0), v2KbPoint(wellU1, wellV0),
      v2KbPoint(wellU1, wellV1), v2KbPoint(wellU0, wellV1), 16
    );
    ctx.save();
    traceWell();
    ctx.fillStyle = rgbaToCss(keyColor);
    ctx.fill();
    ctx.clip();

    const cellGlow = [
      [0, 0, 0, 0, 0, 0, 0, 0],
      [0, 0, 0, 0, 0, 0, 0, 0],
      [0, 0, 0, 0, 0, 0, 0, 0],
      [0, 0, 0, 0, 0, 0, 0, 0]
    ];
    let sparkCenter = null;
    for (const [k, g] of Object.entries(state.keyGlow || {})) {
      if (g <= 0.03) continue;
      const info = v2KeyCellInfo(k);
      cellGlow[info.row][info.col] = Math.max(cellGlow[info.row][info.col], g);
      if (k === state.kbSparkKey) sparkCenter = info.center;
    }

    const warmCore = { r: 1.0, g: 0.96, b: 0.72, a: 1 };
    const rows = 4, cols = 8;
    const glowBlooms = [];
    for (let r = 0; r < rows; r++) {
      const rv0 = lerpNum(wellV0, wellV1, r / rows);
      const rv1 = lerpNum(wellV0, wellV1, (r + 1) / rows);
      for (let c = 0; c < cols; c++) {
        if (r === 3 && c > 2 && c <= 5) continue;
        const cEnd = (r === 3 && c === 2) ? 6 : (c + 1);
        const cu0 = lerpNum(wellU0, wellU1, c / cols);
        const cu1 = lerpNum(wellU0, wellU1, cEnd / cols);
        let g = cellGlow[r][c];
        if (r === 3 && c === 2) {
          for (let sc = 2; sc < 6; sc++) g = Math.max(g, cellGlow[3][sc]);
        }
        const isAccent = (r === 0 && c === 0) || (r === 2 && c === 7);
        if (isAccent || g > 0.03) {
          const pTL = v2KbPoint(cu0, rv0), pTR = v2KbPoint(cu1, rv0);
          const pBR = v2KbPoint(cu1, rv1), pBL = v2KbPoint(cu0, rv1);
          ctx.beginPath();
          ctx.moveTo(pTL.x, pTL.y); ctx.lineTo(pTR.x, pTR.y);
          ctx.lineTo(pBR.x, pBR.y); ctx.lineTo(pBL.x, pBL.y);
          ctx.closePath();
          if (g > 0.03) {
            const vivid = blendRgba(glowColor, warmCore, Math.min(0.55, g * 0.55));
            const fill = blendRgba(isAccent ? accentColor : keyColor, vivid, Math.min(1, 0.45 + g * 0.55));
            ctx.fillStyle = rgbaToCss(fill);
            ctx.fill();
            // Subtle inner top press shadow
            const pInL = lerpPt(pTL, pBL, 0.24), pInR = lerpPt(pTR, pBR, 0.24);
            ctx.beginPath();
            ctx.moveTo(pTL.x, pTL.y); ctx.lineTo(pTR.x, pTR.y);
            ctx.lineTo(pInR.x, pInR.y); ctx.lineTo(pInL.x, pInL.y);
            ctx.closePath();
            ctx.fillStyle = `rgba(0,0,0,${(0.16 * g).toFixed(3)})`;
            ctx.fill();
            glowBlooms.push([
              { x: (pTL.x + pTR.x + pBR.x + pBL.x) * 0.25, y: (pTL.y + pTR.y + pBR.y + pBL.y) * 0.25 },
              g
            ]);
          } else {
            ctx.fillStyle = rgbaToCss(accentColor);
            ctx.fill();
          }
        }
      }
    }

    // Crisp hand-drawn grid lines inside well
    const keyIsDark = colorBrightness(keyColor) < 0.28;
    const gridInk = keyIsDark ? { r: 0.34, g: 0.36, b: 0.41, a: 0.92 } : ink;
    ctx.strokeStyle = rgbaToCss(gridInk);
    ctx.lineWidth = lw * 0.78;
    ctx.beginPath();
    for (let r = 1; r < rows; r++) {
      const rv = lerpNum(wellV0, wellV1, r / rows);
      const a = v2KbPoint(wellU0, rv), b = v2KbPoint(wellU1, rv);
      ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y);
    }
    for (let c = 1; c < cols; c++) {
      const cu = lerpNum(wellU0, wellU1, c / cols);
      const stopV = (c >= 3 && c <= 5) ? lerpNum(wellV0, wellV1, 3 / 4) : wellV1;
      const a = v2KbPoint(cu, wellV0), b = v2KbPoint(cu, stopV);
      ctx.moveTo(a.x, a.y); ctx.lineTo(b.x, b.y);
    }
    ctx.stroke();
    ctx.restore();

    // Bold outer border of keycap well
    traceWell();
    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw * 0.84;
    ctx.stroke();

    // Soft radiant backlit bloom halos
    for (const [ctr, g] of glowBlooms) {
      drawRadialGlow(ctx, ctr, 56.0 + g * 18.0, glowColor, 0.62 * g);
    }

    // Comic impact sparks (\ | /) popping crisply for 0.14s on slap contact
    if ((state.kbSparkTimer || 0) > 0.005 && sparkCenter) {
      const t = clamp(state.kbSparkTimer / 0.14, 0, 1);
      const spread = 14.0 + (1.0 - t) * 26.0;
      const len = 14.0 * t + 6.0;
      ctx.save();
      ctx.globalAlpha *= Math.min(1, t * 1.4);
      ctx.lineCap = 'round';
      for (const ang of [-2.35, -1.55, -0.75]) {
        const p0x = sparkCenter.x + Math.cos(ang) * spread;
        const p0y = sparkCenter.y + Math.sin(ang) * spread - 6;
        const p1x = sparkCenter.x + Math.cos(ang) * (spread + len);
        const p1y = sparkCenter.y + Math.sin(ang) * (spread + len) - 6;
        ctx.strokeStyle = rgbaToCss(ink);
        ctx.lineWidth = 9.5;
        ctx.beginPath(); ctx.moveTo(p0x, p0y); ctx.lineTo(p1x, p1y); ctx.stroke();
        ctx.strokeStyle = rgbaToCss(glowColor);
        ctx.lineWidth = 4.5;
        ctx.beginPath(); ctx.moveTo(p0x, p0y); ctx.lineTo(p1x, p1y); ctx.stroke();
      }
      ctx.restore();
    }

    ctx.restore(); // end keyboard squash/shake transform
  }

  function v2ChubbySlamArm(shoulder, keyPoint) {
    const target = { x: keyPoint.x, y: keyPoint.y - 4 };
    const dx = target.x - shoulder.x, dy = target.y - shoulder.y;
    const dist = Math.max(56.0, Math.hypot(dx, dy));
    const ux = dx / dist, uy = dy / dist;
    let nx = -uy, ny = ux;
    if (ny > 0) { nx = -nx; ny = -ny; }

    const rS = 34.0, rP = 33.0;
    const archLift = 14.0 * Math.abs(ux);
    const shTop = { x: shoulder.x + nx * rS, y: shoulder.y + ny * rS };
    const shBot = { x: shoulder.x - nx * rS, y: shoulder.y - ny * rS };
    const tip = { x: target.x + ux * 16.0, y: target.y + uy * 16.0 };

    const topC1 = { x: lerpNum(shTop.x, target.x + nx * rP, 0.42), y: lerpNum(shTop.y, target.y + ny * rP, 0.42) - archLift };
    const topC2 = { x: target.x + nx * (rP * 1.15) + ux * 8.0, y: target.y + ny * (rP * 1.15) + uy * 8.0 };
    const botC1 = { x: target.x - nx * (rP * 1.15) + ux * 8.0, y: target.y - ny * (rP * 1.15) + uy * 8.0 };
    const botC2 = { x: lerpNum(shBot.x, target.x - nx * rP, 0.42), y: lerpNum(shBot.y, target.y - ny * rP, 0.42) - archLift * 0.4 };

    const tBase = { x: target.x - ux * 10.0, y: target.y - uy * 10.0 };
    const tLen = 24.0;
    const toe1A = { x: tBase.x + nx * 10.0, y: tBase.y + ny * 10.0 };
    const toe1B = { x: toe1A.x + ux * tLen, y: toe1A.y + uy * tLen };
    const toe2A = { x: tBase.x - nx * 12.0, y: tBase.y - ny * 12.0 };
    const toe2B = { x: toe2A.x + ux * tLen, y: toe2A.y + uy * tLen };

    return { shTop, shBot, tip, topC1, topC2, botC1, botC2, toe1A, toe1B, toe2A, toe2B };
  }

  function blendV2Arm(a, b, t) {
    return {
      shTop: lerpPt(a.shTop, b.shTop, t), shBot: lerpPt(a.shBot, b.shBot, t),
      tip: lerpPt(a.tip, b.tip, t),
      topC1: lerpPt(a.topC1, b.topC1, t), topC2: lerpPt(a.topC2, b.topC2, t),
      botC1: lerpPt(a.botC1, b.botC1, t), botC2: lerpPt(a.botC2, b.botC2, t),
      toe1A: lerpPt(a.toe1A, b.toe1A, t), toe1B: lerpPt(a.toe1B, b.toe1B, t),
      toe2A: lerpPt(a.toe2A, b.toe2A, t), toe2B: lerpPt(a.toe2B, b.toe2B, t)
    };
  }

  function drawV2ArmGeometry(ctx, sh, stopMid, stopEnd, cream, taupe, ink, lw) {
    const traceOpen = () => {
      ctx.beginPath();
      ctx.moveTo(sh.shTop.x, sh.shTop.y);
      ctx.bezierCurveTo(sh.topC1.x, sh.topC1.y, sh.topC2.x, sh.topC2.y, sh.tip.x, sh.tip.y);
      ctx.bezierCurveTo(sh.botC1.x, sh.botC1.y, sh.botC2.x, sh.botC2.y, sh.shBot.x, sh.shBot.y);
    };
    ctx.save();
    traceOpen();
    ctx.closePath();
    ctx.clip();
    const endX = (sh.shTop.x + sh.shBot.x) * 0.5;
    const endY = (sh.shTop.y + sh.shBot.y) * 0.5;
    const g = ctx.createLinearGradient(sh.tip.x, sh.tip.y, endX, endY);
    g.addColorStop(0.0, rgbaToCss(taupe));
    g.addColorStop(stopMid, rgbaToCss(taupe));
    g.addColorStop(stopEnd, rgbaToCss(cream));
    ctx.fillStyle = g;
    ctx.fillRect(-100, -100, 1050, 920);
    ctx.restore();

    ctx.strokeStyle = rgbaToCss(ink);
    ctx.lineWidth = lw;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    traceOpen();
    ctx.stroke();

    ctx.lineWidth = lw * 0.88;
    ctx.beginPath();
    ctx.moveTo(sh.toe1A.x, sh.toe1A.y); ctx.lineTo(sh.toe1B.x, sh.toe1B.y);
    ctx.moveTo(sh.toe2A.x, sh.toe2A.y); ctx.lineTo(sh.toe2B.x, sh.toe2B.y);
    ctx.stroke();
  }

  function drawV2LeftArm(ctx, pose, mouseBlend, mouseTarget, slamTarget, cream, taupe, ink, lw, swingVelocity) {
    const poseA = {
      shTop: { x: 416, y: 312 }, shBot: { x: 416, y: 374 },
      tip: { x: 242, y: 346 },
      topC1: { x: 344, y: 288 }, topC2: { x: 260, y: 298 },
      botC1: { x: 242, y: 392 }, botC2: { x: 332, y: 386 },
      toe1A: { x: 256, y: 356 }, toe1B: { x: 258, y: 380 },
      toe2A: { x: 278, y: 360 }, toe2B: { x: 280, y: 384 }
    };
    const poseB = slamTarget
      ? v2ChubbySlamArm({ x: 426, y: 350 }, slamTarget)
      : {
          shTop: { x: 422, y: 326 }, shBot: { x: 432, y: 392 },
          tip: { x: 274, y: 412 },
          topC1: { x: 348, y: 318 }, topC2: { x: 284, y: 356 },
          botC1: { x: 272, y: 454 }, botC2: { x: 348, y: 434 },
          toe1A: { x: 288, y: 414 }, toe1B: { x: 292, y: 440 },
          toe2A: { x: 310, y: 416 }, toe2B: { x: 314, y: 442 }
        };
    const poseC = {
      shTop: { x: 422, y: 262 }, shBot: { x: 434, y: 336 },
      tip: { x: 258, y: 248 },
      topC1: { x: 360, y: 222 }, topC2: { x: 286, y: 204 },
      botC1: { x: 254, y: 298 }, botC2: { x: 352, y: 312 },
      toe1A: { x: 274, y: 246 }, toe1B: { x: 274, y: 278 },
      toe2A: { x: 296, y: 254 }, toe2B: { x: 296, y: 286 }
    };
    const poseM = {
      shTop: { x: 414, y: 298 }, shBot: { x: 414, y: 356 },
      tip: { x: mouseTarget.x - 16, y: mouseTarget.y - 2 },
      topC1: { x: 330, y: 274 }, topC2: { x: mouseTarget.x + 28, y: mouseTarget.y - 36 },
      botC1: { x: mouseTarget.x - 12, y: mouseTarget.y + 38 }, botC2: { x: 312, y: 358 },
      toe1A: { x: mouseTarget.x - 2, y: mouseTarget.y + 6 }, toe1B: { x: mouseTarget.x - 2, y: mouseTarget.y + 28 },
      toe2A: { x: mouseTarget.x + 18, y: mouseTarget.y + 8 }, toe2B: { x: mouseTarget.x + 18, y: mouseTarget.y + 30 }
    };

    const typingShape = pose >= 0
      ? blendV2Arm(poseA, poseC, Math.min(1.05, pose))
      : blendV2Arm(poseA, poseB, Math.min(1.05, -pose));
    const sh = mouseBlend > 0.01 ? blendV2Arm(typingShape, poseM, mouseBlend) : typingShape;

    drawV2ArmGeometry(ctx, sh, 0.42, 0.80, cream, taupe, ink, lw);

    const spd = Math.abs(swingVelocity || 0);
    if (spd > 4.2 && mouseBlend < 0.2) {
      const a = Math.min(0.45, (spd - 4.2) / 10.0);
      const dir = swingVelocity > 0 ? 1 : -1;
      ctx.save();
      ctx.strokeStyle = rgbaToCss(ink, a);
      ctx.lineWidth = lw * 0.55;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(sh.tip.x - 16, sh.tip.y + dir * 22);
      ctx.quadraticCurveTo(sh.tip.x - 24, sh.tip.y + dir * 48, sh.tip.x + 14, sh.tip.y + dir * 68);
      ctx.moveTo(sh.tip.x + 6, sh.tip.y + dir * 28);
      ctx.quadraticCurveTo(sh.tip.x + 2, sh.tip.y + dir * 50, sh.tip.x + 34, sh.tip.y + dir * 64);
      ctx.stroke();
      ctx.restore();
    }
  }

  function drawV2RightArm(ctx, pose, slamTarget, cream, taupe, ink, lw, swingVelocity) {
    const poseA = {
      shTop: { x: 616, y: 344 }, shBot: { x: 618, y: 406 },
      tip: { x: 410, y: 368 },
      topC1: { x: 544, y: 306 }, topC2: { x: 432, y: 306 },
      botC1: { x: 406, y: 414 }, botC2: { x: 522, y: 414 },
      toe1A: { x: 426, y: 374 }, toe1B: { x: 426, y: 402 },
      toe2A: { x: 452, y: 378 }, toe2B: { x: 452, y: 404 }
    };
    const poseB = {
      shTop: { x: 626, y: 344 }, shBot: { x: 608, y: 382 },
      tip: { x: 422, y: 294 },
      topC1: { x: 564, y: 278 }, topC2: { x: 458, y: 226 },
      botC1: { x: 416, y: 344 }, botC2: { x: 518, y: 350 },
      toe1A: { x: 440, y: 296 }, toe1B: { x: 442, y: 332 },
      toe2A: { x: 464, y: 306 }, toe2B: { x: 466, y: 338 }
    };
    const poseC = slamTarget
      ? v2ChubbySlamArm({ x: 604, y: 372 }, slamTarget)
      : {
          shTop: { x: 586, y: 346 }, shBot: { x: 608, y: 404 },
          tip: { x: 406, y: 414 },
          topC1: { x: 498, y: 344 }, topC2: { x: 422, y: 356 },
          botC1: { x: 404, y: 464 }, botC2: { x: 500, y: 446 },
          toe1A: { x: 420, y: 422 }, toe1B: { x: 420, y: 452 },
          toe2A: { x: 446, y: 426 }, toe2B: { x: 446, y: 454 }
        };

    const sh = pose >= 0
      ? blendV2Arm(poseA, poseB, Math.min(1.05, pose))
      : blendV2Arm(poseA, poseC, Math.min(1.05, -pose));

    if (pose > 0.35) {
      ctx.save();
      ctx.fillStyle = rgbaToCss(cream, smoothstep(0.35, 0.85, pose));
      ctx.fillRect(446, 332, 164, 32);
      ctx.restore();
    }

    drawV2ArmGeometry(ctx, sh, 0.45, 0.84, cream, taupe, ink, lw);

    const spd = Math.abs(swingVelocity || 0);
    if (spd > 4.2) {
      const a = Math.min(0.45, (spd - 4.2) / 10.0);
      const dir = swingVelocity > 0 ? 1 : -1;
      ctx.save();
      ctx.strokeStyle = rgbaToCss(ink, a);
      ctx.lineWidth = lw * 0.55;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(sh.tip.x - 14, sh.tip.y + dir * 24);
      ctx.quadraticCurveTo(sh.tip.x - 20, sh.tip.y + dir * 48, sh.tip.x + 12, sh.tip.y + dir * 68);
      ctx.stroke();
      ctx.restore();
    }
  }

  function drawDouniV2InSpace(ctx, state, skin, kbThemeName, eyeThemeName, loafMorph) {
    const ink = skin.palette.outline || DOUNI_INK;
    const cream = skin.palette.body || DOUNI_CREAM;
    const taupe = skin.palette.points || skin.palette.paws || DOUNI_TAUPE;
    const catLW = 10.8;
    const kbLW = 12.6;
    const morph = clamp(loafMorph || 0, 0, 1);

    // Direct independent signed arm poses: -1 = slam on keycap, 0 = ready, +1 = flung high
    const leftPose = clamp(state.douniLeftRaise || 0, -1.08, 1.08);
    const rightPose = clamp(state.douniRightRaise || 0, -1.08, 1.08);
    const mouseBlend = clamp(state.douniMouseBlend || 0, 0, 1);

    const hunt = clamp(state.huntIntensity || 0, 0, 1);
    const dizzy = clamp(state.dizzyIntensity || 0, 0, 1);
    const anger = clamp(state.angerIntensity || 0, 0, 1);
    const isTablet = !!state.isTabletActive;

    const impY = clamp(state.kbImpactY || 0, -0.4, 1.3);
    const impT = clamp(state.kbImpactTilt || 0, -0.9, 0.9);
    const breath = Math.sin(state.time * 2.1) * 1.6;
    const dizzyWobble = Math.sin(state.time * 9.5) * 0.14 * dizzy;
    const angerShakeX = anger > 0.08 ? Math.sin(state.time * 42.0) * 3.2 * anger : 0;
    const headTiltRad = -rightPose * 0.010 + leftPose * 0.012 + state.headTilt * 0.08 + impT * 0.014 + dizzyWobble;
    const headBobY = Math.abs(leftPose - rightPose) * 2.5 + breath * 1.4 + impY * 5.2 +
      hunt * 16.0 + anger * 5.0 + Math.abs(Math.sin(state.time * 4.8)) * 9.0 * dizzy;
    const headSquashY = 1.0 - Math.max(0, impY) * 0.028 - hunt * 0.062 - anger * 0.022;
    const headSquashX = 1.0 + Math.max(0, impY) * 0.014 + hunt * 0.045 + anger * 0.018;
    const earLagX = clamp((state.earTwitch || 0) * 2.8, -9, 9);
    const earFlare = hunt * 9.0 - anger * 10.0;

    const useDefaultRetroGrey = !kbThemeName || kbThemeName === 'classicMilk';
    const th = KB_THEMES[kbThemeName] || KB_THEMES.classicMilk;
    const eye = EYE_THEMES[eyeThemeName] || EYE_THEMES.neonChartreuse;
    const eyePrimary = { r: eye.primary[0] / 255, g: eye.primary[1] / 255, b: eye.primary[2] / 255, a: 1 };
    const kbDeckColor = useDefaultRetroGrey ? { r: 0.725, g: 0.733, b: 0.733, a: 1 } : cssColorToRgba(th.deck);
    const kbBevelColor = useDefaultRetroGrey ? { r: 0.608, g: 0.616, b: 0.612, a: 1 } : cssColorToRgba(th.base);
    const kbKeyColor = useDefaultRetroGrey ? { r: 0.698, g: 0.706, b: 0.702, a: 1 } : cssColorToRgba(th.key);
    const kbAccentColor = useDefaultRetroGrey ? { r: 0.658, g: 0.666, b: 0.662, a: 1 } : cssColorToRgba(th.accent);
    const mouseBodyColor = useDefaultRetroGrey ? { r: 0.745, g: 0.753, b: 0.753, a: 1 } : cssColorToRgba(th.mouseBody);
    const mouseWheelColor = useDefaultRetroGrey ? { r: 0.36, g: 0.35, b: 0.34, a: 1 } : cssColorToRgba(th.wheel);
    const baseGlow = useDefaultRetroGrey
      ? { r: 1.0, g: 0.84, b: 0.24, a: 1 }
      : { r: eye.glow[0] / 255, g: eye.glow[1] / 255, b: eye.glow[2] / 255, a: 1 };
    const glowBaseColor = anger > 0.05
      ? blendRgba(baseGlow, { r: 1.0, g: 0.34, b: 0.14, a: 1 }, Math.min(0.88, anger * 0.88))
      : baseGlow;

    // Choreographed Disney Pop-Squish Morph (zero muddy double ghosting)
    const gearAlpha = 1.0 - smoothstep(0.0, 0.42, morph);
    const gearDropY = smoothstep(0.0, 0.46, morph) * 86.0;
    const catAlpha = 1.0 - smoothstep(0.36, 0.56, morph);
    const catGlide = smoothstep(0.0, 0.54, morph);
    const catMorphX = -catGlide * 185.0;
    const catMorphY = catGlide * 145.0;
    const catMorphSX = 1.0 + catGlide * 0.22;
    const catMorphSY = 1.0 - catGlide * 0.28;

    const mouseCenter = {
      x: 168 + (state.mouseOffsetX || 0) * 5.2,
      y: 286 + (state.mouseOffsetY || 0) * 4.5
    };
    const pressL = Math.max(state.mousePressLeft || 0, state.mousePress || 0);
    const pressR = state.mousePressRight || 0;
    const mousePress = Math.max(pressL, pressR);
    const nibUV = douniTabletUV(state.tabletNib || { x: 56, y: 260 });
    const v2Nib = v2TabletPoint(nibUV.u, nibUV.v);
    const tabletPress = clamp(state.tabletPenPress || 0, 0, 1);

    const v2SpacePerCanvas = 1 / 0.405;
    const v2SpaceAnchors = {
      tailTip: { origin: { x: 758, y: 408 }, rot: 0, kx: 1.0, ky: 1.0, offsetScaleX: 1, offsetScaleY: 1 },
      neck: { origin: { x: 582, y: 374 }, rot: 0.05, kx: 1.26, ky: 1.12, offsetScaleX: 1.2, offsetScaleY: 1.1 },
      headTop: { origin: { x: 597, y: 164 }, rot: 0.06, kx: 1.18, ky: 1.15, offsetScaleX: 1.18, offsetScaleY: 1.15 },
      face: { origin: { x: 585, y: 268 + 6 * v2SpacePerCanvas }, rot: 0.089, kx: 1.38, ky: 1.24, offsetScaleX: 1.3, offsetScaleY: 1.2 }
    };

    ctx.save();
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    // 1. Companion hand-drawn comic Mouse OR Graphics Tablet
    if (gearAlpha > 0.01) {
      ctx.save();
      if (morph > 0.005) {
        ctx.globalAlpha *= gearAlpha;
        ctx.translate(0, gearDropY);
      }
      if (isTablet) {
        drawDouniComicTablet(
          ctx, state, kbDeckColor, kbBevelColor, kbKeyColor, kbAccentColor,
          glowBaseColor, eyePrimary, ink, catLW, mouseBlend < 0.18
        );
      } else {
        drawDouniComicMouse(ctx, mouseCenter, pressL, pressR, mouseBodyColor, kbBevelColor, mouseWheelColor, glowBaseColor, ink, catLW);
      }
      ctx.restore();
    }

    // 2. Cat torso, soft rounded unified head+ears, expressive boba eyes & comic emotion accents
    if (catAlpha > 0.01) {
      ctx.save();
      if (morph > 0.005) {
        ctx.globalAlpha *= catAlpha;
        ctx.translate(582 + catMorphX, 340 + catMorphY);
        ctx.scale(catMorphSX, catMorphSY);
        ctx.translate(-582, -340);
      }
      ctx.translate(582 + angerShakeX, 340 + headBobY);
      ctx.rotate(headTiltRad);
      ctx.scale(headSquashX, headSquashY);
      ctx.translate(-582, -340);

      // 2a0. Tail-tip decorations tucked behind shoulder/back
      drawDecorationsInSpace(ctx, skin, ['tailTip'], v2SpaceAnchors, v2SpacePerCanvas);

      // 2a. Right shoulder / back
      ctx.beginPath();
      ctx.moveTo(416, 316);
      ctx.lineTo(718, 326);
      ctx.bezierCurveTo(742, 358, 758, 402, 762, 444);
      ctx.lineTo(416, 422);
      ctx.closePath();
      ctx.fillStyle = rgbaToCss(cream);
      ctx.fill();
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = catLW;
      ctx.beginPath();
      ctx.moveTo(716, 326);
      ctx.bezierCurveTo(742, 356, 758, 400, 762, 444);
      ctx.stroke();

      // 2a2. Neck decorations over shoulder/chest, behind head, keyboard & arms
      drawDecorationsInSpace(ctx, skin, ['neck'], v2SpaceAnchors, v2SpacePerCanvas);

      // 2b. Whiskers directly traced from Video 2 reference temples (top-left \ \ and top-right / /)
      const wTwitch = hunt > 0.15 ? Math.sin(state.time * 30.0) * 3.5 * hunt : 0;
      ctx.strokeStyle = rgbaToCss(ink);
      ctx.lineWidth = catLW * 0.90;
      ctx.beginPath();
      ctx.moveTo(420, 168 + wTwitch); ctx.lineTo(446, 194);
      ctx.moveTo(444, 146 - wTwitch); ctx.lineTo(462, 178);
      ctx.moveTo(728, 206); ctx.lineTo(754, 174 + wTwitch);
      ctx.moveTo(740, 224); ctx.lineTo(772, 208 - wTwitch);
      ctx.stroke();

      // 2c. Soft round mochi unified head + ears
      drawV2UnifiedHeadAndEars(ctx, earLagX, earFlare, cream, taupe, ink, catLW, anger);

      // 2d. Expressive boba eyes (normal / hunting starry dilated / dizzy spirals / angry slanted V-brows)
      const gazeX = state.eyeGazeX || 0;
      const gazeY = state.eyeGazeY || 0;
      const blink = Math.max(state.blinkProgress || 0, smoothstep(0.04, 0.44, morph));
      const dzPhase = state.dizzyPhase || (state.time * 7.5);
      drawV2BobaEye(ctx, { x: 518, y: 262 }, 47.5, true, gazeX, gazeY, blink, hunt, dizzy, dzPhase, anger, cream, ink, catLW);
      drawV2BobaEye(ctx, { x: 652, y: 274 }, 46.5, false, gazeX, gazeY, blink, hunt, dizzy, dzPhase, anger, cream, ink, catLW);

      // 2e. Comic emotion accents: pulsing red anger vein (💢), alert hunter ticks, orbiting dizzy stars
      if (anger > 0.14) {
        const aAlpha = smoothstep(0.14, 0.55, anger);
        const pulse = (1.0 + Math.sin(state.time * 16.0) * 0.12) * 0.90;
        drawDouniAngerVein(ctx, { x: 682, y: 198 }, pulse, aAlpha);
      }
      if (hunt > 0.35 && dizzy < 0.15) {
        const hAlpha = smoothstep(0.35, 0.78, hunt);
        ctx.save();
        ctx.globalAlpha *= hAlpha;
        ctx.strokeStyle = rgbaToCss(ink);
        ctx.lineWidth = catLW * 0.68;
        ctx.beginPath();
        ctx.moveTo(472, 112); ctx.lineTo(454, 92);
        ctx.moveTo(454, 138); ctx.lineTo(432, 130);
        ctx.stroke();
        ctx.restore();
      }
      if (dizzy > 0.08) {
        drawDouniDizzyStars(ctx, { x: 590, y: 74 }, dzPhase, dizzy, ink);
      }

      // 2f. Face & head-top decorations above head & boba eyes, behind keyboard & flailing arms
      drawDecorationsInSpace(ctx, skin, ['face', 'headTop'], v2SpaceAnchors, v2SpacePerCanvas);

      ctx.restore();
    }

    // 3. Angled hand-drawn wired keyboard with glow, blooms, sparks & recoil
    if (gearAlpha > 0.01) {
      ctx.save();
      if (morph > 0.005) {
        ctx.globalAlpha *= gearAlpha;
        ctx.translate(0, gearDropY);
      }
      drawDouniV2Keyboard(ctx, state, kbDeckColor, kbBevelColor, kbKeyColor, kbAccentColor, glowBaseColor, ink, kbLW, impY, impT);
      ctx.restore();
    }

    // 4-5. Chubby milk-tea arms (+ held comic stylus in tablet mode!)
    if (catAlpha > 0.01) {
      ctx.save();
      if (morph > 0.005) {
        ctx.globalAlpha *= catAlpha;
        ctx.translate(582 + catMorphX, 340 + catMorphY);
        ctx.scale(catMorphSX, catMorphSY);
        ctx.translate(-582, -340);
      }

      const huntTremble = (hunt > 0.18 && Math.abs(leftPose) < 0.25 && Math.abs(rightPose) < 0.25)
        ? Math.sin(state.time * 32.0) * 0.08 * hunt : 0;
      const effectiveLeftPose = leftPose * (1.0 - morph * 0.85) + huntTremble;
      const effectiveRightPose = rightPose * (1.0 - morph * 0.85) - huntTremble;
      const effectiveMouseBlend = mouseBlend * (1.0 - morph);

      const lTarget = state.douniLeftKeyTarget
        ? { x: state.douniLeftKeyTarget.x + impT * 10.0, y: state.douniLeftKeyTarget.y + impY * 8.5 }
        : null;
      const leftToolTarget = isTablet
        ? { x: v2Nib.x + 14, y: v2Nib.y - 12 + tabletPress * 5.0 }
        : { x: mouseCenter.x + 8, y: mouseCenter.y - 2 + mousePress * 8 };

      drawV2LeftArm(
        ctx, effectiveLeftPose, effectiveMouseBlend, leftToolTarget,
        lTarget, cream, taupe, ink, catLW, state.douniLeftRaiseVel || 0
      );

      if (isTablet && effectiveMouseBlend > 0.18) {
        const penAlpha = smoothstep(0.18, 0.42, effectiveMouseBlend);
        ctx.save();
        ctx.globalAlpha *= penAlpha;
        ctx.translate(v2Nib.x, v2Nib.y + tabletPress * 4.0);
        ctx.rotate(-2.14 + (state.tabletPenTilt || 0));
        drawDouniComicStylus(ctx, 124.0, glowBaseColor, eyePrimary, ink, catLW, tabletPress);
        ctx.restore();
      }

      const rTarget = state.douniRightKeyTarget
        ? { x: state.douniRightKeyTarget.x + impT * 10.0, y: state.douniRightKeyTarget.y + impY * 8.5 }
        : null;
      drawV2RightArm(ctx, effectiveRightPose, rTarget, cream, taupe, ink, catLW, state.douniRightRaiseVel || 0);

      ctx.restore();
    }

    ctx.restore();
    return { v2SpaceAnchors, headBobY, headTiltRad, headSquashX, headSquashY, angerShakeX };
  }

  function drawDouniV2InCanvas(ctx, state, skin, kbThemeName, eyeThemeName, loafMorph) {
    ctx.save();
    ctx.translate(14.0 + state.bodyShiftX, 22.0);
    ctx.scale(0.405, 0.405);
    const meta = drawDouniV2InSpace(ctx, state, skin, kbThemeName, eyeThemeName, loafMorph || 0);
    ctx.restore();

    const c = Math.cos(meta.headTiltRad), s = Math.sin(meta.headTiltRad);
    const toCanvas = (px, py) => {
      const dx = (px - 582) * meta.headSquashX;
      const dy = (py - 340) * meta.headSquashY;
      const vx = 582 + (meta.angerShakeX || 0) + dx * c - dy * s;
      const vy = 340 + meta.headBobY + dx * s + dy * c;
      return { x: 14.0 + state.bodyShiftX + vx * 0.405, y: 22.0 + vy * 0.405 };
    };
    const decoAnchors = {};
    for (const key of Object.keys(meta.v2SpaceAnchors)) {
      const sp = meta.v2SpaceAnchors[key];
      decoAnchors[key] = {
        origin: toCanvas(sp.origin.x, sp.origin.y),
        rot: sp.rot + meta.headTiltRad,
        kx: sp.kx * meta.headSquashX,
        ky: sp.ky * meta.headSquashY,
        offsetScaleX: sp.offsetScaleX * meta.headSquashX,
        offsetScaleY: sp.offsetScaleY * meta.headSquashY
      };
    }
    return { headCenter: toCanvas(582, 272), decoAnchors };
  }

  function drawDouniScene(ctx, skin, state, kbThemeName, eyeThemeName) {
    const scruff = clamp(state.scruffDrag || 0, 0, 1);
    const scruffS = smoothstep(0, 1, scruff);
    const wantsLoafFallback = (state.petHead > 0.15 || state.petBody > 0.25 || state.sleepiness > 0.42) ? 1 : 0;
    const loafRaw = clamp(state.douniLoafAnimated ? (state.douniLoafBlend || 0) : wantsLoafFallback, 0, 1);
    const loafS = smoothstep(0, 1, loafRaw);

    if (scruff > 0.02) {
      const deskFade = 1.0 - smoothstep(0.0, 0.48, scruffS);
      if (deskFade > 0.01 && loafS < 0.85) {
        ctx.save();
        ctx.globalAlpha *= deskFade * (1 - loafS);
        ctx.translate(0, scruffS * 28.0);
        drawDouniV2InCanvas(ctx, state, skin, kbThemeName, eyeThemeName, 0);
        ctx.restore();
      }
      if (deskFade > 0.01 && loafS > 0.02) {
        ctx.save();
        ctx.globalAlpha *= deskFade * loafS;
        ctx.translate(0, scruffS * 20.0);
        drawDouniV1LoafInCanvas(ctx, state, skin, 1.0);
        ctx.restore();
      }
      ctx.save();
      ctx.translate((1.0 - scruffS) * 40.0 * (1.0 - loafS), (1.0 - scruffS) * 30.0);
      const dragMeta = drawDouniV1DragInCanvas(ctx, state, skin, scruff);
      ctx.restore();
      return {
        seatX: 184 + state.bodyShiftX,
        seatY: 226,
        headCenter: dragMeta.headCenter,
        decoAnchors: dragMeta.decoAnchors
      };
    }

    let v2Meta = null;
    if (loafS < 0.99) {
      v2Meta = drawDouniV2InCanvas(ctx, state, skin, kbThemeName, eyeThemeName, loafS);
    }

    let loafAnchors = null;
    if (loafS > 0.01) {
      loafAnchors = drawDouniV1LoafInCanvas(ctx, state, skin, loafS);
    }

    const v2Head = (v2Meta && v2Meta.headCenter) || { x: 14 + state.bodyShiftX + 582 * 0.405, y: 22 + 272 * 0.405 };
    const v1Head = { x: 20 + state.bodyShiftX + 380 * 0.345, y: 46 + 472 * 0.345 };
    const activeAnchors = (loafS >= 0.5 && loafAnchors) ? loafAnchors : (v2Meta ? v2Meta.decoAnchors : loafAnchors);
    return {
      seatX: 184 + state.bodyShiftX,
      seatY: 226,
      headCenter: lerpPt(v2Head, v1Head, loafS),
      decoAnchors: activeAnchors
    };
  }

  // --- Universal Front-Facing Long-Cat Scruff-Lifted Drag Pose (1:1 parity with native CatRenderer.swift) ---
  function drawNativeScruffDragCat(ctx, state, skin, eyeThemeName, lift) {
    const liftS = smoothstep(0, 1, lift);
    let startle;
    if (state.isScruffDragging) {
      const hold = state.scruffHoldTime || 0;
      startle = hold < 0.14 ? 1.0 : Math.max(0, 1.0 - (hold - 0.14) / 0.08);
    } else {
      const drop = state.scruffDropTimer || 0;
      startle = drop > 0.01 ? Math.min(1.0, drop / 0.12) : 1.0;
    }

    const swingH = clamp(state.scruffSwing || 0, -1.0, 1.0);
    const swingY = clamp(state.scruffSwingY || 0, -0.92, 0.92);
    const tailSwing = clamp(state.scruffTailSwing || 0, -1.25, 1.25);
    const idleSway = Math.sin(state.time * 4.2) * 0.045 * (1.0 - startle);
    const limbSwing = clamp(swingH * 0.85, -0.82, 0.82) * (1.0 - startle * 0.45) + idleSway;
    const upBillow = Math.max(0, -swingY);

    const pal = skin.palette;
    const bd = getBuild(skin);
    const hs = skin.shape.headScale;
    const bw = skin.shape.bodyWidth;
    const nw = skin.shape.neckWidth || 1.0;
    const isDazed = skin.renderStyle === 'dazed';
    const isBlackRim = isClassicBlackRim(skin);
    const limbInkCss = isBlackRim
      ? 'rgba(240,240,240,0.28)'
      : (pal.outline ? rgbaToCss(pal.outline) : 'rgba(46,38,31,0.26)');

    const isSleek = skin.shape.build === 'sleek';
    const plump = clamp((bw - 1.0) + (isSleek ? -0.15 : 0.0), -0.35, 0.38);
    const limbWScale = clamp((bd.limbStartW / 19.0) * (1.0 + plump * 0.62), 0.72, 1.30);
    const limbLenScale = clamp(1.0 - plump * 0.28 + (isSleek ? 0.07 : 0.0), 0.84, 1.18);

    const baseShiftX = (state.bodyShiftX || 0) + swingH * 10.0;
    const liftY = -liftS * 12.0 + Math.sin(state.time * 3.8) * 2.0 * (1.0 - startle) + swingY * 5.0;
    const headCenter = {
      x: 180.0 + baseShiftX,
      y: lerpNum(94.0, 108.0, startle) + liftY + (hs - 1.0) * 6.0
    };

    const cheekHalfW = bd.cheekHalfW * skin.shape.cheekFullness * hs;
    const crownY = bd.crownBaseY * hs;
    const cheekY = bd.cheekY * hs;
    const chinY = 33.0 * hs;

    const scruffTuftH = lerpNum(6.2, 3.8, startle) * hs;
    const scruffPinch = { x: headCenter.x, y: headCenter.y + crownY - scruffTuftH };

    const torsoLen = lerpNum(124.0 - plump * 18.0 + (isSleek ? 10.0 : 0.0), 104.0 - plump * 12.0, startle) * (1.0 + swingY * 0.08);
    const torsoBottomY = headCenter.y + torsoLen;
    const hipLagX = (swingH * 22.0 + idleSway * 16.0) * (1.0 - startle * 0.35);

    const neckHalfW = Math.max(18.0 * hs, (cheekHalfW - bd.neckPinchX * hs) * nw);
    const waistHalfW = bd.waistCtrlW * bw * lerpNum(0.84 + plump * 0.24, 0.92 + plump * 0.20, startle);
    const hipHalfW = bd.hipHalfW * bw * lerpNum(0.95 + plump * 0.26, 1.00 + plump * 0.20, startle);
    const bottomDip = 12.0 + Math.max(0.0, plump) * 14.0;
    const rumpCenterX = headCenter.x + hipLagX;

    // 1. Unified front-facing long-cat silhouette
    const traceLongCat = () => {
      const tuftBaseX = Math.min(12.5 * hs, cheekHalfW * 0.31);
      const rCheekX = headCenter.x + cheekHalfW;
      const lCheekX = headCenter.x - cheekHalfW;
      const cheekPosY = headCenter.y + cheekY;
      const neckCtrlY = headCenter.y + bd.neckCtrlY * hs;
      const waistY = headCenter.y + torsoLen * 0.48;
      const hipY = torsoBottomY - 14.0;
      const waistCenterX = headCenter.x + hipLagX * 0.34;
      const botHalf = hipHalfW * 0.62;

      ctx.beginPath();
      ctx.moveTo(scruffPinch.x, scruffPinch.y);
      ctx.bezierCurveTo(
        headCenter.x + tuftBaseX * 0.56, scruffPinch.y + scruffTuftH * 0.06,
        headCenter.x + tuftBaseX * 0.80, headCenter.y + crownY - scruffTuftH * 0.14,
        headCenter.x + tuftBaseX, headCenter.y + crownY + 0.5 * hs
      );
      ctx.bezierCurveTo(
        headCenter.x + bd.crownCtrlW * hs, headCenter.y + crownY,
        rCheekX + 1.5 * hs, headCenter.y + bd.cheekCtrlTopY * hs,
        rCheekX, cheekPosY
      );
      ctx.bezierCurveTo(
        headCenter.x + neckHalfW, neckCtrlY,
        waistCenterX + waistHalfW, waistY,
        rumpCenterX + hipHalfW, hipY
      );
      ctx.bezierCurveTo(
        rumpCenterX + hipHalfW * 0.96, torsoBottomY + bottomDip * 0.55,
        rumpCenterX + botHalf, torsoBottomY + bottomDip,
        rumpCenterX, torsoBottomY + bottomDip
      );
      ctx.bezierCurveTo(
        rumpCenterX - botHalf, torsoBottomY + bottomDip,
        rumpCenterX - hipHalfW * 0.96, torsoBottomY + bottomDip * 0.55,
        rumpCenterX - hipHalfW, hipY
      );
      ctx.bezierCurveTo(
        waistCenterX - waistHalfW, waistY,
        headCenter.x - neckHalfW, neckCtrlY,
        lCheekX, cheekPosY
      );
      ctx.bezierCurveTo(
        lCheekX - 1.5 * hs, headCenter.y + bd.cheekCtrlTopY * hs,
        headCenter.x - bd.crownCtrlW * hs, headCenter.y + crownY,
        headCenter.x - tuftBaseX, headCenter.y + crownY + 0.5 * hs
      );
      ctx.bezierCurveTo(
        headCenter.x - tuftBaseX * 0.80, headCenter.y + crownY - scruffTuftH * 0.14,
        headCenter.x - tuftBaseX * 0.56, scruffPinch.y + scruffTuftH * 0.06,
        scruffPinch.x, scruffPinch.y
      );
      ctx.closePath();
    };

    // Exact 1:1 front-view body mapping
    const dragBodyMap = (pt) => {
      const u = (pt.y + 98.0) / 110.0;
      const uC = clamp(u, 0.0, 1.15);
      const ySpan = (torsoBottomY + bottomDip * 0.78) - headCenter.y;
      const mappedY = headCenter.y + u * ySpan;
      const baseRefW = Math.max(1.0, bd.hipHalfW * bw);
      const targetW = lerpNum(waistHalfW / Math.max(1.0, bd.waistCtrlW * bw), hipHalfW / baseRefW, smoothstep(0.35, 0.95, uC));
      const wScale = lerpNum(1.0, targetW, smoothstep(0.18, 0.55, uC));
      const swayX = hipLagX * uC * uC;
      return { x: headCenter.x + pt.x * bw * wScale + swayX, y: mappedY };
    };

    // 2. Dangling limp tail emerging from lower-right behind the hip
    const tailPivot = { x: rumpCenterX + hipHalfW * 0.60, y: torsoBottomY - 16.0 };
    const tailLen = 76.0 * skin.shape.tailLength * (1.0 + swingY * 0.14);
    const tailW = Math.max(8.0, (isDazed ? 13.0 : bd.tailWidth * skin.shape.tailThickness) * 1.08);
    const rawTailAng = (tailSwing * (1.0 - startle * 0.25) + idleSway * 1.5) * 0.85;
    const tailAng = clamp(rawTailAng, -1.05, 0.75);
    const tailC1 = v1RotatePoint({ x: tailPivot.x + 26 + upBillow * 10, y: tailPivot.y + tailLen * 0.22 - upBillow * 10 }, tailPivot, tailAng * 0.45);
    const tailC2 = v1RotatePoint({ x: tailPivot.x + 36 + upBillow * 18, y: tailPivot.y + tailLen * 0.64 - upBillow * 26 }, tailPivot, tailAng * 0.92);
    const tailTip = v1RotatePoint({ x: tailPivot.x + 24 + upBillow * 24, y: tailPivot.y + tailLen - upBillow * 40 }, tailPivot, tailAng * 1.26);
    const traceTailCurve = () => {
      ctx.beginPath();
      ctx.moveTo(tailPivot.x, tailPivot.y);
      ctx.bezierCurveTo(tailC1.x, tailC1.y, tailC2.x, tailC2.y, tailTip.x, tailTip.y);
    };

    // 3. Four symmetric front-facing dangling limbs
    const makeLimb = (rA, tA, rB, tB, w0, w1, bendX, swingScale, splayDir, armSpace, isBehindBody) => {
      const root = lerpPt(rA, rB, startle);
      const baseTip = lerpPt(tA, tB, startle);
      const lenFactor = (1.0 + swingY * 0.16) * limbLenScale;
      const stretched = {
        x: root.x + (baseTip.x - root.x) * lenFactor,
        y: root.y + (baseTip.y - root.y) * lenFactor - upBillow * 6.0
      };
      const ang = limbSwing * swingScale + upBillow * 0.20 * splayDir;
      const tip = v1RotatePoint(stretched, root, ang);
      const dynBendX = bendX + limbSwing * 11.0 * swingScale;
      return { root, tip, startW: w0 * limbWScale, endW: w1 * limbWScale, bendX: dynBendX, armSpace: armSpace || null, isBehindBody: !!isBehindBody };
    };

    const shoulderY = headCenter.y + 42.0 * hs;
    const shoulderInX = Math.max(12.0 * hs, Math.min(neckHalfW * 0.72, waistHalfW * 0.56));
    const armLen = 56.0;
    const frontLeftLimb = makeLimb(
      { x: headCenter.x - shoulderInX + hipLagX * 0.16, y: shoulderY },
      { x: headCenter.x - shoulderInX + 1.5 + hipLagX * 0.30, y: shoulderY + armLen },
      { x: headCenter.x - shoulderInX + hipLagX * 0.12, y: shoulderY + 3.0 },
      { x: headCenter.x - shoulderInX - 4.0 + hipLagX * 0.22, y: shoulderY + armLen * 0.84 },
      13.6, 12.4, -1.5, 0.42, -0.65, 'leftArm', false
    );
    const frontRightLimb = makeLimb(
      { x: headCenter.x + shoulderInX + hipLagX * 0.16, y: shoulderY },
      { x: headCenter.x + shoulderInX - 1.5 + hipLagX * 0.30, y: shoulderY + armLen },
      { x: headCenter.x + shoulderInX + hipLagX * 0.12, y: shoulderY + 3.0 },
      { x: headCenter.x + shoulderInX + 4.0 + hipLagX * 0.22, y: shoulderY + armLen * 0.84 },
      13.6, 12.4, 1.5, 0.42, 0.65, 'rightArm', false
    );

    const hindRootX = Math.max(13.0, hipHalfW * 0.42);
    const hindRootY = torsoBottomY + bottomDip * 0.20 - 12.0;
    const hindLen = 46.0;
    const backLeftLimb = makeLimb(
      { x: rumpCenterX - hindRootX, y: hindRootY },
      { x: rumpCenterX - hindRootX - 2.5, y: hindRootY + hindLen },
      { x: rumpCenterX - hindRootX - 1.0, y: hindRootY - 3.0 },
      { x: rumpCenterX - hindRootX - 7.5, y: hindRootY + hindLen * 0.80 },
      15.2, 12.8, -1.5, 0.68, -0.65, 'hindLeg', true
    );
    const backRightLimb = makeLimb(
      { x: rumpCenterX + hindRootX, y: hindRootY },
      { x: rumpCenterX + hindRootX + 2.5, y: hindRootY + hindLen },
      { x: rumpCenterX + hindRootX + 1.0, y: hindRootY - 3.0 },
      { x: rumpCenterX + hindRootX + 7.5, y: hindRootY + hindLen * 0.80 },
      15.2, 12.8, 1.5, 0.68, 0.65, 'hindLeg', true
    );

    const limbGeo = (limb) => {
      const dx = limb.tip.x - limb.root.x, dy = limb.tip.y - limb.root.y;
      const len = Math.max(1, Math.hypot(dx, dy));
      const ux = dx / len, uy = dy / len, nx = -uy, ny = ux;
      const r0 = limb.startW * 0.5, r1 = limb.endW * 0.5;
      const p0L = { x: limb.root.x + nx * r0, y: limb.root.y + ny * r0 };
      const p0R = { x: limb.root.x - nx * r0, y: limb.root.y - ny * r0 };
      const p1L = { x: limb.tip.x + nx * r1, y: limb.tip.y + ny * r1 };
      const p1R = { x: limb.tip.x - nx * r1, y: limb.tip.y - ny * r1 };
      const midL = { x: (p0L.x + p1L.x) * 0.5 + limb.bendX * 0.5 + nx * 1.2, y: (p0L.y + p1L.y) * 0.5 + ny * 1.2 };
      const midR = { x: (p0R.x + p1R.x) * 0.5 + limb.bendX * 0.5 - nx * 1.2, y: (p0R.y + p1R.y) * 0.5 - ny * 1.2 };
      const pawC1 = { x: p1L.x + ux * r1 * 1.25, y: p1L.y + uy * r1 * 1.25 };
      const pawC2 = { x: p1R.x + ux * r1 * 1.25, y: p1R.y + uy * r1 * 1.25 };
      return { ux, uy, nx, ny, r0, r1, p0L, p0R, p1L, p1R, midL, midR, pawC1, pawC2 };
    };
    const traceLimbRim = (limb) => {
      const g = limbGeo(limb);
      ctx.beginPath();
      ctx.moveTo(g.p0L.x, g.p0L.y);
      ctx.quadraticCurveTo(g.midL.x, g.midL.y, g.p1L.x, g.p1L.y);
      ctx.bezierCurveTo(g.pawC1.x, g.pawC1.y, g.pawC2.x, g.pawC2.y, g.p1R.x, g.p1R.y);
      ctx.quadraticCurveTo(g.midR.x, g.midR.y, g.p0R.x, g.p0R.y);
    };
    const traceLimbFill = (limb) => {
      const g = limbGeo(limb);
      ctx.beginPath();
      ctx.moveTo(g.p0L.x, g.p0L.y);
      ctx.quadraticCurveTo(g.midL.x, g.midL.y, g.p1L.x, g.p1L.y);
      ctx.bezierCurveTo(g.pawC1.x, g.pawC1.y, g.pawC2.x, g.pawC2.y, g.p1R.x, g.p1R.y);
      ctx.quadraticCurveTo(g.midR.x, g.midR.y, g.p0R.x, g.p0R.y);
      ctx.bezierCurveTo(
        g.p0R.x - g.ux * g.r0 * 0.85, g.p0R.y - g.uy * g.r0 * 0.85,
        g.p0L.x - g.ux * g.r0 * 0.85, g.p0L.y - g.uy * g.r0 * 0.85,
        g.p0L.x, g.p0L.y
      );
      ctx.closePath();
    };
    const traceLimbInk = (limb) => {
      const g = limbGeo(limb);
      const t0 = 0.24, omt = 1.0 - t0;
      const s0L = {
        x: omt * omt * g.p0L.x + 2 * omt * t0 * g.midL.x + t0 * t0 * g.p1L.x,
        y: omt * omt * g.p0L.y + 2 * omt * t0 * g.midL.y + t0 * t0 * g.p1L.y
      };
      const scL = { x: omt * g.midL.x + t0 * g.p1L.x, y: omt * g.midL.y + t0 * g.p1L.y };
      const s0R = {
        x: omt * omt * g.p0R.x + 2 * omt * t0 * g.midR.x + t0 * t0 * g.p1R.x,
        y: omt * omt * g.p0R.y + 2 * omt * t0 * g.midR.y + t0 * t0 * g.p1R.y
      };
      const scR = { x: omt * g.midR.x + t0 * g.p1R.x, y: omt * g.midR.y + t0 * g.p1R.y };
      ctx.beginPath();
      ctx.moveTo(s0L.x, s0L.y);
      ctx.quadraticCurveTo(scL.x, scL.y, g.p1L.x, g.p1L.y);
      ctx.bezierCurveTo(g.pawC1.x, g.pawC1.y, g.pawC2.x, g.pawC2.y, g.p1R.x, g.p1R.y);
      ctx.quadraticCurveTo(scR.x, scR.y, s0R.x, s0R.y);
    };

    const drawDragLimb = (limb) => {
      const g = limbGeo(limb);
      if (isDazed && limb.armSpace && !limb.isBehindBody) {
        const leftEdge = [], rightEdge = [];
        for (let i = 0; i <= 8; i++) {
          const t = i / 8.0, omt = 1.0 - t;
          leftEdge.push({
            x: omt * omt * g.p0L.x + 2 * omt * t * g.midL.x + t * t * g.p1L.x,
            y: omt * omt * g.p0L.y + 2 * omt * t * g.midL.y + t * t * g.p1L.y
          });
          rightEdge.push({
            x: omt * omt * g.p0R.x + 2 * omt * t * g.midR.x + t * t * g.p1R.x,
            y: omt * omt * g.p0R.y + 2 * omt * t * g.midR.y + t * t * g.p1R.y
          });
        }
        const pawRect = { x: limb.tip.x - g.r1, y: limb.tip.y - g.r1 * 0.4, w: g.r1 * 2, h: g.r1 * 1.8 };
        drawDazedArm(ctx, () => traceLimbFill(limb), leftEdge, rightEdge, pawRect);
        return;
      }

      ctx.save();
      traceLimbFill(limb);
      ctx.clip();
      const grad = ctx.createLinearGradient(limb.root.x, limb.root.y, limb.tip.x, limb.tip.y);
      if (isDazed) {
        grad.addColorStop(0.0, '#E3C99F');
        grad.addColorStop(1.0, '#C8A77A');
      } else if (skin.colorpoint) {
        const mid = pal.pointsMid || pal.legs;
        grad.addColorStop(0.0, rgbaToCss(pal.body));
        grad.addColorStop(0.20, rgbaToCss(pal.body));
        grad.addColorStop(0.54, rgbaToCss(mid));
        grad.addColorStop(0.88, rgbaToCss(pal.paws));
      } else if (!limb.isBehindBody && !sameColor(pal.legs, pal.body)) {
        grad.addColorStop(0.0, rgbaToCss(pal.body));
        grad.addColorStop(0.14, rgbaToCss(pal.body));
        grad.addColorStop(0.42, rgbaToCss(pal.legs));
        grad.addColorStop(0.82, rgbaToCss(pal.paws));
      } else {
        const legCol = limb.isBehindBody ? (pal.hindLeg || pal.legs) : pal.legs;
        const pawCol = limb.isBehindBody ? (pal.hindPaw || pal.paws) : pal.paws;
        grad.addColorStop(0.22, rgbaToCss(legCol));
        grad.addColorStop(0.75, rgbaToCss(pawCol));
      }
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, 360, 320);

      if (limb.armSpace) {
        const sign = limb.armSpace === 'leftArm' ? -1.0 : ((limb.armSpace === 'hindLeg' && limb.bendX < 0) ? -1.0 : 1.0);
        const armMap = (p) => {
          const t = clamp(p.x / 100.0, 0.0, 1.0);
          const omt = 1.0 - t;
          const lx = omt * omt * g.p0L.x + 2 * omt * t * g.midL.x + t * t * g.p1L.x;
          const ly = omt * omt * g.p0L.y + 2 * omt * t * g.midL.y + t * t * g.p1L.y;
          const rx = omt * omt * g.p0R.x + 2 * omt * t * g.midR.x + t * t * g.p1R.x;
          const ry = omt * omt * g.p0R.y + 2 * omt * t * g.midR.y + t * t * g.p1R.y;
          const cx = (lx + rx) * 0.5, cy = (ly + ry) * 0.5;
          const hx = (rx - lx) * 0.5, hy = (ry - ly) * 0.5;
          const u = sign * (p.y / 10.0);
          return { x: cx + hx * u, y: cy + hy * u };
        };
        for (const r of skin.regions) if (r.space === limb.armSpace) drawRegion(ctx, r, armMap);
        for (const m of skin.marks) if (m.space === limb.armSpace) drawMark(ctx, m, 'round', armMap);
      }
      ctx.restore();

      ctx.save();
      ctx.strokeStyle = limbInkCss;
      ctx.lineWidth = isBlackRim ? 1.15 : 1.2;
      ctx.lineCap = 'round';
      ctx.lineJoin = 'round';
      traceLimbInk(limb);
      ctx.stroke();
      ctx.beginPath();
      for (const side of [-1, 1]) {
        const ax = limb.tip.x + g.nx * side * 2.5 + g.ux * 1.6;
        const ay = limb.tip.y + g.ny * side * 2.5 + g.uy * 1.6;
        const bx = limb.tip.x + g.nx * side * 2.5 + g.ux * (limb.endW * 0.44);
        const by = limb.tip.y + g.ny * side * 2.5 + g.uy * (limb.endW * 0.44);
        ctx.moveTo(ax, ay); ctx.lineTo(bx, by);
      }
      ctx.stroke();
      ctx.restore();
    };

    const drawEars = (outlineOnly) => {
      ctx.save();
      ctx.translate(headCenter.x, headCenter.y);
      ctx.rotate(state.headTilt);
      if (hs !== 1) ctx.scale(hs, hs);
      drawEar3D(ctx, true, state.headYaw * 0.50, state.headPitch * 0.36, state.earFold, state.earTwitch * 0.045, skin, outlineOnly);
      drawEar3D(ctx, false, state.headYaw * 0.50, state.headPitch * 0.36, state.earFold, -state.earTwitch * 0.045, skin, outlineOnly);
      ctx.restore();
    };

    ctx.save();
    ctx.globalAlpha *= smoothstep(0, 0.22, lift);
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';

    const bodyPendulum = clamp(swingH * 0.16, -0.17, 0.17);
    if (Math.abs(bodyPendulum) > 0.001) {
      ctx.translate(scruffPinch.x, scruffPinch.y);
      ctx.rotate(bodyPendulum);
      ctx.translate(-scruffPinch.x, -scruffPinch.y);
    }

    // Pass 0: Universal sticker-style outer white silhouette rim
    if (stateWhiteOutline) {
      const whiteRimW = WHITE_OUTLINE_W * 2.0 + (pal.outline ? 2.0 : 0.0);
      ctx.save();
      ctx.strokeStyle = WHITE_OUTLINE_CSS;
      ctx.lineWidth = tailW + whiteRimW;
      traceTailCurve(); ctx.stroke();
      ctx.lineWidth = whiteRimW;
      for (const l of [backLeftLimb, backRightLimb, frontLeftLimb, frontRightLimb]) {
        traceLimbRim(l); ctx.stroke();
      }
      drawEars(true);
      traceLongCat(); ctx.stroke();
      ctx.restore();
    }

    if (pal.outline && !isDazed) {
      ctx.save();
      ctx.strokeStyle = rgbaToCss(pal.outline);
      ctx.lineWidth = tailW + 2.4;
      traceTailCurve(); ctx.stroke();
      ctx.lineWidth = 2.4;
      for (const l of [backLeftLimb, backRightLimb, frontLeftLimb, frontRightLimb]) {
        traceLimbRim(l); ctx.stroke();
      }
      traceLongCat(); ctx.stroke();
      ctx.restore();
    }

    const hasDecos = skin.decorations && skin.decorations.length > 0;
    const decoAnchors = {
      neck: {
        origin: { x: headCenter.x + hipLagX * 0.08, y: headCenter.y + 36.0 * hs },
        rot: limbSwing * 0.14,
        kx: hs, ky: hs, offsetScaleX: bw, offsetScaleY: hs
      },
      tailTip: {
        origin: { x: tailTip.x, y: tailTip.y },
        rot: tailAng,
        kx: 1, ky: 1, offsetScaleX: 1, offsetScaleY: 1
      }
    };

    // 1. Limp drooping/whipping tail behind lower body
    ctx.save();
    ctx.lineCap = 'round';
    if (!isBlackRim && !pal.outline) {
      ctx.strokeStyle = 'rgba(0,0,0,0.16)';
      ctx.lineWidth = tailW + 2.0;
      traceTailCurve(); ctx.stroke();
    }
    ctx.strokeStyle = isDazed ? '#283157' : rgbaToCss(pal.tail);
    ctx.lineWidth = tailW;
    traceTailCurve(); ctx.stroke();
    if (pal.stripes && !isBlackRim && !skin.colorpoint && !isDazed) {
      const evalTailPt = (t) => {
        const u = 1 - t;
        return {
          x: u * u * u * tailPivot.x + 3 * u * u * t * tailC1.x + 3 * u * t * t * tailC2.x + t * t * t * tailTip.x,
          y: u * u * u * tailPivot.y + 3 * u * u * t * tailC1.y + 3 * u * t * t * tailC2.y + t * t * t * tailTip.y
        };
      };
      ctx.strokeStyle = rgbaToCss(pal.stripes, 0.42);
      ctx.lineWidth = tailW * 0.88;
      ctx.lineCap = 'butt';
      for (const t0 of [0.24, 0.50, 0.74]) {
        const pA = evalTailPt(t0), pB = evalTailPt(t0 + 0.09);
        ctx.beginPath();
        ctx.moveTo(pA.x, pA.y);
        ctx.lineTo(pB.x, pB.y);
        ctx.stroke();
      }
      ctx.lineCap = 'round';
    }
    if (!isDazed && pal.tailTip) {
      const tipStart = lerpPt(tailC2, tailTip, 0.35);
      ctx.strokeStyle = rgbaToCss(pal.tailTip);
      ctx.lineWidth = tailW * 0.92;
      ctx.beginPath();
      ctx.moveTo(tipStart.x, tipStart.y);
      ctx.lineTo(tailTip.x, tailTip.y);
      ctx.stroke();
    }
    ctx.restore();

    if (hasDecos) {
      drawDecorations(ctx, rumpCenterX, torsoBottomY, headCenter, state, skin, { anchors: ['tailTip'], decoAnchors });
    }

    // 2. Dangling hind legs emerging from lower belly hem
    drawDragLimb(backLeftLimb);
    drawDragLimb(backRightLimb);

    // 3. Native 3D Ears behind head crown
    drawEars(false);

    // 4. Unified Front-Facing Long-Cat Head + Subtle Scruff Pinch + Stretched Torso
    const headMap = (p) => headPoint(p, false, skin, headCenter, state);
    const skullMap = (p) => headPoint(p, true, skin, headCenter, state);

    if (isDazed) {
      ctx.save();
      traceLongCat();
      ctx.clip();
      const bellyGrad = ctx.createLinearGradient(headCenter.x, headCenter.y + 30, rumpCenterX, torsoBottomY + bottomDip);
      bellyGrad.addColorStop(0.0, '#F4DFB8');
      bellyGrad.addColorStop(1.0, '#C8A77A');
      ctx.fillStyle = bellyGrad;
      ctx.fillRect(0, 0, 360, 320);

      const tunicBotY = headCenter.y + torsoLen * 0.74;
      ctx.save();
      ctx.beginPath();
      ctx.moveTo(headCenter.x - cheekHalfW * 1.4, headCenter.y);
      ctx.lineTo(headCenter.x + cheekHalfW * 1.4, headCenter.y);
      ctx.lineTo(rumpCenterX + hipHalfW * 1.2, tunicBotY);
      ctx.bezierCurveTo(
        rumpCenterX + hipHalfW * 0.5, tunicBotY + 14,
        rumpCenterX - hipHalfW * 0.5, tunicBotY + 14,
        rumpCenterX - hipHalfW * 1.2, tunicBotY
      );
      ctx.closePath();
      ctx.clip();
      const clothGrad = ctx.createLinearGradient(headCenter.x - 16, headCenter.y + 20, rumpCenterX + 12, tunicBotY + 10);
      clothGrad.addColorStop(0.0, '#555F9B');
      clothGrad.addColorStop(1.0, '#252E58');
      ctx.fillStyle = clothGrad;
      ctx.fillRect(0, 0, 360, 320);
      ctx.restore();

      ctx.strokeStyle = 'rgba(124,134,188,0.65)';
      ctx.lineWidth = 1.3;
      ctx.beginPath();
      ctx.moveTo(rumpCenterX - hipHalfW, tunicBotY - 1);
      ctx.bezierCurveTo(
        rumpCenterX - hipHalfW * 0.45, tunicBotY + 11,
        rumpCenterX + hipHalfW * 0.45, tunicBotY + 11,
        rumpCenterX + hipHalfW, tunicBotY - 1
      );
      ctx.stroke();
      ctx.restore();

      // Fluffy Palico hood around head
      ctx.save();
      ctx.translate(headCenter.x, headCenter.y);
      ctx.rotate(state.headTilt);
      ctx.beginPath();
      ctx.moveTo(0.0, -33.0 - scruffTuftH);
      ctx.bezierCurveTo(28.0, -35.0, 48.0, -16.0, 48.0, 9.0);
      ctx.bezierCurveTo(48.0, 32.0, 25.0, 41.0, 0.0, 41.0);
      ctx.bezierCurveTo(-25.0, 41.0, -48.0, 32.0, -48.0, 9.0);
      ctx.bezierCurveTo(-48.0, -16.0, -28.0, -35.0, 0.0, -33.0 - scruffTuftH);
      ctx.closePath();
      ctx.clip();
      const hoodGrad = ctx.createLinearGradient(-16, -38, 15, 42);
      hoodGrad.addColorStop(0.0, '#6871B4');
      hoodGrad.addColorStop(1.0, '#2F386D');
      ctx.fillStyle = hoodGrad;
      ctx.fillRect(-60, -60, 120, 120);
      ctx.restore();
    } else {
      ctx.save();
      traceLongCat();
      ctx.fillStyle = rgbaToCss(pal.body);
      ctx.fill();
      ctx.clip();

      if (pal.bodyShade) {
        const grad = ctx.createLinearGradient(headCenter.x, headCenter.y - 36, rumpCenterX, torsoBottomY + bottomDip);
        grad.addColorStop(0.15, rgbaToCss(pal.body));
        grad.addColorStop(1.0, rgbaToCss(pal.bodyShade));
        ctx.fillStyle = grad;
        ctx.fillRect(0, 0, 360, 320);
      }

      if (pal.headBody) {
        ctx.save();
        drawPolyline(ctx, jawSide(true, skin.shape.cheekFullness), true, headMap);
        ctx.clip();
        traceLongCat();
        ctx.fillStyle = rgbaToCss(pal.headBody);
        ctx.fill();
        ctx.restore();
      }

      if (pal.points) {
        const mid = pal.pointsMid || pal.points;
        ctx.save();
        ctx.translate(headCenter.x, headCenter.y + 3.0);
        ctx.scale(1.22 * hs, 0.88 * hs);
        const g = ctx.createRadialGradient(0, 0, 0, 0, 0, 31.0);
        g.addColorStop(0.0, rgbaToCss(pal.points, 1));
        g.addColorStop(0.42, rgbaToCss(pal.points, 0.92));
        g.addColorStop(0.74, rgbaToCss(mid, 0.62));
        g.addColorStop(1.0, rgbaToCss(mid, 0));
        ctx.fillStyle = g;
        ctx.beginPath();
        ctx.arc(0, 0, 32, 0, Math.PI * 2);
        ctx.fill();
        ctx.restore();
      }

      const dragBodySide = jawSide(false, skin.shape.cheekFullness);
      for (const r of skin.regions) {
        if (r.space !== 'body') continue;
        ctx.save();
        drawPolyline(ctx, dragBodySide, true, headMap);
        ctx.clip();
        drawRegion(ctx, r, dragBodyMap);
        ctx.restore();
      }
      const dragHeadSide = jawSide(true, skin.shape.cheekFullness);
      for (const r of skin.regions) {
        if (r.space !== 'head' && r.space !== 'skull') continue;
        ctx.save();
        drawPolyline(ctx, dragHeadSide, true, headMap);
        ctx.clip();
        drawRegion(ctx, r, r.space === 'skull' ? skullMap : headMap);
        ctx.restore();
      }
      for (const m of skin.marks) {
        if (m.space !== 'body') continue;
        ctx.save();
        drawPolyline(ctx, dragBodySide, true, headMap);
        ctx.clip();
        drawMark(ctx, m, 'round', dragBodyMap);
        ctx.restore();
      }
      for (const m of skin.marks) {
        if (m.space === 'head' || m.space === 'skull') {
          drawMark(ctx, m, 'round', m.space === 'skull' ? skullMap : headMap);
        }
      }
      ctx.restore();

      if (!isBlackRim) {
        ctx.save();
        ctx.strokeStyle = pal.outline ? rgbaToCss(pal.outline) : 'rgba(46,38,31,0.22)';
        ctx.lineWidth = pal.outline ? 1.5 : 1.05;
        traceLongCat(); ctx.stroke();
        ctx.restore();
      }

      // Subtle soft chin shadow curve
      ctx.save();
      ctx.strokeStyle = isBlackRim
        ? 'rgba(255,255,255,0.16)'
        : (pal.outline ? rgbaToCss(pal.outline, 0.45) : 'rgba(46,38,31,0.16)');
      ctx.lineWidth = isBlackRim ? 1.05 : 1.0;
      ctx.beginPath();
      ctx.moveTo(headCenter.x - cheekHalfW * 0.56, headCenter.y + chinY - 2.5 * hs);
      ctx.quadraticCurveTo(
        headCenter.x, headCenter.y + chinY + 3.5 * hs,
        headCenter.x + cheekHalfW * 0.56, headCenter.y + chinY - 2.5 * hs
      );
      ctx.stroke();
      ctx.restore();
    }

    // 5. Native KuroCat Face Features (normal authentic almond eyes)
    drawHeadFeatures(ctx, headCenter, state, eyeThemeName, skin, 0, 0);

    // 6. Dangling front arms along outer shoulder slopes
    drawDragLimb(frontLeftLimb);
    drawDragLimb(frontRightLimb);

    // 7. Skin stickers / decorations
    if (hasDecos) {
      drawDecorations(ctx, rumpCenterX, torsoBottomY, headCenter, state, skin, { anchors: ['neck', 'face', 'headTop'], decoAnchors });
    }

    ctx.restore();
    return { seatX: rumpCenterX, seatY: torsoBottomY, headCenter, decoAnchors };
  }

  // Spring helper & shared Douni/Drag physics stepping for both Workshop Editor & Plaza Play Modal
  function stepSpring(val, vel, target, dt, stiffness, damping) {
    const accel = (target - val) * stiffness - vel * damping;
    const nVel = vel + accel * dt;
    const nVal = val + nVel * dt;
    return [nVal, nVel];
  }

  function triggerKeyTap(state, skin, cap) {
    const chosen = cap || KEYCAPS[Math.floor(Math.random() * KEYCAPS.length)];
    state.keyGlow[chosen.id] = 1.0;

    // Track recent keystroke timestamps (2.5s rolling window) for live fast-typing anger
    const nowT = state.time || 0;
    if (!state._recentKeyTimes) state._recentKeyTimes = [];
    state._recentKeyTimes.push(nowT);

    const badgeLabel = chosen.isSpace ? 'SPACE' : chosen.legend;

    if (skin && (skin.typingStyle === 'douni' || skin.renderStyle === 'douni')) {
      const info = v2KeyCellInfo(chosen);
      const useLeft = info.prefersLeftPaw;
      state.douniSlapTimer = 0.28;
      state.douniMouseTimer = 0;
      if ((state.douniMouseBlend || 0) > 0.05) {
        state.douniMouseBlendVel = Math.min(state.douniMouseBlendVel || 0, -14.0);
      }
      state.douniLoafBlend = 0;
      const angerBoost = 1.0 + clamp(state.angerIntensity || 0, 0, 1) * 0.35;
      if (useLeft) {
        const chordOther = (nowT - (state.douniLastRightKeyTime ?? -10)) < 0.035;
        state.douniLastLeftKeyTime = nowT;
        state.douniLeftKeyTargetGoal = info.center;
        if ((state.douniLeftRaise || 0) < -0.15) {
          state.douniLeftRaise = 0.50;
          state.douniLeftRaiseVel = -15.5 * angerBoost;
        } else {
          state.douniLeftRaiseVel = -22.0 * angerBoost;
        }
        state.douniLeftRaiseTarget = -1.0;
        if (!chordOther) {
          state.douniRightRaiseTarget = 1.02;
          state.douniRightRaiseVel = Math.max((state.douniRightRaiseVel || 0) + 6.0, 17.0 * angerBoost);
        }
      } else {
        const chordOther = (nowT - (state.douniLastLeftKeyTime ?? -10)) < 0.035;
        state.douniLastRightKeyTime = nowT;
        state.douniRightKeyTargetGoal = info.center;
        if ((state.douniRightRaise || 0) < -0.15) {
          state.douniRightRaise = 0.50;
          state.douniRightRaiseVel = -15.5 * angerBoost;
        } else {
          state.douniRightRaiseVel = -22.0 * angerBoost;
        }
        state.douniRightRaiseTarget = -1.0;
        if (!chordOther) {
          state.douniLeftRaiseTarget = 1.02;
          state.douniLeftRaiseVel = Math.max((state.douniLeftRaiseVel || 0) + 6.0, 17.0 * angerBoost);
        }
      }
      state.kbImpactYVel = (state.kbImpactYVel || 0) + 24.0;
      state.kbImpactTiltVel = (state.kbImpactTiltVel || 0) + (useLeft ? -10.5 : 10.5);
      state.kbSparkTimer = 0.14;
      state.kbSparkKey = chosen.id;
      state.earTwitch = (useLeft ? -1.8 : 1.8);
      spawnBadge(state, badgeLabel, 14.0 + info.center.x * 0.405, 22.0 + info.center.y * 0.405 - 12);
      return chosen;
    }

    const su = 1 - (chosen.xUnits + chosen.widthUnits * 0.5);
    const sv = 1 - (chosen.row + 0.5) / 5;
    const pt = screenQuadPoint(su, sv);
    if (pt.x < 188) {
      state.leftPaw = { x: pt.x, y: pt.y };
      state.leftPawActive = true;
    } else {
      state.rightPaw = { x: pt.x, y: pt.y };
      state.rightPawActive = true;
    }
    state.earTwitch = (pt.x < 188 ? -0.9 : 0.9);
    spawnBadge(state, badgeLabel, pt.x, pt.y - 10);
    return chosen;
  }

  function triggerMouseClick(state, skin, isRight) {
    state.mousePress = 1.0;
    if (isRight) state.mousePressRight = 1.0;
    else state.mousePressLeft = 1.0;
    const label = isRight ? 'RMB' : 'LMB';
    if (skin && skin.renderStyle === 'douni') {
      state.douniSlapTimer = 0;
      state.douniMouseTimer = 1.25;
      spawnBadge(state, label, 82, 150);
    } else {
      state.leftPaw = { x: MOUSE_BASE.x + (state.mouseOffsetX || 0), y: MOUSE_BASE.y + (state.mouseOffsetY || 0) - 2 };
      state.leftPawActive = true;
      spawnBadge(state, label, MOUSE_BASE.x, MOUSE_BASE.y - 16);
    }
  }

  function spawnBadge(state, label, x, y) {
    if (!state.floatingBadges) state.floatingBadges = [];
    state.floatingBadges.push({
      label: String(label || ''),
      x: clamp(x, 28, 332),
      y: clamp(y, 36, 304),
      vx: (Math.random() - 0.5) * 14,
      vy: -36 - Math.random() * 10,
      rot: (Math.random() - 0.5) * 0.12,
      age: 0,
      lifetime: 0.72
    });
    if (state.floatingBadges.length > 14) {
      state.floatingBadges.splice(0, state.floatingBadges.length - 14);
    }
  }

  const KEY_CODE_TO_ID = {
    Escape: '0_0', Backquote: '0_0',
    Digit1: '0_1', Digit2: '0_2', Digit3: '0_3', Digit4: '0_4', Digit5: '0_5',
    Digit6: '0_6', Digit7: '0_7', Digit8: '0_8', Digit9: '0_9', Digit0: '0_10',
    Minus: '0_11', Equal: '0_12', Backspace: '0_13',
    Tab: '1_0',
    KeyQ: '1_1', KeyW: '1_2', KeyE: '1_3', KeyR: '1_4', KeyT: '1_5',
    KeyY: '1_6', KeyU: '1_7', KeyI: '1_8', KeyO: '1_9', KeyP: '1_10',
    BracketLeft: '1_11', BracketRight: '1_12', Backslash: '1_13',
    CapsLock: '2_0',
    KeyA: '2_1', KeyS: '2_2', KeyD: '2_3', KeyF: '2_4', KeyG: '2_5',
    KeyH: '2_6', KeyJ: '2_7', KeyK: '2_8', KeyL: '2_9',
    Semicolon: '2_10', Quote: '2_11', Enter: '2_12', NumpadEnter: '2_12',
    ShiftLeft: '3_0',
    KeyZ: '3_1', KeyX: '3_2', KeyC: '3_3', KeyV: '3_4', KeyB: '3_5',
    KeyN: '3_6', KeyM: '3_7', Comma: '3_8', Period: '3_9', Slash: '3_10',
    ShiftRight: '3_11', ArrowUp: '3_12',
    ControlLeft: '4_0', ControlRight: '4_0',
    AltLeft: '4_1', MetaLeft: '4_2', Space: '4_3',
    MetaRight: '4_4', AltRight: '4_5',
    ArrowLeft: '4_6', ArrowDown: '4_7', ArrowRight: '4_8'
  };

  function mapKeyboardEventToCap(evt) {
    if (!evt) return KEYCAPS[Math.floor(Math.random() * KEYCAPS.length)];
    const id = KEY_CODE_TO_ID[evt.code];
    if (id && KEYCAP_BY_ID.has(id)) return KEYCAP_BY_ID.get(id);
    if (evt.key && evt.key.length === 1) {
      const upper = evt.key.toUpperCase();
      const found = KEYCAPS.find((c) => c.legend === upper);
      if (found) return found;
    }
    return KEYCAPS[Math.floor(Math.random() * KEYCAPS.length)];
  }

  function findNearestKeycap(pos, skin) {
    const isDouni = skin && skin.renderStyle === 'douni';
    let best = null;
    let bestD = Infinity;
    for (const cap of KEYCAPS) {
      let cx, cy;
      if (isDouni) {
        const info = v2KeyCellInfo(cap);
        cx = 14.0 + info.center.x * 0.405;
        cy = 22.0 + info.center.y * 0.405;
      } else {
        const su = 1 - (cap.xUnits + cap.widthUnits * 0.5);
        const sv = 1 - (cap.row + 0.5) / 5;
        const pt = screenQuadPoint(su, sv);
        cx = pt.x; cy = pt.y;
      }
      const d = Math.hypot(pos.x - cx, pos.y - cy);
      if (d < bestD) { bestD = d; best = cap; }
    }
    return best;
  }

  function hitTestCat(pos, skin, state) {
    if (skin && skin.renderStyle === 'douni') {
      const loaf = clamp(state ? (state.douniLoafBlend || 0) : 0, 0, 1);
      const loafS = smoothstep(0, 1, loaf);
      const scruff = clamp(state ? (state.scruffDrag || 0) : 0, 0, 1);
      if (scruff > 0.25) {
        const d = Math.hypot((pos.x - 178) / 118, (pos.y - 165) / 96);
        return d <= 1.0 ? 'body' : 'none';
      }
      if (loafS < 0.45 && pos.x < 118 && pos.y > 100 && pos.y < 180) return 'mouse';
      // Continuously interpolate head hit zone from Video 2 desk -> Video 1 loaf so petting never drops or ping-pongs mid-morph
      const shiftX = state ? (state.bodyShiftX || 0) : 0;
      const hx = lerpNum(250 + shiftX, 151 + shiftX, loafS);
      const hy = lerpNum(132, 209, loafS);
      const hrx = lerpNum(76, 102, loafS);
      const hry = lerpNum(54, 58, loafS);
      const hdx = (pos.x - hx) / hrx;
      const hdy = (pos.y - hy) / hry;
      if (hdx * hdx + hdy * hdy <= 1.0) return 'head';
      if ((loaf > 0.03 || (state && state.petHead > 0.05)) && loaf < 0.90) {
        const v2dx = (pos.x - (250 + shiftX)) / 76;
        const v2dy = (pos.y - 132) / 56;
        if (v2dx * v2dx + v2dy * v2dy <= 1.0) return 'head';
      }
      if (loafS < 0.45 && pos.y > 180) return 'keyboard';
      const bx = lerpNum(250 + shiftX, 205 + shiftX, loafS);
      const by = lerpNum(176, 200, loafS);
      const brx = lerpNum(86, 118, loafS);
      const bry = lerpNum(62, 68, loafS);
      const bdx = (pos.x - bx) / brx;
      const bdy = (pos.y - by) / bry;
      if (bdx * bdx + bdy * bdy <= 1.0) return 'body';
      return 'none';
    }
    const mdx = (pos.x - MOUSE_BASE.x) / 22;
    const mdy = (pos.y - MOUSE_BASE.y) / 24;
    if (mdx * mdx + mdy * mdy <= 1.0) return 'mouse';
    if (pos.x >= 96 && pos.x <= 294 && pos.y >= 234 && pos.y <= 298) return 'keyboard';
    const seatX = SEAT_ANCHOR.x + (state ? state.bodyShiftX : 0);
    const bd = getBuild(skin || { shape: { build: 'standard', headScale: 1, cheekFullness: 1, bodyWidth: 1 } });
    const shape = (skin && skin.shape) || { headScale: 1, cheekFullness: 1, bodyWidth: 1 };
    const headX = seatX + (state ? state.bodyTilt * 36 + state.headYaw * 7.5 : 0);
    const headY = 126 + bd.headLift + (state ? state.headOffsetY : 0);
    const hdx = (pos.x - headX) / (46 * shape.headScale * shape.cheekFullness);
    const hdy = (pos.y - headY) / (42 * shape.headScale);
    if (hdx * hdx + hdy * hdy <= 1.0) return 'head';
    const bdx = (pos.x - seatX) / (56 * shape.bodyWidth);
    const bdy = (pos.y - 188) / 56;
    if (bdx * bdx + bdy * bdy <= 1.0) return 'body';
    return 'none';
  }

  function drawFloatingBadges(ctx, state) {
    const badges = state.floatingBadges;
    if (!badges || !badges.length) return;
    ctx.save();
    for (const b of badges) {
      const p = clamp(b.age / b.lifetime, 0, 1);
      const scale = p < 0.16 ? (0.72 + (p / 0.16) * 0.34) : (p < 0.28 ? (1.06 - ((p - 0.16) / 0.12) * 0.06) : 1.0);
      const alpha = p < 0.65 ? 1.0 : Math.max(0, 1.0 - (p - 0.65) / 0.35);
      ctx.save();
      ctx.globalAlpha = alpha;
      ctx.translate(b.x, b.y);
      ctx.rotate(b.rot);
      ctx.scale(scale, scale);
      ctx.font = '700 9.5px -apple-system, BlinkMacSystemFont, "SF Pro Text", sans-serif';
      const tw = ctx.measureText(b.label).width;
      const w = Math.max(19, tw + 11);
      const h = 17;
      ctx.fillStyle = 'rgba(34, 28, 24, 0.90)';
      roundedRectPath(ctx, -w * 0.5, -h * 0.5, w, h, 5);
      ctx.fill();
      ctx.strokeStyle = 'rgba(255, 255, 255, 0.22)';
      ctx.lineWidth = 0.85;
      ctx.stroke();
      ctx.fillStyle = '#FFFDF9';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      ctx.fillText(b.label, 0, 0.5);
      ctx.restore();
    }
    ctx.restore();
  }

  function drawSparkles(ctx, state) {
    const list = state.sparkles;
    if (!list || !list.length) return;
    ctx.save();
    for (const s of list) {
      const p = clamp(s.age / s.lifetime, 0, 1);
      const alpha = Math.sin(p * Math.PI) * 0.92;
      const r = s.size * (0.65 + Math.sin(p * Math.PI) * 0.45);
      ctx.save();
      ctx.translate(s.x, s.y);
      ctx.globalAlpha = alpha;
      ctx.fillStyle = '#F7B955';
      ctx.beginPath();
      ctx.moveTo(0, -r);
      ctx.quadraticCurveTo(0, 0, r, 0);
      ctx.quadraticCurveTo(0, 0, 0, r);
      ctx.quadraticCurveTo(0, 0, -r, 0);
      ctx.quadraticCurveTo(0, 0, 0, -r);
      ctx.closePath();
      ctx.fill();
      ctx.restore();
    }
    ctx.restore();
  }

  function updateTailHover(state, skin, canvasPt) {
    if (!state || !skin || !canvasPt || skin.renderStyle === 'douni') return;
    if ((state.scruffDrag || 0) > 0.1 || (state.lieIntensity || 0) > 0.35 || (state.dizzyIntensity || 0) > 0.25) return;
    const seatX = SEAT_ANCHOR.x + (state.bodyShiftX || 0);
    const seatY = SEAT_ANCHOR.y;
    const tc = tailCurve(seatX, seatY, state, skin);
    const samples = [tc.c1, tc.c2, tc.tip, lerpPt(tc.c1, tc.c2, 0.5), lerpPt(tc.c2, tc.tip, 0.5)];
    let bestD = Infinity;
    let bestPt = tc.tip;
    for (const p of samples) {
      const d = Math.hypot(canvasPt.x - p.x, canvasPt.y - p.y);
      if (d < bestD) { bestD = d; bestPt = p; }
    }
    if (bestD < 24) {
      const now = state.time || 0;
      if (now - (state._lastTailBrush || -10) > 0.12) {
        state._lastTailBrush = now;
        const dx = bestPt.x - canvasPt.x;
        const dy = bestPt.y - canvasPt.y;
        const n = Math.max(1, Math.hypot(dx, dy));
        state.tailFlickXVel = (state.tailFlickXVel || 0) + (dx / n) * 145;
        state.tailFlickYVel = (state.tailFlickYVel || 0) + (dy / n - 0.55) * 145;
        state.tailCurlVel = (state.tailCurlVel || 0) + (Math.random() > 0.5 ? 1 : -1) * 5.2;
        state.tailAgility = Math.min(1.0, (state.tailAgility || 0) + 0.55);
      }
    }
  }

  function stepPhysicsExtras(state, skin, pose, dt) {
    const isDouni = skin && skin.renderStyle === 'douni';
    const isDouniTyping = skin && (skin.typingStyle === 'douni' || isDouni);
    const nowT = state.time || 0;

    // 1. Scruff-drag state (2D inertia + lagging tail whip for ALL skins!)
    const wantDrag = pose === 'drag';
    if (wantDrag) {
      if (!state.isScruffDragging) {
        state.scruffHoldTime = 0;
        state.scruffDropTimer = 0;
      }
      state.isScruffDragging = true;
      state.scruffHoldTime = (state.scruffHoldTime || 0) + dt;
    } else {
      if (state.isScruffDragging) {
        state.scruffDropTimer = 0.16;
      }
      state.isScruffDragging = false;
      state.scruffDropTimer = Math.max(0, (state.scruffDropTimer || 0) - dt);
      if ((state.scruffDrag || 0) < 0.02) state.scruffHoldTime = 0;
    }
    const [sd, sdv] = stepSpring(state.scruffDrag || 0, state.scruffDragVel || 0, wantDrag ? 1.0 : 0.0, dt, 190, 17);
    state.scruffDrag = clamp(sd, 0, 1.05);
    state.scruffDragVel = sdv;
    const targetSwingX = wantDrag
      ? clamp((state.dragSwingTarget || 0) + Math.sin(nowT * 2.6) * 0.08, -0.92, 0.92)
      : 0;
    const [sw, swv] = stepSpring(state.scruffSwing || 0, state.scruffSwingVel || 0, targetSwingX, dt, 115, 9.5);
    state.scruffSwing = sw;
    state.scruffSwingVel = swv;
    const targetSwingY = wantDrag
      ? clamp((state.dragSwingYTarget || 0) + Math.cos(nowT * 2.1) * 0.04, -0.78, 0.78)
      : 0;
    const [swY, swYv] = stepSpring(state.scruffSwingY || 0, state.scruffSwingYVel || 0, targetSwingY, dt, 108, 8.4);
    state.scruffSwingY = swY;
    state.scruffSwingYVel = swYv;
    const targetTailSwing = wantDrag
      ? clamp((state.dragSwingTarget || 0) * 1.35 + Math.sin(nowT * 2.6 - 0.5) * 0.14, -1.2, 1.2)
      : 0;
    const [swT, swTv] = stepSpring(state.scruffTailSwing || 0, state.scruffTailSwingVel || 0, targetTailSwing, dt, 58, 5.2);
    state.scruffTailSwing = swT;
    state.scruffTailSwingVel = swTv;
    state.dragSwingTarget = lerpNum(state.dragSwingTarget || 0, 0, Math.min(1, dt * 7));
    state.dragSwingYTarget = lerpNum(state.dragSwingYTarget || 0, 0, Math.min(1, dt * 7));
    state.mousePress = Math.max(0, (state.mousePress || 0) - dt * 6.5);
    state.mousePressLeft = Math.max(0, (state.mousePressLeft || 0) - dt * 6.5);
    state.mousePressRight = Math.max(0, (state.mousePressRight || 0) - dt * 6.5);

    // Step floating keycap badges
    if (state.floatingBadges && state.floatingBadges.length) {
      for (let i = state.floatingBadges.length - 1; i >= 0; i--) {
        const b = state.floatingBadges[i];
        b.age += dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.vy *= Math.max(0, 1 - dt * 1.4);
        if (b.age >= b.lifetime) state.floatingBadges.splice(i, 1);
      }
    }

    // Spawn & step petting sparkles
    if (!state.sparkles) state.sparkles = [];
    if ((state.petHead || 0) > 0.35 && !wantDrag) {
      state._sparkleCd = (state._sparkleCd || 0) - dt;
      if (state._sparkleCd <= 0) {
        state._sparkleCd = 0.22 + Math.random() * 0.16;
        state.sparkles.push({
          x: 184 + (Math.random() - 0.5) * 86,
          y: 94 + (Math.random() - 0.5) * 32,
          vy: -18 - Math.random() * 14,
          size: 4.5 + Math.random() * 3.5,
          age: 0,
          lifetime: 0.85
        });
      }
    }
    for (let i = state.sparkles.length - 1; i >= 0; i--) {
      const s = state.sparkles[i];
      s.age += dt;
      s.y += s.vy * dt;
      if (s.age >= s.lifetime) state.sparkles.splice(i, 1);
    }

    // 2. Expressive states: Anger (fast typing >= 240 KPM / pose='anger'), Dizzy (rapid shake 3.6s / pose='dizzy'), Cat-Ball Sleep & Yawn, Tail Hover & 4-Key Tail Typing, Tablet & RGB Marquee
    if (state._recentKeyTimes && state._recentKeyTimes.length) {
      while (state._recentKeyTimes.length && nowT - state._recentKeyTimes[0] > 2.5) {
        state._recentKeyTimes.shift();
      }
    }
    const recentKpm = ((state._recentKeyTimes ? state._recentKeyTimes.length : 0) / 2.5) * 60.0;
    state.marqueePhase = ((state.marqueePhase || 0) + dt * (0.26 + Math.min(0.65, recentKpm / 360))) % 1.0;

    const autoAnger = (!wantDrag && (state.dizzyIntensity || 0) < 0.25 && recentKpm >= 240)
      ? clamp((recentKpm - 240.0) / 140.0, 0, 1) : 0;
    const angerTarget = (pose === 'anger' || (isDouni && pose === 'hindFoot')) ? 1.0 : autoAnger;
    const [angV, angVel] = stepSpring(state.angerIntensity || 0, state.angerIntensityVel || 0, angerTarget, dt, 70, 14);
    state.angerIntensity = clamp(angV, 0, 1);
    state.angerIntensityVel = angVel;

    if (wantDrag) {
      state._dizzyTimer = 0;
      state._fastMouseDuration = 0;
    } else if ((state._dizzyTimer || 0) <= 0 && (state.huntIntensity || 0) > 0.42 && pose !== 'hunt') {
      state._fastMouseDuration = (state._fastMouseDuration || 0) + dt;
      if (state._fastMouseDuration >= 3.6) {
        state._dizzyTimer = 5.5;
        state._fastMouseDuration = 0;
      }
    } else if ((state._dizzyTimer || 0) <= 0) {
      state._fastMouseDuration = Math.max(0, (state._fastMouseDuration || 0) - dt * 1.4);
    }
    let autoDizzy = 0;
    if ((state._dizzyTimer || 0) > 0) {
      state._dizzyTimer = Math.max(0, state._dizzyTimer - dt);
      autoDizzy = 1.0;
    } else if ((state._fastMouseDuration || 0) > 1.8) {
      autoDizzy = clamp((state._fastMouseDuration - 1.8) / 1.8, 0, 1) * 0.48;
    }
    const dizzyTarget = pose === 'dizzy' ? 1.0 : autoDizzy;
    const [dzV, dzVel] = stepSpring(state.dizzyIntensity || 0, state.dizzyIntensityVel || 0, dizzyTarget, dt, 75, 14);
    state.dizzyIntensity = clamp(dzV, 0, 1);
    state.dizzyIntensityVel = dzVel;
    if (state.dizzyIntensity > 0.01) {
      state.dizzyPhase = (state.dizzyPhase || 0) + dt * 7.5;
    }

    // Curled-up cat-ball sleep (lieIntensity) & pre-sleep yawn (yawnAmount)
    const sleepVal = clamp(state.sleepiness || 0, 0, 1);
    const lieTarget = (!wantDrag && state.dizzyIntensity < 0.15)
      ? (pose === 'sleep' ? 1.0 : clamp((sleepVal - 0.55) / 0.35, 0, 1))
      : 0;
    const [liV, liVel] = stepSpring(state.lieIntensity || 0, state.lieIntensityVel || 0, lieTarget, dt, 55, 14);
    state.lieIntensity = clamp(liV, 0, 1);
    state.lieIntensityVel = liVel;

    if (pose === 'yawn') {
      state.yawnAmount = lerpNum(state.yawnAmount || 0, 0.92, Math.min(1, dt * 8));
    } else {
      if (!wantDrag && state.dizzyIntensity < 0.15 && sleepVal > 0.28 && sleepVal < 0.65 && !state._yawnedThisNap) {
        state._yawnTimer = 2.0;
        state._yawnedThisNap = true;
      }
      if (sleepVal < 0.12) state._yawnedThisNap = false;
      if ((state._yawnTimer || 0) > 0) {
        state._yawnTimer = Math.max(0, state._yawnTimer - dt);
        const elapsed = 2.0 - state._yawnTimer;
        let yv = 0;
        if (elapsed < 0.65) yv = smoothstep(0, 0.65, elapsed);
        else if (elapsed < 1.55) yv = 1.0;
        else yv = 1.0 - smoothstep(1.55, 2.0, elapsed);
        state.yawnAmount = yv;
      } else {
        state.yawnAmount = lerpNum(state.yawnAmount || 0, 0, Math.min(1, dt * 10));
      }
    }

    // 4-key tail typing + playful tail hover flick springs
    const tailTypeTarget = (!wantDrag && state.dizzyIntensity < 0.25 && state.lieIntensity < 0.35)
      ? (state.tailTypingTarget || 0) : 0;
    const [ttV, ttVel] = stepSpring(state.tailTyping || 0, state.tailTypingVel || 0, tailTypeTarget, dt, 180, 17);
    state.tailTyping = clamp(ttV, 0, 1);
    state.tailTypingVel = ttVel;
    if (state.tailTipGoal) {
      const curTip = state.tailTip || { x: 256, y: 248 };
      const kTip = Math.min(1, dt * 22);
      state.tailTip = {
        x: lerpNum(curTip.x, state.tailTipGoal.x, kTip),
        y: lerpNum(curTip.y, state.tailTipGoal.y, kTip)
      };
    }
    const [tfx, tfxv] = stepSpring(state.tailFlickX || 0, state.tailFlickXVel || 0, 0, dt, 140, 11);
    state.tailFlickX = clamp(tfx, -26, 26); state.tailFlickXVel = tfxv;
    const [tfy, tfyv] = stepSpring(state.tailFlickY || 0, state.tailFlickYVel || 0, 0, dt, 140, 11);
    state.tailFlickY = clamp(tfy, -26, 26); state.tailFlickYVel = tfyv;
    const [tcV, tcVel] = stepSpring(state.tailCurl || 0, state.tailCurlVel || 0, 0, dt, 90, 9.5);
    state.tailCurl = clamp(tcV, -1.2, 1.2); state.tailCurlVel = tcVel;
    state.tailAgility = Math.max(0, (state.tailAgility || 0) - dt * 0.7);

    // 2.5D Graphics Tablet & Stylus demo (both standard skins and Douni!)
    if (pose === 'tablet') {
      state.isTabletActive = true;
      state._poseTablet = true;
      const tu = 0.50 + Math.sin(nowT * 2.3) * 0.34;
      const tv = 0.50 + Math.sin(nowT * 4.6) * 0.28;
      state.tabletNib = isDouni
        ? { x: 18 + tu * 74, y: 235 + tv * 52 }
        : tabletQuadPoint(tu, tv);
      state.tabletPenPress = 0.72 + Math.sin(nowT * 3.1) * 0.22;
      state.tabletPenTilt = Math.cos(nowT * 2.3) * 0.14;
      if (!isDouni) {
        state.leftPaw = { x: state.tabletNib.x + 5.5, y: state.tabletNib.y - 7.0 };
        state.leftPawActive = true;
      }
      if (!state.tabletTrail) state.tabletTrail = [];
      state.tabletTrail.push({ x: state.tabletNib.x, y: state.tabletNib.y, age: 0, pressure: state.tabletPenPress });
      if (state.tabletTrail.length > 48) state.tabletTrail.splice(0, state.tabletTrail.length - 48);
    } else if (state._poseTablet) {
      state.isTabletActive = false;
      state._poseTablet = false;
    }
    if (state.tabletTrail && state.tabletTrail.length) {
      for (let i = state.tabletTrail.length - 1; i >= 0; i--) {
        state.tabletTrail[i].age += dt;
        if (state.tabletTrail[i].age >= 0.72) state.tabletTrail.splice(i, 1);
      }
    }

    // 3. Douni slap springs, key target springs, keyboard recoil/shake & sparks
    if (isDouniTyping) {
      state.douniSlapTimer = Math.max(0, (state.douniSlapTimer || 0) - dt);
      if (state.douniSlapTimer <= 0.08 && pose !== 'typing' && pose !== 'hindFoot') {
        state.douniLeftRaiseTarget = 0;
        state.douniRightRaiseTarget = 0;
      }
      const [lr, lrv] = stepSpring(state.douniLeftRaise || 0, state.douniLeftRaiseVel || 0, state.douniLeftRaiseTarget || 0, dt, 680, 25);
      state.douniLeftRaise = lr; state.douniLeftRaiseVel = lrv;
      const [rr, rrv] = stepSpring(state.douniRightRaise || 0, state.douniRightRaiseVel || 0, state.douniRightRaiseTarget || 0, dt, 680, 25);
      state.douniRightRaise = rr; state.douniRightRaiseVel = rrv;

      const lGoal = state.douniLeftKeyTargetGoal || { x: 274, y: 412 };
      const rGoal = state.douniRightKeyTargetGoal || { x: 406, y: 414 };
      const lCur = state.douniLeftKeyTarget || { x: 274, y: 412 };
      const rCur = state.douniRightKeyTarget || { x: 406, y: 414 };
      const kt = Math.min(1, dt * 28);
      state.douniLeftKeyTarget = { x: lerpNum(lCur.x, lGoal.x, kt), y: lerpNum(lCur.y, lGoal.y, kt) };
      state.douniRightKeyTarget = { x: lerpNum(rCur.x, rGoal.x, kt), y: lerpNum(rCur.y, rGoal.y, kt) };

      const [iy, iyv] = stepSpring(state.kbImpactY || 0, state.kbImpactYVel || 0, 0, dt, 560, 16);
      state.kbImpactY = iy; state.kbImpactYVel = iyv;
      const [it, itv] = stepSpring(state.kbImpactTilt || 0, state.kbImpactTiltVel || 0, 0, dt, 500, 14);
      state.kbImpactTilt = it; state.kbImpactTiltVel = itv;
      state.kbSparkTimer = Math.max(0, (state.kbSparkTimer || 0) - dt);
    } else {
      state.douniSlapTimer = 0;
      state.douniLeftRaise = 0; state.douniLeftRaiseVel = 0; state.douniLeftRaiseTarget = 0;
      state.douniRightRaise = 0; state.douniRightRaiseVel = 0; state.douniRightRaiseTarget = 0;
      state.kbImpactY = 0; state.kbImpactYVel = 0;
      state.kbImpactTilt = 0; state.kbImpactTiltVel = 0;
      state.kbSparkTimer = 0;
    }

    // 4. Douni Video 1 loaf blend & comic mouse/tablet blend
    if (isDouni) {
      state.douniLoafAnimated = true;
      const wantsLoaf = !wantDrag && state.scruffDrag < 0.08 && (state.petHead > 0.15 || state.petBody > 0.25 || state.sleepiness > 0.42);
      const [lb, lbv] = stepSpring(state.douniLoafBlend || 0, state.douniLoafBlendVel || 0, wantsLoaf ? 1.0 : 0.0, dt, 115, 16);
      state.douniLoafBlend = clamp(lb, 0, 1);
      state.douniLoafBlendVel = lbv;

      const wantsMouse = !wantDrag && !wantsLoaf && state.douniSlapTimer <= 0.03 && ((state.douniMouseTimer || 0) > 0 || pose === 'tablet');
      state.douniMouseTimer = Math.max(0, (state.douniMouseTimer || 0) - dt);
      const [mb, mbv] = stepSpring(state.douniMouseBlend || 0, state.douniMouseBlendVel || 0, wantsMouse ? 1.0 : 0.0, dt, 260, 22);
      state.douniMouseBlend = clamp(mb, 0, 1);
      state.douniMouseBlendVel = mbv;
    } else {
      state.douniLoafBlend = 0; state.douniLoafBlendVel = 0;
      state.douniMouseBlend = 0; state.douniMouseBlendVel = 0;
    }
  }

  function drawScene(ctx, skin, state, options) {
    const opts = options || {};
    const kbTheme = opts.kbTheme || 'classicMilk';
    const eyeTheme = opts.eyeTheme || (skin.colorpoint ? 'siameseBlue' : 'neonChartreuse');
    stateWhiteOutline = opts.whiteOutline !== false;

    ctx.save();
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    const scaleX = ctx.canvas.width / 360;
    const scaleY = ctx.canvas.height / 320;
    ctx.scale(scaleX, scaleY);

    if (skin.renderStyle === 'douni') {
      const xforms = drawDouniScene(ctx, skin, state, kbTheme, eyeTheme);
      drawSparkles(ctx, state);
      drawFloatingBadges(ctx, state);
      if (typeof opts.editDecorationIndex === 'number' && opts.editDecorationIndex >= 0 &&
          skin.decorations && skin.decorations[opts.editDecorationIndex]) {
        drawDecorationOverlay(ctx, skin.decorations[opts.editDecorationIndex], skin, xforms.seatX, xforms.seatY, xforms.headCenter, state, xforms.decoAnchors);
      }
      ctx.restore();
      return xforms;
    }

    const scruff = clamp(state.scruffDrag || 0, 0, 1);
    if (scruff > 0.005) {
      const kbFade = clamp(1.0 - scruff * 2.6, 0, 1);
      if (kbFade > 0.01) {
        ctx.save();
        ctx.globalAlpha *= kbFade;
        if (state.isTabletActive) {
          drawTablet(ctx, state, kbTheme, eyeTheme);
          drawParkedStylus(ctx, kbTheme);
        } else {
          drawMouse(ctx, state, kbTheme);
        }
        drawKeyboard(ctx, state, kbTheme, eyeTheme);
        ctx.restore();
      }
      const xforms = drawNativeScruffDragCat(ctx, state, skin, eyeTheme, scruff);
      drawSparkles(ctx, state);
      drawFloatingBadges(ctx, state);
      if (typeof opts.editDecorationIndex === 'number' && opts.editDecorationIndex >= 0 &&
          skin.decorations && skin.decorations[opts.editDecorationIndex]) {
        drawDecorationOverlay(ctx, skin.decorations[opts.editDecorationIndex], skin, xforms.seatX, xforms.seatY, xforms.headCenter, state, xforms.decoAnchors);
      }
      ctx.restore();
      return xforms;
    }

    const breath = Math.sin(state.time * 2.1) * 1.6;
    const lie = clamp(state.lieIntensity || 0, 0, 1);
    const lieS = lie * lie * (3.0 - 2.0 * lie);
    const dizzy = clamp(state.dizzyIntensity || 0, 0, 1);
    const dizzyFallRaw = clamp((dizzy - 0.50) / 0.50, 0, 1);
    const dizzyFall = dizzyFallRaw * dizzyFallRaw * (3.0 - 2.0 * dizzyFallRaw);
    const preWobble = clamp((dizzy - 0.12) / 0.38, 0, 1) * (1.0 - dizzyFall);

    const seatX = SEAT_ANCHOR.x + state.bodyShiftX;
    const seatY = SEAT_ANCHOR.y + lieS * 10.0;
    const bd = getBuild(skin);
    const headCenter = {
      x: seatX + state.bodyTilt * 36 + state.headYaw * 7.5 - lieS * 4.0,
      y: 126 + bd.headLift + state.headOffsetY + state.headPitch * 4.8 + breath * 1.1 + lieS * 60.0
    };
    const hasDecos = skin.decorations && skin.decorations.length > 0;
    const holdingStylus = !!(state.isTabletActive && state.leftPawActive && dizzy <= 0.35 && lie <= 0.45);

    if (state.isTabletActive) {
      drawTablet(ctx, state, kbTheme, eyeTheme);
      if (!holdingStylus) drawParkedStylus(ctx, kbTheme);
    } else {
      drawMouse(ctx, state, kbTheme);
    }
    drawKeyboard(ctx, state, kbTheme, eyeTheme);

    // 2-stage dizzy transform: wobble -> 90-deg sideways collapse onto the keyboard
    const fallAngle = -dizzyFall * 1.46 + Math.sin(state.time * 9.5) * 0.16 * preWobble;
    const fallShiftX = dizzyFall * 56.0;
    const fallShiftY = dizzyFall * 20.0;
    const tailInFront = (state.tailTyping || 0) > 0.05 || lieS >= 0.45;

    ctx.save();
    if (Math.abs(fallAngle) > 0.001 || Math.abs(fallShiftX) > 0.1 || Math.abs(fallShiftY) > 0.1) {
      ctx.translate(seatX + fallShiftX, seatY + fallShiftY);
      ctx.rotate(fallAngle);
      ctx.translate(-seatX, -seatY);
    }

    const drawEarsLayer = (outlineOnly) => {
      ctx.save();
      ctx.translate(headCenter.x, headCenter.y);
      ctx.rotate(state.headTilt);
      if (skin.shape.headScale !== 1) ctx.scale(skin.shape.headScale, skin.shape.headScale);
      drawEar3D(ctx, true, state.headYaw * 0.50, state.headPitch * 0.36, state.earFold, state.earTwitch * 0.045, skin, outlineOnly);
      drawEar3D(ctx, false, state.headYaw * 0.50, state.headPitch * 0.36, state.earFold, -state.earTwitch * 0.045, skin, outlineOnly);
      ctx.restore();
    };

    const drawHindFootLayer = (outlineOnly) => {
      if (state.hindFootActive <= 0.03 || dizzy >= 0.45 || lie >= 0.45) return;
      const hip = { x: seatX + 26 * skin.shape.bodyWidth, y: seatY + 2 };
      const target = state.hindFoot;
      const mid = { x: (hip.x + target.x) * 0.5 + 10, y: (hip.y + target.y) * 0.5 - 8 };
      drawLimb(ctx, hip, mid, target, 15.0, 12.0, true, 0.85 * state.hindFootActive, skin, false, outlineOnly);
    };

    // Pass 1: Universal sticker-style outer white outline for ALL skins
    // Outline connected parts before fills so the foreground sleeping tail root and reaching shoulders have no white seam (synced with KuroCat PR #22)
    if (stateWhiteOutline) {
      drawTail(ctx, seatX, seatY, state, skin, true);
      drawEarsLayer(true);
      drawUnifiedSilhouette(ctx, seatX, seatY, headCenter, breath, state, skin, true);
      drawHindFootLayer(true);
      drawFrontPaws(ctx, seatX, seatY, breath, state, skin, true);
    }

    if (isDazedSkin(skin)) {
      drawDazedSword(ctx, seatX, seatY, state);
    }

    if (!tailInFront) {
      drawTail(ctx, seatX, seatY, state, skin, false);
      if (hasDecos) {
        drawDecorations(ctx, seatX, seatY, headCenter, state, skin, { anchors: ['tailTip'] });
      }
    }

    // 3D ears
    drawEarsLayer(false);

    drawUnifiedSilhouette(ctx, seatX, seatY, headCenter, breath, state, skin, false);

    drawHindFootLayer(false);

    // Front arms/paws emerge from shoulders underneath neck accessories (collar/scarf/bowtie)
    drawFrontPaws(ctx, seatX, seatY, breath, state, skin);
    if (holdingStylus) {
      drawHeldStylus(ctx, state, kbTheme);
    }

    // Neck decorations over chest & arm roots, behind face features
    if (hasDecos) {
      drawDecorations(ctx, seatX, seatY, headCenter, state, skin, { anchors: ['neck'] });
    }

    drawHeadFeatures(ctx, headCenter, state, eyeTheme, skin);
    // Face & head-top decorations cleanly above head, ears & features
    if (hasDecos) {
      drawDecorations(ctx, seatX, seatY, headCenter, state, skin, { anchors: ['face', 'headTop'] });
    }

    if (tailInFront) {
      drawTail(ctx, seatX, seatY, state, skin, false);
      if (hasDecos) {
        drawDecorations(ctx, seatX, seatY, headCenter, state, skin, { anchors: ['tailTip'] });
      }
    }

    ctx.restore(); // end dizzy transform

    // Upright orbiting golden stars above the rotated head when dizzy!
    if (dizzy > 0.10) {
      const dxH = headCenter.x - seatX;
      const dyH = headCenter.y - seatY;
      const cosA = Math.cos(fallAngle);
      const sinA = Math.sin(fallAngle);
      const worldHeadX = seatX + fallShiftX + dxH * cosA - dyH * sinA;
      const worldHeadY = seatY + fallShiftY + dxH * sinA + dyH * cosA;
      const starAlpha = clamp((dizzy - 0.10) / 0.25, 0, 1);
      const orbitCenterX = worldHeadX;
      const orbitCenterY = worldHeadY - 48.0 - dizzyFall * 4.0;
      const orbitRx = 32.0;
      const orbitRy = 10.0;
      ctx.save();
      for (let i = 0; i < 3; i++) {
        const ang = (state.dizzyPhase || 0) * 0.72 + i * ((Math.PI * 2) / 3);
        const sx = orbitCenterX + Math.cos(ang) * orbitRx;
        const sy = orbitCenterY + Math.sin(ang) * orbitRy;
        const depthScale = 0.76 + 0.28 * (Math.sin(ang) * 0.5 + 0.5);
        const r = 6.2 * depthScale;
        ctx.save();
        ctx.translate(sx, sy);
        ctx.rotate((state.dizzyPhase || 0) * 0.55 + i);
        ctx.beginPath();
        for (let k = 0; k < 8; k++) {
          const rad = (k % 2 === 0) ? r : r * 0.42;
          const a = (k / 8) * Math.PI * 2;
          const px = Math.cos(a) * rad, py = Math.sin(a) * rad;
          if (k === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
        }
        ctx.closePath();
        ctx.fillStyle = `rgba(255,219,82,${(0.95 * starAlpha).toFixed(2)})`;
        ctx.fill();
        ctx.strokeStyle = `rgba(214,132,30,${(0.90 * starAlpha).toFixed(2)})`;
        ctx.lineWidth = 1.0;
        ctx.stroke();
        ctx.restore();
      }
      ctx.restore();
    }

    drawSparkles(ctx, state);
    drawFloatingBadges(ctx, state);

    // Sleep Z or petting sparkles
    if ((state.sleepiness > 0.5 || lie > 0.5) && dizzy < 0.2) {
      const zPhase = (state.time * 0.7) % 1;
      const zPhase2 = (state.time * 0.7 + 0.5) % 1;
      ctx.save();
      ctx.fillStyle = `rgba(226,139,66,${Math.sin(zPhase * Math.PI).toFixed(2)})`;
      ctx.font = `700 ${11 + zPhase * 4}px -apple-system, sans-serif`;
      ctx.fillText('z', headCenter.x + 34 + zPhase * 12, headCenter.y - 34 - zPhase * 22);
      ctx.fillStyle = `rgba(226,139,66,${(Math.sin(zPhase2 * Math.PI) * 0.82).toFixed(2)})`;
      ctx.font = `700 ${13 + zPhase2 * 5}px -apple-system, sans-serif`;
      ctx.fillText('Z', headCenter.x + 44 + zPhase2 * 15, headCenter.y - 44 - zPhase2 * 26);
      ctx.restore();
    }

    // Optional on-canvas Bezier anchor overlay when editing a region
    if (opts.editRegion && opts.editRegion.anchors) {
      drawRegionOverlay(ctx, opts.editRegion, skin, seatX, seatY, headCenter, state);
    }
    if (opts.pen) {
      drawPenOverlay(ctx, opts.pen, skin, seatX, seatY, headCenter, state);
    }
    if (typeof opts.editDecorationIndex === 'number' && opts.editDecorationIndex >= 0 &&
        skin.decorations && skin.decorations[opts.editDecorationIndex]) {
      drawDecorationOverlay(ctx, skin.decorations[opts.editDecorationIndex], skin, seatX, seatY, headCenter, state);
    }

    ctx.restore();
    return { seatX, seatY, headCenter };
  }

  function mapPointToCanvas(pt, space, skin, seatX, seatY, headCenter, state) {
    if (space === 'body') return bodyPoint(pt, skin, seatX, seatY, state);
    if (space === 'skull') return headPoint(pt, true, skin, headCenter, state);
    if (space === 'head') return headPoint(pt, false, skin, headCenter, state);
    if (space === 'tail') return tailCurve(seatX, seatY, state, skin).at(pt);
    return { x: headCenter.x + pt.x, y: headCenter.y + pt.y };
  }

  // canvas -> tail space: nearest station on the tail's centre curve, signed normal offset / thickness
  function mapCanvasToTail(canvasPt, skin, seatX, seatY, state) {
    const curve = tailCurve(seatX, seatY, state, skin);
    let best = null;
    for (let i = 0; i <= 200; i++) {
      const x = i * 0.5;
      const p = curve.at({ x, y: 0 });
      const d = (p.x - canvasPt.x) ** 2 + (p.y - canvasPt.y) ** 2;
      if (!best || d < best.d) best = { x, d, p };
    }
    const q = curve.at({ x: best.x, y: 1 });
    const nx = q.x - best.p.x, ny = q.y - best.p.y;
    const nn = Math.max(1e-6, nx * nx + ny * ny);
    return { x: best.x, y: ((canvasPt.x - best.p.x) * nx + (canvasPt.y - best.p.y) * ny) / nn };
  }

  function mapCanvasToSpace(canvasPt, space, skin, seatX, seatY, headCenter, state) {
    if (space === 'tail' && state) return mapCanvasToTail(canvasPt, skin, seatX, seatY, state);
    if (space === 'body') {
      return {
        x: (canvasPt.x - seatX) / skin.shape.bodyWidth,
        y: canvasPt.y - seatY
      };
    }
    return {
      x: (canvasPt.x - headCenter.x) / skin.shape.headScale,
      y: (canvasPt.y - headCenter.y) / skin.shape.headScale
    };
  }

  function drawRegionOverlay(ctx, rawRegion, skin, seatX, seatY, headCenter, state) {
    const map = (p) => mapPointToCanvas(p, rawRegion.space, skin, seatX, seatY, headCenter, state);
    const flat = global.KuroSpec.flattenRegion(rawRegion.anchors);

    ctx.save();
    drawPolyline(ctx, flat, true, map);
    ctx.strokeStyle = '#E28B42';
    ctx.lineWidth = 1.6;
    ctx.setLineDash([4, 3]);
    ctx.stroke();
    ctx.setLineDash([]);

    rawRegion.anchors.forEach((a) => {
      const p = map(a);
      const pIn = map({ x: a.inX !== undefined ? a.inX : a.x, y: a.inY !== undefined ? a.inY : a.y });
      const pOut = map({ x: a.outX !== undefined ? a.outX : a.x, y: a.outY !== undefined ? a.outY : a.y });

      ctx.strokeStyle = 'rgba(226,139,66,0.75)';
      ctx.lineWidth = 1.0;
      ctx.beginPath();
      ctx.moveTo(pIn.x, pIn.y);
      ctx.lineTo(p.x, p.y);
      ctx.lineTo(pOut.x, pOut.y);
      ctx.stroke();

      // Handles (small diamonds)
      for (const h of [pIn, pOut]) {
        if (Math.hypot(h.x - p.x, h.y - p.y) > 2) {
          ctx.fillStyle = '#FFFFFF';
          ctx.strokeStyle = '#E28B42';
          ctx.lineWidth = 1.3;
          ctx.beginPath();
          ctx.rect(h.x - 3, h.y - 3, 6, 6);
          ctx.fill();
          ctx.stroke();
        }
      }

      // Main anchor circle
      ctx.fillStyle = '#E28B42';
      ctx.strokeStyle = '#FFFFFF';
      ctx.lineWidth = 1.6;
      ctx.beginPath();
      ctx.arc(p.x, p.y, 4.6, 0, Math.PI * 2);
      ctx.fill();
      ctx.stroke();
    });
    ctx.restore();
  }

  // Pen tool overlay: the curve being drawn, its anchors and handles, a rubber band to the cursor and,
  // for a split curve, dashed extensions to where the side regions close.
  function drawPenOverlay(ctx, pen, skin, seatX, seatY, headCenter, state) {
    const map = (p) => mapPointToCanvas(p, pen.space, skin, seatX, seatY, headCenter, state);
    const A = pen.anchors || [];
    const strokeTwice = (width) => {
      ctx.strokeStyle = 'rgba(255,255,255,0.95)';
      ctx.lineWidth = width + 2.2;
      ctx.stroke();
      ctx.strokeStyle = '#E28B42';
      ctx.lineWidth = width;
      ctx.stroke();
    };
    ctx.save();
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    if (pen.freehand && pen.freehand.length >= 2) {
      drawPolyline(ctx, pen.freehand, pen.freehandClosed === true, map);
      ctx.setLineDash([4, 3]);
      strokeTwice(1.5);
      if (pen.mirror) {
        const mir = pen.freehand.map((p) => ({ x: -p.x, y: p.y }));
        drawPolyline(ctx, mir, pen.freehandClosed === true, map);
        ctx.strokeStyle = 'rgba(226,139,66,0.7)';
        ctx.lineWidth = 1.3;
        ctx.stroke();
      }
      ctx.setLineDash([]);
      ctx.restore();
      return;
    }
    if (pen.ends && pen.space !== 'tail') {
      ctx.setLineDash([3, 4]);
      for (const e of pen.ends) {
        const a = map(e.from), b = map(e.to);
        ctx.beginPath();
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
        ctx.strokeStyle = 'rgba(226,139,66,0.55)';
        ctx.lineWidth = 1.2;
        ctx.stroke();
      }
      ctx.setLineDash([]);
    }
    if (pen.mirror && A.length >= 2) {
      // the mirrored half that will be added on the other side
      drawPolyline(ctx, global.KuroSpec.flattenOpen(global.KuroSpec.mirrorAnchors(A)), false, map);
      ctx.setLineDash([5, 4]);
      ctx.strokeStyle = 'rgba(255,255,255,0.85)';
      ctx.lineWidth = 3;
      ctx.stroke();
      ctx.strokeStyle = 'rgba(226,139,66,0.7)';
      ctx.lineWidth = 1.4;
      ctx.stroke();
      ctx.setLineDash([]);
    }
    if (A.length >= 2) {
      const flat = global.KuroSpec.flattenOpen(A);
      drawPolyline(ctx, flat, pen.closedPreview === true, map);
      strokeTwice(1.8);
    }
    if (pen.stage === 'draw' && pen.hover && A.length) {
      const last = map(A[A.length - 1]);
      ctx.setLineDash([4, 3]);
      ctx.beginPath();
      ctx.moveTo(last.x, last.y);
      ctx.lineTo(pen.hover.x, pen.hover.y);
      ctx.strokeStyle = 'rgba(226,139,66,0.85)';
      ctx.lineWidth = 1.3;
      ctx.stroke();
      ctx.setLineDash([]);
    }
    A.forEach((a, i) => {
      const p = map(a);
      for (const key of ['in', 'out']) {
        const hx = a[key + 'X'], hy = a[key + 'Y'];
        if (hx === undefined || Math.hypot(hx - a.x, hy - a.y) < 0.3) continue;
        const h = map({ x: hx, y: hy });
        ctx.beginPath();
        ctx.moveTo(p.x, p.y);
        ctx.lineTo(h.x, h.y);
        ctx.strokeStyle = 'rgba(226,139,66,0.8)';
        ctx.lineWidth = 1;
        ctx.stroke();
        ctx.fillStyle = '#FFFFFF';
        ctx.strokeStyle = '#E28B42';
        ctx.lineWidth = 1.3;
        ctx.beginPath();
        ctx.rect(h.x - 2.8, h.y - 2.8, 5.6, 5.6);
        ctx.fill();
        ctx.stroke();
      }
      const first = i === 0;
      if (first && pen.closeHint) {
        ctx.beginPath();
        ctx.arc(p.x, p.y, 8.5, 0, Math.PI * 2);
        ctx.strokeStyle = '#E28B42';
        ctx.lineWidth = 1.6;
        ctx.stroke();
      }
      ctx.beginPath();
      ctx.arc(p.x, p.y, first ? 4.8 : 4.0, 0, Math.PI * 2);
      ctx.fillStyle = first ? '#FFFFFF' : '#E28B42';
      ctx.strokeStyle = first ? '#E28B42' : '#FFFFFF';
      ctx.lineWidth = 1.7;
      ctx.fill();
      ctx.stroke();
    });
    ctx.restore();
  }

  // --- Decorations (anchored PNG stickers, exact parity with native CatRenderer.drawDecorations) ---
  const decoImageCache = new Map();

  function getDecorationImage(pngB64) {
    if (!pngB64) return null;
    let entry = decoImageCache.get(pngB64);
    if (!entry) {
      const img = new Image();
      entry = { img, loaded: false };
      img.onload = () => { entry.loaded = true; };
      img.src = pngB64.startsWith('data:') ? pngB64 : `data:image/png;base64,${pngB64}`;
      decoImageCache.set(pngB64, entry);
      if (decoImageCache.size > 64) {
        const firstKey = decoImageCache.keys().next().value;
        decoImageCache.delete(firstKey);
      }
    }
    return entry.img.complete && entry.img.naturalWidth > 0 ? entry.img : null;
  }

  function preloadDecorations(decorations) {
    if (!decorations) return;
    for (const d of decorations) {
      if (d && d.png) getDecorationImage(d.png);
    }
  }

  function getDecorationTransform(deco, skin, seatX, seatY, headCenter, state, decoAnchors) {
    const yawRad = state.headYaw * 0.50;
    const pitchRad = state.headPitch * 0.36;
    const override = decoAnchors && decoAnchors[deco.anchor];
    let at = { x: headCenter.x, y: headCenter.y };
    let rot = 0, kx = 1, ky = 1, alpha = 1, hidden = false;
    let offsetScaleX = 1, offsetScaleY = 1;

    if (override) {
      hidden = !!override.hidden;
      alpha = override.alpha !== undefined ? override.alpha : 1;
      rot = override.rot || 0;
      kx = override.kx !== undefined ? override.kx : 1;
      ky = override.ky !== undefined ? override.ky : 1;
      offsetScaleX = override.offsetScaleX !== undefined ? override.offsetScaleX : kx;
      offsetScaleY = override.offsetScaleY !== undefined ? override.offsetScaleY : ky;
      if (typeof override.mapOffset === 'function') {
        at = override.mapOffset(deco.offsetX, deco.offsetY);
      } else {
        const dx = deco.offsetX * offsetScaleX;
        const dy = deco.offsetY * offsetScaleY;
        const c = Math.cos(rot), s = Math.sin(rot);
        at = {
          x: override.origin.x + dx * c - dy * s,
          y: override.origin.y + dx * s + dy * c
        };
      }
    } else if (deco.anchor === 'headTop' || deco.anchor === 'face') {
      const baseY = deco.anchor === 'headTop' ? -30 : 4;
      at = headPoint({ x: deco.offsetX, y: baseY + deco.offsetY }, false, skin, headCenter, state);
      rot = state.headTilt;
      kx = Math.max(0.55, Math.cos(yawRad)) * skin.shape.headScale;
      ky = Math.max(0.70, Math.cos(pitchRad)) * skin.shape.headScale;
      offsetScaleX = skin.shape.headScale;
      offsetScaleY = skin.shape.headScale;
    } else if (deco.anchor === 'neck') {
      const baseNeck = bodyPoint({ x: 0, y: -64 }, skin, seatX, seatY, state);
      const minChinNeckY = headCenter.y + 36.0 * skin.shape.headScale;
      const chinDrop = Math.max(0, minChinNeckY - baseNeck.y);
      const rawPt = bodyPoint({ x: deco.offsetX, y: -64 + deco.offsetY }, skin, seatX, seatY, state);
      at = { x: rawPt.x, y: rawPt.y + chinDrop };
      rot = state.bodyTilt;
      offsetScaleX = skin.shape.bodyWidth;
      offsetScaleY = state.bodySquash || 1;
    } else if (deco.anchor === 'tailTip') {
      const curve = tailCurve(seatX, seatY, state, skin);
      at = { x: curve.tip.x + deco.offsetX, y: curve.tip.y + deco.offsetY };
    }
    const img = getDecorationImage(deco.png);
    const w = img ? img.naturalWidth : 64;
    const h = img ? img.naturalHeight : 64;
    const scale = 0.5 * deco.scale; // 2x authored stickers: 1 image px = 0.5 canvas units
    return {
      at,
      totalRot: rot + (deco.rotation * Math.PI) / 180,
      baseRot: rot,
      sx: scale * kx,
      sy: scale * ky,
      offsetScaleX,
      offsetScaleY,
      w,
      h,
      img,
      alpha,
      hidden
    };
  }

  function drawDecorations(ctx, seatX, seatY, headCenter, state, skin, options) {
    if (!skin.decorations || skin.decorations.length === 0) return;
    const opts = options || {};
    const allow = opts.anchors || null;
    const decoAnchors = opts.decoAnchors || null;
    for (const d of skin.decorations) {
      if (allow && !allow.includes(d.anchor)) continue;
      const tf = getDecorationTransform(d, skin, seatX, seatY, headCenter, state, decoAnchors);
      if (tf.hidden || tf.alpha <= 0.01 || !tf.img) continue;
      ctx.save();
      if (tf.alpha < 1) ctx.globalAlpha *= tf.alpha;
      ctx.translate(tf.at.x, tf.at.y);
      ctx.rotate(tf.totalRot);
      ctx.scale(tf.sx, tf.sy);
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.drawImage(tf.img, -tf.w * 0.5, -tf.h * 0.5, tf.w, tf.h);
      ctx.restore();
    }
  }

  // Helper to draw decoration layers directly inside Douni's local vector space
  // so stickers inherit head bob/tilt/squash & cross-fade cleanly at exact anatomical depth.
  function drawDecorationsInSpace(ctx, skin, anchors, spaceAnchors, spacePerCanvas) {
    if (!skin.decorations || skin.decorations.length === 0) return;
    for (const d of skin.decorations) {
      if (anchors && !anchors.includes(d.anchor)) continue;
      const spec = spaceAnchors[d.anchor];
      if (!spec || spec.hidden) continue;
      const alpha = spec.alpha !== undefined ? spec.alpha : 1;
      if (alpha <= 0.01) continue;
      const img = getDecorationImage(d.png);
      if (!img) continue;
      const w = img.naturalWidth || 64;
      const h = img.naturalHeight || 64;
      const rot = spec.rot || 0;
      const kx = spec.kx !== undefined ? spec.kx : 1;
      const ky = spec.ky !== undefined ? spec.ky : 1;
      const osx = (spec.offsetScaleX !== undefined ? spec.offsetScaleX : kx) * spacePerCanvas;
      const osy = (spec.offsetScaleY !== undefined ? spec.offsetScaleY : ky) * spacePerCanvas;
      const dx = d.offsetX * osx;
      const dy = d.offsetY * osy;
      const c = Math.cos(rot), s = Math.sin(rot);
      const ax = spec.origin.x + dx * c - dy * s;
      const ay = spec.origin.y + dx * s + dy * c;
      const baseScale = 0.5 * d.scale * spacePerCanvas;
      ctx.save();
      if (alpha < 1) ctx.globalAlpha *= alpha;
      ctx.translate(ax, ay);
      ctx.rotate(rot + (d.rotation * Math.PI) / 180);
      ctx.scale(baseScale * kx, baseScale * ky);
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.drawImage(img, -w * 0.5, -h * 0.5, w, h);
      ctx.restore();
    }
  }

  function getDecorationOverlayGeometry(deco, skin, seatX, seatY, headCenter, state, decoAnchors) {
    const tf = getDecorationTransform(deco, skin, seatX, seatY, headCenter, state, decoAnchors);
    const hw = Math.max(12, tf.w * tf.sx * 0.5);
    const hh = Math.max(12, tf.h * tf.sy * 0.5);
    const c = Math.cos(tf.totalRot), s = Math.sin(tf.totalRot);
    const toCanvas = (lx, ly) => ({
      x: tf.at.x + lx * c - ly * s,
      y: tf.at.y + lx * s + ly * c
    });
    return {
      center: tf.at,
      hw,
      hh,
      rot: tf.totalRot,
      baseRot: tf.baseRot,
      offsetScaleX: tf.offsetScaleX,
      offsetScaleY: tf.offsetScaleY,
      hidden: tf.hidden,
      corners: [toCanvas(-hw, -hh), toCanvas(hw, -hh), toCanvas(hw, hh), toCanvas(-hw, hh)],
      scaleHandle: toCanvas(hw, hh),
      rotateHandle: toCanvas(0, -hh - 16),
      topMid: toCanvas(0, -hh)
    };
  }

  function drawDecorationOverlay(ctx, deco, skin, seatX, seatY, headCenter, state, decoAnchors) {
    const g = getDecorationOverlayGeometry(deco, skin, seatX, seatY, headCenter, state, decoAnchors);
    if (g.hidden) return;
    ctx.save();
    // Dashed amber selection box
    ctx.beginPath();
    ctx.moveTo(g.corners[0].x, g.corners[0].y);
    for (let i = 1; i < 4; i++) ctx.lineTo(g.corners[i].x, g.corners[i].y);
    ctx.closePath();
    ctx.strokeStyle = 'rgba(255,255,255,0.92)';
    ctx.lineWidth = 3.0;
    ctx.stroke();
    ctx.strokeStyle = '#E28B42';
    ctx.lineWidth = 1.5;
    ctx.setLineDash([4, 3]);
    ctx.stroke();
    ctx.setLineDash([]);

    // Rotate stem + handle
    ctx.beginPath();
    ctx.moveTo(g.topMid.x, g.topMid.y);
    ctx.lineTo(g.rotateHandle.x, g.rotateHandle.y);
    ctx.strokeStyle = '#E28B42';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    ctx.beginPath();
    ctx.arc(g.rotateHandle.x, g.rotateHandle.y, 5.0, 0, Math.PI * 2);
    ctx.fillStyle = '#FFFFFF';
    ctx.fill();
    ctx.strokeStyle = '#E28B42';
    ctx.lineWidth = 1.8;
    ctx.stroke();

    // Bottom-right scale handle
    ctx.beginPath();
    ctx.arc(g.scaleHandle.x, g.scaleHandle.y, 5.2, 0, Math.PI * 2);
    ctx.fillStyle = '#E28B42';
    ctx.fill();
    ctx.strokeStyle = '#FFFFFF';
    ctx.lineWidth = 1.8;
    ctx.stroke();
    ctx.restore();
  }

  function createDefaultState() {
    return {
      time: 0,
      tailPhase: 0,
      bodyShiftX: 0,
      bodyTilt: 0,
      bodySquash: 1,
      headTilt: 0,
      headYaw: 0,
      headPitch: 0,
      headOffsetY: 0,
      earFold: 0,
      earTwitch: 0,
      eyeGazeX: 0,
      eyeGazeY: 0,
      pupilScale: 1,
      huntIntensity: 0,
      petHead: 0,
      petBody: 0,
      sleepiness: 0,
      blinkProgress: 0,
      mouseOffsetX: 0,
      mouseOffsetY: 0,
      mousePress: 0,
      mousePressLeft: 0,
      mousePressRight: 0,
      leftPaw: { ...LEFT_PAW_REST },
      rightPaw: { ...RIGHT_PAW_REST },
      leftPawActive: false,
      rightPawActive: false,
      hindFoot: { ...HIND_FOOT_REST },
      hindFootActive: 0,
      keyGlow: {},
      scruffDrag: 0,
      scruffDragVel: 0,
      isScruffDragging: false,
      scruffHoldTime: 0,
      scruffDropTimer: 0,
      scruffSwing: 0,
      scruffSwingVel: 0,
      scruffSwingY: 0,
      scruffSwingYVel: 0,
      scruffTailSwing: 0,
      scruffTailSwingVel: 0,
      lieIntensity: 0,
      lieIntensityVel: 0,
      yawnAmount: 0,
      tailTyping: 0,
      tailTypingVel: 0,
      tailTypingTarget: 0,
      tailTip: { x: 256, y: 248 },
      tailTipGoal: { x: 256, y: 248 },
      tailFlickX: 0,
      tailFlickXVel: 0,
      tailFlickY: 0,
      tailFlickYVel: 0,
      tailCurl: 0,
      tailCurlVel: 0,
      tailAgility: 0,
      marqueePhase: 0,
      marqueeEnabled: true,
      douniLoafBlend: 0,
      douniLoafBlendVel: 0,
      douniLoafAnimated: false,
      douniMouseBlend: 0,
      douniMouseBlendVel: 0,
      douniMouseTimer: 0,
      douniLeftRaise: 0,
      douniLeftRaiseVel: 0,
      douniLeftRaiseTarget: 0,
      douniRightRaise: 0,
      douniRightRaiseVel: 0,
      douniRightRaiseTarget: 0,
      douniLeftKeyTarget: { x: 274, y: 412 },
      douniLeftKeyTargetGoal: { x: 274, y: 412 },
      douniRightKeyTarget: { x: 406, y: 414 },
      douniRightKeyTargetGoal: { x: 406, y: 414 },
      douniSlapTimer: 0,
      kbImpactY: 0,
      kbImpactYVel: 0,
      kbImpactTilt: 0,
      kbImpactTiltVel: 0,
      kbSparkTimer: 0,
      kbSparkKey: null,
      angerIntensity: 0,
      angerIntensityVel: 0,
      dizzyIntensity: 0,
      dizzyIntensityVel: 0,
      dizzyPhase: 0,
      isTabletActive: false,
      tabletNib: { x: 56, y: 260 },
      tabletPenPress: 0,
      tabletPenTilt: 0,
      tabletTrail: [],
      douniLastLeftKeyTime: -10,
      douniLastRightKeyTime: -10,
      _recentKeyTimes: [],
      dragSwingTarget: 0,
      dragSwingYTarget: 0,
      floatingBadges: [],
      sparkles: []
    };
  }

  // Generate a crisp 720x640 (2x of 360x320) transparent artist template PNG
  // so creators can paint directly over KuroCat's exact head/ears/eyes/torso/keyboard guides.
  function generateHandDrawTemplateDataUrl(currentSkin) {
    const off = document.createElement('canvas');
    off.width = 720;
    off.height = 640;
    const ctx = off.getContext('2d');
    const baseShape = (currentSkin && currentSkin.shape) ? { ...currentSkin.shape } : {
      build: 'standard', bodyWidth: 1, headScale: 1, cheekFullness: 1, neckWidth: 1,
      earSize: 1, earBaseWidth: 1, earPointiness: 1, tailLength: 1, tailThickness: 1, pawStagger: 0
    };
    const guideSkin = {
      shape: baseShape,
      headSurface: 'dome',
      idleLegsTinted: false,
      colorpoint: false,
      regions: [],
      marks: [],
      decorations: [],
      hasPartArt: () => false,
      palette: {
        body: { r: 0.99, g: 0.96, b: 0.92, a: 0.52 },
        bodyShade: null,
        belly: null,
        legs: { r: 0.99, g: 0.96, b: 0.92, a: 0.52 },
        paws: { r: 0.99, g: 0.96, b: 0.92, a: 0.65 },
        pawPads: null,
        earOuter: { r: 0.96, g: 0.90, b: 0.84, a: 0.58 },
        earInner: { r: 0.94, g: 0.78, b: 0.74, a: 0.45 },
        tail: { r: 0.92, g: 0.86, b: 0.78, a: 0.36 },
        tailTip: null,
        outline: { r: 0.64, g: 0.39, b: 0.22, a: 0.85 },
        crease: { r: 0.64, g: 0.39, b: 0.22, a: 0.45 },
        whiskers: { r: 0.64, g: 0.39, b: 0.22, a: 0.32 },
        nose: { r: 0.86, g: 0.52, b: 0.55, a: 0.55 },
        mouth: { r: 0.64, g: 0.39, b: 0.22, a: 0.45 }
      }
    };

    const st = createDefaultState();
    st.time = 0;
    st.tailPhase = 0.8;
    const seatX = SEAT_ANCHOR.x;
    const seatY = SEAT_ANCHOR.y;
    const bd = getBuild(guideSkin);
    const headCenter = { x: seatX, y: 126 + bd.headLift };

    ctx.save();
    ctx.scale(2, 2);

    // 1. Faint keyboard & mouse desk reference at bottom
    ctx.save();
    ctx.globalAlpha = 0.22;
    drawMouse(ctx, st, 'classicMilk');
    drawKeyboard(ctx, st, 'classicMilk', 'neonChartreuse');
    ctx.restore();

    // 2. Faint tail guide
    ctx.save();
    drawTail(ctx, seatX, seatY, st, guideSkin);
    ctx.restore();

    // 3. Ears + unified silhouette outline
    ctx.save();
    ctx.translate(headCenter.x, headCenter.y);
    if (baseShape.headScale !== 1) ctx.scale(baseShape.headScale, baseShape.headScale);
    drawEar3D(ctx, true, 0, 0, 0, 0, guideSkin);
    drawEar3D(ctx, false, 0, 0, 0, 0, guideSkin);
    ctx.restore();

    drawUnifiedSilhouette(ctx, seatX, seatY, headCenter, 0, st, guideSkin);
    drawFrontPaws(ctx, seatX, seatY, 0, st, guideSkin);

    // 4. Center symmetry axis + eye ovals + nose + neck pivot guide
    const hs = baseShape.headScale || 1;
    ctx.save();
    ctx.setLineDash([3, 3]);
    ctx.strokeStyle = 'rgba(164, 100, 56, 0.32)';
    ctx.lineWidth = 0.8;
    ctx.beginPath();
    ctx.moveTo(seatX, 58);
    ctx.lineTo(seatX, 236);
    ctx.stroke();

    // Neck head/body split line (y = 158)
    const neckY = headCenter.y + 32 * hs;
    ctx.strokeStyle = 'rgba(214, 108, 44, 0.55)';
    ctx.lineWidth = 0.9;
    ctx.beginPath();
    ctx.moveTo(seatX - 64, neckY);
    ctx.lineTo(seatX + 64, neckY);
    ctx.stroke();

    // Eye position guide ovals
    ctx.setLineDash([2.5, 2]);
    ctx.strokeStyle = 'rgba(214, 108, 44, 0.72)';
    ctx.lineWidth = 1.0;
    for (const sign of [-1, 1]) {
      const ex = headCenter.x + sign * 17.2 * hs;
      const ey = headCenter.y - 2.0 * hs;
      ctx.beginPath();
      ctx.ellipse(ex, ey, 8.2 * hs, 8.8 * hs, 0, 0, Math.PI * 2);
      ctx.stroke();
    }

    // Tiny nose marker
    ctx.setLineDash([]);
    ctx.fillStyle = 'rgba(220, 120, 128, 0.65)';
    ctx.beginPath();
    ctx.arc(headCenter.x, headCenter.y + 8 * hs, 2.2 * hs, 0, Math.PI * 2);
    ctx.fill();

    // Clean small guide labels
    ctx.font = '600 7.5px -apple-system, BlinkMacSystemFont, "PingFang SC", sans-serif';
    ctx.fillStyle = 'rgba(130, 82, 48, 0.78)';
    ctx.textAlign = 'center';
    ctx.fillText('KuroCat 手绘参考底图 (720×640) · 新建图层绘制，导出前隐藏本参考层', 180, 18);

    ctx.font = '500 6.8px -apple-system, BlinkMacSystemFont, "PingFang SC", sans-serif';
    ctx.textAlign = 'left';
    ctx.fillStyle = 'rgba(164, 100, 56, 0.72)';
    ctx.fillText('← 头部/躯干分界', seatX + 67, neckY + 2.5);
    ctx.fillText('动态尾巴区(可留空)', seatX + 62, seatY - 38);
    ctx.textAlign = 'center';
    ctx.fillStyle = 'rgba(120, 110, 102, 0.68)';
    ctx.fillText('键盘与打字爪区（猫身画在此线上方）', 194, 306);
    ctx.restore();

    ctx.restore();
    return off.toDataURL('image/png');
  }

  // Render clean 360x320 idle preview PNG (base64 without data: prefix) for embedding in .kurocat
  function renderPreviewBase64(resolvedSkin, kbTheme, eyeTheme) {
    const off = document.createElement('canvas');
    off.width = 360;
    off.height = 320;
    const ctx = off.getContext('2d');
    const st = createDefaultState();
    st.time = 0.6;
    st.tailPhase = 0.9;
    drawScene(ctx, resolvedSkin, st, { kbTheme: kbTheme || 'classicMilk', eyeTheme });
    const dataUrl = off.toDataURL('image/png');
    return dataUrl.replace(/^data:image\/png;base64,/, '');
  }

  function registerKeyboardTheme(id, colors) {
    if (!id || !colors) return;
    KB_THEMES[id] = {
      mouseBody: colors.mouseBody || '#F4F2EC',
      mouseBorder: colors.mouseBorder || '#242429',
      wheel: colors.wheel || '#F5B029',
      base: colors.base || '#1A1A1F',
      deck: colors.deck || '#242429',
      key: colors.key || '#F7F6F0',
      keyBorder: colors.keyBorder || 'rgba(199,196,189,0.75)',
      accent: colors.accent || '#F7B329',
      darkText: colors.darkText !== false
    };
  }

  function getKeyboardThemeColors(id) {
    return KB_THEMES[id] || KB_THEMES.classicMilk;
  }

  // Dedicated zoomed-in 2.5D keyboard + mouse card renderer for Keyboard Plaza
  function drawKeyboardCardPreview(ctx, colors, glowId) {
    const tempId = '__plaza_kb_preview__';
    registerKeyboardTheme(tempId, colors);
    const cw = ctx.canvas.width;
    const ch = ctx.canvas.height;
    ctx.save();
    ctx.clearRect(0, 0, cw, ch);
    // Zoom into the lower-left mouse (x~64..92) + keyboard (x~100..290, y~230..298)
    const scale = Math.min(cw / 254, ch / 114);
    ctx.translate(cw * 0.5 - 176 * scale, ch * 0.5 - 264 * scale);
    ctx.scale(scale, scale);
    const st = createDefaultState();
    if (glowId) st.keyGlow[glowId] = 0.85;
    drawMouse(ctx, st, tempId);
    drawKeyboard(ctx, st, tempId, 'warmAmber');
    ctx.restore();
  }

  global.KuroRenderer = {
    SEAT_ANCHOR,
    LEFT_PAW_REST,
    RIGHT_PAW_REST,
    HIND_FOOT_REST,
    MOUSE_BASE,
    KEYCAPS,
    screenQuadPoint,
    tabletQuadPoint,
    v2KbPoint,
    v2KeyCellInfo,
    createDefaultState,
    triggerKeyTap,
    triggerMouseClick,
    spawnBadge,
    mapKeyboardEventToCap,
    findNearestKeycap,
    hitTestCat,
    updateTailHover,
    stepPhysicsExtras,
    drawScene,
    mapPointToCanvas,
    mapCanvasToSpace,
    mapCanvasToTail,
    preloadDecorations,
    getDecorationTransform,
    getDecorationOverlayGeometry,
    renderPreviewBase64,
    generateHandDrawTemplateDataUrl,
    registerKeyboardTheme,
    getKeyboardThemeColors,
    drawKeyboardCardPreview
  };
})(window);

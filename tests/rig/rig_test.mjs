// Checks the web renderers' part-space mapping (KuroRenderer.mapPointToCanvas) against tests/rig/golden.tsv.
// Usage: node tests/rig/rig_test.mjs [extra renderer dirs...]
//   always checks homepage/pet; pass e.g. ../KuroCat-Workshop to also check the workshop copy.
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '../..');
const golden = fs.readFileSync(path.join(here, 'golden.tsv'), 'utf8').trim().split('\n');

function loadRenderer(dir) {
  // Just enough browser for the IIFEs to define window.KuroSpec / window.KuroRenderer.
  const noop = () => {};
  const fakeCtx = new Proxy({}, { get: () => noop });
  const document = {
    createElement: () => ({ getContext: () => fakeCtx, style: {}, addEventListener: noop }),
    getElementById: () => null, querySelector: () => null, querySelectorAll: () => [],
    addEventListener: noop, documentElement: { style: {} }, body: { appendChild: noop }
  };
  const window = { document, addEventListener: noop, removeEventListener: noop, devicePixelRatio: 1,
    requestAnimationFrame: noop, setTimeout, clearTimeout, console };
  window.window = window;
  window.Image = class { set src(_v) {} addEventListener() {} };
  window.Path2D = class { constructor() { return new Proxy({}, { get: () => noop }); } };
  const ctx = vm.createContext(window);
  for (const file of ['spec.js', 'renderer.js']) {
    vm.runInContext(fs.readFileSync(path.join(dir, file), 'utf8'), ctx, { filename: path.join(dir, file) });
  }
  if (!window.KuroRenderer || !window.KuroRenderer.mapPointToCanvas) throw new Error(`${dir}: no KuroRenderer.mapPointToCanvas`);
  return window.KuroRenderer;
}

function check(dir) {
  const R = loadRenderer(dir);
  const tol = 1e-6;
  let bad = 0;
  for (const line of golden) {
    const [kind, ...rest] = line.split('\t');
    const v = rest.map(Number);
    const [ex, ey] = v.slice(-2);
    let got;
    if (kind === 'head') {
      const [x, y, skull, dome, headScale, headYaw, headPitch, headTilt, hx, hy] = v;
      const skin = { headSurface: dome ? 'dome' : 'plane', shape: { headScale, bodyWidth: 1 } };
      const state = { headYaw, headPitch, headTilt, bodySquash: 1, bodyTilt: 0, lieIntensity: 0 };
      got = R.mapPointToCanvas({ x, y }, skull ? 'skull' : 'head', skin, 184, 226, { x: hx, y: hy }, state);
    } else {
      const [x, y, bodyWidth, bodySquash, lieIntensity, bodyTilt, sx, sy] = v;
      const skin = { headSurface: 'dome', shape: { headScale: 1, bodyWidth } };
      const state = { headYaw: 0, headPitch: 0, headTilt: 0, bodySquash, lieIntensity, bodyTilt };
      got = R.mapPointToCanvas({ x, y }, 'body', skin, sx, sy, { x: 184, y: 126 }, state);
    }
    if (!(Math.abs(got.x - ex) <= tol && Math.abs(got.y - ey) <= tol)) {
      if (++bad <= 10) console.error(`MISMATCH ${dir}: ${line} -> got (${got.x}, ${got.y})`);
    }
  }
  const label = path.relative(root, dir) || dir;
  if (bad) { console.error(`JS rig (${label}): ${bad} of ${golden.length} cases differ`); return false; }
  console.log(`JS rig (${label}): ${golden.length} cases match`);
  return true;
}

const dirs = [path.join(root, 'homepage/pet'), ...process.argv.slice(2).map((d) => path.resolve(d))];
const ok = dirs.map(check).every(Boolean);
process.exit(ok ? 0 : 1);

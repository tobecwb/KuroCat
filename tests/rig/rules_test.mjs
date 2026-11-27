// Skin-rule parity: the web spec.js must accept the same spaces, limits and shape ranges as the desktop parsers
// (shared/SkinSpec.hpp for Windows + Linux, Sources/CatPhysics.swift for macOS).
// Usage: node tests/rig/rules_test.mjs <dir containing spec.js>   (default: homepage/pet)
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..');
const dir = path.resolve(process.argv[2] || path.join(root, 'homepage/pet'));
const ctx = { console, atob: (s) => Buffer.from(s, 'base64').toString('binary') };
ctx.window = ctx;
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(path.join(dir, 'spec.js'), 'utf8'), ctx);
const S = ctx.KuroSpec;
const cpp = fs.readFileSync(path.join(root, 'shared/SkinSpec.hpp'), 'utf8');
const swift = fs.readFileSync(path.join(root, 'Sources/CatPhysics.swift'), 'utf8');

const problems = [];
const same = (what, a, b) => { if (JSON.stringify(a) !== JSON.stringify(b)) problems.push(`${what}: web ${JSON.stringify(a)} vs desktop ${JSON.stringify(b)}`); };

// Spaces
const cppSpaces = JSON.parse('[' + cpp.match(/static const char\* names\[\] = \{([^}]*)\}/)[1] + ']');
same('spaces', [...S.VALID_SPACES].sort(), [...cppSpaces].sort());

// Shape clamp ranges (C++ and Swift must agree with each other too)
const cppShape = Object.fromEntries([...cpp.matchAll(/"shape\.(\w+)",\s*(-?[\d.]+),\s*(-?[\d.]+)/g)].map((m) => [m[1], [+m[2], +m[3]]]));
const swiftShape = Object.fromEntries([...swift.matchAll(/"shape\.(\w+)",\s*(-?[\d.]+),\s*(-?[\d.]+)/g)].map((m) => [m[1], [+m[2], +m[3]]]));
for (const k of new Set([...Object.keys(cppShape), ...Object.keys(S.SHAPE_LIMITS)])) {
  same(`shape.${k} (C++)`, S.SHAPE_LIMITS[k] || null, cppShape[k] || null);
  if (swiftShape[k] || cppShape[k]) same(`shape.${k} (Swift vs C++)`, swiftShape[k] || null, cppShape[k] || null);
}

// Count limits
same('marks limit (C++)', S.MAX_MARKS, +cpp.match(/MaxMarks = (\d+)/)[1]);
same('marks limit (Swift)', S.MAX_MARKS, +swift.match(/static let maxMarks = (\d+)/)[1]);
same('formatVersion (C++)', S.FORMAT_VERSION_MAX, +cpp.match(/FormatVersion = (\d+)/)[1]);
same('formatVersion (Swift)', S.FORMAT_VERSION_MAX, +swift.match(/static let formatVersion = (\d+)/)[1]);
const base = { formatVersion: 1, name: 'x', palette: { body: '#000000' } };
const cppRegions = +cpp.match(/regs->items\.size\(\) > (\d+)/)[1];
const regions = (n) => Array.from({ length: n }, () => ({ space: 'body', fill: '#FFFFFF', anchors: [{ x: 0, y: 0 }, { x: 1, y: 1 }] }));
same('regions limit', [S.validateRawSkin({ ...base, regions: regions(cppRegions) }).ok, S.validateRawSkin({ ...base, regions: regions(cppRegions + 1) }).ok], [true, false]);

if (problems.length) {
  for (const p of problems) console.error('RULE MISMATCH ' + p);
  console.error(`rules (${path.relative(root, dir) || dir}): ${problems.length} mismatch(es)`);
  process.exit(1);
}
console.log(`rules (${path.relative(root, dir) || dir}): spaces, shape ranges, limits and formatVersion match the desktop parsers`);

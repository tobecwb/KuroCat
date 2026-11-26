#!/usr/bin/env node
/**
 * KuroCat MCP Server (Model Context Protocol, JSON-RPC 2.0 over stdio)
 * Zero external runtime dependencies — Node.js 18+ ESM.
 *
 * Official Homepage: https://kurocat.cc/
 * Skin Workshop & Plaza: https://workshop.kurocat.cc/
 * Repository: https://github.com/chanke17/KuroCat
 */

import { mkdir, readdir, readFile, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";

const SERVER_INFO = {
  name: "kurocat-mcp",
  version: "1.5.1",
};

const PROTOCOL_VERSION = "2024-11-05";
const WORKSHOP_BASE = "https://workshop.kurocat.cc";
const HEX_COLOR_RE = /^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/;

// ---------------------------------------------------------------------------
// Cross-platform KuroCat data directories (matches Sources/CatPhysics.swift,
// windows/src/SkinLibrary.hpp, and linux/src/SkinLibrary.hpp)
// ---------------------------------------------------------------------------
function getKuroCatDirs() {
  const platform = process.platform;
  const home = os.homedir();
  if (platform === "darwin") {
    const base = path.join(home, "Library", "Application Support", "KuroCat");
    return {
      skinsDir: path.join(base, "skins"),
      keyboardsDir: path.join(base, "keyboards"),
    };
  }
  if (platform === "win32") {
    const appData = process.env.APPDATA || path.join(home, "AppData", "Roaming");
    const base = path.join(appData, "KuroCat");
    return {
      skinsDir: path.join(base, "skins"),
      keyboardsDir: path.join(base, "keyboards"),
    };
  }
  const xdgData = process.env.XDG_DATA_HOME || path.join(home, ".local", "share");
  const xdgConfig = process.env.XDG_CONFIG_HOME || path.join(home, ".config");
  return {
    skinsDir: path.join(xdgData, "kurocat", "skins"),
    keyboardsDir: path.join(xdgConfig, "kurocat", "keyboards"),
  };
}

// Exact FNV-1a 64-bit content hash used by KuroCat native clients
function fnv1a64Hex(buffer) {
  let h = 1469598103934665603n;
  const prime = 1099511628211n;
  const mask = 0xffffffffffffffffn;
  for (const byte of buffer) {
    h ^= BigInt(byte);
    h = (h * prime) & mask;
  }
  return h.toString(16).padStart(16, "0");
}

function clamp(val, min, max) {
  const n = Number(val);
  if (!Number.isFinite(n)) return min;
  return Math.max(min, Math.min(max, n));
}

function cleanText(str, maxLen = 40) {
  return String(str ?? "")
    .trim()
    .replace(/[\x00-\x1f]/g, "")
    .slice(0, maxLen);
}

// ---------------------------------------------------------------------------
// Strict cross-platform schema validators (.kurocat v1 & .kurokb v1)
// ---------------------------------------------------------------------------
function validateCatSkin(obj) {
  const errors = [];
  if (!obj || typeof obj !== "object" || Array.isArray(obj)) {
    return { valid: false, errors: ["Root value must be a JSON object."] };
  }
  if (obj.formatVersion !== 1) {
    errors.push(`formatVersion must be integer 1 (got ${JSON.stringify(obj.formatVersion)}).`);
  }
  const name = cleanText(obj.name, 40);
  if (!name) errors.push("name is required and cannot be empty.");

  if (!obj.palette || typeof obj.palette !== "object" || Array.isArray(obj.palette)) {
    errors.push("palette object is required.");
  } else {
    if (!HEX_COLOR_RE.test(String(obj.palette.body || ""))) {
      errors.push(`palette.body must be #RRGGBB or #RRGGBBAA (got ${JSON.stringify(obj.palette.body)}).`);
    }
    const colorFields = [
      "bodyShade", "belly", "legs", "hindLeg", "paws", "pawPads", "earOuter", "earInner",
      "nose", "mouth", "iris", "tail", "tailTip", "stripes", "whiskers",
      "crease", "outline", "points", "pointsMid",
    ];
    for (const k of colorFields) {
      if (obj.palette[k] !== undefined && obj.palette[k] !== null && !HEX_COLOR_RE.test(String(obj.palette[k]))) {
        errors.push(`palette.${k} must be #RRGGBB or #RRGGBBAA (got ${JSON.stringify(obj.palette[k])}).`);
      }
    }
  }

  const validSpaces = new Set(["head", "skull", "body", "tail", "leftEar", "rightEar", "hindLeg"]);
  if (obj.regions !== undefined) {
    if (!Array.isArray(obj.regions)) {
      errors.push("regions must be an array.");
    } else if (obj.regions.length > 64) {
      errors.push("regions exceeds maximum limit of 64.");
    } else {
      obj.regions.forEach((r, idx) => {
        if (!r || typeof r !== "object") {
          errors.push(`regions[${idx}] must be an object.`);
          return;
        }
        if (!validSpaces.has(r.space)) errors.push(`regions[${idx}].space invalid: ${r.space}`);
        if (!HEX_COLOR_RE.test(String(r.fill || ""))) errors.push(`regions[${idx}].fill invalid hex: ${r.fill}`);
        if (!Array.isArray(r.anchors) || r.anchors.length < 2 || r.anchors.length > 128) {
          errors.push(`regions[${idx}].anchors must have 2..128 anchor points.`);
        }
      });
    }
  }

  if (obj.marks !== undefined) {
    if (!Array.isArray(obj.marks)) {
      errors.push("marks must be an array.");
    } else if (obj.marks.length > 256) {
      errors.push("marks exceeds maximum limit of 256.");
    } else {
      const validBrushes = new Set(["round", "soft", "airbrush", "smudge"]);
      obj.marks.forEach((m, idx) => {
        if (!m || typeof m !== "object") {
          errors.push(`marks[${idx}] must be an object.`);
          return;
        }
        if (m.brush !== undefined && !validBrushes.has(m.brush)) {
          errors.push(`marks[${idx}].brush must be one of round/soft/airbrush/smudge.`);
        }
        if (m.softness !== undefined && (typeof m.softness !== "number" || m.softness < 0 || m.softness > 1)) {
          errors.push(`marks[${idx}].softness must be a number in 0..1.`);
        }
        if (m.kind === "stroke" && m.width !== undefined && (typeof m.width !== "number" || m.width < 0.2 || m.width > 24)) {
          errors.push(`marks[${idx}].width must be a number in 0.2..24.`);
        }
      });
    }
  }

  if (obj.decorations !== undefined) {
    if (!Array.isArray(obj.decorations)) {
      errors.push("decorations must be an array.");
    } else if (obj.decorations.length > 8) {
      errors.push("decorations exceeds maximum limit of 8.");
    }
  }

  const bytes = Buffer.byteLength(JSON.stringify(obj), "utf8");
  if (bytes > 2 * 1024 * 1024) {
    errors.push(`Serialized size (${bytes} bytes) exceeds 2 MB limit.`);
  }

  return { valid: errors.length === 0, errors, sizeBytes: bytes };
}

function validateKeyboardSkin(obj) {
  const errors = [];
  if (!obj || typeof obj !== "object" || Array.isArray(obj)) {
    return { valid: false, errors: ["Root value must be a JSON object."] };
  }
  if (obj.formatVersion !== undefined && obj.formatVersion !== 1) {
    errors.push(`formatVersion must be 1 (got ${JSON.stringify(obj.formatVersion)}).`);
  }
  if (!cleanText(obj.name, 40)) errors.push("name is required and cannot be empty.");
  if (!obj.colors || typeof obj.colors !== "object" || Array.isArray(obj.colors)) {
    errors.push("colors object is required.");
  } else {
    if (!HEX_COLOR_RE.test(String(obj.colors.key || ""))) {
      errors.push(`colors.key must be #RRGGBB or #RRGGBBAA (got ${JSON.stringify(obj.colors.key)}).`);
    }
    for (const k of ["base", "deck", "keyBorder", "accent", "mouseBody", "mouseBorder", "wheel"]) {
      if (obj.colors[k] !== undefined && obj.colors[k] !== null && !HEX_COLOR_RE.test(String(obj.colors[k]))) {
        errors.push(`colors.${k} must be #RRGGBB or #RRGGBBAA (got ${JSON.stringify(obj.colors[k])}).`);
      }
    }
  }
  const bytes = Buffer.byteLength(JSON.stringify(obj), "utf8");
  if (bytes > 256 * 1024) errors.push(`Serialized size (${bytes} bytes) exceeds 256 KB limit.`);
  return { valid: errors.length === 0, errors, sizeBytes: bytes };
}

// ---------------------------------------------------------------------------
// Breed Presets for quick natural-language cat skin authoring
// ---------------------------------------------------------------------------
const BREED_PRESETS = {
  black: {
    palette: { body: "#181A1F", earInner: "#3A3138", iris: "#C7F03E", nose: "#3A3A40", pawPads: "#3E363E" },
    shape: { build: "standard" },
  },
  siamese: {
    palette: { body: "#EFE5D1", bodyShade: "#DFCFAF", points: "#5A3D2B", pointsMid: "#A27C5A", earInner: "#8B6955", iris: "#5EA3F5", nose: "#493123" },
    shape: { build: "sleek", bodyWidth: 0.94, earSize: 1.06 },
  },
  orange: {
    palette: { body: "#E39B4F", bodyShade: "#CF8138", belly: "#FBF4E8", legs: "#E39B4F", paws: "#FBF4E8", earInner: "#F1B5AA", nose: "#E58B84", pawPads: "#F0A3A0", iris: "#E3A735", stripes: "#B46422" },
    shape: { build: "standard", bodyWidth: 1.15, cheekFullness: 1.12 },
  },
  calico: {
    palette: { body: "#F8F4EC", belly: "#FFFDF9", earOuter: "#2D2A2E", earInner: "#F2B6B0", nose: "#E89292", pawPads: "#F1A6A6", iris: "#D89A32", tail: "#E18D42", tailTip: "#2D2A2E" },
    shape: { build: "standard", cheekFullness: 1.08 },
  },
  tuxedo: {
    palette: { body: "#1E2026", belly: "#F7F6F2", legs: "#1E2026", paws: "#F7F6F2", earInner: "#494048", nose: "#D98E96", pawPads: "#E69BA2", iris: "#E8B938" },
    shape: { build: "standard" },
  },
  britishBlue: {
    palette: { body: "#7E8894", bodyShade: "#697380", earInner: "#A99EA4", nose: "#545C66", pawPads: "#6B6770", iris: "#E4A832" },
    shape: { build: "standard", bodyWidth: 1.22, cheekFullness: 1.22, earSize: 0.88, earPointiness: 0.72 },
  },
  ragdoll: {
    palette: { body: "#F7F3EC", bodyShade: "#ECE4D7", points: "#6D5B50", pointsMid: "#B6A495", belly: "#FFFDF9", paws: "#FFFDF9", earInner: "#D9B9B2", nose: "#E2A2A2", iris: "#74C2F9" },
    shape: { build: "standard", bodyWidth: 1.12, cheekFullness: 1.14, tailThickness: 1.32 },
  },
  tabby: {
    palette: { body: "#8C7D6B", bodyShade: "#756756", belly: "#EFE8DC", paws: "#EFE8DC", earInner: "#C9A59B", nose: "#A96E62", iris: "#C7B24B", stripes: "#4B4136" },
    shape: { build: "standard", bodyWidth: 1.04 },
  },
};

// ---------------------------------------------------------------------------
// Live Community Plaza fetcher (https://workshop.kurocat.cc/)
// ---------------------------------------------------------------------------
async function fetchPlazaCatalog(category) {
  const map = {
    skins: { url: `${WORKSHOP_BASE}/community/index.json`, key: "skins" },
    accessories: { url: `${WORKSHOP_BASE}/community-accessories/index.json`, key: "accessories" },
    keyboards: { url: `${WORKSHOP_BASE}/community-keyboards/index.json`, key: "keyboards" },
  };
  const target = map[category];
  if (!target) throw new Error(`Unknown plaza category: ${category}`);
  const res = await fetch(target.url, { headers: { "User-Agent": "kurocat-mcp/1.4.3" } });
  if (!res.ok) throw new Error(`HTTP ${res.status} fetching ${target.url}`);
  const data = await res.json();
  return { generatedAt: data.generatedAt || null, items: data[target.key] || [] };
}

// ---------------------------------------------------------------------------
// Tool Definitions
// ---------------------------------------------------------------------------
const TOOLS = [
  {
    name: "generate_cat_skin",
    description:
      "Design and validate a custom KuroCat v1 desktop cat skin (.kurocat JSON) from breed presets, hex coat/eye palette, 2.5D body shape sliders, bezier coat regions, or vector stripes. Optionally installs it directly into the local KuroCat app skins directory (macOS/Windows/Linux).",
    inputSchema: {
      type: "object",
      properties: {
        name: { type: "string", description: "Display name of the cat skin (max 40 chars), e.g. 'Caramel Mochi'." },
        author: { type: "string", description: "Author name (optional, max 40 chars)." },
        preset: {
          type: "string",
          enum: ["black", "siamese", "orange", "calico", "tuxedo", "britishBlue", "ragdoll", "tabby"],
          description: "Optional starting breed preset whose colors & shape can be overridden by palette/shape.",
        },
        palette: {
          type: "object",
          description: "Color overrides in #RRGGBB or #RRGGBBAA (body required if no preset; optional: bodyShade, belly, legs, paws, pawPads, earOuter, earInner, nose, mouth, iris, tail, tailTip, stripes, whiskers, crease, outline, points, pointsMid).",
          additionalProperties: { type: "string" },
        },
        shape: {
          type: "object",
          description: "Optional shape sliders: build ('standard'|'sleek'), bodyWidth (0.85..1.35), headScale (0.9..1.15), cheekFullness (0.85..1.25), earSize (0.8..1.25), earPointiness (0.5..1.5), tailLength (0.7..1.3), tailThickness (0.7..1.5), pawStagger (0..1).",
        },
        regions: {
          type: "array",
          description: "Optional cubic bezier coat regions (<= 64) in part spaces: head, skull, body, tail, leftEar, rightEar.",
          items: { type: "object" },
        },
        marks: {
          type: "array",
          description: "Optional vector stripe/spot marks (<= 256).",
          items: { type: "object" },
        },
        headSurface: { type: "string", enum: ["dome", "plane"], description: "3D head projection surface (default: dome)." },
        install: { type: "boolean", description: "If true, writes the validated .kurocat file straight into the local KuroCat skins library." },
      },
      required: ["name"],
    },
  },
  {
    name: "generate_keyboard_theme",
    description:
      "Design and validate a custom KuroCat 65% mechanical keyboard & mouse theme (.kurokb JSON) with custom keycap, deck, base, accent, and mouse colors. Optionally installs it directly into the local KuroCat keyboards directory.",
    inputSchema: {
      type: "object",
      properties: {
        name: { type: "string", description: "Display name of the keyboard theme (max 40 chars), e.g. 'Matcha Latte 65%'." },
        author: { type: "string", description: "Author name (optional, max 40 chars)." },
        colors: {
          type: "object",
          description: "Hex colors (#RRGGBB or #RRGGBBAA): key (required), deck, base, keyBorder, accent, mouseBody, mouseBorder, wheel, plus boolean darkText.",
          properties: {
            key: { type: "string" },
            deck: { type: "string" },
            base: { type: "string" },
            keyBorder: { type: "string" },
            accent: { type: "string" },
            mouseBody: { type: "string" },
            mouseBorder: { type: "string" },
            wheel: { type: "string" },
            darkText: { type: "boolean" },
          },
          required: ["key"],
        },
        install: { type: "boolean", description: "If true, installs the validated .kurokb file directly into the local KuroCat keyboards folder." },
      },
      required: ["name", "colors"],
    },
  },
  {
    name: "browse_community_plaza",
    description:
      "Live-search free community cat skins (.kurocat), 2.5D sticker accessories, and 65% mechanical keyboard themes (.kurokb) from the official KuroCat Skin Workshop & Community Plaza (https://workshop.kurocat.cc/).",
    inputSchema: {
      type: "object",
      properties: {
        category: {
          type: "string",
          enum: ["skins", "accessories", "keyboards"],
          description: "Which plaza gallery to browse (default: skins).",
        },
        query: {
          type: "string",
          description: "Optional search keyword matching title, slug, author, tags, or description.",
        },
      },
    },
  },
  {
    name: "install_community_asset",
    description:
      "Download a free community cat skin (.kurocat) or keyboard theme (.kurokb) by slug from https://workshop.kurocat.cc/ and install it straight into the local KuroCat app library (macOS / Windows / Linux).",
    inputSchema: {
      type: "object",
      properties: {
        category: {
          type: "string",
          enum: ["skins", "keyboards"],
          description: "Asset type to install ('skins' or 'keyboards').",
        },
        slug: {
          type: "string",
          description: "Exact slug from browse_community_plaza, e.g. 'xiao-douni', 'dazed-cat', 'brown-tabby', 'sakura-milk'.",
        },
      },
      required: ["category", "slug"],
    },
  },
  {
    name: "list_installed_skins",
    description:
      "List all custom .kurocat cat skins and .kurokb keyboard themes currently installed in the local KuroCat desktop app directory.",
    inputSchema: {
      type: "object",
      properties: {},
    },
  },
  {
    name: "validate_kurocat_json",
    description:
      "Validate any .kurocat v1 cat skin JSON or .kurokb keyboard theme JSON against KuroCat's cross-platform native parser rules before importing or opening a PR to KuroCat-Workshop.",
    inputSchema: {
      type: "object",
      properties: {
        kind: { type: "string", enum: ["cat", "keyboard"], description: "Schema kind to validate ('cat' for .kurocat, 'keyboard' for .kurokb)." },
        json: { type: "object", description: "Parsed JSON object to validate." },
      },
      required: ["kind", "json"],
    },
  },
];

// ---------------------------------------------------------------------------
// Resources & Prompts
// ---------------------------------------------------------------------------
const RESOURCES = [
  {
    uri: "kurocat://spec/skin-format",
    name: "KuroCat .kurocat v1 Skin & .kurokb Keyboard Format Specification",
    description: "Coordinate system (360x320 part spaces), palette keys, cubic bezier regions, shape bounds, and local install paths across macOS/Windows/Linux.",
    mimeType: "text/markdown",
  },
  {
    uri: "kurocat://plaza/catalog",
    name: "KuroCat Community Plaza Live Catalog",
    description: "Live JSON summary of free community skins, accessories, and keyboards hosted at https://workshop.kurocat.cc/.",
    mimeType: "application/json",
  },
];

const PROMPTS = [
  {
    name: "design_my_cat",
    description: "Turn a user's description of their real pet cat into a validated KuroCat .kurocat v1 skin and install it.",
    arguments: [
      { name: "description", description: "Breed, coat colors, markings, eye color, and personality of the cat.", required: true },
      { name: "cat_name", description: "Name of the cat.", required: false },
    ],
  },
  {
    name: "design_matching_setup",
    description: "Design a coordinated KuroCat desktop cat skin (.kurocat) and 65% mechanical keyboard theme (.kurokb) pair.",
    arguments: [
      { name: "aesthetic", description: "Desired desk aesthetic, e.g. 'Quiet Luxury Espresso', 'Matcha Forest', 'Cyber Vapor'.", required: true },
    ],
  },
];

const SPEC_SUMMARY_MD = `# KuroCat .kurocat v1 & .kurokb Specification

Official Website: https://kurocat.cc/
Skin Workshop & Plaza: https://workshop.kurocat.cc/
Full Spec: https://github.com/chanke17/KuroCat/blob/main/docs/skin-format.md

## .kurocat v1 Top-Level Fields
- formatVersion: 1 (required)
- name: string (required, <= 40 chars)
- author: string (optional, <= 40 chars)
- palette: { body (required #RRGGBB/#RRGGBBAA), bodyShade, belly, legs, hindLeg, paws, pawPads, earOuter, earInner, nose, mouth, iris, tail, tailTip, stripes, whiskers, crease, outline, points, pointsMid }
- shape: { build: "standard"|"sleek", bodyWidth: 0.85..1.35, headScale: 0.90..1.15, cheekFullness: 0.85..1.25, earSize: 0.80..1.25, earPointiness: 0.50..1.50, tailLength: 0.70..1.30, tailThickness: 0.70..1.50, pawStagger: 0..1 }
- regions: <= 64 closed cubic bezier regions in space ("head"|"skull"|"body"|"tail"|"leftEar"|"rightEar"|"hindLeg")
- marks: <= 256 vector strokes/polygons (stroke width 0.2..24; brush: "round"|"soft"|"airbrush"|"smudge"; softness: 0..1; rendered consistently across sitting & lifted poses in KuroCat v1.4.9+)
- partGradients: optional per-part gradient overrides (head/body/tail/ears/legs)
- decorations: <= 8 custom PNG/sticker attachments (anchors: face/headTop/neck/tailTip)
- renderStyle: optional style preset (e.g. "dazed")
- headSurface: "dome" (default) | "plane"

## .kurokb v1 Top-Level Fields
- formatVersion: 1
- name: string (required, <= 40 chars)
- author: string (optional)
- colors: { key (required), deck, base, keyBorder, accent, mouseBody, mouseBorder, wheel, darkText }

## Native Install Locations (FNV-1a 64-bit 16-hex filename)
- macOS: ~/Library/Application Support/KuroCat/skins/<hash>.kurocat & .../keyboards/<hash>.kurokb
- Windows: %APPDATA%\\KuroCat\\skins\\<hash>.kurocat & ...\\keyboards\\<hash>.kurokb
- Linux / UOS / Kylin: ~/.local/share/kurocat/skins/<hash>.kurocat & ~/.config/kurocat/keyboards/<hash>.kurokb
`;

// ---------------------------------------------------------------------------
// Tool Execution
// ---------------------------------------------------------------------------
async function handleToolCall(name, args = {}) {
  const dirs = getKuroCatDirs();

  if (name === "generate_cat_skin") {
    const presetObj = args.preset && BREED_PRESETS[args.preset] ? BREED_PRESETS[args.preset] : null;
    const palette = { ...(presetObj?.palette || {}), ...(args.palette || {}) };
    const rawShape = { ...(presetObj?.shape || {}), ...(args.shape || {}) };
    const shape = {};
    if (rawShape.build === "sleek" || rawShape.build === "standard") shape.build = rawShape.build;
    const bounds = {
      bodyWidth: [0.85, 1.35],
      headScale: [0.9, 1.15],
      cheekFullness: [0.85, 1.25],
      earSize: [0.8, 1.25],
      earPointiness: [0.5, 1.5],
      tailLength: [0.7, 1.3],
      tailThickness: [0.7, 1.5],
      pawStagger: [0, 1],
    };
    for (const [k, [mn, mx]] of Object.entries(bounds)) {
      if (rawShape[k] !== undefined) shape[k] = Number(clamp(rawShape[k], mn, mx).toFixed(3));
    }

    const skin = {
      formatVersion: 1,
      name: cleanText(args.name, 40),
      ...(args.author ? { author: cleanText(args.author, 40) } : { author: "KuroCat MCP" }),
      palette,
      ...(Object.keys(shape).length ? { shape } : {}),
      ...(args.headSurface ? { headSurface: args.headSurface } : {}),
      ...(Array.isArray(args.regions) && args.regions.length ? { regions: args.regions } : {}),
      ...(Array.isArray(args.marks) && args.marks.length ? { marks: args.marks } : {}),
    };

    const check = validateCatSkin(skin);
    if (!check.valid) {
      return { isError: true, content: [{ type: "text", text: `Validation failed:\n- ${check.errors.join("\n- ")}` }] };
    }

    const payload = JSON.stringify(skin, null, 2) + "\n";
    const buf = Buffer.from(payload, "utf8");
    const contentId = fnv1a64Hex(buf);
    let installedPath = null;

    if (args.install) {
      await mkdir(dirs.skinsDir, { recursive: true });
      installedPath = path.join(dirs.skinsDir, `${contentId}.kurocat`);
      await writeFile(installedPath, buf);
    }

    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            {
              ok: true,
              contentId,
              installedPath,
              hint: installedPath
                ? "Skin installed into your local KuroCat skins library! Reopen the KuroCat skin menu (or restart KuroCat) to wear it."
                : "Save the JSON below as <name>.kurocat and drag it onto your KuroCat desktop cat (or re-run with install: true).",
              workshopUrl: `${WORKSHOP_BASE}/`,
              skin,
            },
            null,
            2
          ),
        },
      ],
    };
  }

  if (name === "generate_keyboard_theme") {
    const kb = {
      formatVersion: 1,
      name: cleanText(args.name, 40),
      ...(args.author ? { author: cleanText(args.author, 40) } : { author: "KuroCat MCP" }),
      colors: args.colors || {},
    };
    const check = validateKeyboardSkin(kb);
    if (!check.valid) {
      return { isError: true, content: [{ type: "text", text: `Validation failed:\n- ${check.errors.join("\n- ")}` }] };
    }

    const payload = JSON.stringify(kb, null, 2) + "\n";
    const buf = Buffer.from(payload, "utf8");
    const contentId = fnv1a64Hex(buf);
    let installedPath = null;

    if (args.install) {
      await mkdir(dirs.keyboardsDir, { recursive: true });
      installedPath = path.join(dirs.keyboardsDir, `${contentId}.kurokb`);
      await writeFile(installedPath, buf);
    }

    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            {
              ok: true,
              contentId,
              installedPath,
              hint: installedPath
                ? "Keyboard theme installed into your local KuroCat keyboards library!"
                : "Save the JSON below as <name>.kurokb and drag it onto KuroCat (or re-run with install: true).",
              keyboard: kb,
            },
            null,
            2
          ),
        },
      ],
    };
  }

  if (name === "browse_community_plaza") {
    const category = args.category || "skins";
    const q = String(args.query || "").trim().toLowerCase();
    const { generatedAt, items } = await fetchPlazaCatalog(category);
    const filtered = q
      ? items.filter((it) => {
          const hay = [it.slug, it.title, it.author, it.description, ...(it.tags || [])].join(" ").toLowerCase();
          return hay.includes(q);
        })
      : items;
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify(
            {
              category,
              plazaUrl: `${WORKSHOP_BASE}/#${category === "skins" ? "plaza" : category}`,
              generatedAt,
              totalMatched: filtered.length,
              items: filtered.map((it) => ({
                slug: it.slug,
                title: it.title,
                author: it.author,
                tags: it.tags || [],
                description: it.description,
                downloadUrl: it.skinPath
                  ? `${WORKSHOP_BASE}/${it.skinPath.replace(/^\.\//, "")}`
                  : it.keyboardPath
                    ? `${WORKSHOP_BASE}/${it.keyboardPath.replace(/^\.\//, "")}`
                    : it.pngPath
                      ? `${WORKSHOP_BASE}/${it.pngPath.replace(/^\.\//, "")}`
                      : null,
              })),
            },
            null,
            2
          ),
        },
      ],
    };
  }

  if (name === "install_community_asset") {
    const category = args.category;
    const slug = String(args.slug || "").trim();
    const { items } = await fetchPlazaCatalog(category);
    const match = items.find((it) => it.slug === slug);
    if (!match) {
      return {
        isError: true,
        content: [{ type: "text", text: `Slug "${slug}" not found in community ${category}. Available: ${items.map((i) => i.slug).join(", ")}` }],
      };
    }
    const relPath = category === "skins" ? match.skinPath : match.keyboardPath;
    const assetUrl = `${WORKSHOP_BASE}/${String(relPath).replace(/^\.\//, "")}`;
    const res = await fetch(assetUrl, { headers: { "User-Agent": "kurocat-mcp/1.4.3" } });
    if (!res.ok) throw new Error(`Failed downloading ${assetUrl}: HTTP ${res.status}`);
    const rawBuf = Buffer.from(await res.arrayBuffer());
    const parsed = JSON.parse(rawBuf.toString("utf8"));

    if (category === "skins") {
      const check = validateCatSkin(parsed);
      if (!check.valid) throw new Error(`Invalid community skin: ${check.errors.join("; ")}`);
      await mkdir(dirs.skinsDir, { recursive: true });
      const contentId = fnv1a64Hex(rawBuf);
      const target = path.join(dirs.skinsDir, `${contentId}.kurocat`);
      await writeFile(target, rawBuf);
      return {
        content: [{ type: "text", text: JSON.stringify({ ok: true, slug, title: match.title, contentId, installedPath: target }, null, 2) }],
      };
    } else {
      const check = validateKeyboardSkin(parsed);
      if (!check.valid) throw new Error(`Invalid community keyboard: ${check.errors.join("; ")}`);
      await mkdir(dirs.keyboardsDir, { recursive: true });
      const contentId = fnv1a64Hex(rawBuf);
      const target = path.join(dirs.keyboardsDir, `${contentId}.kurokb`);
      await writeFile(target, rawBuf);
      return {
        content: [{ type: "text", text: JSON.stringify({ ok: true, slug, title: match.title, contentId, installedPath: target }, null, 2) }],
      };
    }
  }

  if (name === "list_installed_skins") {
    const readDirItems = async (dirPath, ext) => {
      try {
        const files = (await readdir(dirPath)).filter((f) => f.endsWith(ext));
        const out = [];
        for (const f of files) {
          try {
            const raw = await readFile(path.join(dirPath, f), "utf8");
            const parsed = JSON.parse(raw);
            out.push({
              file: f,
              path: path.join(dirPath, f),
              name: parsed.name || "(unnamed)",
              author: parsed.author || "",
            });
          } catch {
            out.push({ file: f, path: path.join(dirPath, f), error: "unreadable JSON" });
          }
        }
        return out;
      } catch {
        return [];
      }
    };
    const [skins, keyboards] = await Promise.all([
      readDirItems(dirs.skinsDir, ".kurocat"),
      readDirItems(dirs.keyboardsDir, ".kurokb"),
    ]);
    return {
      content: [
        {
          type: "text",
          text: JSON.stringify({ platform: process.platform, directories: dirs, skins, keyboards }, null, 2),
        },
      ],
    };
  }

  if (name === "validate_kurocat_json") {
    const result = args.kind === "keyboard" ? validateKeyboardSkin(args.json) : validateCatSkin(args.json);
    return {
      content: [{ type: "text", text: JSON.stringify({ kind: args.kind, ...result }, null, 2) }],
    };
  }

  return { isError: true, content: [{ type: "text", text: `Unknown tool: ${name}` }] };
}

// ---------------------------------------------------------------------------
// JSON-RPC 2.0 Dispatcher
// ---------------------------------------------------------------------------
async function handleMessage(msg) {
  if (!msg || typeof msg !== "object") return null;
  const { id, method, params } = msg;

  // Notifications have no id
  if (id === undefined || id === null) {
    return null;
  }

  try {
    if (method === "initialize") {
      return {
        jsonrpc: "2.0",
        id,
        result: {
          protocolVersion: PROTOCOL_VERSION,
          capabilities: {
            tools: { listChanged: false },
            resources: { listChanged: false },
            prompts: { listChanged: false },
          },
          serverInfo: SERVER_INFO,
          instructions:
            "KuroCat MCP Server connects AI assistants with the KuroCat native desktop cat pet (https://kurocat.cc/) and Skin Workshop (https://workshop.kurocat.cc/). Use browse_community_plaza to discover community skins, generate_cat_skin / generate_keyboard_theme to create custom assets, and install_community_asset to install directly into KuroCat.",
        },
      };
    }

    if (method === "ping") {
      return { jsonrpc: "2.0", id, result: {} };
    }

    if (method === "tools/list") {
      return { jsonrpc: "2.0", id, result: { tools: TOOLS } };
    }

    if (method === "tools/call") {
      const res = await handleToolCall(params?.name, params?.arguments || {});
      return { jsonrpc: "2.0", id, result: res };
    }

    if (method === "resources/list") {
      return { jsonrpc: "2.0", id, result: { resources: RESOURCES } };
    }

    if (method === "resources/read") {
      const uri = params?.uri;
      if (uri === "kurocat://spec/skin-format") {
        return {
          jsonrpc: "2.0",
          id,
          result: { contents: [{ uri, mimeType: "text/markdown", text: SPEC_SUMMARY_MD }] },
        };
      }
      if (uri === "kurocat://plaza/catalog") {
        const [skins, accessories, keyboards] = await Promise.all([
          fetchPlazaCatalog("skins"),
          fetchPlazaCatalog("accessories"),
          fetchPlazaCatalog("keyboards"),
        ]);
        return {
          jsonrpc: "2.0",
          id,
          result: {
            contents: [
              {
                uri,
                mimeType: "application/json",
                text: JSON.stringify(
                  {
                    plazaUrl: WORKSHOP_BASE,
                    counts: { skins: skins.items.length, accessories: accessories.items.length, keyboards: keyboards.items.length },
                    skins: skins.items.map((i) => ({ slug: i.slug, title: i.title, author: i.author, tags: i.tags })),
                    accessories: accessories.items.map((i) => ({ slug: i.slug, title: i.title, author: i.author })),
                    keyboards: keyboards.items.map((i) => ({ slug: i.slug, title: i.title, author: i.author })),
                  },
                  null,
                  2
                ),
              },
            ],
          },
        };
      }
      return { jsonrpc: "2.0", id, error: { code: -32602, message: `Unknown resource URI: ${uri}` } };
    }

    if (method === "prompts/list") {
      return { jsonrpc: "2.0", id, result: { prompts: PROMPTS } };
    }

    if (method === "prompts/get") {
      const pName = params?.name;
      const pArgs = params?.arguments || {};
      if (pName === "design_my_cat") {
        return {
          jsonrpc: "2.0",
          id,
          result: {
            description: "Design a custom KuroCat .kurocat v1 skin from a real pet cat description.",
            messages: [
              {
                role: "user",
                content: {
                  type: "text",
                  text: `Read kurocat://spec/skin-format if needed, then call generate_cat_skin to design a faithful .kurocat v1 skin for my cat${pArgs.cat_name ? ` named "${pArgs.cat_name}"` : ""}: ${pArgs.description || "(ask user for details)"}. Pick accurate hex coat colors, eye iris, body shape sliders, and cubic bezier coat regions, then offer to install it directly into KuroCat.`,
                },
              },
            ],
          },
        };
      }
      if (pName === "design_matching_setup") {
        return {
          jsonrpc: "2.0",
          id,
          result: {
            description: "Design a coordinated KuroCat cat coat and 65% mechanical keyboard theme.",
            messages: [
              {
                role: "user",
                content: {
                  type: "text",
                  text: `Design a harmonious KuroCat desktop pet setup matching this aesthetic: "${pArgs.aesthetic || "Quiet Luxury"}". Call generate_cat_skin and generate_keyboard_theme with cohesive hex palettes so the cat and keyboard complement each other.`,
                },
              },
            ],
          },
        };
      }
      return { jsonrpc: "2.0", id, error: { code: -32602, message: `Unknown prompt: ${pName}` } };
    }

    return { jsonrpc: "2.0", id, error: { code: -32601, message: `Method not found: ${method}` } };
  } catch (err) {
    return {
      jsonrpc: "2.0",
      id,
      error: { code: -32603, message: err instanceof Error ? err.message : String(err) },
    };
  }
}

// ---------------------------------------------------------------------------
// Dual-mode stdio transport (supports newline-delimited JSON AND Content-Length)
// ---------------------------------------------------------------------------
let buffer = Buffer.alloc(0);
let useContentLengthFraming = false;

function writeResponse(resp) {
  if (!resp) return;
  const json = JSON.stringify(resp);
  if (useContentLengthFraming) {
    const bytes = Buffer.byteLength(json, "utf8");
    process.stdout.write(`Content-Length: ${bytes}\r\n\r\n${json}`);
  } else {
    process.stdout.write(`${json}\n`);
  }
}

async function processBuffer() {
  while (buffer.length > 0) {
    // Trim leading whitespace/newlines when looking for framing header
    const strHead = buffer.slice(0, Math.min(buffer.length, 64)).toString("utf8");
    if (/^Content-Length:\s*\d+/i.test(strHead)) {
      useContentLengthFraming = true;
      const sepIdx = buffer.indexOf("\r\n\r\n");
      if (sepIdx === -1) return;
      const header = buffer.slice(0, sepIdx).toString("utf8");
      const m = header.match(/Content-Length:\s*(\d+)/i);
      if (!m) {
        buffer = buffer.slice(sepIdx + 4);
        continue;
      }
      const len = Number(m[1]);
      const total = sepIdx + 4 + len;
      if (buffer.length < total) return;
      const body = buffer.slice(sepIdx + 4, total).toString("utf8");
      buffer = buffer.slice(total);
      try {
        const resp = await handleMessage(JSON.parse(body));
        writeResponse(resp);
      } catch (e) {
        writeResponse({ jsonrpc: "2.0", id: null, error: { code: -32700, message: `Parse error: ${e.message}` } });
      }
    } else {
      const nlIdx = buffer.indexOf("\n");
      if (nlIdx === -1) return;
      const line = buffer.slice(0, nlIdx).toString("utf8").trim();
      buffer = buffer.slice(nlIdx + 1);
      if (!line) continue;
      try {
        const resp = await handleMessage(JSON.parse(line));
        writeResponse(resp);
      } catch (e) {
        writeResponse({ jsonrpc: "2.0", id: null, error: { code: -32700, message: `Parse error: ${e.message}` } });
      }
    }
  }
}

process.stdin.on("data", (chunk) => {
  buffer = Buffer.concat([buffer, chunk]);
  void processBuffer();
});

process.stdin.on("end", () => {
  process.exit(0);
});

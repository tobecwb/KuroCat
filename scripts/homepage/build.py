#!/usr/bin/env python3
"""Render the kurocat.cc homepage (zh + en) from one template.

    python3 scripts/homepage/build.py

Inputs (this folder): template.html, zh.json, en.json, release.json
Outputs: homepage/index.html, homepage/en/index.html

Update release.json when a new version ships; the download links, sizes,
checksums and JSON-LD version all come from it. The FAQ list in each language
file feeds both the visible FAQ and the FAQPage JSON-LD, so they stay in sync.
"""
import hashlib
import html
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
OUT = {"zh": ROOT / "homepage" / "index.html", "en": ROOT / "homepage" / "en" / "index.html"}

ASSET_KEYS = {
    "mac": "mac", "win": "win",
    "debamd": "linDebAmd", "taramd": "linTarAmd",
    "debarm": "linDebArm", "tararm": "linTarArm",
}
ELECTRON_MAX_KB = 150 * 1024


def strip_tags(s):
    return re.sub(r"<[^>]+>", "", s)


def kb(n_bytes):
    return f"{int(n_bytes / 1024 + 0.5)} KB"  # half-up, so 818.5 KiB reads 819 KB everywhere


def asset_hash():
    """Cache-busting key: changes whenever any shipped script, style or skin bundle changes."""
    h = hashlib.sha1()
    home = ROOT / "homepage"
    for name in ("site.css", "site.js", "keyboard3d.js", "pet/spec.js", "pet/renderer.js", "pet/web-pet.js", "pet/skins.json"):
        h.update((home / name).read_bytes())
    return h.hexdigest()[:10]


def release_values(rel):
    v = {"version": rel["version"], "tag": rel["tag"], "mirror": rel["mirror"], "build": asset_hash()}
    for short, key in ASSET_KEYS.items():
        a = rel["assets"][key]
        v[f"{short}_file"] = a["file"]
        v[f"{short}_size"] = kb(a["bytes"])
        v[f"{short}_sha"] = a["sha256"]
        v[f"{short}_cn"] = rel["mirrorFile"].format(file=a["file"])
        v[f"{short}_gh"] = rel["github"].format(file=a["file"])
        v[f"{short}_ratio"] = f"{(a['bytes'] / 1024) / ELECTRON_MAX_KB:.5f}"
    return v


def faq_html(items):
    out = []
    for i, (q, a) in enumerate(items):
        out.append(
            f'          <details class="qa"{" open" if i == 0 else ""}>\n'
            f'            <summary><span>{html.escape(q)}</span><svg class="i i-xs" aria-hidden="true"><use href="#i-down" /></svg></summary>\n'
            f'            <p>{html.escape(a)}</p>\n'
            f'          </details>'
        )
    return "\n".join(out)


def jsonld(lang, d, v):
    zh = lang == "zh"
    graph = [
        {
            "@type": "Person", "@id": "https://kurocat.cc/#creator", "name": "chanke17",
            "url": "https://github.com/chanke17",
            "sameAs": ["https://github.com/chanke17", "https://github.com/chanke17/KuroCat"],
        },
        {
            "@type": "WebSite", "@id": "https://kurocat.cc/#website", "name": "KuroCat",
            "alternateName": ["KuroCat 官网", "KuroCat 桌宠", "KuroCat Desktop Pet", "Kuro Cat"],
            "url": "https://kurocat.cc/", "inLanguage": ["zh-CN", "en"],
            "publisher": {"@id": "https://kurocat.cc/#creator"},
        },
        {
            "@type": "SoftwareApplication", "@id": "https://kurocat.cc/#software", "name": "KuroCat",
            "alternateName": ["Kuro Cat", "KuroCat 桌面萌猫", "KuroCat Desktop Cat Pet"],
            "applicationCategory": "UtilitiesApplication",
            "operatingSystem": "macOS 11+, Windows 7 SP1, Windows 8, Windows 10, Windows 11, Linux, UnionTech UOS 20, Kylin V10",
            "softwareVersion": v["version"],
            "dateModified": "2026-11-26",
            "fileSize": v["win_size"].replace(" ", ""),
            "license": "https://opensource.org/licenses/MIT",
            "isAccessibleForFree": True,
            "offers": {"@type": "Offer", "price": "0", "priceCurrency": "CNY" if zh else "USD"},
            "url": d["canonical"],
            "downloadUrl": v["mirror"],
            "installUrl": "https://github.com/chanke17/KuroCat/releases/latest",
            "codeRepository": "https://github.com/chanke17/KuroCat",
            "sameAs": ["https://github.com/chanke17/KuroCat", "https://workshop.kurocat.cc/"],
            "image": "https://kurocat.cc/assets/og-cover.png",
            "screenshot": [
                "https://kurocat.cc/assets/preview_siamese.jpg",
                "https://kurocat.cc/assets/preview_black.jpg",
                "https://kurocat.cc/assets/preview_orangeWhite.jpg",
            ],
            "featureList": [strip_tags(d[k]) for k in ("s1_t", "s2_t", "s3_t", "s4_t", "s5_t", "native_h", "skins_h")],
            "description": d["meta_desc"],
            "inLanguage": d["html_lang"],
            "author": {"@id": "https://kurocat.cc/#creator"},
            "publisher": {"@id": "https://kurocat.cc/#creator"},
        },
        {
            "@type": "FAQPage",
            "inLanguage": d["html_lang"],
            "mainEntity": [
                {"@type": "Question", "name": q, "acceptedAnswer": {"@type": "Answer", "text": a}}
                for q, a in d["faq"]
            ],
        },
    ]
    text = json.dumps({"@context": "https://schema.org", "@graph": graph}, ensure_ascii=False, indent=2)
    return text.replace("</", "<\\/")


def render(lang):
    d = json.loads((HERE / f"{lang}.json").read_text(encoding="utf-8"))
    rel = json.loads((HERE / "release.json").read_text(encoding="utf-8"))
    v = release_values(rel)
    values = {**v, **{k: val for k, val in d.items() if isinstance(val, str)}}
    values["@faq_html"] = faq_html(d["faq"])
    values["@jsonld"] = jsonld(lang, d, v)
    values["@i18n_js"] = json.dumps(d["js"], ensure_ascii=False).replace("</", "<\\/")

    tpl = (HERE / "template.html").read_text(encoding="utf-8")
    missing = set()

    def sub(m):
        key = m.group(1)
        if key not in values:
            missing.add(key)
            return m.group(0)
        return values[key]

    page = re.sub(r"\{\{\s*(@?[\w.]+)\s*\}\}", sub, tpl)
    if missing:
        sys.exit(f"[{lang}] missing keys: {', '.join(sorted(missing))}")
    OUT[lang].parent.mkdir(parents=True, exist_ok=True)
    OUT[lang].write_text(page, encoding="utf-8")
    print(f"wrote {OUT[lang].relative_to(ROOT)} ({len(page) // 1024} KB)")


if __name__ == "__main__":
    for lang in ("zh", "en"):
        render(lang)

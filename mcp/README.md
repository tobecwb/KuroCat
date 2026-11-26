# KuroCat MCP Server (`kurocat-mcp`)

Official **Model Context Protocol (MCP)** server for **[KuroCat](https://kurocat.cc/)** — the 802 KB native open-source desktop cat pet for macOS, Windows, Linux, and UnionTech UOS.

Zero external npm dependencies (pure Node.js 18+ ESM). Connect any MCP-compatible AI client (Claude Desktop, Cursor, Windsurf, LobeChat, Antigravity) to design custom `.kurocat` v1 cat skins, author `.kurokb` 65% mechanical keyboard themes, and browse or install free creations directly from the **[KuroCat Community Plaza](https://workshop.kurocat.cc/)**.

## Quick Start

### Recommended: Run via `npx` (Zero Install)

```json
{
  "mcpServers": {
    "kurocat": {
      "command": "npx",
      "args": ["-y", "kurocat-mcp"]
    }
  }
}
```

### Alternative: Run from Source

```bash
git clone https://github.com/chanke17/KuroCat.git
node KuroCat/mcp/server.mjs
```

```json
{
  "mcpServers": {
    "kurocat": {
      "command": "node",
      "args": ["/path/to/KuroCat/mcp/server.mjs"]
    }
  }
}
```

## Available MCP Tools

| Tool | Description |
| :--- | :--- |
| `generate_cat_skin` | Design & validate a `.kurocat` v1 cat skin from breed presets, hex palette, 2.5D body shape sliders, and cubic bezier coat regions — with optional 1-click install into your local KuroCat app. |
| `generate_keyboard_theme` | Design & validate a custom `.kurokb` 65% mechanical keyboard & mouse theme with optional 1-click local install. |
| `browse_community_plaza` | Live-search free community skins (Xiao Douni, Monster Hunter Palico, Li Hua Tabby, etc.), 2.5D accessories, and keyboards on https://workshop.kurocat.cc/. |
| `install_community_asset` | Download and install any community `.kurocat` skin or `.kurokb` keyboard by slug straight into macOS / Windows / Linux KuroCat. |
| `list_installed_skins` | Inspect custom skins and keyboards currently installed in your local KuroCat data directory. |
| `validate_kurocat_json` | Strictly validate `.kurocat` or `.kurokb` JSON against KuroCat's cross-platform v1 schema. |

## Resources & Prompts
- **Resources**: `kurocat://spec/skin-format` · `kurocat://plaza/catalog`
- **Prompts**: `design_my_cat` · `design_matching_setup`

## Official Links
- Website: https://kurocat.cc/ (Chinese) · https://kurocat.cc/en/ (English)
- OBS & Streaming Setup Guide: https://kurocat.cc/obs-guide/
- Online Skin Workshop & Community Plaza: https://workshop.kurocat.cc/
- Source Code: https://github.com/chanke17/KuroCat

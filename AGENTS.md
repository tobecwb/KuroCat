# KuroCat Agent 开发规范（必读）

任何 AI Agent 或开发者在修改本仓库代码、回退提交或发布新版本之前，**必须先完整阅读**：

- [docs/PRODUCT_PROGRESS.md](docs/PRODUCT_PROGRESS.md)

## 强制执行清单

1. **改前必读**：开始任何任务前，先阅读 `docs/PRODUCT_PROGRESS.md` 第一节「不可违背的产品铁律」与第二节「明确待修 Bug 与待办进度」，确认本次改动不会破坏已确认的视觉/交互/性能红线。
2. **严禁误回退**：执行 `git checkout` / `git revert` 或重构渲染器（尤其是 `drawNativeScruffDragCat`、`drawDragLimb`、`drawMark`）时，必须逐条核对红线清单（例如：**拎起前臂必须与坐姿前臂完全一致**、**听歌拖拽保留耳机**、**点击处即对齐后颈**、**喷枪印章缓存不可退化**）。
3. **三端同步**：macOS（`Sources/`）、Windows（`windows/src/`）、Linux（`linux/src/`）的行为、配置项与修复必须同步；涉及坐标参数必须修改 `shared/rig.json` 并运行 `python3 scripts/gen-rig.py`。
4. **改后必测**：
   - `bash scripts/check-rig.sh`
   - `bash scripts/check-skins.sh`
   - `bash scripts/check-eye-closure.sh`
   - `c++ -std=c++17 -O2 -I shared tests/skin-format/parse_test.cpp -o /tmp/parse_test && /tmp/parse_test`
   - `./build.sh`（macOS）
5. **改后必更新文档**：每次完成 Bug 修复、功能变更或发版后，必须同步更新 `docs/PRODUCT_PROGRESS.md` 中的待办状态与版本履历。

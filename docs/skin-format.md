# `.kurocat` 皮肤格式 · formatVersion 1 / 2

KuroCat 的猫是**实时绘制的矢量**（头会转、爪子会去够键盘、眼睛会眨），所以皮肤不是贴图，而是一份**数据**：
部位颜色 + 曲线分区 + 矢量花纹 + 体型参数 + 跟随部位的贴纸。三端（macOS / Windows / Linux）用同一份
数据、各自的原生渲染器画出来；皮肤工坊网页（M1）产出的也正是这个格式。

- 文件：一个 UTF-8 JSON 对象，扩展名 `.kurocat`，**不超过 2 MB**。
- 解析规则（三端一致）：
  - **数字会被夹到合法范围**（例如 `bodyWidth: 9` 按 `1.35` 处理）；
  - **未知字段一律忽略**（方便以后加字段，旧版本照样能读）；
  - **类型错误、颜色写错、缺必填项、JSON 语法错误 → 整个文件拒绝导入**，并给出中文原因；
  - `formatVersion` 大于本版本支持的值 → 拒绝，提示“请先升级 KuroCat”；`minAppVersion` 比正在运行的 KuroCat 新 → 同样拒绝并提示升级（1.5.3 起，见 §1.1）。
- 参考实现：`shared/SkinSpec.hpp`（C++，Windows/Linux）、`Sources/CatPhysics.swift` 末尾的 `SkinSpec`（macOS）、
  `KuroCat-Workshop/spec.js`（网页；`homepage/pet/spec.js` 是它的原样拷贝，用 `scripts/sync-web-spec.sh` 同步）。
- 部位坐标怎么映射到画布（转头角度、球面尺寸、身体压扁）只写在 `shared/rig.json` 一处，各端由 `scripts/gen-rig.py`
  生成常数；`bash scripts/check-rig.sh` 检查各端映射与规则是否一致（CI 的 macOS 任务会跑）。
- 模板：`skins/black.kurocat`、`skins/siamese.kurocat`、`skins/shorthair.kurocat`、`skins/orangeWhite.kurocat`
  （四款内置毛色，由 `scripts/generate-coat-patterns.py` 生成）；示例：`skins/samples/curve-regions-demo.kurocat`。

## 1. 顶层字段

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `formatVersion` | 整数 | ✔ | `1` 或 `2`，见 §1.1 |
| `minAppVersion` | 字符串 |  | 至少需要哪个版本的 KuroCat，如 `"1.4.8"`；皮肤工坊导出时自动计算，见 §1.1 |
| `name` | 字符串 | ✔ | 皮肤名，显示在「猫咪皮肤」菜单里；去掉首尾空白后不能为空，超过 40 字截断 |
| `author` | 字符串 |  | 作者，超过 40 字截断 |
| `palette` | 对象 | ✔ | 部位颜色，见 §3 |
| `regions` | 数组 |  | 曲线分区（钢笔工具画出来的色块），≤ 64 个，见 §4 |
| `marks` | 数组 |  | 矢量花纹（多边形 / 描边），≤ 1024 条（1.5.2 及更早 ≤ 256），见 §5 |
| `shape` | 对象 |  | 体型参数，见 §6 |
| `headSurface` | `"dome"` \| `"plane"` |  | 头部花纹贴在哪种曲面上转动，默认 `"dome"`，见 §2.2 |
| `idleLegs` | `"legs"` \| `"chest"` |  | 坐着时两条前腿的画法：`"legs"`（缺省）用 `palette.legs` 从肩膀往下渐显；`"chest"` 保持胸口颜色、只画爪子和腿缝线（四款内置毛色和模板用它） |
| `decorations` | 数组 |  | 贴纸（PNG），≤ 8 个，见 §7 |
| `partGradients` | 对象 |  | 按部位的渐变底色，键为 `body` / `head` / `tail` / `leftArm` / `rightArm` / `hindLeg`，值为 `{ "from": "#…", "to": "#…", "angle": 0, "stop": 0.5 }`（`from`/`to` 也可写 `start`/`end`；`angle` −360…360 度；`stop` 0.1…0.9 是中点位置） |
| `renderStyle` | 字符串 |  | 整只猫换成特殊造型：`"douni"`（小豆泥）、`"dazed"`（呆猫）、`"beagle"`（比格）；其他值按普通小猫画。网页另有 `"damiao"`、`"naimi"` 两种，**桌宠还不支持**，导入后显示成普通小猫 |
| `typingStyle` | 字符串 |  | 打字动作：`"douni"` 为小豆泥式拍键盘，缺省为普通 |
| `preview` | 字符串 |  | base64 PNG 预览图（≤ 512 KB），给网页/图库用，桌宠本身不显示 |

颜色一律写成 `"#RRGGBB"` 或 `"#RRGGBBAA"`（AA 为不透明度，`FF` = 不透明）。

### 1.1 版本：`formatVersion` 与 `minAppVersion`

字段是一版一版加进来的，旧版桌宠遇到不认识的字段会**忽略**（画出来和网页不一样），遇到不认识的部位名会**整份拒绝**。
为了让用户看到的永远是“请先升级”而不是一句看不懂的错误：

- 皮肤工坊导出时，按皮肤**实际用到**的功能算出 `minAppVersion`（只有用到 1.4.0 之后的功能时才写）：

  | 从这个版本起 | 功能 |
  |---|---|
  | 1.4.0 | `.kurocat` 皮肤本身、`idleLegs` |
  | 1.4.1 | `renderStyle: "douni"`、`typingStyle` |
  | 1.4.4 | `renderStyle: "dazed"`、`leftArm` / `rightArm` 部位、`shape.earBaseWidth` / `neckWidth` |
  | 1.4.5 | `hindLeg` 部位、`partGradients`、`palette.headBody` / `hindLeg` / `hindPaw` / `irisLeft` / `irisRight` / `pupil` / `pupilLeft` / `pupilRight`、`shape.armLength` / `armRootY` / `armSpacing` / `earSpacing` / `tailRootY` |
  | 1.4.6 | `partGradients.*.stop`、`shape.hindLegReach` / `hindLegRootX` / `hindLegRootY` / `hindLegWidth` |
  | 1.4.8 | 花纹笔刷 `brush`（`soft` / `airbrush` / `smudge`）与 `softness` |
  | 1.5.0 | `renderStyle: "beagle"` |
  | 1.5.3 | 超过 256 条 `marks`（上限 1024） |

  体型参数只有**偏离缺省值**时才算“用到”；颜色只有和它的后备色（如 `hindLeg` 之于 `legs`）不同才算。
- `minAppVersion` 高于 1.5.2 时写 `formatVersion: 2`，否则写 `1`。1.5.2 及更早只认 1，所以会直接提示升级；
  1.5.3 起认 1 和 2，并在 `minAppVersion` 比自己新时拒绝、提示升级。
- 规则写在 `KuroCat-Workshop/spec.js` 的 `requiredAppVersion()`；**新加一个皮肤字段时，同时在那里登记它的首发版本**，并更新上表。

## 2. 坐标系（部位空间）

整只猫画在 **360 × 320** 的逻辑画布上（y 轴向下），然后按用户的缩放设置放大。皮肤里的每个坐标都属于
某个**部位空间**，跟着那个部位一起动、一起变形——这样头转过去，脸上的花纹也跟着转过去。

![部位坐标系示意图](skin-format-parts.png)

```
                 画布 (0,0) ───────────────────────────────▶ x (360)
                  │            leftEar        rightEar
                  │           (-50,0)→(50,0) 是耳根，(0,-100) 是耳尖
                  │               ╱╲            ╱╲
                  │              ╱  ╲  head    ╱  ╲
                  │             │  ┌──────────────┐ │       原点 = 头部中心（静止时约 (184,126)）
                  │             │  │  ●  (0,0)  ● │ │       x 向右、y 向下，单位 = 画布单位
                  │             │  │   眼  ▾  眼   │ │       脸约占 x∈[-46,46]、y∈[-38,30]
                  │             └──┴──────────────┴─┘
                  │        ╲ ─ ─ 下颌线 (±44,8)→(0,34) ─ ─ ╱    头部分区在线上方，身体分区在线下方
                  │               ╭──────────────╮
                  │              ╱                ╲  body     原点 = 坐垫锚点 (184,226)
                  │             │        ·         │          身体约占 x∈[-52,52]、y∈[-100,17]
                  │             │                  │   tail   tail: x = 0(尾根)…100(尾尖)
                  │              ╲______(0,0)______╱ ~~~~~~~  y = 垂直于尾巴的偏移
                  ▼ y (320)              [ 键 盘 ]
```

### 2.1 各部位

| `space` | 原点与轴 | 范围（默认体型） | 跟随什么 |
|---|---|---|---|
| `body` | 坐垫锚点 `(184, 226)`；x 右、y 下 | 剪影约 `x∈[-52,52]`，`y∈[-100,17]`（`y=-14` 是胯部最宽处，`y=12` 是坐着的底边） | 身体倾斜、压扁回弹（呼吸/打字时的 squash），`shape.bodyWidth` 乘在 x 上 |
| `head` | 头部中心；x 右、y 下 | 脸 `x∈[-46,46]`，`y∈[-38,30]`；眼睛中心 `(±17.2,-2)`，胡须根 `(±24, 10…18)` | 头的左右转（yaw）、抬低头（pitch）、歪头（tilt）+ `shape.headScale` |
| `skull` | 同 `head` | 同 `head` | **只**跟随歪头和头部位移，**不做 3D 转动**。头顶、额头、刘海、眼罩都应该画在 `head`——画在 `skull` 上的花纹转头时不动，眼睛会从花纹里滑出去。皮肤工坊已不再提供这个部位，打开含 `skull` 的旧皮肤会提示一键改为 `head`（正脸外观不变） |
| `tail` | x = 沿尾巴中线的位置，`0` 尾根 → `100` 尾尖；y = 垂直偏移 | 尾巴半宽 = `5.5 × shape.tailThickness`，所以 `y∈[-5.5,5.5]` 就盖满尾巴 | 摇尾巴、捕猎时压低；`shape.tailLength` / `tailThickness`（y 会随粗细一起缩放） |
| `leftEar` / `rightEar` | 耳朵自己的框：`(-50,0)` 内侧耳根，`(50,0)` 外侧耳根，`(0,-100)` 耳尖 | 两只耳朵**自动镜像**（x 总是“由内向外”） | 转头、折耳、抖耳朵、`shape.earSize` / `earPointiness` |
| `leftArm` / `rightArm` | x = 沿胳膊中线，`0` 肩膀 → `100` 爪尖；y = 横向，`±10` 是胳膊两侧边缘 | `y∈[-10,10]` 盖满胳膊 | 打字、拍键盘时胳膊怎么弯，花纹就怎么弯；两条胳膊的 y 方向互为镜像 |
| `hindLeg` | 同胳膊：x `0` 胯部 → `100` 后爪；y `±10` 是两侧边缘 | 同上 | 踢键盘的那条后腿 |

尾巴的 y 方向：沿着尾巴从根走向尖时，`y > 0` 在行进方向的右手边（静止时尾根一段朝右伸，`y>0` 就是下侧）。

### 2.2 头部的 3D 转动（`headSurface`）

`head` 空间的点会被投影到一个立体表面上，再随转头旋转：

- `"dome"`（默认）：椭球面，深度 `z = 29.6·√(1 − x²/46² − y²/40²)`。越靠近脸的边缘，转头时位移越小——适合任意位置的花纹。
- `"plane"`：一张 `z = 27.5` 的平面贴在脸前面（内置美短的脸部虎斑用的就是它）。

转头角度：yaw = `headYaw × 0.50` 弧度，pitch = `headPitch × 0.36` 弧度，先 yaw 后 pitch，正交投影，然后整体歪头 tilt、乘 `headScale`。

### 2.3 裁剪：一条曲线只需要“穿过”部位

所有分区和花纹都会被**裁剪到所属部位的轮廓**里，轮廓外的部分直接切掉：

| 部位 | `regions` 裁剪到 | `marks` 裁剪到 |
|---|---|---|
| `head` / `skull` | 猫剪影 ∩ 下颌线**上方** | 整个猫剪影（头+身体） |
| `body` | 猫剪影 ∩ 下颌线**下方** | 整个猫剪影（头+身体） |
| `tail` | 尾巴 | 尾巴 |
| `leftEar` / `rightEar` | 那只耳朵 | 那只耳朵 |

**下颌线**是头部空间里的一条曲线：从一侧腮帮下沿 `(−44·cheekFullness, 8)` 弯到下巴 `(0, 34)` 再到另一侧
`(44·cheekFullness, 8)`，两端水平延伸到画面外；它和头部花纹一样做 3D 投影、随转头/歪头移动（实现：`skinJawSide()`）。
腮帮以外的部分天然落在剪影边缘，所以头、身两种颜色不同时，看到的分界就是一条自然的下颌线，而不是脖子上的一道横线。

所以「把身体分成橘色背部 / 白色胸口」只要一条从剪影左边穿到右边的分界曲线，再把两侧各自闭合到剪影外面即可；
下颌线保证身体分区不会染到脸上（头部分区也不会流到胸口）。体型变胖变瘦后轮廓会变，分区最好比轮廓**多画出去 10–20 单位**；
转头/抬头时下巴会露出更多脖子，身体分区的上端最好收窄到下巴下方（示例皮肤就是这么做的）。

## 3. `palette`

只有 `body` 必填，其余缺省时按下表推导。

| 键 | 画在哪里 | 缺省值 |
|---|---|---|
| `body` | 头+身体剪影的底色 | **必填** |
| `bodyShade` | 有它时底色变成从上 `body` 到下 `bodyShade` 的竖向渐变 | 无（纯色） |
| `belly` | 胸口/白色区域：伸出去的前腿从肩膀处的 `belly` 渐变到 `legs` | 无（不渐变） |
| `legs` | 伸出去的前腿、第三只脚 | `body` |
| `paws` | 坐着时露出的后爪、前爪的“白袜子”；与 `legs` 相同则不画袜子 | 有 `points` 时 = `points`，否则 = `legs` |
| `pawPads` | 后爪上的小肉垫 | 无（不画） |
| `earOuter` | 耳朵 | 有 `points` 时 = `points`，否则 = `legs` |
| `earInner` | 耳朵内侧 | 无（不画） |
| `nose` / `mouth` | 鼻子 / “ω”嘴 | 无（不画） |
| `iris` | 虹膜；设置后**覆盖**菜单里选的眼睛颜色 | 无（用菜单里的眼睛颜色） |
| `irisLeft` / `irisRight` | 左 / 右眼虹膜（异瞳）| `iris` |
| `pupil` / `pupilLeft` / `pupilRight` | 瞳孔颜色 | 无（深色瞳孔） |
| `headBody` | 头部底色；与 `body` 不同时沿下颌线（§2.3）分开 | `body` |
| `hindLeg` / `hindPaw` | 踢键盘那条后腿 / 后爪 | `legs` / `paws` |
| `tail` | 尾巴 | `earOuter` |
| `tailTip` | 尾巴最后 22%（t ∈ [0.78, 1] 那一段） | 无 |
| `stripes` | 没写 `color` 的 `marks` 用这个颜色 | 无 |
| `whiskers` | 胡须 | 深色身体 `#E0E6EBAD`，浅色身体 `#8D8170AD` |
| `crease` | 腿上的轮廓线、后爪的弧线 | 深色身体 `#D1EBE3C2`，浅色身体 `#8B817199` |
| `outline` | 剪影外描边（1 单位）；有 `points` 时也用作前腿外缘线 | 无 |
| `points` | **重点色**（暹罗式）：有它就画随头转动的面部重点色、渐变到 `points` 的腿和袜子 | 无 |
| `pointsMid` | 重点色渐变的中间色 | `body` 与 `points` 的平均 |

“深色身体”= `body` 的亮度 `0.2126R+0.7152G+0.0722B < 0.45`。

## 4. `regions` —— 曲线分区（钢笔工具）

给网页编辑器的「曲线分区」用：点锚点、拖手柄弯出一条分界线，点两侧各填一种颜色。每个分区是**一条闭合的三次贝塞尔路径**。

```json
{
  "space": "body",
  "fill": "#F7F1E6",
  "alpha": 1,
  "anchors": [
    { "x": -70, "y": -40, "inX": -70, "inY": -40, "outX": -30, "outY": -62 },
    { "x":  70, "y": -40, "inX":  30, "inY": -62, "outX":  70, "outY": -40 },
    { "x":  70, "y":  30 },
    { "x": -70, "y":  30 }
  ]
}
```

- `space`：`head` / `skull` / `body` / `tail` / `leftEar` / `rightEar` / `leftArm` / `rightArm` / `hindLeg`（§2）。
- `fill`：填充色；`alpha`（0–1，缺省 1）再乘到 `fill` 的不透明度上。
- `anchors`：2–128 个锚点。`x,y` 是锚点，`inX,inY` / `outX,outY` 是**进入 / 离开**手柄的**绝对坐标**（同一部位空间）。
  省略手柄 = 手柄与锚点重合（尖角）。锚点 *i* 到 *i+1* 的一段是
  `P0=(x_i,y_i)  P1=(outX_i,outY_i)  P2=(inX_{i+1},inY_{i+1})  P3=(x_{i+1},y_{i+1})`，最后一个锚点连回第一个（自动闭合）。
- 坐标夹到 `[-400, 400]`。
- 渲染：每段三次曲线均匀细分 16 段，逐点映射到部位空间（头部会做 3D 投影），非零环绕规则填充，再按 §2.3 裁剪。
- 叠放顺序：**底色 → regions（按数组顺序）→ 重点色 → marks**。

## 5. `marks` —— 矢量花纹

与内置毛色的 `CoatMark` 同一个模型（内置美短/橘白的虎斑就是这样存的）：

```json
{ "space": "head", "kind": "stroke", "color": "#A57543", "alpha": 0.48, "width": 2.1,
  "points": [[43, 3], [38, 6.5], [29, 7]] }
```

- `kind`：`"polygon"`（闭合填充，≥ 3 点）或 `"stroke"`（圆角折线，≥ 2 点，`width` 0.2–24，缺省 2）。
  描边两端是圆头；`tail` 空间的描边是**平头**（它们是绕尾巴一圈的色带，内置花纹猫的两道尾环就是这样画的）。
- `color` 缺省时用 `palette.stripes`（两者都没有则拒绝）；`alpha` 0–1 乘到颜色上。
- `points`：≤ 512 个 `[x, y]`，坐标夹到 `[-400, 400]`。
- 描边可加 `brush`：`"round"`（缺省）、`"soft"`（柔边）、`"airbrush"`（喷枪）、`"smudge"`（涂抹），柔和程度 `softness` 0–1（缺省 0.65）。1.4.8 起支持。
- `head` / `skull` / `body` 的花纹裁剪到整个猫剪影，`tail` / 耳朵的裁剪到各自部位（§2.3）。

## 6. `shape` —— 体型

所有参数缺省 = 现在的小黑猫。缩放都以**坐垫锚点**为基准，所以前爪仍然够得到键盘和鼠标，摸头/摸背的判定区域也同步变化；
`regions` / `marks` / 贴纸都挂在各自部位上，会跟着变形。

| 键 | 范围 | 缺省 | 作用 |
|---|---|---|---|
| `build` | `"standard"` / `"sleek"` | `"standard"` | 基础骨架：`sleek` 是暹罗那种更修长的比例（头略高、腰更收、尾巴略细、耳朵更小更圆；配「暹罗宝石蓝」瞳色时眼型也更小巧） |
| `bodyWidth` | 0.85 – 1.35 | 1 | 胖瘦：身体、胯部、后爪、肩膀、尾根的 x 方向 |
| `headScale` | 0.90 – 1.15 | 1 | 头身比：整个头（含眼睛、胡须、耳朵、头部花纹） |
| `cheekFullness` | 0.85 – 1.25 | 1 | 腮帮子宽度 |
| `earSize` | 0.80 – 1.25 | 1 | 耳朵大小（绕耳根中点缩放） |
| `earPointiness` | 0.50 – 1.50 | 1 | 耳尖：小于 1 更圆，大于 1 更尖 |
| `tailLength` | 0.70 – 1.30 | 1 | 尾巴长度（绕尾根缩放） |
| `tailThickness` | 0.70 – 1.50 | 1 | 尾巴粗细 |
| `pawStagger` | 0 – 1 | 1 | 右后爪比左后爪低多少（1 = 小黑猫那种一高一低，0 = 对称） |
| `neckWidth` | 0.65 – 1.40 | 1 | 脖子粗细 |
| `earBaseWidth` | 0.65 – 1.40 | 1 | 耳根宽度 |
| `earSpacing` | 0.60 – 1.50 | 1 | 两耳间距 |
| `tailRootY` | −30 – 30 | 0 | 尾根上下位置 |
| `armSpacing` | 0.50 – 1.60 | 1 | 两条前腿的间距 |
| `armRootY` | −30 – 30 | 0 | 前腿根部上下位置 |
| `armLength` | 0.60 – 1.50 | 1 | 前腿长度 |
| `hindLegRootX` / `hindLegRootY` | −24 – 24 | 0 | 后腿根部位置 |
| `hindLegWidth` | 0.60 – 1.60 | 1 | 后腿粗细 |
| `hindLegReach` | 0.65 – 1.50 | 1 | 后腿伸多远 |

（皮肤工坊的滑块范围可能更窄；读文件时一律按上表夹取，三端和网页一致，`tests/rig/rules_test.mjs` 会检查。）

## 7. `decorations` —— 贴纸

```json
{ "png": "iVBORw0KGgo…", "anchor": "headTop", "offsetX": 0, "offsetY": -6, "scale": 1, "rotation": -8 }
```

- `png`：base64 PNG（可带 `data:image/png;base64,` 前缀），解码后 ≤ 1 MB、≤ 1024×1024 像素。
- 贴纸按 **2 倍图**理解：`scale = 1` 时 1 个图片像素 = 0.5 个画布单位（一张 100 px 宽的图 = 50 单位宽）。`scale` 0.05–4。
- `anchor`：
  - `headTop`：头顶 `head (0,-30)`；`face`：脸中央 `head (0,4)` —— 两者都随转头移动、随歪头旋转，并按转头角度横向压缩；
  - `neck`：脖子 `body (0,-64)`，随身体倾斜；
  - `tailTip`：尾尖，随摇尾移动（贴纸本身保持正立）。
- `offsetX/offsetY`：在锚点所属部位空间里的偏移，−120 … 120；`rotation`：额外旋转角度（度），−180 … 180。
- 贴纸画在眼睛/胡须之上、前爪之下。

## 8. 应用内的行为

- 菜单「猫咪皮肤 🐱 → 导入皮肤…」打开 **「换装间」** 窗口（三端同一套设计：奶油底色、左上角实时预览正在穿的皮肤、
  虚线拖放区、橘色「选择文件…」按钮；点拖放区也能选文件）。也可以直接把 `.kurocat` 拖到桌面上的小猫身上。

  ![换装间](screenshots/dressing-room-linux.png) ![导入失败时的提示](screenshots/dressing-room-linux-error.png)
- 导入时先完整校验；通过后复制到用户数据目录，文件名为内容哈希（FNV-1a 64 位，16 个十六进制字符）：
  - macOS：`~/Library/Application Support/KuroCat/skins/`
  - Windows：`%APPDATA%\KuroCat\skins\`
  - Linux：`$XDG_DATA_HOME/kurocat/skins/`（缺省 `~/.local/share/kurocat/skins/`）
- 导入的皮肤列在「猫咪皮肤」子菜单里，可在「删除皮肤」里删除；选择会被记住（配置项 `customSkin`），文件丢失时自动回到内置毛色。
- 校验失败时给出中文原因，例如：`palette.nose 颜色格式应为 #RRGGBB 或 #RRGGBBAA，现在是 "pink"`——
  换装间开着时显示在窗口里的红色状态行，直接拖到小猫身上时弹出对话框；成功时显示绿色的「✓ 已换上「…」」。

## 9. 内置毛色与模板

四款内置毛色在三端代码里都写成了 `SkinSpec`（`builtInSkin()` / `SkinSpec.builtIn(_:)`），颜色沿用各端渲染器原来的精确数值，
所以重构前后像素完全一致（Linux 上 4 款 × 19 个姿势逐字节对比通过，见 `tests/linux-render/render_poses.cpp`）。
`skins/*.kurocat` 是它们的可导入模板，由 `scripts/generate-coat-patterns.py` 和花纹原稿一起生成；颜色取整到 `#RRGGBB(AA)`，
导入后与内置版在预乘色彩空间里每通道相差不超过 3/255。皮肤工坊可以直接以这些模板为起点。

## 10. 完整示例

见 `skins/samples/curve-regions-demo.kurocat`：橘色背 / 白胸口由**同一条** U 形分界曲线切开（两个分区共用这条曲线的锚点），
白色口鼻（head 分区，会随转头移动）、深色尾尖（tail 分区）、深色耳尖（leftEar/rightEar 分区）、内耳粉、肉垫、额头三道花纹，
以及 `bodyWidth 1.25` 的胖胖体型。下图是 Linux 版在 Xvfb 下的真实截图（待机 / 打字 / 转头）：

![待机](screenshots/curve-regions-demo-idle.png) ![打字](screenshots/curve-regions-demo-typing.png) ![转头](screenshots/curve-regions-demo-head-turned.png)

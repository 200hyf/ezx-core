# 更新日志

## 2.2.0

### 新增（场景 / 碰撞 / 相机）

- `RectF`：浮点矩形（世界坐标用，避免每帧取整丢精度），带 `right/bottom/cx/cy/toRect`；
- `overlap(a, b)`：两个 `RectF` 是否相交；
- `Camera2D`：`setViewSize` / `setPosition` / `follow(cx, cy, smooth)` / `clampToWorld` / `toDesign`（世界 → 设计坐标）；
- `Entity`：精灵 + 世界坐标（脚下中心）+ 速度 + 碰撞盒 + `layer` + `onUpdate` 回调；`box()` / `drawRect()` / `update()` / `draw()`；
- `Scene`：`entities`（`deque`，拿到的引用不会失效）+ `solids` 静态碰撞体 + `add/update/draw/removeInactive`；
- `moveAndCollide(e, dx, dy, solids)`：轴分离移动（先横后纵），返回 `hitX / hitY / grounded`。

### 示例

- 新增 `examples/platformer_demo.cpp`：重力、跳跃、平台碰撞、相机横向跟随；
- 新增 `build_platformer_demo.bat`；自测 `platformer_demo.exe --selftest 240`（脚本化输入跑 240 帧）。

### 兼容性

- 只新增接口，原有 API 行为不变。

## 2.1.0

### 新增（精灵与动画）

- `Image::drawPart(src, dst)`：按源矩形绘制，图集切帧用；
- `Image::crop(src)`：截一块作为新图（`SpriteSheet` 内部用它切帧）；
- `SpriteSheet`：`loadGrid`（一张图集按网格切帧，列数可传 0 自动算）/ `loadSequence`（一帧一个 png）；
- `Animation`：帧区间、FPS、循环、`play/stop/reset/update`、`finished()`；
- `Sprite`：位置、缩放、锚点（默认脚底中心）、当前帧、`update/draw/bounds`；
- `Input::keyHeld(vk)`：键现在是否按着 —— 做 WASD 连续移动用这个（`keyDown` 只是「这一帧刚按下」）。

### 示例与构建

- 新增 `examples/sprite_demo.cpp`：4 帧图集动画 + WASD 移动，支持 `--selftest N` 跑 N 帧自动退出；
- 新增 `build_sprite_demo.bat`：和 `build_demo.bat` 一样自动找 VS，找不到回退 `D:\VisualStudio2026` 等常见路径；
- 新增 `res/sprite_sheet.png`：自制 4 帧示例素材（32×32，共 128×32）。

### 兼容性

- 只新增接口，原有 API 行为不变；已有工程不用改代码。

## 2.0.0

### 结构

- 文档统一收敛到 `docs/`：教程、原理、FAQ、编辑器；
- 删除重复的 `使用说明.md`，内容合并进 FAQ/教程；
- 编辑器截图统一放到 `editor/images/`；
- 新增 `VERSION`、`CHANGELOG.md`、`build_demo.bat`。

### 库

- 保持两个文件（`easyx_core.h/.cpp`）即可接入；
- 窗口重建、全屏、动态限帧、Viewport 适配、输入、图片、TextInput、Pages 稳定；
- 修正 Debug `/MT` + `_DEBUG` 的 `LNK2038` 配置说明；
- 继续使用 UTF-8 BOM 源码 + `/utf-8`。

### UI 编辑器

- 可视化编辑：按钮 / 文字 / 面板 / 图片 / 文本输入 / 伸缩菜单；
- 按屏幕比例布局（`pctRect`）或固定设计坐标；
- 任意行列等分参考线 + 吸附；
- 多页面、背景图、按钮图标、控件背景图、自动居中；
- 伸缩菜单：上下左右/扇形展开、点击/悬停、主/子按钮宽分开、每项单独跳转/颜色/字号/逻辑；
- 预览：页面跳转、文本输入、菜单展开、其他逻辑日志提示；
- 生成 `ui_generated.cpp`：每个按钮/菜单项独立回调，页面跳转显式 `goPage`；
- 修复属性输入串到下一个控件的问题（属性字段绑定控件 UID）。

### 文档

- README 精简为快速接入 + 核心 API + 文档索引；
- 教程保留零基础 C++/窗口/适配/图片/输入/页面内容；
- 原理保留消息循环、双缓冲、窗口重建、编码、IME、alpha；
- 新增 FAQ，集中编译错误、资源路径、输入、发布问题。

## 1.0.0

- 初版：Window / Viewport / Input / Image / TextInput / Pages；
- 5 页 demo、详细教程、原理文档、UI 编辑器初版。

# 常见问题（FAQ）

> 详细用法看 [教程.md](教程.md)，内部原理看 [原理.md](原理.md)，编辑器看 [编辑器.md](编辑器.md)。

## 1. 为什么库不提供按钮？

按钮外观差异太大（圆角、图标、动画、九宫格），库只提供坐标、输入、绘制、页面这些底层能力。一个按钮 20 行就能写：

```cpp
struct UiButton
{
    Rect rect;
    std::string label;
    bool hovered = false;

    bool update(const Input& in, const Viewport& vp)
    {
        hovered = vp.R(rect).contains(in.pos());
        return hovered && in.clicked();
    }

    void draw(const Viewport& vp) const
    {
        vp.fill(rect, hovered ? RGB(60,86,132) : RGB(34,42,62));
        vp.textIn(rect, label, 28, RGB(236,242,255));
    }
};
```

不想手写坐标，直接用 [UI 编辑器](编辑器.md) 拖控件并生成按钮/菜单代码。

## 2. 编译/链接错误

### `LNK2038: RuntimeLibrary 不匹配`

EasyX 的库是 `/MT` 编译的，工程运行库必须选「多线程 (/MT)」，Debug/Release 都一样。
选 `/MD` 或 `/MTd` 会报：

```
MT_StaticRelease 不匹配 MDd_DynamicDebug
```

### `LNK2038: _ITERATOR_DEBUG_LEVEL 不匹配`

Debug 配置还定义了 `_DEBUG`，STL 进入 debug 模式。解决：

- 预处理器去掉 `_DEBUG`，加 `NDEBUG`；
- 或直接用 [Directory.Build.targets](../Directory.Build.targets)：复制到项目根目录，它会自动设置 `/MT`、`/utf-8`、`/U_DEBUG`、`NDEBUG`。

### 中文乱码

- 源文件保存为 **UTF-8 带 BOM**；
- 工程加 **`/utf-8`**；
- 只加 BOM 不够：BOM 决定源码怎么读，`/utf-8` 决定字符串按什么编码写进 exe。

### `D9025: 重写 /D_DEBUG`

正常。`/U_DEBUG` 会把 `/D_DEBUG` 去掉，避免 `MTd` 冲突。

## 3. 窗口

### 为什么每帧要 `win.poll()`？

`poll()` 负责：

- 处理拖拽/最大化/还原；
- 发现客户区尺寸变化后自动重建 EasyX 窗口；
- 重建完成后触发 `onResize`。

不调用 `poll()`，窗口被拖大后画面会错位。

### `onResize` 里做什么？

通常两件事：

```cpp
win.onResize([&](int w, int h) {
    vp.fit(w, h);
    pages.layoutAll(vp);
});
```

### `Fullscreen` / `WorkArea` / `Windowed`

- `Windowed`：普通窗口；
- `WorkArea`：最大化但不盖任务栏；
- `Fullscreen`：无边框、置顶、盖住任务栏。

没有“无边框但非全屏”的独立模式。

## 4. 分辨率适配

### 为什么要 `Viewport`？

按 1280×720 写坐标，窗口变成 1920×1080 时自动等比缩放，并留黑边。所有绘制都用设计坐标，鼠标命中用 `vp.hit(rect, in.pos())`。

### 黑边能去掉吗？

```cpp
vp.setScaleMode(ScaleMode::Cover);   // 裁边铺满
```

`Contain` 留黑边，`Cover` 铺满但可能裁掉边缘。

### 鼠标坐标为什么对不上？

鼠标是窗口坐标，绘制是设计坐标。命中判断用：

```cpp
if (vp.hit(rect, in.pos())) { ... }        // 推荐
// 或
Point p = vp.toDesign(in.pos());
if (rect.contains(p)) { ... }
```

## 5. 图片

### 图片应该放哪？

放 exe 旁边的 `res/`，加载时用：

```cpp
img.load(assetPath("res/logo.png"));
```

不要依赖“当前工作目录”，否则从 VS 启动和双击 exe 的路径可能不同。

### png 透明没生效？

`Image::load` 会处理 png alpha；用 `draw` / `drawFit` / `drawCover` 绘制。不要直接 `putimage`，那样会丢掉透明。

### 什么时候加载？

窗口创建之后、第一次绘制之前。通常放在 `init()` 或页面初始化里。

## 6. 中文输入

`Input` 已经处理 `WM_CHAR` / `WM_IME_CHAR`：

```cpp
for (wchar_t ch : in.chars()) { ... }       // UTF-16
std::string utf8 = in.charsUtf8();          // UTF-8
```

`TextInput` 直接支持中文，中文算 1 个字符，`maxLen` 也按字符算。

候选框位置：

```cpp
in.setImePosition(vp.R(box).x + 8, vp.R(box).bottom());
```

## 7. TextInput

- 先 `in.update()`，再 `textInput.update(in)`；
- 点击输入框后调用 `placeCursor(...)`；
- `draw(...)` 不裁剪，超长内容请限制 `maxLen`；
- 密码、只数字、数值钳制都有现成字段/方法，见 [教程.md](教程.md) 第 8 章。

## 8. 页面

一个页面三个函数：

```cpp
page.layout = [](const Viewport& vp) { /* 窗口尺寸变了才调用 */ };
page.update = [](const Input& in)    { /* 每帧处理输入 */ };
page.draw   = [](const Viewport& vp) { /* 每帧绘制 */ };
```

切换：

```cpp
pages.go("settings");
```

`Pages` 内部用 `std::deque`，`Page&` 引用在添加新页面后仍然有效。

## 9. 编辑器

- 打不开？用 Edge/Chrome 双击 `editor/ui_editor.html`，不需要服务器；
- 生成代码怎么接？看 [编辑器.md](编辑器.md)；
- 按钮逻辑写哪？每个按钮生成 `onButton_xxx()`，菜单项生成 `onMenu_xxx_ItemN()`；
- 预览里的“其他逻辑”只提示，不会真的执行 C++；真正运行要编译。

## 10. 发布

- `/MT` 静态链接，目标机器不需要 VC++ 运行库；
- 把 `exe` 和 `res/` 一起发；
- 图片路径用 `assetPath`；
- 发布前用 Release x64 编译。

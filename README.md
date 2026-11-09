# ezx-core —— EasyX 界面底座

只提供"底层能力"，**不提供任何按钮外观**。按钮、面板、列表都由你自己画。

| 模块 | 负责 |
|---|---|
| `Window` | 建窗、切全屏、最大化/最小化、拖拽缩放后自动重建 |
| `Viewport` | 分辨率适配：按 1280×720 写坐标，自动等比缩放 + 黑边 |
| `Page` / `Pages` | 页面框架：每页自己的 排版 / 逻辑 / 绘制 |
| `Image` | 图片加载与绘制（拉伸 / 等比 / 铺满，支持 png 透明） |
| `Input` | 鼠标键盘：位置、按下、松开、点击、双击、拖拽、滚轮；中文输入法（IME）定位 |
| `TextInput` | 文本编辑：UTF-8、中文、光标、退格、字符过滤、长度限制、数值钳制 |

![主菜单](preview/01-menu.png)

## 三步接入

1. 把 `easyx_core.h` + `easyx_core.cpp` 复制进工程，VS 里「添加 → 现有项」；
2. `#include "easyx_core.h"`；
3. 编译。EasyX 的 Windows 库已用 `#pragma comment(lib, ...)` 写进头文件；但 **运行库 `/MT`、`/utf-8`、EasyX 的 lib 目录**是工程级设置，源码管不了。

**不想手动配置**：把 `Directory.Build.targets` 也复制到**项目根目录**（和 `.vcxproj` 同级）。MSBuild 会自动导入它，强制 `/MT`、`/utf-8`、`/U_DEBUG`、加 EasyX 头文件/库目录。只复制两个源文件时，仍按下面“编译”一节手动设置。

> 看到一条 `D9025: 正在重写 /D_DEBUG（用 /U_DEBUG）` 是正常的：这是为了把 Debug 的 `_DEBUG` 去掉，否则会要求 `MTd`。

> 源文件保存为 **UTF-8 带 BOM**（已带），并且工程要加 **`/utf-8`** 编译选项。
> BOM 只决定“源码怎么读”，`/utf-8` 还决定“字符串按什么编码写进 exe”；不加时中文字面量会变成 GBK，和运行时 UTF-8 文本拼接会乱码。Unicode / 多字节工程都能编。

## 最小骨架

```cpp
#include "easyx_core.h"
using namespace ezx;

int main()
{
    Window::enableDpiAwareness();

    Window   win;
    Viewport vp;
    Input    in;
    Pages    pages;

    vp.setDesign(1280, 720);                 // 设计分辨率

    win.onResize([&](int w, int h) {         // 窗口一变 → 重新算缩放 + 重新排版
        vp.fit(w, h);
        pages.layoutAll(vp);
    });

    if (!win.open(1024, 640)) return 1;

    Page& home = pages.add("home");
    home.layout = [](const Viewport& vp) { /* 算位置 */ };
    home.update = [](const Input& in) { /* 处理输入 */ };
    home.draw   = [](const Viewport& vp) {
        vp.textIn(Rect{ 0, 100, 1280, 80 }, "你好", 54, RGB(255,255,255));
    };

    vp.fit(win.width(), win.height());
    pages.layoutAll(vp);
    pages.go("home");

    while (true)
    {
        win.poll();
        in.update();
        pages.update(in);

        win.beginFrame(RGB(10,10,14));
        vp.paintLetterbox(RGB(10,10,14));
        pages.draw(vp);

        win.endFrame();       // 动态限帧，默认 60 FPS
    }
}
```

## API

### Window

| 调用 | 说明 |
|---|---|
| `open(w, h, mode)` | 建窗；`Windowed` / `WorkArea`（保留任务栏）/ `Fullscreen`（无边框、置顶、盖住任务栏） |
| `poll()` | **每帧调用**：拖拽、最大化、还原后自动重建并触发 `onResize` |
| `onResize(fn)` | 尺寸变化回调，参数是新客户区宽高 |
| `applyMode(mode, w, h)` | 运行时切换（全屏按钮） |
| `minimize()` / `maximize()` / `restore()` | 最小化 / 最大化 / 还原 |
| `beginFrame()` / `endFrame()` | 清屏 / 翻页并按目标帧率动态限帧 |
| `setTargetFps(fps)` | 目标帧率，默认 60；0 = 不限帧 |
| `endFrame(10)` | 旧写法：固定睡 10ms，不推荐 |
| `setTitle("x")` | 标题（重建后自动恢复） |

> `Fullscreen` 是无边框+置顶（能盖住任务栏）；`WorkArea` 保留任务栏。没有“无边框但非全屏”的独立模式。

### Viewport（适配）

| 调用 | 说明 |
|---|---|
| `setDesign(1280, 720)` | 设计分辨率 |
| `fit(w, h)` | 窗口尺寸变了就调，重算缩放和黑边 |
| `setScaleMode(ScaleMode::Contain / Cover)` | 留黑边 / 裁边铺满；默认 Contain |
| `R(rect)` / `R(x,y,w,h)` / `scale()` | 设计矩形 → 屏幕矩形 / 当前缩放 |
| `hit(rect, 鼠标pos)` | 鼠标是否在设计矩形里（最常用的命中判断） |
| `toDesign(x, y)` | 鼠标坐标 → 设计坐标（命中判断用） |
| `fill` / `frame` / `text` / `textIn` | 用设计坐标画矩形/框/字 |
| `image` / `imageFit` / `imageCover` | 用设计坐标画图（拉伸 / 等比 / 铺满） |
| `paintLetterbox(color)` | 把黑边涂成指定颜色 |

### Input（鼠标键盘）

| 调用 | 说明 |
|---|---|
| `pos()` / `pressPos()` | 鼠标位置 / 本次按下的位置 |
| `down()` / `up()` / `held()` | 刚按下 / 刚松开 / 一直按着 |
| `clicked()` | 这一帧完成了一次点击（按下 → 几乎没动 → 松开） |
| `doubleClicked()` | 双击（400ms、8 像素内） |
| `dragging()` | 按住并移动超过 4 像素 |
| `wheel()` | 滚轮增量（正 = 上滚） |
| `keyDown(vk)` | 这一帧按了某键（`VK_ESCAPE`、`VK_F11`…） |
| `chars()` / `charsUtf8()` | 这一帧输入的字符（UTF-16 / UTF-8），中文也在里面 |
| `keys()` | 这一帧按下的虚拟键码 |
| `mouseIn(屏幕矩形)` / `clickedIn(屏幕矩形)` | 鼠标在不在矩形里 / 是否点中了它 |
| `setHandCursor(bool)` | 悬停在可点区域时换小手 |
| `setImePosition(x, y)` | 把输入法候选框挪到客户区坐标 (x,y) |

### Image

| 调用 | 说明 |
|---|---|
| `load(path)` | 加载（窗口建好后才能加载） |
| `draw(x, y)` / `draw(rect)` | 原尺寸 / 拉伸铺满 |
| `drawFit(rect)` | 等比放入（可能留边） |
| `drawCover(rect)` | 等比铺满（裁掉多余） |

### 资源路径（打包发布）

| 调用 | 说明 |
|---|---|
| `appDir()` | exe 所在目录（UTF-8，带结尾斜杠） |
| `assetPath("res/bg.png")` | exe 目录 + 相对路径；发布时把 `res` 放在 exe 旁边 |

```cpp
img.load(ezx::assetPath("res/bg.png"));   // 不依赖“当前工作目录”
```

### TextInput

| 字段 / 方法 | 说明 |
|---|---|
| `text` / `cursor` | 内容（UTF-8）/ 光标字节下标（不要手改） |
| `maxLen` / `digitsOnly` / `password` | 长度（按字符，中文算 1）/ 只允许数字 / 画成 `*` |
| `length()` | 当前字符数（不是字节数） |
| `update(in)` | 每帧处理键盘（先 `in.update()`），支持中文 |
| `submitted()` / `canceled()` | 回车 / Esc（用 `consume()` 清掉） |
| `number(fallback)` / `clampNumber(lo, hi)` | 取数值 / 越界钳制并写回 |
| `placeCursor(vp, box, size, mouseX)` | 鼠标点击后把光标放到最近的字符边界 |
| `draw(vp, box, size, color, caretOn)` | 在框里画文字 + 光标（不裁剪，超长请限制 `maxLen`） |
| `setImePosition`（在 Input 上） | 让输入法候选框贴着输入框 |

中文输入不用额外解码，`TextInput` 已经处理：

```cpp
TextInput name;
name.maxLen = 20;                 // 20 个字符，中文也算 1 个
name.focused = true;

// 每帧
if (in.clickedIn(vp.R(box)))      // 点框 → 放光标
    name.placeCursor(vp, box, 32, in.pos().x);
name.update(in);
in.setImePosition(vp.R(box).x + 8, vp.R(box).bottom());   // 候选框贴住输入框

// 画
name.draw(vp, box, 32, RGB(236,242,255), (GetTickCount() / 500) % 2 == 0);
```

完整说明见 [详细教程.md](详细教程.md) 的 8.7 节。

### Page / Pages

| 调用 | 说明 |
|---|---|
| `pages.add("名字")` | 添加页面，返回稳定的 `Page&`（内部用 deque） |
| `page.layout / update / draw` | 三段函数，可以只写其中几个 |
| `pages.go("名字")` | 切换（会自动调用新页面的 `layout`） |
| `pages.layoutAll(vp)` | 所有页面重新排版 |
| `pages.current()` / `is("名字")` | 当前页面 |

## 自己写按钮

库不提供按钮，因为就 20 行：

```cpp
struct UiButton
{
    Rect rect;
    std::string label;
    bool hovered = false;

    bool update(const Input& in, const Viewport& vp)
    {
        hovered = vp.R(rect).contains(in.pos());   // 鼠标在不在里面
        return hovered && in.clicked();            // 在 + 点了一下
    }

    void draw(const Viewport& vp) const
    {
        vp.fill(rect, hovered ? RGB(60,86,132) : RGB(34,42,62));
        vp.textIn(rect, label, 28, RGB(236,242,255));
    }
};
```

完整例子见 `demo_main.cpp`。

## 可视化 UI 编辑器

不想手写坐标？双击 **`editor/ui_editor.html`**，用 Edge/Chrome 打开（单文件、离线、零依赖）：

- 拖按钮/文字/面板/图片/文本输入，拖拽移动、八向缩放、8px 网格吸附；
- 多页面 + 页面跳转、背景图、按钮图标、水平/垂直/页面居中；
- 右侧调坐标、颜色、字号、圆角、边框、点击逻辑；
- 一键生成 `ui_generated.cpp`，按钮点击逻辑以 `case` 留好接口；
- 保存/打开 JSON 工程，支持撤销/重做、图层顺序。

![UI 编辑器](editor/editor_preview.png)

使用说明见 **[editor/README.md](editor/README.md)**。

## 编译

**最简单**：双击包里的 `build_demo.bat`，它会自动找到 VS + EasyX，用 `/MT /utf-8 /D_UNICODE` 编译出 `demo.exe`。

命令行等价写法（先 `vcvars64.bat`）：

```
cl /std:c++17 /EHsc /MT /utf-8 /D_UNICODE /DUNICODE /I. easyx_core.cpp demo_main.cpp /link /LIBPATH:"%VCINSTALLDIR%Auxiliary\VS\lib\x64"
```

VS 里直接新建空项目、加两个 `.cpp` + 头文件、`F5` 即可。图片放在工程目录的 `res/` 下。

> 本机实测：这套 EasyX 的 `EasyXa.lib` 是 `/MT` 编译的，工程运行库要选「多线程 (/MT)」，Debug/Release 都一样；选 `/MD` 或 `/MTd` 会报 LNK2038。Debug 还要去掉 `_DEBUG`（或定义 `NDEBUG`），否则 STL 会进入 debug 模式，仍然报 `_ITERATOR_DEBUG_LEVEL=2`。详见 [详细教程.md](详细教程.md) 第 11 章。

## 文档

- 本文件：怎么用
- **Directory.Build.targets**：可选，复制到项目根自动适配 `/MT`、`/utf-8`、库目录
- **[editor/README.md](editor/README.md)**：可视化 UI 编辑器使用说明（拖控件、生成 C++、点击接口）
- **[详细教程.md](详细教程.md)**：零基础版，从 C++ 语法讲到图片资源、中文输入、窗口设置、逐段拆 demo
- **[项目原理.md](项目原理.md)**：内部原理图解（消息循环、双缓冲、窗口重建、编码、IME、alpha、内存）
- **[使用说明.md](使用说明.md)**：每部分为什么这么设计、常见坑、改造配方

## 许可

MIT（派生自 200hyf 的 gomoku-annotated-learning）。

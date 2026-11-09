# ezx-core UI 编辑器

一个**单文件、离线、零依赖**的可视化界面编辑器：拖控件、调参数，然后直接生成能粘贴进 ezx-core 工程的 C++ 代码。点击按钮后的逻辑以 `case` 形式留好接口，你只需要往里面填代码。

![编辑器](editor_preview.png)

## 打开

双击 `ui_editor.html`，用 Edge / Chrome 打开即可，**不需要装任何东西、不需要起服务器**。

推荐用 Edge/Chrome 打开；IE 不支持。

## 界面

```
┌─────────────────────────────────────────────────────────────┐
│ 新建 / 示例 / 打开 / 保存 / 撤销 / 重做 / 预览 / 生成代码      │
├──────────┬──────────────────────────────────┬───────────────┤
│ 添加控件  │                                  │ 属性面板       │
│ 按钮      │        1280 × 720 画布            │ 位置/大小      │
│ 文字      │        （可缩放、网格吸附）        │ 颜色/字号      │
│ 面板      │                                  │ 点击逻辑       │
│ 图片      │                                  │ 图层顺序       │
│ 输入框    │                                  │               │
├──────────┴──────────────────────────────────┴───────────────┤
│ 图层列表（点击选中，↑↓ 调层级，● 显示/隐藏）                  │
└─────────────────────────────────────────────────────────────┘
```

## 基本操作

| 操作 | 说明 |
|---|---|
| 添加 | 点左侧「按钮 / 文字 / 面板 / 图片 / 输入框」 |
| 选中 | 点画布里的控件；点空白取消 |
| 移动 | 按住控件拖动，默认吸附 8px 网格 |
| 缩放 | 拖动选中框的 8 个小白点 |
| 调参 | 右侧属性面板；X/Y/宽/高/字号/颜色都能改 |
| 层级 | 图层列表的 ↑↓，或属性面板「上移/下移/置顶/置底」 |
| 复制/删除 | 属性面板按钮；或 `Ctrl+D` / `Delete` |
| 撤销/重做 | `Ctrl+Z` / `Ctrl+Y` |
| 保存工程 | `Ctrl+S` 或「保存」→ 下载 `.json` |
| 打开工程 | 「打开」选择之前保存的 `.json` |
| 预览 | 「预览」隐藏编辑面板，只看界面；`Esc` 退出 |

画布默认 **1280×720**，代码里的坐标就是画布坐标。底部可以切换 50%/75%/100%/125% 或「适应」。

## 控件与生成代码

### 按钮

- `C++ ID`：例如 `BTN_START`，生成 `UI_BTN_START`。
- 「点击逻辑」里写 C++ 代码，会原样放进 `onClick` 的 `case`：

```cpp
case UI_BTN_START:
    // 点击“开始游戏”
    // TODO: 开始游戏
    // startGame();
    break;
```

- 悬停色、普通底色、边框、圆角、字号都会生成。
- 「可用」取消勾选 → 生成禁用样式，并且不响应点击。

### 文字 / 面板

- 文字：内容、字号、颜色、左/中/右对齐。
- 面板：填充色、边框色、边框粗细、圆角。

### 图片

- 「图片路径」写相对 exe 的路径，例如 `res/logo.png`。
- 缩放方式：
  - 等比完整 → `vp.imageFit`
  - 等比铺满 → `vp.imageCover`
  - 拉伸 → `vp.image`
- 「预览图」只用于编辑器显示，不会写进代码。

### 输入框

- 生成 `TextInput`，自动处理点击聚焦、`placeCursor`、光标闪烁。
- 占位文字、初始文字、最大长度、只数字、密码、聚焦边框色都会生成。
- 生成的 `update()` 会先处理输入框焦点，再处理按钮点击。

## 生成代码

点右上角「生成代码」：

- 代码显示在弹窗里，可以「复制」或「下载 .cpp」。
- 文件建议命名 `ui_generated.cpp`。
- 代码是一个独立模块，包含：

```cpp
namespace ui {
    void init();
    void update(const ezx::Input& in, const ezx::Viewport& vp);
    void draw(const ezx::Viewport& vp, const ezx::Input& in);
    void onClick(int id);   // 你填点击逻辑
}
```

### 接入工程

1. 把 `ui_generated.cpp`、`easyx_core.h`、`easyx_core.cpp` 放进项目（根目录或 `src\` 都行）。
2. 在 VS 里「添加 → 现有项」把两个 `.cpp` 加进去。
3. 在 `main.cpp` 里声明并调用：

```cpp
#include "easyx_core.h"

namespace ui {
    void init();
    void update(const ezx::Input& in, const ezx::Viewport& vp);
    void draw(const ezx::Viewport& vp, const ezx::Input& in);
}

using namespace ezx;

int main()
{
    Window::enableDpiAwareness();
    Window win;
    Viewport vp;
    Input in;

    vp.setDesign(1280, 720);                 // 和编辑器里的设计分辨率一致
    win.onResize([&](int w, int h) { vp.fit(w, h); });
    if (!win.open(1024, 640)) return 1;

    vp.fit(win.width(), win.height());
    ui::init();                              // 图片、输入框初始化

    while (true)
    {
        win.poll();
        in.update();
        if (in.keyDown(VK_ESCAPE)) break;

        ui::update(in, vp);                  // 先更新交互
        win.beginFrame(RGB(8, 8, 12));
        vp.paintLetterbox(RGB(8, 8, 12));
        ui::draw(vp, in);                    // 再画
        win.endFrame();
    }
    return 0;
}
```

> `ui::init()` 必须在窗口创建之后调用；图片用 `assetPath("res/...")` 加载，所以 `res` 要跟 exe 放一起。

### 点击逻辑写在哪

生成代码底部有一个 `onClick`：

```cpp
void onClick(int id)
{
    switch (id)
    {
    case UI_BTN_START:
        // 这里写你的逻辑
        // startGame();
        break;
    case UI_BTN_QUIT:
        // PostQuitMessage(0);
        break;
    default:
        break;
    }
}
```

你可以调用自己项目里的函数、改页面状态、播放声音等；编辑器只负责生成**外壳和接口**。

## 保存格式

保存的 `.json` 就是画布数据，可以提交到 git、发给别人；编辑器启动时会自动从浏览器 `localStorage` 恢复上次编辑。

「新建」清空当前界面；「示例」载入内置的 6 控件示例（见 `example-ui.json`）。

## 限制（当前版本）

- 生成的是**静态坐标布局**，不是运行时布局引擎；窗口缩放由 `Viewport` 统一缩放。
- 文字是单行；没有换行排版、滚动列表、动画时间轴。
- 按钮只支持文字，不支持图标+文字混排（可以用「图片」控件叠在按钮上）。
- 输入框只支持单击聚焦、键盘输入；没有右键菜单、选中文本。
- 图片预览只存在浏览器内存里，刷新后需要重新选预览图（代码只使用路径）。
- 生成代码是 C++17 + EasyX + ezx-core，Windows/MSVC；需要 `/MT`、`/utf-8`（`Directory.Build.targets` 会自动加）。

这些都可以继续扩展；当前版本先保证「拖完 → 生成 → 能编译 → 能点」。

## 文件

```
editor/
├─ ui_editor.html        编辑器（单文件，直接双击）
├─ README.md             本文件
├─ example-ui.json       示例工程（可在编辑器里打开）
├─ example-generated.cpp 示例工程生成的 C++ 代码（可直接编译）
├─ editor_preview.png    界面截图
└─ editor_selected.png   选中按钮时的属性面板截图
```

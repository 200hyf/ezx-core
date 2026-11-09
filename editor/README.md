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
│ 添加控件  │ 页面标签：主页面 / 设置页 / +页面 │ 属性面板       │
│ 按钮      │                                  │ 位置/大小      │
│ 文字      │        1280 × 720 画布            │ 颜色/字号      │
│ 面板      │        （可缩放、网格吸附）        │ 点击逻辑       │
│ 图片      │                                  │ 跳转页面       │
│ 文本输入  │                                  │ 图标/居中      │
├──────────┴──────────────────────────────────┴───────────────┤
│ 图层列表（点击选中，↑↓ 调层级，● 显示/隐藏）                  │
└─────────────────────────────────────────────────────────────┘
```

## 基本操作

| 操作 | 说明 |
|---|---|
| 添加 | 点左侧「按钮 / 文字 / 面板 / 图片 / 文本输入」 |
| 选中 | 点画布里的控件；点空白取消 |
| 移动 | 按住控件拖动，默认吸附 8px 网格 |
| 缩放 | 拖动选中框的 8 个小白点 |
| 调参 | 右侧属性面板；X/Y/宽/高/字号/颜色都能改 |
| 居中 | 属性面板「水平居中 / 垂直居中 / 页面居中」 |
| 层级 | 图层列表的 ↑↓，或属性面板「上移/下移/置顶/置底」 |
| 复制/删除 | 属性面板按钮；或 `Ctrl+D` / `Delete` |
| 撤销/重做 | `Ctrl+Z` / `Ctrl+Y` |
| 保存工程 | `Ctrl+S` 或「保存」→ 下载 `.json` |
| 打开工程 | 「打开」选择之前保存的 `.json` |
| 预览 | 「预览」隐藏编辑面板，只看界面；`Esc` 退出 |

画布默认 **1280×720**，代码里的坐标就是画布坐标。底部可以切换 50%/75%/100%/125% 或「适应」。

## 页面（跳转）

画布上方的标签就是页面：

- **`+ 页面`**：新增页面；
- 点标签：切换页面；
- **双击标签**：重命名；
- 标签上的 **×**：删除页面（至少保留一个）。

每个页面有自己的控件、图层和按钮。按钮属性里有 **「跳转页面」**：

- 选一个目标页面，点击按钮时会先执行「点击逻辑」，然后自动 `goPage(...)`；
- 也可以在点击逻辑里手写：

```cpp
ui::goPage(UI_PAGE_SETTINGS);
```

生成代码会包含：

```cpp
enum UiPage
{
    UI_PAGE_MAIN = 0,
    UI_PAGE_SETTINGS = 1
};

void goPage(int page);
int currentPage();
```

`update()` / `draw()` 会按当前页面自动分发，不需要你写 `switch`。

![第二页](editor_page2.png)

## 背景图

在**不选中任何控件**时，右侧「项目」里有：

- **背景图**：相对 exe 的路径，例如 `res/bg.png`；
- **背景填充**：等比铺满 / 等比完整 / 拉伸；
- **背景预览**：只用于编辑器显示，不写进代码。

生成代码会在 `draw()` 最前面先填背景色，再画背景图：

```cpp
vp.fill(Rect{ 0, 0, 1280, 720 }, RGB(18, 20, 28));
vp.imageCover(g_bg, Rect{ 0, 0, 1280, 720 });
```

`init()` 里会自动 `g_bg.load(assetPath("res/bg.png"));`。

> 编辑器里如果没选「背景预览」，画布只显示背景色；生成代码仍然会用路径加载真实图片。

## 控件与生成代码

### 按钮

- `C++ ID`：例如 `BTN_START`。单页面生成 `UI_BTN_START`；多页面生成 `UI_MAIN_BTN_START`（页面 ID 做前缀，避免重名）。
- 「点击逻辑」里写 C++ 代码，会原样放进 `onClick` 的 `case`：

```cpp
case UI_MAIN_BTN_START:
    // 点击“开始游戏”
    // TODO: 开始游戏
    // startGame();
    break;
```

- 「跳转页面」：选中目标页面后，点击会自动切页。
- **按钮图标**：
  - 「图标路径」：例如 `res/start.png`；
  - 「图标大小」「图标间距」「图标位置（左/右/上）」；
  - 「图标预览」只用于编辑器。
- 生成代码会自动 `load` 图标，并按左/右/上排版图标和文字。
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

### 文本输入

- 左侧「文本输入」添加一个 `TextInput`。
- 可调：占位文字、初始文字、字号、最大长度、只数字、密码、背景/边框/聚焦边框/文字颜色、圆角。
- 生成代码自动处理：
  - 点击输入框聚焦；
  - `placeCursor` 把光标放到点击位置；
  - 光标闪烁；
  - 多个输入框时，点哪个哪个获得焦点。

## 自动居中

选中控件后，属性面板底部有：

- **水平居中**：`x = (设计宽 - 控件宽) / 2`；
- **垂直居中**：`y = (设计高 - 控件高) / 2`；
- **页面居中**：两者同时。

设置后，改设计分辨率或改控件宽高时会**自动重新居中**；手动拖动控件会取消居中标记（因为你要自己摆位置）。

生成代码里是当前计算出的固定坐标；运行时窗口缩放由 `Viewport` 统一缩放。

## 生成代码

点右上角「生成代码」：

- 代码显示在弹窗里，可以「复制」或「下载 .cpp」；
- 文件建议命名 `ui_generated.cpp`；
- 多页面会生成：

```cpp
namespace ui {
    void init();
    void update(const ezx::Input& in, const ezx::Viewport& vp);
    void draw(const ezx::Viewport& vp, const ezx::Input& in);
    void onClick(int id);
    void goPage(int page);
    int  currentPage();
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
    void goPage(int page);
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

生成代码里有 `onClick`：

```cpp
void onClick(int id)
{
    switch (id)
    {
    case UI_MAIN_BTN_START:
        // 这里写你的逻辑
        // startGame();
        break;
    case UI_MAIN_BTN_SETTINGS:
        // 不写也行，按钮的「跳转页面」会自动切到设置页
        break;
    default:
        break;
    }
}
```

页面切换也可以手写：

```cpp
ui::goPage(UI_PAGE_SETTINGS);
```

## 保存格式

保存的 `.json` 包含所有页面；可以提交到 git、发给别人；编辑器启动时会自动从浏览器 `localStorage` 恢复上次编辑。

「新建」清空当前界面；「示例」载入内置的双页面示例（见 `example-ui.json`）。

## 限制（当前版本）

- 生成的是**静态坐标布局**，不是运行时布局引擎；窗口缩放由 `Viewport` 统一缩放。
- 文字是单行；没有换行排版、滚动列表、动画时间轴。
- 按钮图标是图片 + 文字，不支持九宫格/圆角裁切。
- 输入框只支持单击聚焦、键盘输入；没有选中文本、右键菜单。
- 图片/背景预览只存在浏览器内存里，刷新后需要重新选预览图（代码只使用路径）。
- 生成代码是 C++17 + EasyX + ezx-core，Windows/MSVC；需要 `/MT`、`/utf-8`（`Directory.Build.targets` 会自动加）。

## 文件

```
editor/
├─ ui_editor.html        编辑器（单文件，直接双击）
├─ README.md             本文件
├─ example-ui.json       双页面示例工程（可在编辑器里打开）
├─ example-generated.cpp 示例工程生成的 C++ 代码（可直接编译）
├─ editor_preview.png    界面截图
├─ editor_selected.png   选中按钮时的属性面板截图
└─ editor_page2.png      第二页（页面跳转）截图
```

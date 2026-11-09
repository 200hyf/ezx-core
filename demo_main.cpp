// ============================================================================
//  demo_main.cpp  ——  界面底座演示（没有按钮外观，按钮是"手画"的）
// ----------------------------------------------------------------------------
//  演示 6 件事：
//    1) 窗口：建窗、F11 切全屏、拖大拖小自适应
//    2) 适配：所有坐标按 1280×720 设计尺寸写，自动等比缩放 + 黑边
//    3) 页面：主菜单 / 设置 / 游戏 三个页面来回切
//    4) 图片：背景铺满（cover）、logo 等比放入（fit）
//    5) 鼠标：悬停变色、点击、滚轮、坐标换算、手型光标
//    6) 文本输入：只能输数字、回车确认并钳制范围、Esc 取消
//
//  注意：这里的 UiButton 是**演示用**的，只有 20 行。
//        库本身不提供按钮，就是让你按需要自己画。
// ============================================================================
#include "easyx_core.h"

#include <chrono>
#include <cstdio>
#include <string>

using namespace ezx;

namespace {

long long nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// ---------------------------------------------------------------------------
//  全局状态
// ---------------------------------------------------------------------------
Viewport g_vp;
Pages    g_pages;
Input    g_input;

Image g_bg;       // 背景
Image g_logo;     // 图标

bool g_running = true;

int  g_winW = 0, g_winH = 0;      // 最近一次窗口大小（左下角信息用）
Window* g_win = nullptr;          // 用来读当前窗口模式（只读，不要在这里改窗口）
int  g_volume = 60;               // 设置页里编辑的数值
int  g_wheel = 0;                 // 游戏页滚轮演示
TextInput g_volumeInput;          // 设置页：只许数字
TextInput g_nameInput;            // 中文输入页：中英文都行
Rect      g_nameBox{ 390, 300, 500, 72 };   // 中文输入框（设计坐标）

// ---------------------------------------------------------------------------
//  一个"手画"的按钮：只有 位置 + 文字 + 悬停状态
//  这就是库不给你按钮的原因——自己写反而更自由。
// ---------------------------------------------------------------------------
struct UiButton
{
    Rect rect;              // 设计坐标（1280×720 里的一块区域）
    std::string label;
    int  fontSize = 26;
    bool hovered = false;
    bool selected = false;
    bool enabled = true;

    // 每帧调用：更新悬停状态，返回"这一帧有没有点它"
    bool update(const Input& in, const Viewport& vp)
    {
        hovered = enabled && vp.R(rect).contains(in.pos());   // 鼠标在不在这个矩形里
        return hovered && in.clicked();                        // 在 + 这一帧点了一下
    }

    void draw(const Viewport& vp) const
    {
        const COLORREF fill = !enabled ? RGB(26, 30, 40)
                            : selected ? RGB(52, 96, 150)
                            : hovered  ? RGB(62, 86, 132)
                                       : RGB(34, 42, 62);
        const COLORREF border = !enabled ? RGB(60, 64, 74)
                              : selected ? RGB(150, 200, 255)
                                         : RGB(92, 124, 196);
        const COLORREF text = !enabled ? RGB(110, 110, 120) : RGB(236, 242, 255);

        vp.fill(rect, fill);
        vp.frame(rect, border, 2);
        vp.textIn(rect, label, fontSize, text);
    }
};

UiButton g_menuButtons[5];        // 开始 / 音频 / 中文输入 / 窗口设置 / 退出
UiButton g_back;                  // 返回

// 窗口设置页的选项
UiButton g_modeButtons[3];        // 窗口 / 工作区 / 全屏
UiButton g_resButtons[3];         // 1024×640 / 1280×720 / 1600×900
UiButton g_fpsButtons[4];         // 30 / 60 / 120 / 不限
UiButton g_scaleButtons[2];       // 留黑边 / 裁边铺满
UiButton g_applyButton;

WinMode    g_pendingMode  = WinMode::Windowed;
ScaleMode  g_pendingScale = ScaleMode::Contain;
int        g_pendingW = 1024, g_pendingH = 640;
int        g_pendingFps = 60;
std::string g_windowStatus = "修改后点“应用”";

const int kResW[3] = { 1024, 1280, 1600 };
const int kResH[3] = { 640, 720, 900 };
const int kFps[4]  = { 30, 60, 120, 0 };

// ---------------------------------------------------------------------------
//  排版：设计尺寸固定 1280×720，所以这里就是"写死坐标"。
//  （窗口怎么变由 Viewport 缩放，不用你算）
// ---------------------------------------------------------------------------
const int DESIGN_W = 1280;
const int DESIGN_H = 720;

void layoutCommon(const Viewport& vp)
{
    const int W = vp.designW();
    const int H = vp.designH();

    // 五个菜单按钮：宽 320、高 52，水平居中，从 y=290 开始往下排
    const int bw = 320, bh = 52, gap = 12;
    int y = 290;
    for (int i = 0; i < 5; ++i)
    {
        g_menuButtons[i].rect = Rect{ (W - bw) / 2, y, bw, bh };
        g_menuButtons[i].fontSize = 26;
        y += bh + gap;
    }
    g_menuButtons[0].label = "开始游戏";
    g_menuButtons[1].label = "音频设置";
    g_menuButtons[2].label = "中文输入";
    g_menuButtons[3].label = "窗口设置";
    g_menuButtons[4].label = "退出";

    // 返回按钮：左下角（设置页 / 游戏页 / 中文输入页共用）
    g_back.rect = Rect{ 60, H - 160, 180, 60 };
    g_back.label = "返回";
    g_back.fontSize = 26;
}

// ---------------------------------------------------------------------------
//  主菜单页
// ---------------------------------------------------------------------------
void menuLayout(const Viewport& vp) { layoutCommon(vp); }

void syncWindowSettings();          // 下面实现

void menuUpdate(const Input& in)
{
    // 键盘快捷方式：T 中文输入页，W 窗口设置页（方便测试/演示）
    if (in.keyDown('T'))
    {
        g_nameInput.focused = true;
        g_pages.go("text");
        return;
    }
    if (in.keyDown('W'))
    {
        syncWindowSettings();
        g_pages.go("window");
        return;
    }

    bool hand = false;
    for (int i = 0; i < 5; ++i)
    {
        const bool hit = g_menuButtons[i].update(in, g_vp);
        hand = hand || g_menuButtons[i].hovered;

        if (!hit) continue;
        if (i == 0)      g_pages.go("game");
        else if (i == 1) { g_volumeInput.setText(std::to_string(g_volume)); g_pages.go("settings"); }
        else if (i == 2) { g_nameInput.focused = true; g_pages.go("text"); }
        else if (i == 3) { syncWindowSettings(); g_pages.go("window"); }
        else             g_running = false;
    }
    g_input.setHandCursor(hand);       // 悬停在按钮上就换成小手
}

void menuDraw(const Viewport& vp)
{
    // 背景：铺满整个设计区（等比 + 裁边）
    vp.imageCover(g_bg, Rect{ 0, 0, DESIGN_W, DESIGN_H });
    // logo：等比放进一个 150×150 的框里
    vp.imageFit(g_logo, Rect{ DESIGN_W / 2 - 75, 60, 150, 150 });

    vp.textIn(Rect{ 0, 210, DESIGN_W, 70 }, "界面底座演示", 54, RGB(240, 246, 255));

    for (const auto& b : g_menuButtons) b.draw(vp);

    // 左下角实时信息：鼠标的设计坐标、窗口大小、当前缩放
    const Point p = vp.toDesign(g_input.pos());
    char buf[256];
    snprintf(buf, sizeof(buf), "鼠标(设计坐标): %d, %d", p.x, p.y);
    vp.text(40, 620, buf, 18, RGB(140, 160, 200));

    const bool full = (g_win && g_win->mode() == WinMode::Fullscreen);
    snprintf(buf, sizeof(buf), "窗口: %d x %d   缩放: %.2f   模式: %s   音量: %d",
             g_winW, g_winH, vp.scale(), full ? "全屏" : "窗口", g_volume);
    vp.text(40, 648, buf, 18, RGB(140, 160, 200));
}

// ---------------------------------------------------------------------------
//  设置页：文本输入
// ---------------------------------------------------------------------------
void settingsLayout(const Viewport& vp) { layoutCommon(vp); }

void settingsUpdate(const Input& in)
{
    g_volumeInput.update(in);          // 处理打字、退格、左右键

    if (g_volumeInput.submitted())     // 回车：取值 → 钳制 → 回主菜单
    {
        g_volumeInput.clampNumber(0, 100);
        g_volume = g_volumeInput.number(0);
        g_volumeInput.consume();
        g_pages.go("menu");
    }
    if (g_volumeInput.canceled())      // Esc：放弃
    {
        g_volumeInput.consume();
        g_pages.go("menu");
    }

    const bool hit = g_back.update(in, g_vp);
    g_input.setHandCursor(g_back.hovered);
    if (hit) g_pages.go("menu");
}

void settingsDraw(const Viewport& vp)
{
    vp.imageCover(g_bg, Rect{ 0, 0, DESIGN_W, DESIGN_H });
    vp.fill(Rect{ 0, 0, DESIGN_W, DESIGN_H }, RGB(12, 16, 26));   // 压暗一点，突出内容

    vp.textIn(Rect{ 0, 90, DESIGN_W, 70 }, "设置", 48, RGB(240, 246, 255));
    vp.textIn(Rect{ 0, 200, DESIGN_W, 40 }, "音量（只能输入数字，0~100，回车确认）", 22,
               RGB(150, 170, 210));

    // 输入框：自己画的框 + TextInput 负责里面的字和光标
    const Rect box{ DESIGN_W / 2 - 150, 260, 300, 64 };
    vp.fill(box, RGB(24, 30, 44));
    vp.frame(box, RGB(110, 170, 255), 2);

    const bool caretOn = (nowMs() / 500) % 2 == 0;   // 每 500ms 闪一下
    g_volumeInput.draw(vp, box, 32, RGB(236, 242, 255), caretOn);

    vp.textIn(Rect{ 0, 360, DESIGN_W, 36 }, "当前值：" + std::to_string(g_volume), 22,
              RGB(160, 200, 255));

    g_back.draw(vp);
    vp.textIn(Rect{ 0, DESIGN_H - 80, DESIGN_W, 30 }, "回车 = 保存并返回     Esc = 放弃", 18,
              RGB(120, 140, 180));
}

// ---------------------------------------------------------------------------
//  中文输入页：演示 TextInput 接收中文、按字符移动光标、输入法候选框定位
// ---------------------------------------------------------------------------
void textLayout(const Viewport& vp) { layoutCommon(vp); }

void textUpdate(const Input& in)
{
    // 点输入框：把焦点给它，并按鼠标位置放光标
    if (in.clickedIn(g_vp.R(g_nameBox)))
    {
        g_nameInput.focused = true;
        g_nameInput.placeCursor(g_vp, g_nameBox, 32, in.pos().x);
    }

    g_nameInput.update(in);            // 中文/英文、退格、左右键都在里面处理

    // 输入法候选框跟着输入框走（位置没变时内部不会重复设置）
    if (g_nameInput.focused)
    {
        const Rect r = g_vp.R(g_nameBox);
        in.setImePosition(r.x + 8, r.bottom());
    }

    // 回车 / Esc：结束输入，回主菜单
    if (g_nameInput.submitted() || g_nameInput.canceled())
    {
        g_nameInput.consume();
        g_nameInput.focused = false;
        in.clearImePosition();
        g_pages.go("menu");
        return;
    }

    const bool hit = g_back.update(in, g_vp);
    g_input.setHandCursor(g_back.hovered || in.mouseIn(g_vp.R(g_nameBox)));

    if (hit)
    {
        g_nameInput.focused = false;
        in.clearImePosition();
        g_pages.go("menu");
    }
}

void textDraw(const Viewport& vp)
{
    vp.imageCover(g_bg, Rect{ 0, 0, DESIGN_W, DESIGN_H });
    vp.fill(Rect{ 0, 0, DESIGN_W, DESIGN_H }, RGB(12, 16, 26));

    vp.textIn(Rect{ 0, 90, DESIGN_W, 70 }, "中文输入", 48, RGB(240, 246, 255));
    vp.textIn(Rect{ 0, 190, DESIGN_W, 40 }, "点一下输入框，用输入法打中文；中英文都能输入", 22,
               RGB(150, 170, 210));

    // 输入框：底色/边框自己画，TextInput 只负责里面的字和光标
    vp.fill(g_nameBox, RGB(24, 30, 44));
    vp.frame(g_nameBox, g_nameInput.focused ? RGB(110, 170, 255) : RGB(70, 90, 130), 2);

    if (g_nameInput.text.empty())      // 占位提示（core 版没有 placeholder 字段）
        vp.textIn(g_nameBox.inset(12), "在这里输入……", 24, RGB(110, 130, 165), Align::Left);

    const bool caretOn = (nowMs() / 500) % 2 == 0;
    g_nameInput.draw(vp, g_nameBox, 32, RGB(236, 242, 255), caretOn);

    vp.textIn(Rect{ 0, 410, DESIGN_W, 40 }, "已输入：" + g_nameInput.text, 26,
              RGB(160, 200, 255));
    vp.textIn(Rect{ 0, 460, DESIGN_W, 34 },
              "字符数：" + std::to_string(g_nameInput.length()) + "（中文算 1 个）", 20,
              RGB(140, 160, 200));

    g_back.draw(vp);
    vp.textIn(Rect{ 0, DESIGN_H - 80, DESIGN_W, 30 },
              "回车 / Esc / 返回 = 回主菜单     ← → 移动光标     Backspace 删一个字", 18,
              RGB(120, 140, 180));
}

// ---------------------------------------------------------------------------
//  窗口设置页：模式 / 分辨率 / 帧率 / 缩放方式
// ---------------------------------------------------------------------------
void syncWindowSettings()
{
    if (!g_win) return;

    g_pendingMode  = g_win->mode();
    g_pendingFps   = g_win->targetFps();
    g_pendingScale = g_vp.scaleMode();

    // 全屏/工作区下，width() 是屏幕尺寸，不要覆盖用户上次选的窗口分辨率
    if (g_win->mode() == WinMode::Windowed)
    {
        g_pendingW = g_win->width();
        g_pendingH = g_win->height();
    }

    g_windowStatus = "修改后点“应用”";
}

void windowLayout(const Viewport& vp)
{
    layoutCommon(vp);                 // 先把 g_back 等公共控件摆好

    const int W = vp.designW();
    const int H = vp.designH();

    // 显示模式
    const char* modeLabel[3] = { "窗口", "工作区", "全屏" };
    int x = 380;
    for (int i = 0; i < 3; ++i)
    {
        g_modeButtons[i].rect = Rect{ x, 140, 150, 48 };
        g_modeButtons[i].label = modeLabel[i];
        g_modeButtons[i].fontSize = 22;
        x += 164;
    }

    // 分辨率
    const char* resLabel[3] = { "1024 × 640", "1280 × 720", "1600 × 900" };
    x = 380;
    for (int i = 0; i < 3; ++i)
    {
        g_resButtons[i].rect = Rect{ x, 220, 150, 48 };
        g_resButtons[i].label = resLabel[i];
        g_resButtons[i].fontSize = 22;
        x += 164;
    }

    // 目标帧率
    const char* fpsLabel[4] = { "30 帧", "60 帧", "120 帧", "不限帧" };
    x = 380;
    for (int i = 0; i < 4; ++i)
    {
        g_fpsButtons[i].rect = Rect{ x, 300, 110, 48 };
        g_fpsButtons[i].label = fpsLabel[i];
        g_fpsButtons[i].fontSize = 22;
        x += 124;
    }

    // 缩放方式
    const char* scaleLabel[2] = { "留黑边", "裁边铺满" };
    x = 380;
    for (int i = 0; i < 2; ++i)
    {
        g_scaleButtons[i].rect = Rect{ x, 380, 170, 48 };
        g_scaleButtons[i].label = scaleLabel[i];
        g_scaleButtons[i].fontSize = 22;
        x += 190;
    }

    g_applyButton.rect = Rect{ W / 2 - 120, 470, 240, 54 };
    g_applyButton.label = "应用";
    g_applyButton.fontSize = 26;

    g_back.rect = Rect{ 60, H - 100, 160, 48 };
    g_back.label = "返回";
    g_back.fontSize = 24;
}

void applyWindowSettings()
{
    if (!g_win) { g_windowStatus = "窗口未初始化"; return; }

    g_vp.setScaleMode(g_pendingScale);          // 先改缩放模式

    bool ok = false;
    if (g_pendingMode == WinMode::Windowed)
        ok = g_win->applyMode(WinMode::Windowed, g_pendingW, g_pendingH);
    else
        ok = g_win->applyMode(g_pendingMode);

    g_win->setTargetFps(g_pendingFps);          // 再改帧率（会重置帧计时）

    if (!ok) { g_windowStatus = "应用失败"; return; }

    if (g_win->mode() == WinMode::Windowed)
    {
        g_pendingW = g_win->width();
        g_pendingH = g_win->height();
    }

    const char* modeName = (g_win->mode() == WinMode::Windowed) ? "窗口"
                         : (g_win->mode() == WinMode::WorkArea) ? "工作区"
                                                                : "全屏";
    std::string info = std::string("已应用：") + modeName;
    if (g_win->mode() == WinMode::Windowed)
        info += " / " + std::to_string(g_pendingW) + "×" + std::to_string(g_pendingH);
    info += " / " + (g_pendingFps == 0 ? std::string("不限帧")
                                       : std::to_string(g_pendingFps) + " FPS");
    info += std::string(" / ") + (g_pendingScale == ScaleMode::Contain ? "留黑边" : "裁边铺满");
    g_windowStatus = info;
}

void windowUpdate(const Input& in)
{
    bool hand = false;

    // 键盘快捷键：1/2/3 模式，4/5/6 分辨率，7/8/9/0 帧率，C/V 缩放，Enter 应用
    if (in.keyDown('1')) { g_pendingMode = WinMode::Windowed;   g_windowStatus = "已选择窗口模式"; }
    if (in.keyDown('2')) { g_pendingMode = WinMode::WorkArea;   g_windowStatus = "已选择工作区模式"; }
    if (in.keyDown('3')) { g_pendingMode = WinMode::Fullscreen; g_windowStatus = "已选择全屏模式"; }
    if (in.keyDown('4')) { g_pendingW = kResW[0]; g_pendingH = kResH[0]; }
    if (in.keyDown('5')) { g_pendingW = kResW[1]; g_pendingH = kResH[1]; }
    if (in.keyDown('6')) { g_pendingW = kResW[2]; g_pendingH = kResH[2]; }
    if (in.keyDown('7')) g_pendingFps = kFps[0];
    if (in.keyDown('8')) g_pendingFps = kFps[1];
    if (in.keyDown('9')) g_pendingFps = kFps[2];
    if (in.keyDown('0')) g_pendingFps = kFps[3];
    if (in.keyDown('C')) g_pendingScale = ScaleMode::Contain;
    if (in.keyDown('V')) g_pendingScale = ScaleMode::Cover;
    if (in.keyDown(VK_RETURN)) applyWindowSettings();

    // 显示模式
    for (int i = 0; i < 3; ++i)
    {
        const bool hit = g_modeButtons[i].update(in, g_vp);
        hand = hand || g_modeButtons[i].hovered;
        if (hit)
        {
            g_pendingMode = (i == 0) ? WinMode::Windowed
                          : (i == 1) ? WinMode::WorkArea
                                     : WinMode::Fullscreen;
            g_windowStatus = "已选择模式，点“应用”生效";
        }
    }

    // 分辨率（全屏/工作区忽略，所以禁用）
    const bool resEnabled = (g_pendingMode == WinMode::Windowed);
    for (int i = 0; i < 3; ++i)
    {
        g_resButtons[i].enabled = resEnabled;
        const bool hit = g_resButtons[i].update(in, g_vp);
        hand = hand || g_resButtons[i].hovered;
        if (hit)
        {
            g_pendingW = kResW[i];
            g_pendingH = kResH[i];
            g_windowStatus = "已选择分辨率，点“应用”生效";
        }
    }

    // 帧率
    for (int i = 0; i < 4; ++i)
    {
        const bool hit = g_fpsButtons[i].update(in, g_vp);
        hand = hand || g_fpsButtons[i].hovered;
        if (hit)
        {
            g_pendingFps = kFps[i];
            g_windowStatus = "已选择帧率，点“应用”生效";
        }
    }

    // 缩放方式
    for (int i = 0; i < 2; ++i)
    {
        const bool hit = g_scaleButtons[i].update(in, g_vp);
        hand = hand || g_scaleButtons[i].hovered;
        if (hit)
        {
            g_pendingScale = (i == 0) ? ScaleMode::Contain : ScaleMode::Cover;
            g_windowStatus = "已选择缩放方式，点“应用”生效";
        }
    }

    // 应用
    if (g_applyButton.update(in, g_vp))
        applyWindowSettings();
    hand = hand || g_applyButton.hovered;

    // 返回
    const bool back = g_back.update(in, g_vp);
    hand = hand || g_back.hovered;
    g_input.setHandCursor(hand);

    if (in.keyDown(VK_ESCAPE) || back)
        g_pages.go("menu");
}

void windowDraw(const Viewport& vp)
{
    vp.imageCover(g_bg, Rect{ 0, 0, DESIGN_W, DESIGN_H });
    vp.fill(Rect{ 0, 0, DESIGN_W, DESIGN_H }, RGB(12, 16, 26));

    vp.textIn(Rect{ 0, 60, DESIGN_W, 60 }, "窗口设置", 44, RGB(240, 246, 255));

    const COLORREF labelColor = RGB(160, 180, 215);
    vp.textIn(Rect{ 140, 140, 220, 48 }, "显示模式", 24, labelColor, Align::Left);
    vp.textIn(Rect{ 140, 220, 220, 48 }, "分辨率", 24, labelColor, Align::Left);
    vp.textIn(Rect{ 140, 300, 220, 48 }, "目标帧率", 24, labelColor, Align::Left);
    vp.textIn(Rect{ 140, 380, 220, 48 }, "缩放方式", 24, labelColor, Align::Left);

    // 选中态
    g_modeButtons[0].selected = (g_pendingMode == WinMode::Windowed);
    g_modeButtons[1].selected = (g_pendingMode == WinMode::WorkArea);
    g_modeButtons[2].selected = (g_pendingMode == WinMode::Fullscreen);

    for (int i = 0; i < 3; ++i)
        g_resButtons[i].selected = (g_pendingW == kResW[i] && g_pendingH == kResH[i]);

    for (int i = 0; i < 4; ++i)
        g_fpsButtons[i].selected = (g_pendingFps == kFps[i]);

    g_scaleButtons[0].selected = (g_pendingScale == ScaleMode::Contain);
    g_scaleButtons[1].selected = (g_pendingScale == ScaleMode::Cover);

    for (int i = 0; i < 3; ++i) { g_modeButtons[i].draw(vp);  g_resButtons[i].draw(vp); }
    for (int i = 0; i < 4; ++i) g_fpsButtons[i].draw(vp);
    for (int i = 0; i < 2; ++i) g_scaleButtons[i].draw(vp);
    g_applyButton.draw(vp);

    vp.textIn(Rect{ 0, 545, DESIGN_W, 40 }, g_windowStatus, 22, RGB(160, 200, 255));
    vp.textIn(Rect{ 0, 590, DESIGN_W, 30 },
              "全屏 / 工作区会忽略分辨率；应用后窗口会重建", 18, RGB(120, 140, 180));

    g_back.draw(vp);
}

// ---------------------------------------------------------------------------
//  游戏页（占位），演示 Esc 返回 + 滚轮
// ---------------------------------------------------------------------------
void gameLayout(const Viewport& vp) { layoutCommon(vp); }

void gameUpdate(const Input& in)
{
    if (in.keyDown(VK_ESCAPE)) { g_pages.go("menu"); return; }
    if (in.wheel() != 0) g_wheel += (in.wheel() > 0) ? 1 : -1;

    const bool hit = g_back.update(in, g_vp);
    g_input.setHandCursor(g_back.hovered);
    if (hit) g_pages.go("menu");
}

void gameDraw(const Viewport& vp)
{
    vp.imageCover(g_bg, Rect{ 0, 0, DESIGN_W, DESIGN_H });
    vp.fill(Rect{ 0, 0, DESIGN_W, DESIGN_H }, RGB(10, 14, 24));

    vp.textIn(Rect{ 0, 160, DESIGN_W, 80 }, "游戏页面（这里换成你的东西）", 44, RGB(240, 246, 255));
    vp.textIn(Rect{ 0, 260, DESIGN_W, 40 }, "按 Esc 返回菜单", 24, RGB(150, 170, 210));
    vp.textIn(Rect{ 0, 320, DESIGN_W, 40 }, "滚轮值：" + std::to_string(g_wheel), 24,
              RGB(160, 200, 255));

    g_back.draw(vp);
}

// ---------------------------------------------------------------------------
//  把三个页面装进 Pages
// ---------------------------------------------------------------------------
void buildPages()
{
    Page& menu = g_pages.add("menu");
    menu.layout = menuLayout;
    menu.update = menuUpdate;
    menu.draw   = menuDraw;

    Page& settings = g_pages.add("settings");
    settings.layout = settingsLayout;
    settings.update = settingsUpdate;
    settings.draw   = settingsDraw;

    Page& game = g_pages.add("game");
    game.layout = gameLayout;
    game.update = gameUpdate;
    game.draw   = gameDraw;

    Page& text = g_pages.add("text");
    text.layout = textLayout;
    text.update = textUpdate;
    text.draw   = textDraw;

    Page& winPage = g_pages.add("window");
    winPage.layout = windowLayout;
    winPage.update = windowUpdate;
    winPage.draw   = windowDraw;

    g_volumeInput.maxLen = 3;
    g_volumeInput.digitsOnly = true;

    g_nameInput.maxLen = 20;        // 20 个字符（中文也算 1 个）
    g_nameInput.focused = false;    // 进页面再打开
}

} // namespace

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main()
{
    Window::enableDpiAwareness();     // 高分屏不发糊（可选）

    Window win;
    g_win = &win;                     // 让绘制函数能读到当前窗口模式（只读）
    g_vp.setDesign(1280, 720);        // 设计分辨率：以后写坐标都按这个尺寸

    // 窗口尺寸一变：重新算缩放，并让所有页面重新排版
    win.onResize([&](int w, int h)
    {
        g_winW = w;
        g_winH = h;
        g_vp.fit(w, h);
        g_pages.layoutAll(g_vp);
    });

    if (!win.open(1024, 640)) return 1;
    win.setTitle("界面底座演示");
    win.setTargetFps(60);              // 动态限帧：按 60 FPS 计算每帧该睡多久

    // 注意：图片要在窗口建好之后再加载（EasyX 需要绘图环境）
    // 先按 exe 目录找（发布时 res 和 exe 放一起），失败再按当前工作目录找（VS 调试）
    if (!g_bg.load(assetPath("res/bg.png")))
        g_bg.load("res/bg.png");
    if (!g_logo.load(assetPath("res/logo.png")))
        g_logo.load("res/logo.png");

    buildPages();

    g_winW = win.width();
    g_winH = win.height();
    g_vp.fit(g_winW, g_winH);
    g_pages.layoutAll(g_vp);
    g_pages.go("menu");

    while (g_running)
    {
        win.poll();               // ① 窗口被拖大/最大化 → 自动重建 + 重新排版

        g_input.update();         // ② 收这一帧的鼠标键盘

        if (g_input.keyDown(VK_F11))          // ③ F11 切全屏（窗口操控演示）
        {
            if (win.mode() == WinMode::Fullscreen)
                win.applyMode(WinMode::Windowed, 1024, 640);
            else
                win.applyMode(WinMode::Fullscreen);
        }

        g_pages.update(g_input);  // ④ 当前页面的逻辑

        win.beginFrame(RGB(8, 8, 12));        // ⑤ 清屏
        g_vp.paintLetterbox(RGB(8, 8, 12));   //    把黑边涂成一样的颜色
        g_pages.draw(g_vp);                   // ⑥ 当前页面绘制

        win.endFrame();           // ⑦ 翻页 + 动态限帧（默认 60 FPS）
    }

    return 0;
}

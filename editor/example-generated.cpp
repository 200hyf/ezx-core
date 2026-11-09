// ============================================================
//  ui_generated.cpp
//  由 ezx-core UI 编辑器生成
//  设计分辨率: 1280 x 720
//  页面: 主页面 (UI_PAGE_MAIN) / 设置页 (UI_PAGE_SETTINGS)
//  布局: 按屏幕比例（pctRect，随窗口缩放）
//
//  用法（在 main 里）：
//      ui::init();                 // 建窗后调用一次
//      ...
//      ui::update(in, vp);         // 每帧
//      ui::draw(vp, in);           // 每帧，beginFrame 之后
//
//  按钮逻辑：
//      每个按钮都有一个独立的 onButton_xxx() 函数，
//      页面跳转、保存数据、开始游戏等逻辑都写在那里，一眼能看懂。
//
//  页面切换：
//      ui::goPage(UI_PAGE_SETTINGS);
// ============================================================
#include "easyx_core.h"
#include <chrono>

namespace ui {

    using namespace ezx;

    enum UiPage
    {
        UI_PAGE_MAIN = 0,
        UI_PAGE_SETTINGS = 1
    };

    enum UiId
    {
        UI_MAIN_BTN_START = 1,
        UI_MAIN_BTN_SETTINGS = 2,
        UI_MAIN_BTN_QUIT = 3,
        UI_SETTINGS_BTN_BACK = 4
    };

    static int g_page = UI_PAGE_MAIN;
    static Image g_bg;
    static Image g_p0_img_1;
    static Image g_p1_bg_1;
    static Image g_p1_icon_1;
    static TextInput g_p0_input_1;

    // 把“屏幕百分比”换算成当前 Viewport 的设计坐标
    static Rect pctRect(const Viewport& vp, float x, float y, float w, float h)
    {
        const Rect a = vp.area();
        const Point p0 = vp.toDesign(a.x + (int)(a.w * x), a.y + (int)(a.h * y));
        const Point p1 = vp.toDesign(a.x + (int)(a.w * (x + w)), a.y + (int)(a.h * (y + h)));
        return Rect{ p0.x, p0.y, p1.x - p0.x, p1.y - p0.y };
    }

    static void fillRound(const Rect& r, int radius, COLORREF color)
    {
        ::setfillcolor(color);
        if (radius > 0)
            ::solidroundrect(r.x, r.y, r.right(), r.bottom(), radius * 2, radius * 2);
        else
            ::solidrectangle(r.x, r.y, r.right() - 1, r.bottom() - 1);
    }

    static void frameRound(const Rect& r, int radius, int width, COLORREF color)
    {
        if (width <= 0) return;
        ::setlinecolor(color);
        ::setlinestyle(PS_SOLID, width);
        if (radius > 0)
            ::roundrect(r.x, r.y, r.right(), r.bottom(), radius * 2, radius * 2);
        else
            ::rectangle(r.x, r.y, r.right(), r.bottom());
        ::setlinestyle(PS_SOLID, 1);
    }

    static bool caretOn()
    {
        using namespace std::chrono;
        const auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        return (ms / 500) % 2 == 0;
    }

    void goPage(int page)
    {
        if (page >= 0 && page < 2) g_page = page;
    }

    int currentPage()
    {
        return g_page;
    }

    void init()
    {
        g_bg.load(assetPath("res/bg.png"));
        g_p0_img_1.load(assetPath("res/logo.png"));
        g_p0_input_1.maxLen = 12;
        g_p0_input_1.digitsOnly = false;
        g_p0_input_1.password = false;
        g_p1_bg_1.load(assetPath("res/bg.png"));
        g_p1_icon_1.load(assetPath("res/logo.png"));
        g_page = UI_PAGE_MAIN;
    }

// ============================================================
//  按钮回调：每个按钮一个函数，逻辑写在这里
// ============================================================
    // 按钮：开始游戏
    static void onButton_MAIN_BTN_START()
    {
        // 这个按钮没有配置页面跳转，逻辑写在这里：
        // TODO: 开始游戏
        // startGame();
    }

    // 按钮：设置（跳转到“设置页”）
    static void onButton_MAIN_BTN_SETTINGS()
    {
        // 页面跳转（编辑器里配置的）
        goPage(UI_PAGE_SETTINGS);
        
        // 其他逻辑写在这里：
        // TODO: 在这里写点击逻辑
    }

    // 按钮：退出
    static void onButton_MAIN_BTN_QUIT()
    {
        // 这个按钮没有配置页面跳转，逻辑写在这里：
        // TODO: 在这里写点击逻辑
    }

    // 按钮：返回（跳转到“主页面”）
    static void onButton_SETTINGS_BTN_BACK()
    {
        // 页面跳转（编辑器里配置的）
        goPage(UI_PAGE_MAIN);
        
        // 其他逻辑写在这里：
        // TODO: 在这里写点击逻辑
    }

// 按钮分发：一般不用改这里
    static void onButtonClick(UiId id)
    {
        switch (id)
        {
        case UI_MAIN_BTN_START: onButton_MAIN_BTN_START(); break;
        case UI_MAIN_BTN_SETTINGS: onButton_MAIN_BTN_SETTINGS(); break;
        case UI_MAIN_BTN_QUIT: onButton_MAIN_BTN_QUIT(); break;
        case UI_SETTINGS_BTN_BACK: onButton_SETTINGS_BTN_BACK(); break;
        default: break;
        }
    }

    static void updatePage0(const Input& in, const Viewport& vp)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };

        // 输入框焦点
        if (in.clicked())
        {
            g_p0_input_1.focused = inArea && pctRect(vp, 0.375, 0.7778, 0.25, 0.0833).contains(m);
            if (g_p0_input_1.focused)
                g_p0_input_1.placeCursor(vp, pctRect(vp, 0.375, 0.7778, 0.25, 0.0833), 24, in.pos().x);
        }
        g_p0_input_1.update(in);

        // 按钮点击（后画的优先）
        if (in.clicked() && inArea)
        {
            if (pctRect(vp, 0.375, 0.6444, 0.25, 0.0833).contains(m)) { onButtonClick(UI_MAIN_BTN_QUIT); return; }
            if (pctRect(vp, 0.375, 0.5444, 0.25, 0.0833).contains(m)) { onButtonClick(UI_MAIN_BTN_SETTINGS); return; }
            if (pctRect(vp, 0.375, 0.4444, 0.25, 0.0833).contains(m)) { onButtonClick(UI_MAIN_BTN_START); return; }
        }
    }

    static void updatePage1(const Input& in, const Viewport& vp)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };

        // 按钮点击（后画的优先）
        if (in.clicked() && inArea)
        {
            if (pctRect(vp, 0.375, 0.6944, 0.25, 0.0833).contains(m)) { onButtonClick(UI_SETTINGS_BTN_BACK); return; }
        }
    }

    void update(const Input& in, const Viewport& vp)
    {
        switch (g_page)
        {
        case UI_PAGE_MAIN:
            updatePage0(in, vp);
            break;
        case UI_PAGE_SETTINGS:
            updatePage1(in, vp);
            break;
        default:
            break;
        }
    }

    static void drawPage0(const Viewport& vp, const Input& in)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };
        // 图片
        vp.imageFit(g_p0_img_1, pctRect(vp, 0.4414, 0.0764, 0.1172, 0.2083));

        // 文字
        vp.textIn(pctRect(vp, 0, 0.3056, 1, 0.0972), "我的游戏", 52, RGB(242, 246, 255), Align::Center);

        // 按钮：开始游戏
        {
            const Rect r = pctRect(vp, 0.375, 0.4444, 0.25, 0.0833);
            const bool hover = r.contains(m);
            fillRound(r, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(r, 8, 2, RGB(92, 124, 196));
            vp.textIn(r, "开始游戏", 28, RGB(238, 242, 255), Align::Center);
        }

        // 按钮：设置
        {
            const Rect r = pctRect(vp, 0.375, 0.5444, 0.25, 0.0833);
            const bool hover = r.contains(m);
            fillRound(r, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(r, 8, 2, RGB(92, 124, 196));
            vp.textIn(r, "设置", 28, RGB(238, 242, 255), Align::Center);
        }

        // 按钮：退出
        {
            const Rect r = pctRect(vp, 0.375, 0.6444, 0.25, 0.0833);
            const bool hover = r.contains(m);
            fillRound(r, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(r, 8, 2, RGB(92, 124, 196));
            vp.textIn(r, "退出", 28, RGB(238, 242, 255), Align::Center);
        }

        // 输入框：INPUT_NAME
        {
            const Rect r = pctRect(vp, 0.375, 0.7778, 0.25, 0.0833);
            fillRound(r, 8, RGB(24, 30, 44));
            frameRound(r, 8, 2, g_p0_input_1.focused ? RGB(110, 170, 255) : RGB(74, 90, 122));
            if (g_p0_input_1.text.empty())
                vp.textIn(r.inset(12), "输入昵称…", 24, RGB(124, 138, 165), Align::Left);
            g_p0_input_1.draw(vp, r, 24, RGB(232, 238, 252), caretOn());
        }

    }

    static void drawPage1(const Viewport& vp, const Input& in)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };
        // 面板
        fillRound(pctRect(vp, 0.3047, 0.1389, 0.3906, 0.7222), 12, RGB(26, 32, 48));
        vp.image(g_p1_bg_1, pctRect(vp, 0.3047, 0.1389, 0.3906, 0.7222));
        frameRound(pctRect(vp, 0.3047, 0.1389, 0.3906, 0.7222), 12, 2, RGB(58, 74, 106));

        // 文字
        vp.textIn(pctRect(vp, 0.3047, 0.1944, 0.3906, 0.0972), "设置", 44, RGB(242, 246, 255), Align::Center);

        // 按钮：返回
        {
            const Rect r = pctRect(vp, 0.375, 0.6944, 0.25, 0.0833);
            const bool hover = r.contains(m);
            fillRound(r, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(r, 8, 2, RGB(92, 124, 196));
            const int is = 36, gap = 8;
            vp.imageFit(g_p1_icon_1, Rect{ r.x + 12, r.y + (r.h - is) / 2, is, is });
            vp.textIn(Rect{ r.x + 12 + is + gap, r.y, 252, r.h }, "返回", 28, RGB(238, 242, 255), Align::Center);
        }

    }

    void draw(const Viewport& vp, const Input& in)
    {
        vp.fill(pctRect(vp, 0, 0, 1, 1), RGB(18, 20, 28));
        vp.imageCover(g_bg, pctRect(vp, 0, 0, 1, 1));
        switch (g_page)
        {
        case UI_PAGE_MAIN:
            drawPage0(vp, in);
            break;
        case UI_PAGE_SETTINGS:
            drawPage1(vp, in);
            break;
        default:
            break;
        }
    }

} // namespace ui

// 在 main 中调用：
//   ui::init();
//   ui::update(in, vp);
//   ui::draw(vp, in);
//   ui::goPage(UI_PAGE_MAIN);  // 切页面

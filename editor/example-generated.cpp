// ============================================================
//  ui_generated.cpp
//  由 ezx-core UI 编辑器生成
//  设计分辨率: 1280 x 720
//
//  用法（在 main 里）：
//      ui::init();                 // 建窗后调用一次
//      ...
//      ui::update(in, vp);         // 每帧
//      ui::draw(vp, in);           // 每帧，beginFrame 之后
// ============================================================
#include "easyx_core.h"
#include <chrono>

namespace ui {

    using namespace ezx;

    enum UiId
    {
        UI_BTN_START = 1,
        UI_BTN_SETTINGS = 2,
        UI_BTN_QUIT = 3
    };

    static Image g_img_1;
    static TextInput g_input_1;

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

    void init()
    {
        g_img_1.load(assetPath("res/logo.png"));
        g_input_1.maxLen = 12;
        g_input_1.digitsOnly = false;
        g_input_1.password = false;
    }

    void onClick(int id)
    {
        switch (id)
        {
        case UI_BTN_START:
        // 点击“开始游戏”
            // TODO: 开始游戏
            // startGame();
            break;
        case UI_BTN_SETTINGS:
        // 点击“设置”
            // TODO: 在这里写点击逻辑
            break;
        case UI_BTN_QUIT:
        // 点击“退出”
            // TODO: 在这里写点击逻辑
            break;
        default:
            break;
        }
    }

    void update(const Input& in, const Viewport& vp)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };

        // 输入框焦点
        if (in.clicked())
        {
            g_input_1.focused = inArea && Rect{ 480, 560, 320, 60 }.contains(m);
            if (g_input_1.focused)
                g_input_1.placeCursor(vp, Rect{ 480, 560, 320, 60 }, 24, in.pos().x);
        }
        g_input_1.update(in);

        // 按钮点击（后画的优先）
        if (in.clicked() && inArea)
        {
            if (Rect{ 480, 464, 320, 60 }.contains(m)) { onClick(UI_BTN_QUIT); return; }
            if (Rect{ 480, 392, 320, 60 }.contains(m)) { onClick(UI_BTN_SETTINGS); return; }
            if (Rect{ 480, 320, 320, 60 }.contains(m)) { onClick(UI_BTN_START); return; }
        }
    }

    void draw(const Viewport& vp, const Input& in)
    {
        const bool inArea = vp.inside(in.pos().x, in.pos().y);
        const Point m = inArea ? vp.toDesign(in.pos()) : Point{ -1, -1 };
        // 图片
        vp.imageFit(g_img_1, Rect{ 565, 55, 150, 150 });

        // 文字
        vp.textIn(Rect{ 0, 220, 1280, 70 }, "我的游戏", 52, RGB(242, 246, 255), Align::Center);

        // 按钮：开始游戏
        {
            const bool hover = Rect{ 480, 320, 320, 60 }.contains(m);
            fillRound(Rect{ 480, 320, 320, 60 }, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(Rect{ 480, 320, 320, 60 }, 8, 2, RGB(92, 124, 196));
            vp.textIn(Rect{ 480, 320, 320, 60 }, "开始游戏", 28, RGB(238, 242, 255), Align::Center);
        }

        // 按钮：设置
        {
            const bool hover = Rect{ 480, 392, 320, 60 }.contains(m);
            fillRound(Rect{ 480, 392, 320, 60 }, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(Rect{ 480, 392, 320, 60 }, 8, 2, RGB(92, 124, 196));
            vp.textIn(Rect{ 480, 392, 320, 60 }, "设置", 28, RGB(238, 242, 255), Align::Center);
        }

        // 按钮：退出
        {
            const bool hover = Rect{ 480, 464, 320, 60 }.contains(m);
            fillRound(Rect{ 480, 464, 320, 60 }, 8, hover ? RGB(62, 86, 132) : RGB(34, 42, 62));
            frameRound(Rect{ 480, 464, 320, 60 }, 8, 2, RGB(92, 124, 196));
            vp.textIn(Rect{ 480, 464, 320, 60 }, "退出", 28, RGB(238, 242, 255), Align::Center);
        }

        // 输入框：INPUT_NAME
        fillRound(Rect{ 480, 560, 320, 60 }, 8, RGB(24, 30, 44));
        frameRound(Rect{ 480, 560, 320, 60 }, 8, 2, g_input_1.focused ? RGB(110, 170, 255) : RGB(74, 90, 122));
        if (g_input_1.text.empty())
            vp.textIn(Rect{ 480, 560, 320, 60 }.inset(12), "输入昵称…", 24, RGB(124, 138, 165), Align::Left);
        g_input_1.draw(vp, Rect{ 480, 560, 320, 60 }, 24, RGB(232, 238, 252), caretOn());

    }

} // namespace ui

// 在 main 中调用：
//   ui::init();
//   ui::update(in, vp);
//   ui::draw(vp, in);

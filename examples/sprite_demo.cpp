// ============================================================================
//  examples/sprite_demo.cpp —— ezx-core v2.1 精灵 / 图集 / 动画演示
// ----------------------------------------------------------------------------
//  演示 4 件事：
//    1) SpriteSheet::loadGrid  把一张 4 帧图集切成帧
//    2) Animation              按 8 FPS 循环播放
//    3) Sprite                 位置、缩放、锚点，画到 Viewport 上
//    4) Input::keyHeld         WASD 连续移动（不是"按下这一帧"）
//
//  自测模式：sprite_demo.exe --selftest 120   跑 120 帧后自动退出并打印结果
// ============================================================================
#include "easyx_core.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ezx;

int main(int argc, char** argv)
{
    int selftest = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--selftest") == 0 && i + 1 < argc)
            selftest = std::atoi(argv[i + 1]);
    }

    Window::enableDpiAwareness();

    Window win;
    if (!win.open(960, 540)) return 1;
    win.setTitle("ezx-core v2.1 - Sprite / SpriteSheet / Animation");

    Viewport vp;
    vp.setDesign(960, 540);
    vp.fit(win.width(), win.height());
    win.onResize([&](int w, int h) { vp.fit(w, h); });

    Sprite hero;
    if (!hero.loadGrid(assetPath("res/sprite_sheet.png"), 32, 32, 4))
    {
        std::printf("[error] 加载 res/sprite_sheet.png 失败\n");
        return 2;
    }
    hero.setScale(4.f);          // 32 像素放大 4 倍 = 128
    hero.setPosition(480.f, 430.f);
    hero.anim().setFps(8.f);
    hero.anim().setLoop(true);

    Input in;
    auto last = std::chrono::steady_clock::now();
    int frames = 0;

    while (win.alive())
    {
        win.poll();
        in.update();
        if (in.keyDown(VK_ESCAPE)) break;

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float, std::milli>(now - last).count();
        last = now;

        const float speed = 0.22f * dt;           // 每秒 220 设计像素
        float dx = 0.f, dy = 0.f;
        if (in.keyHeld('A')) dx -= 1.f;
        if (in.keyHeld('D')) dx += 1.f;
        if (in.keyHeld('W')) dy -= 1.f;
        if (in.keyHeld('S')) dy += 1.f;
        if (dx != 0.f || dy != 0.f)
        {
            hero.move(dx * speed, dy * speed);
            if (hero.x() < 40.f)  hero.setPosition(40.f, hero.y());
            if (hero.x() > 920.f) hero.setPosition(920.f, hero.y());
            if (hero.y() < 120.f) hero.setPosition(hero.x(), 120.f);
            if (hero.y() > 470.f) hero.setPosition(hero.x(), 470.f);
        }

        hero.update(dt);                          // 推进动画

        win.beginFrame(RGB(24, 28, 38));
        vp.paintLetterbox(RGB(24, 28, 38));
        vp.fill(Rect{ 0, 0, 960, 540 }, RGB(30, 36, 48));
        vp.fill(Rect{ 0, 460, 960, 80 }, RGB(40, 52, 44));
        vp.fill(Rect{ 0, 458, 960, 2  }, RGB(58, 78, 62));

        hero.draw(vp);                            // 精灵

        vp.text(20, 16, "WASD 移动   Esc 退出", 22, RGB(200, 210, 225));
        vp.text(20, 48, "frame " + std::to_string(hero.frame()) + " / " +
                        std::to_string(hero.sheet().count()), 20, RGB(140, 160, 180));
        win.endFrame();

        ++frames;
        if (selftest > 0 && frames >= selftest) break;
    }

    std::printf("[ok] frames=%d pos=(%.0f,%.0f) frame=%d count=%d\n",
                frames, hero.x(), hero.y(), hero.frame(), hero.sheet().count());
    return 0;
}

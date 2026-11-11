// ============================================================================
//  examples/platformer_demo.cpp —— ezx-core v2.2 场景 / 碰撞 / 相机演示
// ----------------------------------------------------------------------------
//  演示：
//    1) Scene + Entity      实体容器、静态碰撞体（地面、平台、墙）
//    2) moveAndCollide      轴分离移动：撞墙停住、落地站住
//    3) Camera2D            横向跟随 + 限制在世界范围内
//    4) Sprite/Animation    角色动画（跑起来快、站住慢）
//
//  操作：A / D 移动，空格跳，Esc 退出。
//  自测：platformer_demo.exe --selftest 240   脚本化输入跑 240 帧后退出
// ============================================================================
#include "easyx_core.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ezx;

namespace {

constexpr float kWorldW  = 1920.f;
constexpr float kWorldH  = 540.f;
constexpr float kGravity = 0.0028f;   // 像素 / 毫秒²
constexpr float kJumpV   = -0.95f;    // 像素 / 毫秒
constexpr float kSpeed   = 0.32f;

} // namespace

int main(int argc, char** argv)
{
    int selftest = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--selftest") == 0 && i + 1 < argc)
            selftest = std::atoi(argv[i + 1]);
    }

    Window win;
    if (!win.open(960, 540)) return 1;
    win.setTitle("ezx-core v2.2 - Scene / Collision / Camera");

    Viewport vp;
    vp.setDesign(960, 540);
    vp.fit(win.width(), win.height());
    win.onResize([&](int w, int h) { vp.fit(w, h); });

    // ---- 关卡 ----
    Scene scene;
    scene.solids = {
        RectF{ 0.f,   500.f, kWorldW, 40.f },        // 地面
        RectF{ 0.f,   0.f,   16.f,   540.f },        // 左墙
        RectF{ kWorldW - 16.f, 0.f, 16.f, 540.f },   // 右墙
        RectF{ 420.f, 410.f, 180.f, 20.f },          // 平台
        RectF{ 760.f, 330.f, 160.f, 20.f },
        RectF{ 1120.f, 400.f, 220.f, 20.f },
    };

    Entity& player = scene.add();
    if (!player.sprite.loadGrid(assetPath("res/sprite_sheet.png"), 32, 32, 4))
    {
        std::printf("[error] 加载 res/sprite_sheet.png 失败\n");
        return 2;
    }
    player.sprite.setScale(1.5f);
    player.w = 28.f;
    player.h = 44.f;
    player.x = 160.f;
    player.y = 500.f;
    player.layer = 1;

    Camera2D cam;
    cam.setViewSize(960.f, 540.f);
    cam.setPosition(480.f, 270.f);
    cam.clampToWorld(kWorldW, kWorldH);

    Input in;
    auto last = std::chrono::steady_clock::now();
    int   frames = 0;
    bool  grounded = false;

    while (win.alive())
    {
        win.poll();
        in.update();
        if (in.keyDown(VK_ESCAPE)) break;

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float, std::milli>(now - last).count();
        last = now;

        // ---- 输入（自测时用脚本代替键盘）----
        float move = 0.f;
        bool  jump = false;
        if (selftest > 0)
        {
            move = 1.f;
            jump = (frames == 30 || frames == 90);
        }
        else
        {
            if (in.keyHeld('A')) move -= 1.f;
            if (in.keyHeld('D')) move += 1.f;
            jump = in.keyDown(VK_SPACE) || in.keyDown('W');
        }

        if (jump && grounded) player.vy = kJumpV;

        // ---- 物理 ----
        player.vx = move * kSpeed;
        player.vy += kGravity * dt;
        if (player.vy > 2.f) player.vy = 2.f;

        const CollideResult hit = moveAndCollide(player, player.vx * dt, player.vy * dt, scene.solids);
        if (hit.hitY) player.vy = 0.f;
        grounded = hit.grounded;

        // ---- 动画 + 场景 ----
        player.sprite.anim().setFps(move != 0.f ? 10.f : 4.f);
        scene.update(dt);

        // ---- 相机跟随 ----
        cam.follow(player.x, 270.f, 0.12f);
        cam.clampToWorld(kWorldW, kWorldH);

        // ---- 绘制 ----
        win.beginFrame(RGB(16, 20, 28));
        vp.paintLetterbox(RGB(16, 20, 28));
        vp.fill(Rect{ 0, 0, 960, 540 }, RGB(22, 28, 40));
        vp.fill(Rect{ 0, 300, 960, 240 }, RGB(28, 36, 50));

        for (const RectF& s : scene.solids)
        {
            if (s.w < 30.f) continue;                 // 墙不画
            const Rect d = cam.toDesign(s).toRect();
            vp.fill(d, RGB(52, 66, 58));
            vp.fill(Rect{ d.x, d.y, d.w, 4 }, RGB(78, 98, 84));
        }

        scene.draw(vp, cam);

        vp.text(20, 16, "A / D 移动    空格跳跃    Esc 退出", 20, RGB(200, 210, 225));
        vp.text(20, 46, "x=" + std::to_string((int)player.x) +
                        "  grounded=" + (grounded ? "1" : "0"), 18, RGB(140, 160, 180));
        win.endFrame();

        ++frames;
        if (selftest > 0 && frames >= selftest) break;
    }

    std::printf("[ok] frames=%d pos=(%.0f,%.0f) grounded=%d solids=%d\n",
                frames, player.x, player.y, grounded ? 1 : 0, (int)scene.solids.size());
    return 0;
}

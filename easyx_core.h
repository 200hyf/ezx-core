// ============================================================================
//  easyx_core.h  ——  EasyX 界面底座（不含任何按钮外观）
// ----------------------------------------------------------------------------
//  提供 7 样东西，按钮长什么样由你自己画：
//    1) Window    窗口操控：建窗、切全屏、拖拽/最大化后自动重建
//    2) Viewport  分辨率适配：按 1280×720 之类的"设计尺寸"写坐标，自动等比缩放 + 黑边
//    3) Input     鼠标/键盘：位置、按下、松开、点击、双击、拖拽、滚轮
//    4) Image     图片绘制：加载、拉伸、等比、铺满（支持 png 透明）
//    5) TextInput 文本输入逻辑：光标、删除、过滤字符、长度限制、数值钳制
//    6) Page/Pages 页面框架：每个页面自己的 排版/逻辑/绘制 三段函数
//    7) SpriteSheet / Animation / Sprite  图集切帧、帧动画、精灵移动
//
//  用法：把 easyx_core.h + easyx_core.cpp 加进工程，#include "easyx_core.h"。
//  详见 README.md 和 使用说明.md。
// ============================================================================
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <chrono>
#include <functional>

#include <graphics.h>
#include <windows.h>

// EasyX 运行时需要的 Windows 库，写在这里省得新工程第一次链接就报 LNK2019
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "imm32.lib")     // 中文输入法（IME）定位用

namespace ezx {

// ===========================================================================
//  基础几何
// ===========================================================================
struct Point
{
    int x = 0;
    int y = 0;
};

// 矩形：界面里所有"一块区域"都用它表示（按钮范围、文本框、面板……）
struct Rect
{
    int x = 0;      // 左上角
    int y = 0;
    int w = 0;      // 宽高
    int h = 0;

    int right()  const { return x + w; }
    int bottom() const { return y + h; }
    Point center() const { return Point{ x + w / 2, y + h / 2 }; }

    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
    bool contains(const Point& p) const { return contains(p.x, p.y); }
    bool empty() const { return w <= 0 || h <= 0; }

    Rect inset(int d) const          { return Rect{ x + d, y + d, w - d * 2, h - d * 2 }; }
    Rect inset(int dx, int dy) const { return Rect{ x + dx, y + dy, w - dx * 2, h - dy * 2 }; }
    // 在自身内部居中放一个 cw×ch 的矩形
    Rect centered(int cw, int ch) const { return Rect{ x + (w - cw) / 2, y + (h - ch) / 2, cw, ch }; }

    Rect takeLeft(int partW)   const { return Rect{ x, y, partW, h }; }
    Rect takeRight(int partW)  const { return Rect{ right() - partW, y, partW, h }; }
    Rect takeTop(int partH)    const { return Rect{ x, y, w, partH }; }
    Rect takeBottom(int partH) const { return Rect{ x, bottom() - partH, w, partH }; }
};

// 把一块区域横向/纵向等分成 count 份（gap = 每份之间的空隙）
std::vector<Rect> splitRow(const Rect& area, int count, int gap = 0);
std::vector<Rect> splitCol(const Rect& area, int count, int gap = 0);

// ===========================================================================
//  文字
// ===========================================================================
// 设置全局默认字体（"宋体"、"微软雅黑"……），第一次画字前调用一次即可
void setDefaultFont(const std::string& name);
const std::string& defaultFont();

// 换当前字体（画字/量字之前会自动调用，一般不用你手动调）
void useFont(int fontSize, const std::string& font = "");

// 量一段文字画出来多宽、多高（像素）
int textWidth(const std::string& s, int fontSize, const std::string& font = "");
int textHeight(const std::string& s, int fontSize, const std::string& font = "");

// 画字（x,y = 左上角）
void drawText(int x, int y, const std::string& s, int fontSize, COLORREF color,
              const std::string& font = "");

// ---------------------------------------------------------------------------
// 路径：打包发布时用这两个，不要写死 D:\... 的绝对路径
//   appDir()              = exe 所在目录（UTF-8，带结尾斜杠）
//   assetPath("res/x.png") = appDir() + "res/x.png"
// 这样不管从哪个目录启动，都能找到 exe 旁边的资源。
// ---------------------------------------------------------------------------
std::string appDir();
std::string assetPath(const std::string& relative);

// ===========================================================================
//  图片
// ===========================================================================
class Image
{
public:
    // 加载图片（png/jpg/bmp）。w/h 传 0 = 原尺寸；传值 = 加载时就缩放
    bool load(const std::string& path, int w = 0, int h = 0);

    bool ok() const { return img_ != nullptr; }
    int  width()  const { return w_; }
    int  height() const { return h_; }

    // 按原尺寸画在 (x,y)
    void draw(int x, int y) const;
    // 拉伸铺满整个矩形（宽高比可能变）
    void draw(const Rect& dst) const;
    // 等比缩放，完整放进矩形（可能有留边）
    void drawFit(const Rect& dst) const;
    // 等比缩放，铺满矩形（多出来的部分裁掉）
    void drawCover(const Rect& dst) const;
    // 从图里 src 这块（像素坐标）缩放画到 dst —— 图集的每一帧就靠它
    void drawPart(const Rect& src, const Rect& dst) const;
    // 截一块（像素坐标）作为新图（图集切帧用）
    Image crop(const Rect& src) const;

private:
    void blit(int dx, int dy, int dw, int dh, const Rect& clip) const;   // 内部：带裁剪的绘制
    // 内部：把 src 这块源图缩放画到 (dx,dy,dw,dh)，只画 clip 里的部分
    void blitFrom(const Rect& src, int dx, int dy, int dw, int dh, const Rect& clip) const;

    std::shared_ptr<IMAGE> img_;
    int  w_ = 0;
    int  h_ = 0;
    bool alpha_ = false;     // 图里是否有透明像素（有的话要自己混合）
};

// ===========================================================================
//  精灵与动画（v2.1）
// ---------------------------------------------------------------------------
//  SpriteSheet  一帧一帧的图：一张图集按网格切，或者一帧一个文件
//  Animation    帧序列播放：帧区间、帧率、循环、播放/停止
//  Sprite       位置 + 缩放 + 锚点 + 当前帧，负责画到 Viewport 上
//
//  典型用法：
//      SpriteSheet sheet;
//      sheet.loadGrid(assetPath("res/hero.png"), 32, 32, 4);   // 4 列
//      Animation walk;
//      walk.setFrames(0, 3); walk.setFps(10);
//      Sprite hero; hero.setSheet(sheet); hero.setPosition(320, 400);
//      // 每帧： hero.update(dtMs); hero.draw(vp);
// ===========================================================================
class Viewport;   // 前置声明：Sprite::draw 只用到引用

class SpriteSheet
{
public:
    // 网格图集：每帧 frameW×frameH，cols 列（0 = 按图宽自动算），count 帧（0 = 全切）
    bool loadGrid(const std::string& path, int frameW, int frameH, int cols = 0, int count = 0);

    // 文件序列：prefix 含目录和前缀（"res/anim_"），first..last，digits 补零位数
    bool loadSequence(const std::string& prefix, const std::string& suffix,
                      int first, int last, int digits = 4);

    bool ok() const { return !frames_.empty(); }
    int  count() const { return (int)frames_.size(); }
    int  frameW() const { return fw_; }
    int  frameH() const { return fh_; }

    const Image& frame(int index) const;

private:
    std::vector<Image> frames_;
    int fw_ = 0;
    int fh_ = 0;
};

class Animation
{
public:
    void setFrames(int first, int last);   // 用图集里的 [first, last] 帧
    void setFps(float fps);                // 每秒几帧
    void setLoop(bool loop);               // 播完是否从头再来
    void play(bool restart = true);        // 开始播（restart = 回到第一帧）
    void stop();                           // 停在当前帧
    void reset();                          // 回到第一帧、清空计时

    // 每帧调用：dtMs = 这一帧过了多少毫秒（用 GetTickCount64 算）
    void update(float dtMs);

    bool  playing() const { return playing_; }
    bool  finished() const { return finished_; }   // 非循环播完了
    int   frame() const { return cur_; }
    int   first() const { return first_; }
    int   last()  const { return last_; }
    float fps()   const { return fps_; }

private:
    int   first_ = 0;
    int   last_ = 0;
    int   cur_ = 0;
    float fps_ = 12.f;
    float acc_ = 0.f;
    bool  loop_ = true;
    bool  playing_ = true;
    bool  finished_ = false;
};

class Sprite
{
public:
    void setSheet(const SpriteSheet& sheet);   // 拷贝一份（帧图是共享的，很便宜）

    bool loadGrid(const std::string& path, int frameW, int frameH, int cols = 0, int count = 0);
    bool loadSequence(const std::string& prefix, const std::string& suffix,
                      int first, int last, int digits = 4);

    const SpriteSheet& sheet() const { return sheet_; }
    Animation&         anim()        { return anim_; }
    const Animation&   anim()  const { return anim_; }

    void setPosition(float x, float y);
    void move(float dx, float dy);

    void  setScale(float scale);           // 1 = 原始像素大小
    float scale() const { return scale_; }

    // 锚点：0~1，(0,0)=左上，(0.5,1)=脚底中心（默认）
    void setAnchor(float ax, float ay);

    void setFrame(int index);              // 不走动画时直接指定显示哪帧
    int  frame() const { return frame_; }

    void update(float dtMs);               // 推进动画
    void draw(const Viewport& vp) const;   // 用设计坐标画到屏幕上

    // 当前这一帧在设计坐标里的矩形（碰撞、点击判定用）
    Rect bounds() const;

    float x() const { return x_; }
    float y() const { return y_; }

private:
    SpriteSheet sheet_;
    Animation   anim_;
    float x_ = 0.f;
    float y_ = 0.f;
    float scale_ = 1.f;
    float ax_ = 0.5f;
    float ay_ = 1.f;
    int   frame_ = 0;
};

// ===========================================================================
//  输入（鼠标 + 键盘）
// ===========================================================================
class Input
{
public:
    void update();          // 每帧调用一次：把这一帧的消息全部取出来

    // ---- 鼠标 ----
    Point pos()       const { return pos_; }        // 当前位置（窗口客户区坐标）；没动过是 (-1,-1)
    Point pressPos()  const { return pressPos_; }   // 本次按下的位置
    bool  down()      const { return down_; }       // 这一帧刚按下
    bool  up()        const { return up_; }         // 这一帧刚松开
    bool  held()      const { return held_; }       // 一直按着
    bool  clicked()   const { return clicked_; }    // 这一帧完成了一次点击（按下→几乎没动→松开）
    bool  doubleClicked() const { return dblClick_; }
    bool  dragging()  const { return dragging_; }   // 按住并移动超过阈值
    int   wheel()     const { return wheel_; }      // 这一帧滚轮（正 = 往上滚）

    // 便利：鼠标是否在某个"屏幕矩形"里 / 是否点中了它（配合 vp.R(设计矩形) 使用）
    bool mouseIn(const Rect& screenRect) const { return screenRect.contains(pos_); }
    bool clickedIn(const Rect& screenRect) const { return screenRect.contains(pos_) && clicked_; }

    // ---- 键盘 ----
    bool keyDown(int vk) const;                       // 这一帧有没有按这个键
    bool keyHeld(int vk) const;                       // 这个键现在是否按着（做移动用）
    const std::vector<int>& keys() const { return keys_; }   // 这一帧按下的虚拟键码

    // 这一帧输入的字符（UTF-16，中文/emoji 都在里面）。
    // 一般不用直接读：交给 TextInput::update() 即可。
    const std::wstring& chars() const { return chars_; }

    // 把这一帧字符转成 UTF-8（每次调用都转换，别在一帧里反复调）
    std::string charsUtf8() const;

    // 悬停在可点击的东西上时换成"小手"（传 false 换回箭头）
    void setHandCursor(bool on);

    // ---- 中文输入法 ----
    // 把输入法候选框挪到光标附近。参数是"窗口客户区"坐标。
    // 通常这样算：Rect r = vp.R(输入框设计矩形); in.setImePosition(r.x, r.bottom());
    // 这两个函数可以在 const Input& 上调用（页面 update 的参数就是 const Input&）。
    void setImePosition(int clientX, int clientY) const;
    void clearImePosition() const;

private:
    void appendChar(wchar_t ch);       // 追加一个 UTF-16 码元（处理代理对）
#ifndef _UNICODE
    void appendAnsiByte(char c);       // 多字节工程：拼 DBCS 双字节，再转 Unicode
#endif
    void applyImePosition() const;

    Point pos_{ -1, -1 };
    Point pressPos_{ -1, -1 };
    bool  down_ = false, up_ = false, held_ = false;
    bool  clicked_ = false, dblClick_ = false, dragging_ = false;
    int   wheel_ = 0;
    std::wstring chars_;
    std::vector<int> keys_;

    bool    hand_ = false;
    bool    pressedLast_ = false;
    long long lastClickTime_ = 0;   // steady_clock 毫秒
    Point   lastClickPos_{ 0, 0 };
    HWND    hwnd_ = nullptr;

    wchar_t       pendingHigh_ = 0;    // 上一帧没配对的 UTF-16 高位代理
    unsigned char ansiLead_    = 0;    // 多字节工程：待配对的 DBCS 前导字节

    mutable Point imePos_{ -1, -1 };
    mutable Point imeApplied_{ -1, -1 };
    mutable HWND  imeHwnd_ = nullptr;
    mutable bool  hasImePos_ = false;
};

// ===========================================================================
//  文本输入（只做"编辑逻辑"，长什么样你自己画）
// ===========================================================================
class Viewport;   // 下面 TextInput::draw 会用到它（这里只是先打个招呼）

class TextInput
{
public:
    std::string text;          // 当前内容（UTF-8；中文一个字占 3 字节，但算 1 个字符）
    int  cursor = 0;           // 光标位置（text 里的"字节下标"，不要手改，用下面方法）
    int  maxLen = 16;          // 最多几个"字符"（中文算 1 个）；0 = 不限
    bool digitsOnly = false;   // 只允许输入 0-9（中文会被忽略）
    bool password = false;     // true = 每个字符画成 *
    bool focused = true;       // false = 不响应键盘

    void clear();
    void setText(const std::string& s);   // 赋值（自动截断到 maxLen 个字符，光标移到末尾）
    int  length() const;                  // 当前有几个"字符"（不是字节）
    int  number(int fallback = 0) const;  // 当数字用；内容非法返回 fallback
    void clampNumber(int lo, int hi);     // 空/非法不动，否则把数值钳到 [lo,hi] 并写回

    // 处理这一帧的键盘输入（要先调用 input.update()）。
    // 中文由 Input 收集成 UTF-16，这里会转成 UTF-8 插入，光标按"字符"移动。
    void update(const Input& in);

    // 鼠标点在输入框里时，把光标放到离鼠标最近的字符边界上。
    // fontSize 必须和 draw() 传的一致；mouseX 是窗口客户区坐标。
    void placeCursor(const Viewport& vp, const Rect& box, int fontSize, int mouseX);

    // 这一帧按了回车 / Esc 吗（自己清空：处理过就调用 consume()）
    bool submitted() const { return submitted_; }
    bool canceled()  const { return canceled_; }
    void consume();

    // 在 box 里画出文字和光标（design 坐标，由 Viewport 负责缩放）。
    // 只画字，不画边框/背景 —— 想要框自己 fill 一个矩形。
    void draw(const Viewport& vp, const Rect& box, int fontSize, COLORREF color,
              bool caretVisible) const;

private:
    bool submitted_ = false;
    bool canceled_ = false;
};

// ===========================================================================
//  Viewport：分辨率适配
// ---------------------------------------------------------------------------
//  你按"设计尺寸"（例如 1280×720）写坐标，它负责：
//    - 窗口比设计尺寸大 → 整体放大，保持比例，多出来的地方留黑边；
//    - 窗口比设计尺寸小 → 整体缩小；
//    - 鼠标坐标反向换算回设计坐标，命中判断仍然用设计坐标写。
// ===========================================================================
enum class Align { Left, Center, Right };

// 设计内容和窗口比例不一致时怎么办：
//   Contain（默认）= 完整显示，多出来的地方留黑边
//   Cover          = 铺满窗口，超出边缘的部分裁掉（没有黑边）
enum class ScaleMode
{
    Contain,
    Cover
};

class Viewport
{
public:
    void setDesign(int w, int h);     // 设计尺寸（默认 1280×720）
    void fit(int winW, int winH);     // 窗口尺寸变化时调用：重新算缩放和黑边

    // 切换"留黑边"还是"裁边铺满"，会立即按当前窗口重算
    void setScaleMode(ScaleMode mode);
    ScaleMode scaleMode() const { return mode_; }

    float scale()   const { return scale_; }
    int   offsetX() const { return ox_; }
    int   offsetY() const { return oy_; }
    int   designW() const { return designW_; }
    int   designH() const { return designH_; }
    // 设计区在窗口里占的矩形（屏幕坐标）
    Rect  area()    const;

    // 鼠标：窗口坐标 → 设计坐标
    Point toDesign(int screenX, int screenY) const;
    Point toDesign(const Point& screen) const { return toDesign(screen.x, screen.y); }
    // 鼠标是否落在设计区内（黑边上返回 false）
    bool  inside(int screenX, int screenY) const { return area().contains(screenX, screenY); }

    // ---- 用"设计坐标"画东西（内部自动换算 + 缩放）----
    Rect  R(const Rect& r) const;                       // 矩形换算成屏幕矩形
    Rect  R(int x, int y, int w, int h) const { return R(Rect{ x, y, w, h }); }
    // 鼠标（屏幕坐标）是否落在某个设计矩形里 —— 最常用的命中判断
    bool  hit(const Rect& designRect, const Point& screenPos) const
    {
        return R(designRect).contains(screenPos);
    }
    void  fill(const Rect& r, COLORREF color) const;    // 实心矩形
    void  frame(const Rect& r, COLORREF color, int thickness = 2) const;   // 空心框

    void  text(int x, int y, const std::string& s, int fontSize, COLORREF color) const;
    void  textIn(const Rect& r, const std::string& s, int fontSize, COLORREF color,
                 Align align = Align::Center) const;    // 在矩形里垂直居中 + 水平对齐

    void  image(const Image& img, const Rect& dst) const;       // 拉伸铺满
    void  imageFit(const Image& img, const Rect& dst) const;    // 等比放入
    void  imageCover(const Image& img, const Rect& dst) const;  // 等比铺满（裁边）

    // 用某个颜色把黑边涂掉（在 beginFrame 之后调一次）
    void  paintLetterbox(COLORREF color) const;

private:
    int   designW_ = 1280;
    int   designH_ = 720;
    float scale_ = 1.0f;
    int   ox_ = 0;
    int   oy_ = 0;
    int   winW_ = 0;
    int   winH_ = 0;
    ScaleMode mode_ = ScaleMode::Contain;
};

// ===========================================================================
//  页面框架
// ---------------------------------------------------------------------------
//  一个页面 = 三个函数：
//      layout(vp)  窗口尺寸变了就调用一次（算按钮/文本框的位置）
//      update(in)  每帧调用（处理鼠标键盘、改状态）
//      draw(vp)    每帧调用（画东西）
//  没写的函数可以直接留空。
// ===========================================================================
struct Page
{
    std::string name;
    std::function<void(const Viewport&)> layout;
    std::function<void(const Input&)>    update;
    std::function<void(const Viewport&)> draw;
};

class Pages
{
public:
    Page& add(const std::string& name);          // 添加页面，返回它（可以接着赋值三个函数）
    void  go(const std::string& name);           // 切换页面（会调用新页面的 layout）
    bool  is(const std::string& name) const;
    Page& current();                             // 当前页面
    const std::string& currentName() const;

    void layoutAll(const Viewport& vp);          // 让所有页面重新排版
    void update(const Input& in);                // 当前页面 逻辑
    void draw(const Viewport& vp);               // 当前页面 绘制

    int count() const { return (int)pages_.size(); }

private:
    // 用 deque 而不是 vector：push_back 不会让已经拿到的 Page& 失效。
    // 这样 Page& p = pages.add(...); 之后再加页面，p 仍然有效。
    std::deque<Page> pages_;
    int  current_ = -1;
    Viewport lastVp_;
    bool hasLayout_ = false;
};

// ===========================================================================
//  窗口
// ===========================================================================
enum class WinMode
{
    Windowed,    // 普通窗口：有标题栏，能拖边缩放
    WorkArea,    // 铺满屏幕但不盖任务栏
    Fullscreen   // 无边框真全屏（盖住任务栏）
};

class Window
{
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool open(int w, int h, WinMode mode = WinMode::Windowed);
    void close();

    // 运行时切换（全屏按钮、还原……）
    bool applyMode(WinMode mode, int w = 0, int h = 0);
    void minimize();
    void maximize();
    void restore();

    // 每帧调用：窗口被拖大/最大化/还原后，自动重建窗口并通知 onResize
    bool poll();

    void onResize(std::function<void(int w, int h)> cb);   // 参数 = 新的客户区宽高
    void setTitle(const std::string& text);

    void beginFrame(COLORREF bg = CLR_INVALID);   // 清屏（CLR_INVALID = 黑）

    // 翻页 + 限帧。
    //   endFrame()       = 按 targetFps 动态计算要睡多久（推荐，默认 60 FPS）
    //   endFrame(10)     = 旧写法：固定 Sleep(10)，不推荐（不同机器帧率会飘）
    void endFrame();
    void endFrame(int delayMs);

    // 目标帧率：0 = 不限帧；默认 60。改完下一帧立即生效。
    void setTargetFps(int fps);
    int  targetFps() const { return targetFps_; }

    HWND   handle() const { return hWnd_; }
    int    width()  const { return w_; }
    int    height() const { return h_; }
    WinMode mode()  const { return mode_; }
    bool   alive()  const { return hWnd_ != nullptr; }

    static void enableDpiAwareness();   // 高分屏不发糊（启动时调一次，可选）

private:
    bool create(int w, int h, WinMode mode);
    // 关闭旧窗口 → 按新尺寸重建 → 重开双缓冲；notify=true 时再调 onResize
    bool recreate(int w, int h, WinMode mode, bool notify);

    HWND   hWnd_ = nullptr;
    int    w_ = 0, h_ = 0;
    WinMode mode_ = WinMode::Windowed;
    int    preW_ = -1, preH_ = -1;   // 上一帧的尺寸（防抖）
    int    lastWinW_ = 0, lastWinH_ = 0;   // 还原时用
    bool   batching_ = false;
    std::string title_;
    std::function<void(int, int)> onResize_;

    int  targetFps_ = 60;
    bool hasFrameTime_ = false;                 // 第一帧不睡
    std::chrono::steady_clock::time_point lastFrameTime_{};
    bool timerActive_ = false;                  // timeBeginPeriod(1) 是否已调用
};

} // namespace ezx

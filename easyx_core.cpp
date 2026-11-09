// ============================================================================
//  easyx_core.cpp  ——  easyx_core.h 的实现
//  建议对照 使用说明.md 看：那里讲了每个模块"为什么这么写"。
// ============================================================================
#include "easyx_core.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <thread>

#include <imm.h>      // ImmGetContext / ImmSetCompositionWindow（中文输入法定位）

namespace ezx {

// ===========================================================================
//  内部工具
// ===========================================================================
namespace detail {

// EasyX 画字要的是 TCHAR 字符串：
//   多字节工程 → char（GBK）；Unicode 工程 → wchar_t。
// 我们统一用 std::string(UTF-8) 传参，这里负责转过去。
#ifdef _UNICODE
using tstring = std::wstring;
#else
using tstring = std::string;
#endif

inline bool isAscii(const std::string& s)
{
    for (unsigned char c : s)
        if (c > 0x7F) return false;
    return true;
}

inline bool decodeUtf8(const std::string& s, std::wstring& w)
{
    w.clear();
    if (s.empty()) return true;
    const int n = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(),
                                        (int)s.size(), nullptr, 0);
    if (n <= 0) return false;
    w.resize((size_t)n);
    ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), (int)s.size(), &w[0], n);
    return true;
}

inline std::wstring decodeAnsi(const std::string& s)
{
    if (s.empty()) return std::wstring();
    const int n = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

inline std::string encodeAnsi(const std::wstring& w)
{
    if (w.empty()) return std::string();
    const int n = ::WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string a((size_t)n, '\0');
    ::WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &a[0], n, nullptr, nullptr);
    return a;
}

inline std::wstring decodeAny(const std::string& s)
{
    std::wstring w;
    if (decodeUtf8(s, w)) return w;      // 合法 UTF-8
    return decodeAnsi(s);                // 否则当成 GBK 字面量
}

inline tstring toT(const std::string& s)
{
#ifdef _UNICODE
    return decodeAny(s);
#else
    if (s.empty() || isAscii(s)) return s;
    if (::GetACP() == CP_UTF8) return s;
    return encodeAnsi(decodeAny(s));
#endif
}

// UTF-16 → UTF-8（中文输入、emoji 都走这里）。代理对由系统 API 处理。
inline std::string utf16ToUtf8(const std::wstring& w)
{
    if (w.empty()) return std::string();
    const int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                        nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string s((size_t)n, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

// 这个位置上的 UTF-8 字符占几个字节（非法/截断时按 1 字节处理）
inline int utf8LenAt(const std::string& s, int i)
{
    if (i < 0 || i >= (int)s.size()) return 0;
    const unsigned char c = (unsigned char)s[i];
    int n = 1;
    if ((c & 0xE0) == 0xC0) n = 2;
    else if ((c & 0xF0) == 0xE0) n = 3;
    else if ((c & 0xF8) == 0xF0) n = 4;
    if (i + n > (int)s.size()) n = 1;
    return n;
}

// 前一个字符的起始字节（i 是当前字符的起始）
inline int utf8Prev(const std::string& s, int i)
{
    if (i <= 0) return 0;
    int p = i - 1;
    while (p > 0 && ((unsigned char)s[p] & 0xC0) == 0x80) --p;   // 跳过 10xxxxxx
    return p;
}

inline int utf8Next(const std::string& s, int i)
{
    if (i >= (int)s.size()) return (int)s.size();
    int j = i + utf8LenAt(s, i);
    return (j > (int)s.size()) ? (int)s.size() : j;
}

inline int utf8Count(const std::string& s)
{
    int n = 0;
    for (int i = 0; i < (int)s.size(); i = utf8Next(s, i)) ++n;
    return n;
}

// 只保留前 maxChars 个字符（maxChars <= 0 = 不限）
inline std::string utf8Truncate(const std::string& s, int maxChars)
{
    if (maxChars <= 0) return s;
    int n = 0, i = 0;
    while (i < (int)s.size() && n < maxChars)
    {
        i = utf8Next(s, i);
        ++n;
    }
    return s.substr(0, (size_t)i);
}

} // namespace detail

// ===========================================================================
//  文字
// ===========================================================================
static std::string& fontRef()
{
    static std::string f = "宋体";
    return f;
}

void setDefaultFont(const std::string& name) { fontRef() = name; }
const std::string& defaultFont() { return fontRef(); }

void useFont(int fontSize, const std::string& font)
{
    if (fontSize < 1) fontSize = 1;
    const std::string& face = font.empty() ? fontRef() : font;
    detail::tstring name = detail::toT(face);
    ::settextstyle(fontSize, 0, name.c_str());   // 高、宽(0=按比例)、字体名
    ::setbkmode(TRANSPARENT);                    // 字背景透明
}

int textWidth(const std::string& s, int fontSize, const std::string& font)
{
    useFont(fontSize, font);
    detail::tstring t = detail::toT(s);
    return ::textwidth(t.c_str());
}

int textHeight(const std::string& s, int fontSize, const std::string& font)
{
    useFont(fontSize, font);
    detail::tstring t = detail::toT(s.empty() ? std::string("0") : s);
    return ::textheight(t.c_str());
}

void drawText(int x, int y, const std::string& s, int fontSize, COLORREF color,
              const std::string& font)
{
    if (s.empty()) return;
    useFont(fontSize, font);
    ::settextcolor(color);
    detail::tstring t = detail::toT(s);
    ::outtextxy(x, y, t.c_str());
}

// exe 所在目录（UTF-8，带结尾斜杠）
std::string appDir()
{
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::string();

    std::wstring w(buf, n);
    const size_t pos = w.find_last_of(L"\\/");
    if (pos != std::wstring::npos) w.resize(pos + 1);
    return detail::utf16ToUtf8(w);
}

// 把相对路径拼到 exe 目录后面：assetPath("res/bg.png")
std::string assetPath(const std::string& relative)
{
    return appDir() + relative;
}

// ===========================================================================
//  区域切分
// ===========================================================================
std::vector<Rect> splitRow(const Rect& area, int count, int gap)
{
    std::vector<Rect> out;
    if (count <= 0) return out;
    const int each = (area.w - gap * (count - 1)) / count;
    int x = area.x;
    for (int i = 0; i < count; ++i)
    {
        const int w = (i == count - 1) ? (area.right() - x) : each;   // 最后一个吃掉除不尽的余数
        out.push_back(Rect{ x, area.y, w, area.h });
        x += w + gap;
    }
    return out;
}

std::vector<Rect> splitCol(const Rect& area, int count, int gap)
{
    std::vector<Rect> out;
    if (count <= 0) return out;
    const int each = (area.h - gap * (count - 1)) / count;
    int y = area.y;
    for (int i = 0; i < count; ++i)
    {
        const int h = (i == count - 1) ? (area.bottom() - y) : each;
        out.push_back(Rect{ area.x, y, area.w, h });
        y += h + gap;
    }
    return out;
}

// ===========================================================================
//  图片
// ===========================================================================
bool Image::load(const std::string& path, int w, int h)
{
    if (path.empty()) return false;

    const detail::tstring p = detail::toT(path);
    // 先看文件在不在：loadimage 失败会弹一个系统对话框
    if (::GetFileAttributes(p.c_str()) == INVALID_FILE_ATTRIBUTES) return false;

    auto img = std::make_shared<IMAGE>();
    ::loadimage(img.get(), p.c_str(), w, h, (w > 0 && h > 0));

    if (img->getwidth() <= 0 || img->getheight() <= 0) return false;

    img_ = img;
    w_ = img->getwidth();
    h_ = img->getheight();

    // 扫一遍像素，看有没有透明点（有的话后面要手动混合）
    alpha_ = false;
    if (const DWORD* buf = ::GetImageBuffer(img.get()))
    {
        const int total = w_ * h_;
        for (int i = 0; i < total; ++i)
            if ((buf[i] >> 24) != 0xFFu) { alpha_ = true; break; }
    }
    return true;
}

// 把图缩放到 (dx,dy,dw,dh) 画出来，并且只画 clip 里面的部分。
// EasyX 的 putimage 不认 alpha（透明处会变成黑色），所以带透明的图自己混合。
void Image::blit(int dx, int dy, int dw, int dh, const Rect& clip) const
{
    if (!img_ || dw <= 0 || dh <= 0) return;

    // 目标矩形和裁剪矩形求交集
    const int x0 = (dx > clip.x) ? dx : clip.x;
    const int y0 = (dy > clip.y) ? dy : clip.y;
    const int x1 = (dx + dw < clip.right()) ? (dx + dw) : clip.right();
    const int y1 = (dy + dh < clip.bottom()) ? (dy + dh) : clip.bottom();
    if (x1 <= x0 || y1 <= y0) return;

    if (!alpha_)     // 不透明图：用 EasyX 原生绘制（快）
    {
        if (x0 == dx && y0 == dy && x1 == dx + dw && y1 == dy + dh)
        {
            ::putimage(dx, dy, dw, dh, img_.get(), 0, 0);
            return;
        }
        HRGN rgn = ::CreateRectRgn(x0, y0, x1, y1);
        ::setcliprgn(rgn);
        ::putimage(dx, dy, dw, dh, img_.get(), 0, 0);
        ::clearcliprgn();
        ::DeleteObject(rgn);
        return;
    }

    DWORD* src = ::GetImageBuffer(img_.get());
    DWORD* dst = ::GetImageBuffer();
    if (!src || !dst)
    {
        ::putimage(dx, dy, dw, dh, img_.get(), 0, 0);
        return;
    }

    const int devW = ::getwidth();
    const int devH = ::getheight();
    const int sw = w_;
    const int sh = h_;

    for (int py = y0; py < y1; ++py)
    {
        if (py < 0 || py >= devH) continue;
        const int sy = (py - dy) * sh / dh;                 // 目标行 → 源图行（最近邻）
        const DWORD* srow = src + (size_t)sy * sw;
        DWORD* drow = dst + (size_t)py * devW;

        for (int px = x0; px < x1; ++px)
        {
            if (px < 0 || px >= devW) continue;
            const int sx = (px - dx) * sw / dw;

            const DWORD s = srow[sx];
            const unsigned a = s >> 24;                     // 透明度 0~255
            if (a == 0) continue;                           // 全透明：保留背景
            if (a == 255) { drow[px] = s; continue; }       // 不透明：直接覆盖

            const DWORD d = drow[px];
            const unsigned ia = 255 - a;
            const unsigned r = (((s >> 16) & 0xFF) * a + ((d >> 16) & 0xFF) * ia) / 255;
            const unsigned g = (((s >> 8) & 0xFF) * a + ((d >> 8) & 0xFF) * ia) / 255;
            const unsigned b = ((s & 0xFF) * a + (d & 0xFF) * ia) / 255;
            drow[px] = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

void Image::draw(int x, int y) const
{
    blit(x, y, w_, h_, Rect{ x, y, w_, h_ });
}

void Image::draw(const Rect& dst) const
{
    blit(dst.x, dst.y, dst.w, dst.h, dst);
}

void Image::drawFit(const Rect& dst) const
{
    if (!img_ || dst.empty()) return;
    const float sw = (float)dst.w / w_;
    const float sh = (float)dst.h / h_;
    const float s  = (sw < sh) ? sw : sh;    // 取小的 = 完整显示
    const int dw = (int)(w_ * s);
    const int dh = (int)(h_ * s);
    const int dx = dst.x + (dst.w - dw) / 2;
    const int dy = dst.y + (dst.h - dh) / 2;
    blit(dx, dy, dw, dh, dst);
}

void Image::drawCover(const Rect& dst) const
{
    if (!img_ || dst.empty()) return;
    const float sw = (float)dst.w / w_;
    const float sh = (float)dst.h / h_;
    const float s  = (sw > sh) ? sw : sh;    // 取大的 = 铺满
    const int dw = (int)(w_ * s);
    const int dh = (int)(h_ * s);
    const int dx = dst.x + (dst.w - dw) / 2;
    const int dy = dst.y + (dst.h - dh) / 2;
    blit(dx, dy, dw, dh, dst);      // 多出来的部分由 dst 裁掉
}

// ===========================================================================
//  输入
// ===========================================================================
void Input::update()
{
    // 每帧先清掉"一次性"的状态
    down_ = up_ = clicked_ = dblClick_ = false;
    wheel_ = 0;
    chars_.clear();
    keys_.clear();

    // 窗口可能被 Window::poll()/applyMode() 重建，所以每帧都重新取 HWND。
    // 不更新的话，setHandCursor / IME 会作用在已经销毁的旧窗口上。
    const HWND cur = ::GetHWnd();
    if (cur != hwnd_)
    {
        hwnd_ = cur;
        hand_ = false;                 // 新窗口的光标是默认箭头，允许重新设置
        imeHwnd_ = nullptr;
        imeApplied_ = Point{ -1, -1 };
    }
    applyImePosition();

    ExMessage msg;
    while (::peekmessage(&msg, EX_MOUSE | EX_KEY | EX_CHAR))
    {
        if (msg.message == WM_MOUSEMOVE || msg.message == WM_LBUTTONDOWN ||
            msg.message == WM_LBUTTONUP || msg.message == WM_MOUSEWHEEL)
        {
            pos_.x = msg.x;
            pos_.y = msg.y;
        }

        if (msg.message == WM_LBUTTONDOWN)
        {
            down_ = true;
            pressedLast_ = true;
            pressPos_ = pos_;
        }
        else if (msg.message == WM_LBUTTONUP)
        {
            up_ = true;
            pressedLast_ = false;

            // 按下和松开位置几乎一样 → 算一次"点击"
            const int dx = pos_.x - pressPos_.x;
            const int dy = pos_.y - pressPos_.y;
            if (dx * dx + dy * dy <= 36)      // 6 像素以内
            {
                clicked_ = true;
                const long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                if (now - lastClickTime_ < 400 &&
                    std::abs(pos_.x - lastClickPos_.x) < 8 &&
                    std::abs(pos_.y - lastClickPos_.y) < 8)
                {
                    dblClick_ = true;
                    lastClickTime_ = 0;       // 三连击不算两次双击
                }
                else
                {
                    lastClickTime_ = now;
                }
                lastClickPos_ = pos_;
            }
        }
        else if (msg.message == WM_MOUSEWHEEL)
        {
            wheel_ += msg.wheel;
        }
        else if (msg.message == WM_KEYDOWN)
        {
            keys_.push_back(msg.vkcode);
        }
        else if (msg.message == WM_CHAR)
        {
#ifdef _UNICODE
            appendChar((wchar_t)msg.ch);       // Unicode 工程：中文直接就是宽字符
#else
            appendAnsiByte((char)msg.ch);      // 多字节工程：可能是 GBK 的前导/后继字节
#endif
        }
    }

    held_ = pressedLast_;

    const int dx = pos_.x - pressPos_.x;
    const int dy = pos_.y - pressPos_.y;
    dragging_ = held_ && (dx * dx + dy * dy > 16);   // 按住移动超过 4 像素
}

// 把这一帧收到的 UTF-16 字符转成 UTF-8。
std::string Input::charsUtf8() const
{
    return detail::utf16ToUtf8(chars_);
}

// 追加一个 UTF-16 码元。代理对（emoji 之类）要两个码元拼起来才是一个字符。
void Input::appendChar(wchar_t ch)
{
    if (ch >= 0xD800 && ch <= 0xDBFF)           // 高位代理：先存着，等低位
    {
        if (pendingHigh_) chars_.push_back(pendingHigh_);
        pendingHigh_ = ch;
        return;
    }
    if (ch >= 0xDC00 && ch <= 0xDFFF)           // 低位代理
    {
        if (pendingHigh_)
        {
            chars_.push_back(pendingHigh_);
            chars_.push_back(ch);
            pendingHigh_ = 0;
        }
        else
        {
            chars_.push_back(ch);               // 孤立的低位：转换时会变成替换字符
        }
        return;
    }

    if (pendingHigh_)                           // 高位后面跟了普通字符：把高位吐出来
    {
        chars_.push_back(pendingHigh_);
        pendingHigh_ = 0;
    }
    chars_.push_back(ch);
}

#ifndef _UNICODE
// 多字节工程：一个中文在 WM_CHAR 里会分成"前导字节 + 后继字节"两条消息。
void Input::appendAnsiByte(char c)
{
    const unsigned char b = (unsigned char)c;

    if (ansiLead_)                              // 上一条是前导字节：拼成双字节再转
    {
        const char two[2] = { (char)ansiLead_, (char)b };
        const std::wstring w = detail::decodeAnsi(std::string(two, 2));
        for (wchar_t ch : w) appendChar(ch);
        ansiLead_ = 0;
        return;
    }

    if (::IsDBCSLeadByteEx(CP_ACP, b))          // 这是前导字节
    {
        ansiLead_ = b;
        return;
    }

    appendChar((wchar_t)b);                     // 普通 ASCII（含退格 8、回车 13）
}
#endif

void Input::setImePosition(int clientX, int clientY) const
{
    imePos_ = Point{ clientX, clientY };
    hasImePos_ = true;
}

void Input::clearImePosition() const
{
    hasImePos_ = false;
    imeApplied_ = Point{ -1, -1 };
}

// 把输入法候选框挪到 imePos_。位置没变就不重复设置。
void Input::applyImePosition() const
{
    if (!hasImePos_ || !hwnd_) return;

    if (hwnd_ == imeHwnd_ && imePos_.x == imeApplied_.x && imePos_.y == imeApplied_.y)
        return;

    const HIMC himc = ::ImmGetContext(hwnd_);
    if (!himc) return;                          // 这个窗口没有输入法上下文

    COMPOSITIONFORM cf = {};
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = imePos_.x;
    cf.ptCurrentPos.y = imePos_.y;
    ::ImmSetCompositionWindow(himc, &cf);
    ::ImmReleaseContext(hwnd_, himc);

    imeHwnd_ = hwnd_;
    imeApplied_ = imePos_;
}

bool Input::keyDown(int vk) const
{
    return std::find(keys_.begin(), keys_.end(), vk) != keys_.end();
}

#ifndef IDC_HAND
#define IDC_HAND MAKEINTRESOURCE(32649)   // 老版本头文件里没有这个光标常量
#endif

void Input::setHandCursor(bool on)
{
    if (on == hand_) return;
    hand_ = on;

    HCURSOR c = ::LoadCursor(nullptr, on ? IDC_HAND : IDC_ARROW);
    ::SetCursor(c);
    if (hwnd_) ::SetClassLongPtr(hwnd_, GCLP_HCURSOR, (LONG_PTR)c);
}

// ===========================================================================
//  Viewport
// ===========================================================================
void Viewport::setDesign(int w, int h)
{
    if (w > 0) designW_ = w;
    if (h > 0) designH_ = h;
    fit(winW_ > 0 ? winW_ : designW_, winH_ > 0 ? winH_ : designH_);
}

void Viewport::setScaleMode(ScaleMode mode)
{
    mode_ = mode;
    fit(winW_ > 0 ? winW_ : designW_, winH_ > 0 ? winH_ : designH_);
}

void Viewport::fit(int winW, int winH)
{
    winW_ = winW;
    winH_ = winH;
    if (winW <= 0 || winH <= 0) return;

    const float sx = (float)winW / (float)designW_;
    const float sy = (float)winH / (float)designH_;

    // Contain：取小的 → 完整显示，多出来的留黑边
    // Cover  ：取大的 → 铺满窗口，超出边缘的部分裁掉
    if (mode_ == ScaleMode::Cover)
        scale_ = (sx > sy) ? sx : sy;
    else
        scale_ = (sx < sy) ? sx : sy;

    const int w = (int)(designW_ * scale_ + 0.5f);
    const int h = (int)(designH_ * scale_ + 0.5f);
    ox_ = (winW - w) / 2;
    oy_ = (winH - h) / 2;
}

Rect Viewport::area() const
{
    return Rect{ ox_, oy_, (int)(designW_ * scale_ + 0.5f), (int)(designH_ * scale_ + 0.5f) };
}

Point Viewport::toDesign(int screenX, int screenY) const
{
    if (scale_ <= 0.0f) return Point{ 0, 0 };
    return Point{ (int)((screenX - ox_) / scale_), (int)((screenY - oy_) / scale_) };
}

Rect Viewport::R(const Rect& r) const
{
    return Rect{
        ox_ + (int)(r.x * scale_ + 0.5f),
        oy_ + (int)(r.y * scale_ + 0.5f),
        (int)(r.w * scale_ + 0.5f),
        (int)(r.h * scale_ + 0.5f)
    };
}

void Viewport::fill(const Rect& r, COLORREF color) const
{
    const Rect s = R(r);
    ::setfillcolor(color);
    ::solidrectangle(s.x, s.y, s.right() - 1, s.bottom() - 1);
}

void Viewport::frame(const Rect& r, COLORREF color, int thickness) const
{
    if (thickness < 1) thickness = 1;
    fill(Rect{ r.x, r.y, r.w, thickness }, color);                    // 上
    fill(Rect{ r.x, r.y + r.h - thickness, r.w, thickness }, color);  // 下
    fill(Rect{ r.x, r.y, thickness, r.h }, color);                    // 左
    fill(Rect{ r.x + r.w - thickness, r.y, thickness, r.h }, color);  // 右
}

void Viewport::text(int x, int y, const std::string& s, int fontSize, COLORREF color) const
{
    int fs = (int)(fontSize * scale_ + 0.5f);
    if (fs < 1) fs = 1;
    drawText(ox_ + (int)(x * scale_ + 0.5f), oy_ + (int)(y * scale_ + 0.5f), s, fs, color);
}

void Viewport::textIn(const Rect& r, const std::string& s, int fontSize, COLORREF color,
                      Align align) const
{
    if (s.empty()) return;
    const Rect sr = R(r);
    int fs = (int)(fontSize * scale_ + 0.5f);
    if (fs < 1) fs = 1;

    const int tw = textWidth(s, fs);
    const int th = textHeight(s, fs);

    int tx = sr.x;
    if (align == Align::Center)      tx = sr.x + (sr.w - tw) / 2;
    else if (align == Align::Right)  tx = sr.right() - tw;

    const int ty = sr.y + (sr.h - th) / 2;
    drawText(tx, ty, s, fs, color);
}

void Viewport::image(const Image& img, const Rect& dst) const
{
    img.draw(R(dst));
}

void Viewport::imageFit(const Image& img, const Rect& dst) const
{
    img.drawFit(R(dst));
}

void Viewport::imageCover(const Image& img, const Rect& dst) const
{
    img.drawCover(R(dst));
}

void Viewport::paintLetterbox(COLORREF color) const
{
    const Rect a = area();
    ::setfillcolor(color);
    if (a.y > 0)                       ::solidrectangle(0, 0, winW_ - 1, a.y - 1);
    if (a.bottom() < winH_)            ::solidrectangle(0, a.bottom(), winW_ - 1, winH_ - 1);
    if (a.x > 0)                       ::solidrectangle(0, a.y, a.x - 1, a.bottom() - 1);
    if (a.right() < winW_)             ::solidrectangle(a.right(), a.y, winW_ - 1, a.bottom() - 1);
}

// ===========================================================================
//  TextInput
// ===========================================================================
namespace {

// 把光标夹到 [0, size]，并且保证落在 UTF-8 字符的起始字节上。
int clampCursor(const std::string& s, int c)
{
    if (c <= 0) return 0;
    if (c >= (int)s.size()) return (int)s.size();
    while (c > 0 && ((unsigned char)s[c] & 0xC0) == 0x80) --c;   // 10xxxxxx = 续字节
    return c;
}

int inputFontSize(const Viewport& vp, int fontSize)
{
    int fs = (int)(fontSize * vp.scale() + 0.5f);
    return fs < 1 ? 1 : fs;
}

int inputPad(const Viewport& vp)
{
    return (int)(8 * vp.scale() + 0.5f);     // 输入框左边距（屏幕像素）
}

} // namespace

void TextInput::clear()
{
    text.clear();
    cursor = 0;
    submitted_ = canceled_ = false;
}

void TextInput::setText(const std::string& s)
{
    text = detail::utf8Truncate(s, maxLen);     // 按"字符"截断，不是按字节
    cursor = (int)text.size();
}

int TextInput::length() const
{
    return detail::utf8Count(text);
}

int TextInput::number(int fallback) const
{
    if (text.empty()) return fallback;
    int v = 0;
    bool neg = false;
    size_t i = 0;
    if (text[0] == '-' || text[0] == '+') { neg = (text[0] == '-'); i = 1; }
    if (i >= text.size()) return fallback;
    for (; i < text.size(); ++i)
    {
        if (text[i] < '0' || text[i] > '9') return fallback;
        v = v * 10 + (text[i] - '0');
        if (v > 100000000) return fallback;
    }
    return neg ? -v : v;
}

void TextInput::clampNumber(int lo, int hi)
{
    if (text.empty()) return;
    int v = number(0x7FFFFFFF);
    if (v == 0x7FFFFFFF) return;            // 内容不是数字，不动
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    setText(std::to_string(v));
}

void TextInput::update(const Input& in)
{
    submitted_ = canceled_ = false;
    if (!focused) return;

    cursor = clampCursor(text, cursor);

    // ---- 1) 这一帧输入的字符（UTF-8，中文一个字符可能 3 字节）----
    const std::string typed = in.charsUtf8();
    int count = detail::utf8Count(text);          // 当前字符数（不是字节数）

    for (int i = 0; i < (int)typed.size(); )
    {
        const unsigned char c = (unsigned char)typed[i];
        const int len = detail::utf8LenAt(typed, i);

        if (c == 8)                               // 退格：删掉光标前一个"字符"
        {
            if (cursor > 0)
            {
                const int start = detail::utf8Prev(text, cursor);
                text.erase((size_t)start, (size_t)(cursor - start));
                cursor = start;
                --count;
            }
            i += len;
            continue;
        }

        if (c < 32) { i += len; continue; }       // 回车/制表等控制字符交给 keys()

        if (digitsOnly && !(len == 1 && c >= '0' && c <= '9'))
        {
            i += len;
            continue;                             // 中文/字母/符号都被过滤
        }

        if (maxLen > 0 && count >= maxLen) { i += len; continue; }

        // 只插入完整的 UTF-8 字符（len 可能是 1~4）
        text.insert((size_t)cursor, typed, (size_t)i, (size_t)len);
        cursor += len;
        ++count;
        i += len;
    }

    // ---- 2) 控制键 ----
    for (int k : in.keys())
    {
        switch (k)
        {
        case VK_LEFT:   cursor = detail::utf8Prev(text, cursor); break;
        case VK_RIGHT:  cursor = detail::utf8Next(text, cursor); break;
        case VK_HOME:   cursor = 0; break;
        case VK_END:    cursor = (int)text.size(); break;
        case VK_DELETE:
            if (cursor < (int)text.size())
            {
                const int next = detail::utf8Next(text, cursor);
                text.erase((size_t)cursor, (size_t)(next - cursor));
            }
            break;
        case VK_RETURN: submitted_ = true; break;
        case VK_ESCAPE: canceled_ = true; break;
        default: break;
        }
    }
}

void TextInput::consume()
{
    submitted_ = canceled_ = false;
}

void TextInput::placeCursor(const Viewport& vp, const Rect& box, int fontSize, int mouseX)
{
    const int  fs = inputFontSize(vp, fontSize);
    const Rect sr = vp.R(box);
    const int  tx = sr.x + inputPad(vp);

    int best = 0;
    int bestDist = 1 << 30;
    for (int i = 0; ; )
    {
        const int px = tx + textWidth(text.substr(0, (size_t)i), fs);
        const int d = std::abs(mouseX - px);
        if (d < bestDist) { bestDist = d; best = i; }

        if (i >= (int)text.size()) break;
        i = detail::utf8Next(text, i);
    }
    cursor = best;                              // best 一定是字符边界
}

void TextInput::draw(const Viewport& vp, const Rect& box, int fontSize, COLORREF color,
                     bool caretVisible) const
{
    const int cur = clampCursor(text, cursor);

    // 密码模式：一个"字符"画一个 *（不是一字节一个）
    const std::string shown = password
        ? std::string((size_t)detail::utf8Count(text), '*')
        : text;

    // 密码模式下光标位置要换算成 shown 里的字节下标（* 是 ASCII，1 字节）
    const int shownCursor = password
        ? detail::utf8Count(text.substr(0, (size_t)cur))
        : cur;

    const int  fs = inputFontSize(vp, fontSize);
    const Rect sr = vp.R(box);
    if (sr.empty()) return;                     // 缩放后小到看不见，就别画

    const int  pad = inputPad(vp);
    const int  th = textHeight("0", fs);
    const int  ty = sr.y + (sr.h - th) / 2;
    const int  tx = sr.x + pad;

    if (!shown.empty()) drawText(tx, ty, shown, fs, color);

    // 光标：竖线，位置 = 左边距 + "光标前那段文字"的宽度
    if (focused && caretVisible)
    {
        const std::string before = shown.substr(0, (size_t)shownCursor);
        const int cx = tx + textWidth(before, fs);
        ::setcolor(color);
        ::line(cx, ty, cx, ty + th);
    }
}

// ===========================================================================
//  Pages
// ===========================================================================
Page& Pages::add(const std::string& name)
{
    Page p;
    p.name = name;
    pages_.push_back(std::move(p));
    if (current_ < 0) current_ = 0;
    return pages_.back();
}

void Pages::go(const std::string& name)
{
    for (int i = 0; i < (int)pages_.size(); ++i)
    {
        if (pages_[(size_t)i].name != name) continue;
        current_ = i;
        if (hasLayout_ && pages_[(size_t)i].layout) pages_[(size_t)i].layout(lastVp_);
        return;
    }
}

bool Pages::is(const std::string& name) const
{
    return current_ >= 0 && current_ < (int)pages_.size() &&
           pages_[(size_t)current_].name == name;
}

Page& Pages::current()
{
    static Page empty;
    if (current_ < 0 || current_ >= (int)pages_.size()) return empty;
    return pages_[(size_t)current_];
}

const std::string& Pages::currentName() const
{
    static const std::string kEmpty;
    if (current_ < 0 || current_ >= (int)pages_.size()) return kEmpty;
    return pages_[(size_t)current_].name;
}

void Pages::layoutAll(const Viewport& vp)
{
    lastVp_ = vp;
    hasLayout_ = true;
    for (auto& p : pages_)
        if (p.layout) p.layout(vp);
}

void Pages::update(const Input& in)
{
    if (current_ < 0 || current_ >= (int)pages_.size()) return;
    Page& p = pages_[(size_t)current_];
    if (p.update) p.update(in);
}

void Pages::draw(const Viewport& vp)
{
    if (current_ < 0 || current_ >= (int)pages_.size()) return;
    Page& p = pages_[(size_t)current_];
    if (p.draw) p.draw(vp);
}

// ===========================================================================
//  Window
// ===========================================================================
Window::~Window()
{
    close();
}

void Window::enableDpiAwareness()
{
    // 高分屏上不声明 DPI 感知会被系统整体拉伸 → 字发虚。
    // 动态取函数地址，避免依赖工程的 _WIN32_WINNT 宏。
    using Fn = BOOL(WINAPI*)();
    HMODULE user32 = ::GetModuleHandleA("user32.dll");
    if (!user32) user32 = ::LoadLibraryA("user32.dll");
    if (!user32) return;
    auto fn = (Fn)::GetProcAddress(user32, "SetProcessDPIAware");
    if (fn) fn();
}

bool Window::create(int w, int h, WinMode mode)
{
    int cw = w, ch = h;

    if (mode == WinMode::Fullscreen)
    {
        cw = ::GetSystemMetrics(SM_CXSCREEN);
        ch = ::GetSystemMetrics(SM_CYSCREEN);
    }
    else if (mode == WinMode::WorkArea)
    {
        RECT rc = {};
        ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        cw = rc.right - rc.left;
        ch = rc.bottom - rc.top;
    }
    if (cw <= 0 || ch <= 0) return false;

    hWnd_ = ::initgraph(cw, ch);
    if (!hWnd_) return false;

    LONG style = (LONG)::GetWindowLongPtr(hWnd_, GWL_STYLE);

    if (mode == WinMode::Fullscreen)
    {
        // 无边框 + 置顶：置顶才能真正盖住任务栏，否则任务栏会一直压在上面
        style &= ~(WS_CAPTION | WS_THICKFRAME);
        ::SetWindowLongPtr(hWnd_, GWL_STYLE, (LONG_PTR)style);
        ::SetWindowPos(hWnd_, HWND_TOPMOST, 0, 0, cw, ch,
                       SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    else
    {
        // EasyX 默认窗口缺"最大化"和"可拖边框"，补上
        style |= WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
        ::SetWindowLongPtr(hWnd_, GWL_STYLE, (LONG_PTR)style);
        ::SetWindowPos(hWnd_, nullptr, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        if (mode == WinMode::WorkArea)
        {
            RECT rc = {};
            ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
            ::SetWindowPos(hWnd_, nullptr, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }

    w_ = cw;
    h_ = ch;
    mode_ = mode;
    preW_ = preH_ = -1;
    if (!title_.empty()) setTitle(title_);   // 重建后标题要设回去
    return true;
}

// 销毁旧窗口 → 按新尺寸重建 → 重开双缓冲。
// applyMode() 和 poll() 原本各写了一遍这段顺序，抽出来避免改一处漏一处。
bool Window::recreate(int w, int h, WinMode mode, bool notify)
{
    if (!timerActive_) { ::timeBeginPeriod(1); timerActive_ = true; }

    if (hWnd_)
    {
        if (batching_) { ::EndBatchDraw(); batching_ = false; }
        ::closegraph();
        hWnd_ = nullptr;
    }

    if (!create(w, h, mode)) return false;

    ::flushmessage();          // 清掉旧窗口积压的消息，防止误点击
    ::BeginBatchDraw();        // closegraph 会销毁双缓冲，必须重开
    batching_ = true;
    hasFrameTime_ = false;     // 重建后重新起算帧间隔

    if (notify && onResize_) onResize_(w_, h_);
    return true;
}

bool Window::open(int w, int h, WinMode mode)
{
    if (hWnd_) close();

    // Sleep 默认精度约 15.6ms，动态限帧会不准；临时提到 1ms
    if (!timerActive_) { ::timeBeginPeriod(1); timerActive_ = true; }

    if (!create(w, h, mode)) return false;

    ::flushmessage();          // 丢掉建窗时积压的消息
    ::BeginBatchDraw();        // 双缓冲
    batching_ = true;
    lastWinW_ = w;
    lastWinH_ = h;
    hasFrameTime_ = false;     // 新窗口第一帧不睡
    return true;
}

void Window::close()
{
    if (timerActive_) { ::timeEndPeriod(1); timerActive_ = false; }

    if (!hWnd_) return;
    if (batching_) { ::EndBatchDraw(); batching_ = false; }
    ::closegraph();
    hWnd_ = nullptr;
    w_ = h_ = 0;
    hasFrameTime_ = false;
}

bool Window::applyMode(WinMode mode, int w, int h)
{
    const int nw = (w > 0) ? w : (lastWinW_ > 0 ? lastWinW_ : 1024);
    const int nh = (h > 0) ? h : (lastWinH_ > 0 ? lastWinH_ : 768);

    if (!recreate(nw, nh, mode, true)) return false;
    if (mode == WinMode::Windowed) { lastWinW_ = w_; lastWinH_ = h_; }
    return true;
}

void Window::minimize() { if (hWnd_) ::ShowWindow(hWnd_, SW_MINIMIZE); }
void Window::maximize() { if (hWnd_) ::ShowWindow(hWnd_, SW_MAXIMIZE); }
void Window::restore()  { if (hWnd_) ::ShowWindow(hWnd_, SW_RESTORE); }

bool Window::poll()
{
    if (!hWnd_ || mode_ != WinMode::Windowed) { preW_ = preH_ = -1; return false; }

    RECT rc = {};
    ::GetClientRect(hWnd_, &rc);
    const int cw = rc.right - rc.left;
    const int ch = rc.bottom - rc.top;
    if (cw <= 0 || ch <= 0) return false;                     // 最小化时是 0

    if (cw == w_ && ch == h_) { preW_ = preH_ = -1; return false; }

    // 拖着边框时尺寸每帧都在变：连续两帧一样才认为"松手了"
    if (cw != preW_ || ch != preH_) { preW_ = cw; preH_ = ch; return false; }

    const bool zoomed = (::IsZoomed(hWnd_) != FALSE);

    // 先不通知 onResize：最大化状态恢复后尺寸可能再微调 1~2 像素
    if (!recreate(cw, ch, WinMode::Windowed, false)) return false;

    if (zoomed)
    {
        ::ShowWindow(hWnd_, SW_MAXIMIZE);
        RECT r2 = {};
        ::GetClientRect(hWnd_, &r2);
        w_ = r2.right - r2.left;      // 系统可能微调 1~2 像素，按实际值记
        h_ = r2.bottom - r2.top;
    }

    preW_ = preH_ = -1;
    if (onResize_) onResize_(w_, h_);
    return true;
}

void Window::onResize(std::function<void(int, int)> cb)
{
    onResize_ = std::move(cb);
}

void Window::beginFrame(COLORREF bg)
{
    if (!hWnd_) return;
    ::setbkcolor(bg == CLR_INVALID ? RGB(0, 0, 0) : bg);
    ::cleardevice();
}

void Window::endFrame()
{
    if (!hWnd_) return;

    if (targetFps_ > 0)
    {
        const auto frameTime = std::chrono::microseconds(1000000 / targetFps_);
        const auto now = std::chrono::steady_clock::now();

        if (!hasFrameTime_)
        {
            // 第一帧只记录起点，不睡
            lastFrameTime_ = now;
            hasFrameTime_ = true;
        }
        else
        {
            const auto deadline = lastFrameTime_ + frameTime;
            if (now < deadline)
            {
                const auto remainMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();

                // 先睡掉大部分时间（留 1ms 余量），最后用 yield 精确等到点
                if (remainMs > 1) ::Sleep((DWORD)(remainMs - 1));
                while (std::chrono::steady_clock::now() < deadline)
                    std::this_thread::yield();
            }
            lastFrameTime_ = std::chrono::steady_clock::now();
        }
    }

    ::FlushBatchDraw();
}

void Window::endFrame(int delayMs)
{
    if (!hWnd_) return;
    ::FlushBatchDraw();
    if (delayMs > 0) ::Sleep((DWORD)delayMs);
    hasFrameTime_ = false;      // 下次用无参 endFrame() 时重新起算
}

void Window::setTargetFps(int fps)
{
    if (fps < 0) fps = 0;
    if (fps > 1000) fps = 1000;
    targetFps_ = fps;
    hasFrameTime_ = false;
}

void Window::setTitle(const std::string& text)
{
    title_ = text;
    if (!hWnd_) return;
    detail::tstring t = detail::toT(text);
    ::SetWindowText(hWnd_, t.c_str());
}

} // namespace ezx

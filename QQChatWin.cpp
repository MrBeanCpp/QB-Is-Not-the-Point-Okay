#include "QQChatWin.h"

QQChatWin::QQChatWin()
{
}

bool QQChatWin::isChatWin(HWND hwnd) //可能会误判 //图片查看器 翻译 屏幕识图 类与样式难以同Chat区分↓
{
    static WCHAR buffer[128];
    GetClassNameW(hwnd, buffer, _countof(buffer)); //sizeof字节数 会溢出
    QString className = QString::fromWCharArray(buffer);
    if (className == "TXGuiFoundation") { //类名
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (style == 0x960F0000) { //样式名
            if (Win::getProcessName(hwnd) == "QQ.exe") { //增加对进程名的判断，防止Tencent其他产品乱入（腾讯会议）
                return true;
            }
        }
    }
    return false;
}

void QQChatWin::setWindow(HWND hwnd)
{
    this->qqHwnd = hwnd;
}

HWND QQChatWin::winId()
{
    return qqHwnd;
}

QRect QQChatWin::rect()
{
    if (isNull()) return QRect(-1, -1, -1, -1); //失败不能返回QRect() 否则0会混淆
    RECT rect;
    GetWindowRect(qqHwnd, &rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

bool QQChatWin::isMini() //负坐标
{
    return qqHwnd && IsIconic(qqHwnd);
}

bool QQChatWin::isUnderMouse()
{
    return qqHwnd && rect().contains(QCursor::pos());
}

bool QQChatWin::isExist()
{
    return IsWindow(qqHwnd);
}

bool QQChatWin::isInSameThread(HWND hwnd)
{
    return Win::isInSameThread(qqHwnd, hwnd);
}

void QQChatWin::move(int x, int y, bool bRepaint)
{
    if (isNull()) return;
    QRect qqRect = rect();
    MoveWindow(qqHwnd, x, y, qqRect.width(), qqRect.height(), bRepaint);
}

void QQChatWin::move(int x, int y, int width, int height, bool bRepaint)
{
    if (isNull()) return;
    MoveWindow(qqHwnd, x, y, width, height, bRepaint);
}

bool QQChatWin::isNull()
{
    return qqHwnd == nullptr;
}

void QQChatWin::repaint()
{ //SendMessage会阻塞
    PostMessageA(qqHwnd, WM_PAINT, 0, 0); //重绘(否则消息不能更新) (UpdateWindow无效)
}

void QQChatWin::setAlwaysTop(bool bTop)
{
    Win::setAlwaysTop(qqHwnd, bTop);
}

bool QQChatWin::isTopMost()
{
    return Win::isTopMost(qqHwnd);
}

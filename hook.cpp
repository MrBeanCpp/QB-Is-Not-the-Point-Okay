#include "hook.h"
#include <QDebug>
#include <QWheelEvent>
HHOOK Hook::h_mouse = nullptr;
QWidget* Hook::receiver = nullptr;

void Hook::setMouseHook()
{
    if (h_mouse == nullptr) {
        h_mouse = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(NULL), 0);
        qDebug("Mouse-Hook 开始监听滚轮");
    }
}

void Hook::unHook()
{
    if (h_mouse != nullptr) {
        UnhookWindowsHookEx(h_mouse);
        h_mouse = nullptr;
        qDebug("Mouse-UnHook 结束监听滚轮");
    }
}

void Hook::setReceiver(QWidget* rec)
{
    receiver = rec;
}

bool Hook::isOn()
{
    return h_mouse != nullptr;
}

LRESULT Hook::mouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_MOUSEWHEEL && receiver) {
        MSLLHOOKSTRUCT* data = (MSLLHOOKSTRUCT*)lParam;
        short delta = (short)HIWORD(data->mouseData);
        QPoint pos(data->pt.x, data->pt.y);
        if (!receiver->geometry().contains(pos)) //自身窗体的事件不能重复发送
            qApp->postEvent(receiver, new QWheelEvent(pos, delta, Qt::NoButton, Qt::NoModifier));
        //return true;//阻断消息
    }
    return CallNextHookEx(h_mouse, nCode, wParam, lParam);
}

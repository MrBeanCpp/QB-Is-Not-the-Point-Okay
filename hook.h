#ifndef HOOK_H
#define HOOK_H
#include <QApplication>
#include <QWidget>
#include <windows.h>
class Hook
{
public:
    Hook() = delete;
    static void setMouseHook(void);
    static void unHook(void);
    static void setReceiver(QWidget* rec);
    static bool isOn(void);

private:
    static HHOOK h_mouse;
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    static QWidget* receiver;
};

#endif // HOOK_H

#ifndef HOOK_H
#define HOOK_H
#include <QApplication>
#include <QWidget>
#include <windows.h>
class Hook {
    using checkerFunc = std::function<bool(MSLLHOOKSTRUCT* data, bool* bBlock)>;

public:
    Hook() = delete;
    static void setMouseHook(void);
    static void unHook(void);
    static void setReceiver(QWidget* rec);
    static bool isOn(void);
    static void setChecker(checkerFunc func);

private:
    static HHOOK h_mouse;
    static LRESULT CALLBACK mouseProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    static QWidget* receiver;
    static checkerFunc checker;
};

#endif // HOOK_H

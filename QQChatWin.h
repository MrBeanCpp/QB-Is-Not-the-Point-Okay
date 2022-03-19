#ifndef QQCHATWIN_H
#define QQCHATWIN_H
#include "WinUtility.h"
#include <QCursor>
#include <QRect>
#include <windows.h>

class QQChatWin {
public:
    QQChatWin();
    static bool isChatWin(HWND hwnd);
    void setWindow(HWND hwnd);
    HWND winId(void);
    QRect rect(void);
    bool isMini(void);
    bool isUnderMouse(void);
    bool isExist(void);
    bool isInSameThread(HWND hwnd);
    void move(int x, int y, bool bRepaint = true);
    void move(int x, int y, int width, int height, bool bRepaint = true); //效率更高
    bool isNull(void);
    void repaint(void);

private:
    HWND qqHwnd = nullptr;
};

#endif // QQCHATWIN_H

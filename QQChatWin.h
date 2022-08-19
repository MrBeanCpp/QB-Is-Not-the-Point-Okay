#ifndef QQCHATWIN_H
#define QQCHATWIN_H
#include "WinUtility.h"
#include <QCursor>
#include <QRect>
#include <windows.h>
#include <QObject>

class QQChatWin: public QObject {
    Q_OBJECT
private:
    QQChatWin(QObject* parent = nullptr);
    QQChatWin(const QQChatWin&) = delete;
    QQChatWin& operator=(const QQChatWin&) = delete;
public:
    static QQChatWin& instance(void); //单例模式
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
    void setAlwaysTop(bool bTop = true);
    bool isTopMost(void);

private:
    HWND qqHwnd = nullptr;
};

#endif // QQCHATWIN_H

#ifndef WIDGET_H
#define WIDGET_H

#include "systemtray.h"
#include <QPropertyAnimation>
#include <QTime>
#include <QTimeLine>
#include <QTimer>
#include <QWidget>
#include <windows.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    enum STATE {
        QQ,
        ME,
        OTHER
    };

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    bool isForeQQChatWindow(HWND* qqHwnd = nullptr);
    bool isForeMyself(void);
    QRect getQQRect(void);
    QPoint getQQRightTop(void);
    void jumpToTop(void);
    void setAlwaysTop(HWND hwnd, bool isTop = true);
    void moveQQWindow(int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint = true);
    void moveToQQSide(void);
    bool isQQMini(void);
    HWND getHwnd(void);
    void moveIn(void);
    void moveOut(void);
    void moveToSide(void); //==moveOut
    bool isTimeLineRunning(void);
    bool isInSameThread(HWND hwnd_1, HWND hwnd_2);
    void getInputFocus(void);
    void miniAndShow(void); //最小化然后弹出以获取焦点
    void setInputFocus(HWND hwnd); //设置其他窗口焦点 前提是自己已获得焦点
    bool isQQHideState(void);
    bool isQQSideState(void);
    void stopTraceAnima(void);
    bool isCursorOnQQ(void);
    bool isCursorOnMe(void);
    void setBGColor(const QColor& color);

private:
    Ui::Widget* ui;

    HWND qqHwnd = nullptr;
    bool isQQTop = false;
    QTimeLine* timeLine = nullptr;
    QPropertyAnimation* anima_trace = nullptr;
    QTimer* timer_trace = nullptr;

    const int Extend = 4;

    QPair<QPoint, QTime> enterInfo;
    bool isAutoHide = true; //自动Hide && 左滑手势 moveToSide 可视为isActive 只剩下follow能力
    QColor defaultColor = QColor(190, 255, 198);
    QColor sleepColor = Qt::gray;

    // QWidget interface
protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

    // QWidget interface
protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

    // QWidget interface
protected:
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
};
#endif // WIDGET_H

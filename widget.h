#ifndef WIDGET_H
#define WIDGET_H

#include "QQChatWin.h"
#include "WinUtility.h"
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
    enum State {
        QQ,
        QQMini,
        QQsub,
        ME,
        OTHER,
        MISS
    };

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    //bool isForeQQChatWindow(HWND* qqHwnd = nullptr);
    bool isForeMyself(void);
    //QRect getQQRect(void);
    QPoint getQQRightTop(void);
    //void jumpToTop(void);
    //void setAlwaysTop(HWND hwnd, bool isTop = true);
    void moveQQWindow(int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint = true);
    void moveToQQSide(void);
    //bool isQQMini(void);
    HWND winID(void);
    void moveIn(void);
    void moveOut(void);
    void moveToSide(void); //==moveOut
    bool isTimeLineRunning(void);
    //bool isInSameThread(HWND hwnd_1, HWND hwnd_2);
    //void getInputFocus(void);
    //void miniAndShow(void); //最小化然后弹出以获取焦点
    //void setInputFocus(HWND hwnd); //设置其他窗口焦点 前提是自己已获得焦点
    bool isQQHideState(void);
    bool isQQSideState(void);
    bool isQQAllVisible(void);
    bool isQQInvisible(void); //in the left
    void stopTraceAnima(void);
    //bool isCursorOnQQ(void);
    //bool isCursorOnMe(void);
    void setBGColor(const QColor& color);
    QRect getAbsorbRect(void); //获取QQ RightTop 周围可吸附区域
    //QString getWindowText(HWND hwnd);
    void setAutoHide(bool bAuto);
    //bool isTopMost(HWND hwnd);
    //QString getProcessName(HWND hwnd);
    void setState(State _state);
    bool isState(State _state);

signals:
    void stateChanged(State curState, State preState);
    void qqChatWinChanged(HWND curHwnd, HWND preHwnd);

private:
    Ui::Widget* ui;

    QQChatWin qq;

    //HWND qqHwnd = nullptr;
    QTimeLine* timeLine = nullptr;
    QPropertyAnimation* anima_trace = nullptr;
    QTimer* timer_trace = nullptr;

    //constexpr在编译期确定 而数据成员在运行期初始化 矛盾 所以只能是static
    static constexpr int Extend = 5; //Hide后 露出部分
    static constexpr int MarginTop = 15;

    QPair<QPoint, QTime> enterInfo;
    bool isAutoHide = true; //自动Hide && 左滑手势 moveToSide 可视为isActive 只剩下follow能力
    const QColor defaultColor = QColor(253, 227, 200); //QColor(242, 203, 108); //QColor(190, 255, 198);
    const QColor sleepColor = QColor(52, 11, 11);
    const QColor dangerColor = QColor(234, 24, 33);

    //mouseEvent 拖拽Folloer 部分
    QPoint curPos; //cursor pos
    int mouseMoveLen = 0;
    QRect qqAbsorbRect; //可吸附区域
    QColor preColor;

    State state = MISS;

    // QWidget interface
protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

    // QWidget interface
protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

    // QWidget interface
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
};
#endif // WIDGET_H

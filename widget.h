#ifndef WIDGET_H
#define WIDGET_H

#include "QQChatWin.h"
#include "WinUtility.h"
#include "hook.h"
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

    using KeyList = QList<BYTE>;

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    bool isForeMyself(void);
    QPoint getQQStickPos(void);
    void moveQQWindow(int X, int Y = NaN, int nWidth = NaN, int nHeight = NaN, WINBOOL bRepaint = true);
    void moveToQQSide(void);
    HWND winID(void);
    void moveIn(void);
    void moveOut(void);
    void moveToSide(void); //==moveOut
    bool isTimeLineRunning(void);
    bool isQQHideState(void);
    bool isQQSideState(void);
    bool isQQAllVisible(void);
    bool isQQInvisible(void); //in the left
    void stopTraceAnima(void);
    void setBGColor(const QColor& color);
    QRect getQQStickRect(void); //获取QQ RightTop 周围可吸附区域
    void setAutoHide(bool bAuto);
    void setState(State _state);
    bool isState(State _state);
    bool inRange(int min, int val, int max);

signals:
    void stateChanged(State curState, State preState);
    void qqChatWinChanged(HWND curHwnd, HWND preHwnd);

private:
    Ui::Widget* ui;

    QQChatWin qq;

    QTimeLine* timeLine = nullptr;
    QPropertyAnimation* anima_trace = nullptr;
    QTimer* timer_trace = nullptr;
    SystemTray* sysTray = nullptr;

    //constexpr在编译期确定 而数据成员在运行期初始化 矛盾 所以只能是static
    static constexpr int Extend = 5; //Hide后 露出部分
    static constexpr int NaN = INT_MIN;
    int MarginTop = 15;

    const QPair<int, int> StickX { -20, 50 }; //QQ窗口吸附范围(x - 屏幕边缘)

    QPair<QPoint, QTime> enterInfo;
    bool isAutoHide = true; //自动Hide && 左滑手势 moveToSide 可视为isActive 只剩下follow能力
    const QColor defaultColor = QColor(253, 227, 200); //QColor(242, 203, 108); //QColor(190, 255, 198);
    const QColor sleepColor = QColor(52, 11, 11);
    const QColor dangerColor = QColor(234, 24, 33);

    //mouseEvent 拖拽Folloer 部分
    QPoint curPos; //cursor pos
    QRect qqStickRect; //可吸附区域
    QColor preColor;
    bool isStick = true;

    State state = MISS;
    HWND lastOtherWin = nullptr;

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
    void wheelEvent(QWheelEvent* event) override;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
};
#endif // WIDGET_H

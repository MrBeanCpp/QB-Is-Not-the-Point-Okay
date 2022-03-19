#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QtWin>
#include <cmath>
#define GetKey(X) (GetAsyncKeyState(X) & 0x8000)

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    setWindowTitle("QB Is Not the Point Okay?");

    setWindowFlag(Qt::FramelessWindowHint);
    setWindowFlag(Qt::WindowStaysOnTopHint);

    setFocusPolicy(Qt::StrongFocus);
    //setMouseTracking(true);
    setWindowOpacity(0.9); //做一下方块动画------------------------
    //showMinimized(); //
    QtWin::taskbarDeleteTab(this); //删除任务栏图标 //showMinimized()之后delete 否则size有些不正常 或者在窗体构造之后mini

    setAutoFillBackground(true); //to使用QPalette
    setBGColor(defaultColor);

    SystemTray* sysTray = new SystemTray(this);
    sysTray->show();

    timeLine = new QTimeLine(500, this);
    timeLine->setUpdateInterval(10);
    connect(timeLine, &QTimeLine::frameChanged, [=](int frame) {
        QRect qqRect = qq.rect();
        moveQQWindow(frame, qqRect.y(), qqRect.width(), qqRect.height(), true); //不repaint可能不刷新？(其实 repaint也不刷新 还得手动sendMessage)
    });
    connect(timeLine, &QTimeLine::finished, [=]() { stateChanged(state, state); }); //为了防止isTimeLineRunning时，某些操作无法执行，结束后再发送一遍

    anima_trace = new QPropertyAnimation(this, "pos");
    anima_trace->setDuration(100);

    timer_trace = new QTimer(this);
    timer_trace->callOnTimeout([=]() {
        if (!isState(QQ) || isTimeLineRunning()) { //move动画时 不要trace 否则鬼畜
            anima_trace->stop();
            moveToQQSide();
            return;
        }
        //moveToQQSide();
        anima_trace->stop();
        if ((getQQRightTop() - pos()).manhattanLength() > 2) { //距离过小 animate无法靠近
            anima_trace->setStartValue(this->pos());
            anima_trace->setEndValue(getQQRightTop());
            anima_trace->start();
        } else
            moveToQQSide();
    });

    QTimer* timer_find = new QTimer(this);
    timer_find->callOnTimeout([=]() {
        HWND foreWin = GetForegroundWindow();
        QString title = Win::getWindowText(foreWin);
        static const QStringList BlackList = { "图片查看", "屏幕识图", "翻译" }; //类与样式难以同Chat区分↓

        if (QQChatWin::isChatWin(foreWin) && !BlackList.contains(title)) { //规避图片查看器 难以区分 （如果好友叫"图片查看"就寄了）
            qDebug() << "Find QQ" << title;

            if (qq.winId() != foreWin) //new Found
                emit qqChatWinChanged(foreWin, qq.winId());
            setState(QQ);

            if (!GetKey(VK_LBUTTON) && isAutoHide) { //松开鼠标（避免在拖动窗口）
                QRect qqRect = qq.rect();
                if (qqRect.x() > 0 && qqRect.x() <= 50) { //吸附效果
                    moveQQWindow(0, qqRect.y(), qqRect.width(), qqRect.height());
                } else if (!isTimeLineRunning() && qqRect.x() < 0 && qqRect.right() > 0) { //x not in [吸附范围] && x < 0 自动 move in
                    Win::getInputFocus(winID()); //转移焦点 否则 isQQHideState()会moveOut();
                    moveIn();
                }
            }
        } else {
            stopTraceAnima();

            if (qq.isExist()) {
                if (qq.isMini()) { //点击任务栏图标moveOut 再次点击moveIn 但实际上会最小化(OS) 所以只能手动show
                    setState(QQMini);
                } else {
                    if (isForeMyself()) {
                        qDebug() << "this";
                        setState(ME);
                    } else if (qq.isInSameThread(foreWin)) { //排除相同线程窗口情况(表情窗口)
                        qDebug() << "QQ subWin";
                        setState(QQsub);
                        if (BlackList.contains(title) && !Win::isTopMost(foreWin)) //防止重复置顶 导致右键菜单无法弹出
                            Win::setAlwaysTop(foreWin); //需要置顶 否则被遮挡
                    } else {
                        qDebug() << "other";
                        setState(OTHER);
                        if (isAutoHide && !isTimeLineRunning() && isQQSideState() && !qq.isUnderMouse() && !underMouse()) { //需要持续监测鼠标 所以不能放在stateChanged
                            //转移焦点 否则 当其他窗口关闭时 操作系统会默认将焦点转移至QQ导致isQQHideState()会moveOut();
                            /*
                            //getInputFocus(); //该函数会导致实际抢夺其他窗口焦点
                            //SwitchToThisWindow(getHwnd(), true); //利用该函数的bug达成目的：实际不抢夺焦点 //遇到系统窗口还是会失败
                            //setFocus();*/
                            BringWindowToTop(winID()); //根据ShowWindow()中SW_MINIMIZE的解释：最小化指定窗口并激活 Z-Order 中的下一个顶级窗口
                                //所以只要提升到同类Z序顶端即可（注：之前误以为是获取焦点的最后一个窗口 是因为取得焦点相当于bringToTop in 同级别）
                            moveIn();
                        }
                    }
                }
            } else {
                setState(MISS);
            }
        }
    });
    timer_find->start(200);

    connect(this, &Widget::stateChanged, [=](State curState, State preState) {
        Q_UNUSED(preState)

        switch (curState) {
        case QQ:
            if (isMinimized()) {
                showNormal();
                SwitchToThisWindow(qq.winId(), true); //转移焦点 to QQ
            }
            if (!timer_trace->isActive() && !isTimeLineRunning()) {
                timer_trace->start(40);
                qDebug() << "start Timer";
            }
            //不能用!isQQAllVisible否则导致 [鼠标拖拽入屏幕边缘 自动moveIn] 无法执行
            if (!isTimeLineRunning() && isQQInvisible()) { //点击任务栏窗口 || Alt+Tab激活 且qq处于 侧边栏隐藏状态× 非完全可见状态√ (打开群消息 width会增加 right>>)
                moveOut();
                qq.repaint(); //重绘(否则消息不能更新)
            }
            break;
        case QQsub:

            break;
        case QQMini:
            ShowWindowAsync(qq.winId(), SW_SHOWNOACTIVATE); //会闪烁 但没办法了 //异步防止QQ无响应
            Win::miniAndShow(winID()); //转移焦点 否则 isQQHideState()会moveOut(); 按住任务栏->other 松开->miniQQ 然后焦点转移 QQshow 焦点又回来了...
            moveIn();
            break;
        case ME:

            break;
        case OTHER:

            break;
        case MISS:
            showMinimized();
            break;
        default:
            break;
        }
    });

    connect(this, &Widget::qqChatWinChanged, [=](HWND curHwnd, HWND preHWND) {
        Win::setAlwaysTop(preHWND, false); //取消前一个置顶
        Win::setAlwaysTop(curHwnd);
        qq.setWindow(curHwnd);
        //qqHwnd = curHwnd;
    });
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isForeMyself()
{
    return GetForegroundWindow() == winID();
}

QPoint Widget::getQQRightTop()
{
    QRect qqRect = qq.rect();
    return QPoint(qqRect.right(), qqRect.top() + MarginTop);
}

void Widget::moveQQWindow(int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint)
{
    if (qq.isNull()) return;

    qq.move(X, Y, nWidth, nHeight, bRepaint);
    move(X + nWidth - 1, Y + MarginTop); //-1是因为从0开始计数 修正与qqRect.right()的差异
}

void Widget::moveToQQSide()
{
    move(getQQRightTop());
}

HWND Widget::winID()
{
    return (HWND)this->winId();
}

void Widget::moveIn()
{
    stopTraceAnima();
    QRect qqRect = qq.rect();
    timeLine->stop();
    timeLine->setFrameRange(qqRect.x(), -(qqRect.width() + width() - Extend));
    if (timeLine->startFrame() == timeLine->endFrame()) return;
    timeLine->start();
}

void Widget::moveOut()
{
    stopTraceAnima();
    QRect qqRect = qq.rect();
    timeLine->stop();
    timeLine->setFrameRange(qqRect.x(), 0);
    if (timeLine->startFrame() == timeLine->endFrame()) return;
    timeLine->start();
}

void Widget::moveToSide()
{
    moveOut();
}

bool Widget::isTimeLineRunning()
{
    return timeLine->state() == QTimeLine::Running;
}

bool Widget::isQQHideState()
{
    QRect qqRect = qq.rect();
    return qqRect.x() == -(qqRect.width() + width() - Extend);
}

bool Widget::isQQSideState()
{
    QRect qqRect = qq.rect();
    return qqRect.x() == 0;
}

bool Widget::isQQAllVisible()
{
    QRect qqRect = qq.rect();
    return qqRect.x() >= 0;
}

bool Widget::isQQInvisible() //与isQQHideState区分 不那么精确 只要在左边看不见即可
{
    QRect qqRect = qq.rect();
    return qqRect.right() <= 0;
}

void Widget::stopTraceAnima()
{
    if (timer_trace->isActive() || anima_trace->state() == QAbstractAnimation::Running) {
        timer_trace->stop();
        anima_trace->stop();
        moveToQQSide();
    }
}

void Widget::setBGColor(const QColor& color)
{
    QPalette palette(this->palette());
    palette.setColor(QPalette::Window, color);
    setPalette(palette);
}

QRect Widget::getAbsorbRect()
{
    QPoint RT = getQQRightTop();
    QRect Rect(RT, RT);
    return Rect.marginsAdded(QMargins(32, 30, 32, 50));
}

void Widget::setAutoHide(bool bAuto)
{
    if (isAutoHide == bAuto) return;
    this->isAutoHide = bAuto;
    setBGColor(isAutoHide ? defaultColor : sleepColor);
}

void Widget::setState(State _state)
{
    if (state != _state) {
        emit stateChanged(_state, state);
        this->state = _state;
    }
}

bool Widget::isState(State _state)
{
    return this->state == _state;
}

void Widget::enterEvent(QEvent* event)
{
    Q_UNUSED(event)
    if (isTimeLineRunning() == false) { //模拟click会误触其他位置
        //getInputFocus(); //获取焦点 否则还会缩回去//某些特殊窗口 如任务管理器在前台时 会阻止该函数
        //moveOut(); √
        ////SwitchToThisWindow(qqHwnd, false); //将自己的focus转移很easy 获取很困难//不能用于获取focus //该函数会闪烁不太好
        //setInputFocus(qqHwnd);
        //#No! 此种情况 鼠标触碰弹出 不应该获取焦点 只moveOut，鼠标移开后自动moveIn（不影响看视频等操作 Just View）

        if (isQQHideState()) //隐藏状态弹出
            moveOut();
        else //可能为左滑手势
            enterInfo = qMakePair(static_cast<QEnterEvent*>(event)->globalPos(), QTime::currentTime()); //记录入点信息//QCursor::pos()是目前状态 不是事件发生时的pos
    }
}

void Widget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)
    static constexpr int SlideTL = 50; //slide Time Limit(ms)
    if (isTimeLineRunning() == false && isAutoHide) {
        QPoint leavePos = QCursor::pos(); //没有QLeaveEvent（只有QEnterEvent） 无法获取当时pos 为啥没有呀？？
        QTime leaveTime = QTime::currentTime();
        //qDebug() << "slide:" << enterInfo.second.msecsTo(leaveTime) << leavePos.x() << enterInfo.first.x();
        if (leavePos.x() < enterInfo.first.x() && enterInfo.second.msecsTo(leaveTime) < SlideTL) { //左滑手势
            if (isQQSideState()) { //此时焦点在QQ or this上
                Win::getInputFocus(winID()); //转移焦点 否则 isQQHideState()会moveOut();
                moveIn();
            } else
                moveToSide();
        }
    }
}

bool Widget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = (MSG*)message;
    static const UINT WM_TASKBARCREATED = RegisterWindowMessageW(L"TaskbarCreated");
    if (msg->message == WM_TASKBARCREATED) { //获取任务栏重启信息，再次隐藏任务栏图标
        qDebug() << "TaskbarCreated";
        QtWin::taskbarDeleteTab(this);
        return true;
    }
    return false; //此处返回false，留给其他事件处理器处理
}

void Widget::mousePressEvent(QMouseEvent* event) //双击也会收到press
{
    Q_UNUSED(event)
    if (isTimeLineRunning()) return;

    curPos = event->screenPos().toPoint();
    mouseMoveLen = 0;
    qqAbsorbRect = getAbsorbRect();
    preColor = this->palette().color(QPalette::Window); //保存原色
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)

    if (qqAbsorbRect.contains(pos()))
        moveToQQSide();
    else //不在安全区域：quit
        qApp->quit();
}

void Widget::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (isTimeLineRunning()) return;

    setAutoHide(!isAutoHide);
}

void Widget::mouseMoveEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    if (isTimeLineRunning()) return;
    if (!(event->buttons() & Qt::LeftButton)) return; //左键按下

    static constexpr int MOVELIMIT = 250;
    QPoint mousePos = event->screenPos().toPoint();
    mouseMoveLen += (mousePos - curPos).manhattanLength();
    mouseMoveLen = qMin(mouseMoveLen, MOVELIMIT); //防止溢出

    if (mouseMoveLen >= MOVELIMIT) { //Hard to drag
        QPoint newPos = this->pos() + mousePos - curPos;
        curPos = mousePos;
        move(newPos);

        setBGColor(qqAbsorbRect.contains(pos()) ? preColor : dangerColor);
    } else
        QCursor::setPos(curPos); //坚韧不拔
}

void Widget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    static QPen pen(Qt::darkGray, 2); //有1px在外部
    painter.setPen(pen);
    painter.drawRect(this->rect());

    static constexpr int Margin_X = 8, Margin_Y = 15;
    static QPen penLine(QColor(80, 80, 80, 135), 4, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(penLine);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawLine(rect().topRight() + QPoint(-Margin_X, Margin_Y + 1), rect().bottomRight() + QPoint(-Margin_X, -Margin_Y));
}

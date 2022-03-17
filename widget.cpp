#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QtWin>
#include <cmath>
#include <psapi.h>
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
    showMinimized(); //
    QtWin::taskbarDeleteTab(this); //删除任务栏图标 //showMinimized()之后delete 否则size有些不正常

    setAutoFillBackground(true); //to使用QPalette
    setBGColor(defaultColor);

    SystemTray* sysTray = new SystemTray(this);
    sysTray->show();

    timeLine = new QTimeLine(500, this);
    timeLine->setUpdateInterval(10);
    connect(timeLine, &QTimeLine::frameChanged, [=](int frame) {
        QRect qqRect = getQQRect();
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
        HWND hwnd;
        HWND foreWindow = GetForegroundWindow();
        QString title = getWindowText(foreWindow);
        static const QStringList BlackList = { "图片查看", "屏幕识图", "翻译" }; //类与样式难以同Chat区分↓

        if (isForeQQChatWindow(&hwnd) && !BlackList.contains(title)) { //规避图片查看器 难以区分 （如果好友叫"图片查看"就寄了）
            qDebug() << "Find QQ" << title;

            if (qqHwnd != hwnd) //new Found
                emit qqChatWinChanged(hwnd, qqHwnd);
            setState(QQ);

            if (!GetKey(VK_LBUTTON) && isAutoHide) { //松开鼠标（避免在拖动窗口）
                QRect qqRect = getQQRect();
                if (qqRect.x() > 0 && qqRect.x() <= 50) { //吸附效果
                    moveQQWindow(0, qqRect.y(), qqRect.width(), qqRect.height());
                } else if (!isTimeLineRunning() && qqRect.x() < 0 && qqRect.right() > 0) { //x not in [吸附范围] && x < 0 自动 move in
                    getInputFocus(); //转移焦点 否则 isQQHideState()会moveOut();
                    moveIn();
                }
            }
        } else {
            stopTraceAnima();

            if (IsWindow(qqHwnd)) {
                if (isQQMini()) { //点击任务栏图标moveOut 再次点击moveIn 但实际上会最小化(OS) 所以只能手动show
                    setState(QQMini);
                } else {
                    if (isForeMyself()) {
                        qDebug() << "this";
                        setState(ME);
                    } else if (isInSameThread(qqHwnd, foreWindow)) { //排除相同线程窗口情况(表情窗口)
                        qDebug() << "QQ subWin";
                        setState(QQsub);
                        if (BlackList.contains(title) && !isTopMost(foreWindow)) //防止重复置顶 导致右键菜单无法弹出
                            setAlwaysTop(foreWindow); //需要置顶 否则被遮挡
                    } else {
                        qDebug() << "other";
                        setState(OTHER);
                        if (isAutoHide && !isTimeLineRunning() && isQQSideState() && !isCursorOnQQ() && !isCursorOnMe()) { //需要持续监测鼠标 所以不能放在stateChanged
                            //转移焦点 否则 当其他窗口关闭时 操作系统会默认将焦点转移至QQ导致isQQHideState()会moveOut();
                            /*
                            //getInputFocus(); //该函数会导致实际抢夺其他窗口焦点
                            //SwitchToThisWindow(getHwnd(), true); //利用该函数的bug达成目的：实际不抢夺焦点 //遇到系统窗口还是会失败
                            //setFocus();*/
                            BringWindowToTop(getHwnd()); //根据ShowWindow()中SW_MINIMIZE的解释：最小化指定窗口并激活 Z-Order 中的下一个顶级窗口
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
                SwitchToThisWindow(qqHwnd, true); //转移焦点 to QQ
            }
            if (!timer_trace->isActive() && !isTimeLineRunning()) {
                timer_trace->start(40);
                qDebug() << "start Timer";
            }
            //不能用!isQQAllVisible否则导致 [鼠标拖拽入屏幕边缘 自动moveIn] 无法执行
            if (!isTimeLineRunning() && isQQInvisible()) { //点击任务栏窗口 || Alt+Tab激活 且qq处于 侧边栏隐藏状态× 非完全可见状态√ (打开群消息 width会增加 right>>)
                moveOut();
                SendMessageA(qqHwnd, WM_PAINT, 0, 0); //重绘(否则消息不能更新) (UpdateWindow无效)
            }
            break;
        case QQsub:

            break;
        case QQMini:
            ShowWindow(qqHwnd, SW_SHOWNOACTIVATE); //会闪烁 但没办法了
            miniAndShow(); //转移焦点 否则 isQQHideState()会moveOut();
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
        setAlwaysTop(preHWND, false); //取消前一个置顶
        setAlwaysTop(curHwnd);
        qqHwnd = curHwnd;
    });
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isForeQQChatWindow(HWND* qqHwnd)
{
    static WCHAR buffer[128];
    HWND hwnd = GetForegroundWindow();
    GetClassNameW(hwnd, buffer, sizeof(buffer));
    QString className = QString::fromWCharArray(buffer);
    if (className == "TXGuiFoundation") { //类名
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (style == 0x960F0000) { //样式名
            if (getProcessName(hwnd) == "QQ.exe") { //增加对进程名的判断，防止Tencent其他产品乱入（腾讯会议）
                if (qqHwnd) *qqHwnd = hwnd;
                return true;
            }
        }
    }
    return false;
}

bool Widget::isForeMyself()
{
    return GetForegroundWindow() == getHwnd();
}

QRect Widget::getQQRect()
{
    if (qqHwnd == nullptr) return QRect(-1, -1, -1, -1); //失败不能返回QRect() 否则0会混淆
    RECT rect;
    GetWindowRect(qqHwnd, &rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

QPoint Widget::getQQRightTop()
{
    QRect qqRect = getQQRect();
    return QPoint(qqRect.right(), qqRect.top() + MarginTop);
}

void Widget::jumpToTop()
{
    SetWindowPos(getHwnd(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(getHwnd(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void Widget::setAlwaysTop(HWND hwnd, bool isTop)
{
    if (hwnd)
        SetWindowPos(hwnd, isTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW); //持续置顶
}

void Widget::moveQQWindow(int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint)
{
    if (qqHwnd == nullptr) return;

    MoveWindow(qqHwnd, X, Y, nWidth, nHeight, bRepaint);
    move(X + nWidth - 1, Y + MarginTop); //-1是因为从0开始计数 修正与qqRect.right()的差异
}

void Widget::moveToQQSide()
{
    move(getQQRightTop());
}

bool Widget::isQQMini() //负坐标
{
    return qqHwnd ? IsIconic(qqHwnd) : false;
}

HWND Widget::getHwnd()
{
    return (HWND)this->winId();
}

void Widget::moveIn()
{
    stopTraceAnima();
    QRect qqRect = getQQRect();
    timeLine->stop();
    timeLine->setFrameRange(qqRect.x(), -(qqRect.width() + width() - Extend));
    if (timeLine->startFrame() == timeLine->endFrame()) return;
    timeLine->start();
}

void Widget::moveOut()
{
    stopTraceAnima();
    QRect qqRect = getQQRect();
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

bool Widget::isInSameThread(HWND hwnd_1, HWND hwnd_2)
{
    return GetWindowThreadProcessId(hwnd_1, NULL) == GetWindowThreadProcessId(hwnd_2, NULL);
}

void Widget::getInputFocus()
{
    HWND foreHwnd = GetForegroundWindow();
    DWORD foreTID = GetWindowThreadProcessId(foreHwnd, NULL);
    DWORD threadId = GetCurrentThreadId();
    HWND hwnd = getHwnd();
    if (foreHwnd == hwnd) {
        qDebug() << "Already getFocus";
        return;
    }
    bool res = AttachThreadInput(threadId, foreTID, TRUE); //会导致短暂的Windows误认为this==QQ激活状态 导致点击任务栏图标 持续最小化（参见下方解决法案）
    qDebug() << "attach:" << res;
    if (res == false) { //如果遇到系统窗口而失败 只能最小化再激活获取焦点
        miniAndShow();
    } else {
        SetForegroundWindow(foreHwnd); //刷新QQ任务栏图标状态 防止保持焦点状态 不更新 导致点击后 最小化 而非获取焦点
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(threadId, foreTID, FALSE);
    }
}

void Widget::miniAndShow()
{
    showMinimized();
    showNormal();
    activateWindow();
}

void Widget::setInputFocus(HWND hwnd)
{
    DWORD threadId = GetCurrentThreadId();
    AttachThreadInput(GetWindowThreadProcessId(hwnd, NULL), threadId, TRUE);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    AttachThreadInput(GetWindowThreadProcessId(hwnd, NULL), threadId, FALSE);
}

bool Widget::isQQHideState()
{
    QRect qqRect = getQQRect();
    return qqRect.x() == -(qqRect.width() + width() - Extend);
}

bool Widget::isQQSideState()
{
    QRect qqRect = getQQRect();
    return qqRect.x() == 0;
}

bool Widget::isQQAllVisible()
{
    QRect qqRect = getQQRect();
    return qqRect.x() >= 0;
}

bool Widget::isQQInvisible() //与isQQHideState区分 不那么精确 只要在左边看不见即可
{
    QRect qqRect = getQQRect();
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

bool Widget::isCursorOnQQ()
{
    if (qqHwnd == nullptr) return false;
    return getQQRect().contains(QCursor::pos());
}

bool Widget::isCursorOnMe()
{
    return geometry().contains(QCursor::pos());
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

QString Widget::getWindowText(HWND hwnd)
{
    static WCHAR text[128];
    GetWindowTextW(hwnd, text, sizeof(text));
    return QString::fromWCharArray(text);
}

void Widget::setAutoHide(bool bAuto)
{
    if (isAutoHide == bAuto) return;
    this->isAutoHide = bAuto;
    setBGColor(isAutoHide ? defaultColor : sleepColor);
}

bool Widget::isTopMost(HWND hwnd)
{
    if (hwnd == nullptr) return false;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return style & WS_EX_TOPMOST;
}

QString Widget::getProcessName(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    DWORD PID = -1; //not NULL
    GetWindowThreadProcessId(hwnd, &PID);

    static WCHAR path[128];
    HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    GetProcessImageFileNameW(Process, path, sizeof(path));
    CloseHandle(Process);

    QString pathS = QString::fromWCharArray(path);
    return pathS.mid(pathS.lastIndexOf('\\') + 1);
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
                getInputFocus(); //转移焦点 否则 isQQHideState()会moveOut();
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

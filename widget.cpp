#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QMessageBox>
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
    setAcceptDrops(true);
    //setMouseTracking(true);
    setWindowOpacity(0.9); //做一下方块动画------------------------
    //showMinimized(); //
    QtWin::taskbarDeleteTab(this); //删除任务栏图标 //showMinimized()之后delete 否则size有些不正常 或者在窗体构造之后mini

    setAutoFillBackground(true); //to使用QPalette
    setBGColor(defaultColor);

    sysTray = new SystemTray(this);
    sysTray->show();

    timeLine = new QTimeLine(500, this);
    timeLine->setUpdateInterval(10);
    connect(timeLine, &QTimeLine::frameChanged, [=](int frame) {
        moveQQWindow(frame); //不repaint可能不刷新？(其实 repaint也不刷新 还得手动sendMessage)
    });
    connect(timeLine, &QTimeLine::finished, [=]() {
        stateChanged(state, state); //为了防止isTimeLineRunning时，某些操作无法执行，结束后再发送一遍

        if (isQQInvisible())
            emit moveInFinished();
    });
    connect(this, &Widget::moveInFinished, [=]() {
        if (Win::isUnderCursor(winID())) { //当窗体move而鼠标静止时 不会触发EnterEvent 需要手动检测；主要用于setEntireHide(false)后无法触发moveOut
            QPoint curPos = QCursor::pos();
            qApp->postEvent(this, new QEnterEvent(mapFromGlobal(curPos), curPos, curPos));
        }
    });

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
        if ((getQQStickPos() - pos()).manhattanLength() > 2) { //距离过小 animate无法靠近
            anima_trace->setStartValue(this->pos());
            anima_trace->setEndValue(getQQStickPos());
            anima_trace->start();
        } else
            moveToQQSide();
    });

    QTimer* timer_cursor = new QTimer(this);
    timer_cursor->callOnTimeout([=]() {
        static bool isEntireHide = false;
        //bool isGaming = !Win::isCursorVisible() && Win::getClipCursor().width() == 0; //游戏全屏会限制鼠标区域并hideCursor
        bool isBusy = !Win::isCursorVisible() && Win::isForeFullScreen(); //因为游戏不一定限制鼠标区域，& 全屏看视频也需要hide
        if (isBusy ^ isEntireHide) { //逻辑异或（isGameing != isEntireHide）
            isEntireHide = isBusy;
            setEntireHide(isEntireHide);
        }
        /*冗长写法
        if (isGaming) {
            if (!isEntireHide) {
                isEntireHide = true;
                Extend = 0;
                moveIn();
            }
        } else {
            if (isEntireHide) {
                isEntireHide = false;
                Extend = defaultExtend;
                moveIn();
            }
        }*/
    });
    timer_cursor->setInterval(1000);
    timer_cursor->start();

    connect(sysTray, &SystemTray::askForEntireHide, [=](bool bEntire) {
        setEntireHide(bEntire);
        bEntire ? timer_cursor->stop() : timer_cursor->start(); //手动设置EntireHide后 无需开启指针锁定检测
    });

    QTimer* timer_find = new QTimer(this);
    timer_find->callOnTimeout([=]() {
        HWND foreWin = GetForegroundWindow();
        QString title = Win::getWindowText(foreWin);
        static const QStringList BlackList = { "图片查看", "屏幕识图", "翻译" }; //类与样式难以同Chat区分↓

        if (QQChatWin::isChatWin(foreWin) && !BlackList.contains(title)) { //规避图片查看器 难以区分 （如果好友叫"图片查看"就寄了）
            //qDebug() << "Find QQ" << title;
            if (qq.winId() != foreWin) //new Found
                emit qqChatWinChanged(foreWin, qq.winId());
            setState(QQ);

            if (!qq.isTopMost()) //确保QQ窗口前置（出现过莫名其妙失去前置的bug）
                qq.setAlwaysTop();

            if (!GetKey(VK_LBUTTON) && isAutoHide && !isTimeLineRunning()) { //松开鼠标（避免在拖动窗口）
                QRect qqRect = qq.rect();
                if (qqRect.x() && inRange(StickX.first, qqRect.x(), StickX.second)) { //吸附效果 & x!=0防止反复吸附0
                    moveQQWindow(0, qqRect.y(), qqRect.width(), qqRect.height());
                } else if (qqRect.x() < StickX.first && qqRect.right() > 0) { //x not in [吸附范围] && x < 0 自动 move in
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
                        //qDebug() << "this";
                        setState(ME);
                    } else if (qq.isInSameThread(foreWin)) { //排除相同线程窗口情况(表情窗口)
                        //qDebug() << "QQ subWin";
                        setState(QQsub);
                        if (BlackList.contains(title) && !Win::isTopMost(foreWin)) { //防止重复置顶 导致右键菜单无法弹出
                            Win::setAlwaysTop(foreWin); //需要置顶 否则被遮挡
                            qDebug() << "setTop:" << foreWin << title;
                        }
                    } else {
                        //qDebug() << "other";
                        if (foreWin) lastOtherWin = foreWin; //有时候会是NULL
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
            qDebug() << "Find QQ";
            if (isMinimized()) {
                showNormal();
                SwitchToThisWindow(qq.winId(), true); //转移焦点 to QQ
            }
            if (!timer_trace->isActive() && !isTimeLineRunning()) {
                timer_trace->start(40);
                qDebug() << "start Timer";
            }
            //不能用!isQQAllVisible否则导致 [鼠标拖拽入屏幕边缘 自动moveIn] 无法执行
            //可以用!isQQAllVisible 因为改为了stateChanged机制 只会触发一次 否则//打开群消息 width会增加 right++ >0 导致不能moveOut 直接moveIn
            if (!isTimeLineRunning() && !isQQAllVisible() && !GetKey(VK_LBUTTON)) { //点击任务栏窗口 || Alt+Tab激活 且qq处于 侧边栏隐藏状态× 非完全可见状态√ (打开群消息 width会增加 right>>)
                moveOut();
                qq.repaint(); //重绘(否则消息不能更新)
            }
            break;
        case QQsub:
            qDebug() << "QQ sub";
            break;
        case QQMini:
            qDebug() << "QQ mini";
            ShowWindow(qq.winId(), SW_SHOWNOACTIVATE); //会闪烁 但没办法了 //同步函数 防止qq还未还原 就移动 导致窗口错位
            Win::miniAndShow(winID()); //转移焦点 否则 isQQHideState()会moveOut(); 按住任务栏->other 松开->miniQQ 然后焦点转移 QQshow 焦点又回来了...
            moveIn();
            break;
        case ME:
            qDebug() << "this";
            break;
        case OTHER:
            qDebug() << "other";
            break;
        case MISS:
            qDebug() << "#miss";
            showMinimized();
            break;
        default:
            break;
        }
    });

    connect(this, &Widget::qqChatWinChanged, [=](HWND curHwnd, HWND preHWND) {
        //Win::setAlwaysTop(preHWND, false); //取消前一个置顶
        qq.setAlwaysTop(false); //取消前一个置顶
        qq.setWindow(curHwnd);
        qq.setAlwaysTop();

        if (preHWND == nullptr) //first found
            Hook::setMouseHook();
    });

    connect(qApp, &QApplication::aboutToQuit, [=]() {
        Hook::unHook();
    });

    sysTray->showMessage("Info", "QB Started");

    Hook::setReceiver(this);
    Hook::setChecker([=](MSLLHOOKSTRUCT* data, bool* bBlock) -> bool {
        *bBlock = true; //拦截消息 防止模拟按键时 Ctrl+滚轮导致缩放文本框
        QPoint pos(data->pt.x, data->pt.y);
        QRect qqRect = qq.rect();
        static constexpr int WheelWidth = 66; //qq可响应wheel宽度(px)
        QRect qqWheelRect(qqRect.topLeft(), QSize(WheelWidth, qqRect.height()));
        bool isQQDownToMouse = WindowFromPoint(data->pt) == qq.winId(); //防止中间人横插一脚（如图片查看器）
        return qqWheelRect.contains(pos) && isQQDownToMouse;
    });
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::isForeMyself()
{
    return Win::isForeWindow(winID());
}

QPoint Widget::getQQStickPos()
{
    QRect qqRect = qq.rect();
    return QPoint(qqRect.right(), qqRect.top() + MarginTop);
}

void Widget::moveQQWindow(int X, int Y, int nWidth, int nHeight, WINBOOL bRepaint)
{
    if (qq.isNull()) return;

    static auto isNaN = [=](int num) { return num == NaN; };
    if (isNaN(Y) || isNaN(nWidth) || isNaN(nHeight)) { //如果置空 则自动填充
        QRect qqRect = qq.rect();
        if (isNaN(Y)) Y = qqRect.y();
        if (isNaN(nWidth)) nWidth = qqRect.width();
        if (isNaN(nHeight)) nHeight = qqRect.height();
    }

    qq.move(X, Y, nWidth, nHeight, bRepaint);
    move(X + nWidth - 1, Y + MarginTop); //-1是因为从0开始计数 修正与qqRect.right()的差异
}

void Widget::moveToQQSide()
{
    move(getQQStickPos());
}

HWND Widget::winID()
{
    return (HWND)this->winId();
}

void Widget::moveIn(int duration)
{
    stopTraceAnima();
    QRect qqRect = qq.rect();
    timeLine->stop();
    timeLine->setFrameRange(qqRect.x(), -(qqRect.width() + width() - Extend));
    if (timeLine->startFrame() == timeLine->endFrame()) return;
    timeLine->setDuration(duration);
    timeLine->start();

    Hook::unHook();
}

void Widget::moveOut(int duration)
{
    stopTraceAnima();
    QRect qqRect = qq.rect();
    timeLine->stop();
    timeLine->setFrameRange(qqRect.x(), 0);
    if (timeLine->startFrame() == timeLine->endFrame()) return;
    timeLine->setDuration(duration);
    timeLine->start();

    Hook::setMouseHook();
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
    return inRange(StickX.first, qqRect.x(), StickX.second); //范围
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

QRect Widget::getQQStickRect()
{
    QRect qqRect = qq.rect();
    QRect Rect(qqRect.topRight(), qqRect.topRight());
    return Rect.marginsAdded(QMargins(32, 30, 32, qqRect.height()));
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

bool Widget::inRange(int min, int val, int max)
{
    return val >= min && val <= max;
}

void Widget::setEntireHide(bool bEntire) //Extend = 0完全hide 防止Game碍眼 & 误触
{
    qDebug() << "#EntireHide:" << bEntire;
    this->Extend = bEntire ? 0 : defaultExtend;
    if (isQQInvisible())
        moveIn(100); //直接调整
}

void Widget::enterEvent(QEvent* event) //指针应该很安全 不检查了
{
    Q_UNUSED(event)
    qDebug() << "enter";

    if (isTimeLineRunning() == false) { //模拟click会误触其他位置
        //getInputFocus(); //获取焦点 否则还会缩回去//某些特殊窗口 如任务管理器在前台时 会阻止该函数
        //moveOut(); √
        ////SwitchToThisWindow(qqHwnd, false); //将自己的focus转移很easy 获取很困难//不能用于获取focus //该函数会闪烁不太好
        //setInputFocus(qqHwnd);
        //#No! 此种情况 鼠标触碰弹出 不应该获取焦点 只moveOut，鼠标移开后自动moveIn（不影响看视频等操作 Just View）

        if (!isQQAllVisible()) //隐藏状态弹出 //可以尝试改为!isQQAllVisible增加鲁棒性//休眠后QQ窗口会向左位移 不能用isQQHideState判断
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

                static QMetaObject::Connection conn; //static自动捕获
                conn = connect(timeLine, &QTimeLine::finished, [=] { //在动画结束后检测鼠标下的窗体 防止遮挡
                    qDebug() << "MoveIn Finished & check for Focus Back";
                    if (Win::topWinFromPoint(QCursor::pos()) == lastOtherWin) { //增加鼠标下窗体检测 防止出现令用户意外的焦点转移 //foreGroundWin一般是父窗口
                        //Win::getInputFocus(lastOtherWin); //返还焦点 attach失败时会miniAndShow 观感不好
                        SwitchToThisWindow(lastOtherWin, true); //此时this已获得焦点 所以可以使用该函数；且不会因为系统窗口而Attach失败
                        qDebug() << "#Focus back to:" << lastOtherWin << Win::getProcessName(lastOtherWin) << Win::getWindowClass(lastOtherWin);
                    }
                    disconnect(conn); //单次连接 过河拆桥
                });
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
    if (isTimeLineRunning() || event->button() != Qt::LeftButton) return;
    stopTraceAnima(); //防止move时 timer滞后检测到this 并moveToSide()导致鬼畜

    curPos = event->globalPos();
    isStick = true;
    qqStickRect = getQQStickRect();
    preColor = this->palette().color(QPalette::Window); //保存原色
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)

    if (getQQStickRect().contains(pos())) { //实时计算 增加可靠性
        MarginTop = pos().y() - qq.rect().top(); //更新MarginTop
        moveToQQSide();
    } else { //不在安全区域：quit
        sysTray->showMessage("Info", QString("About to [Quit] bye\nstate: %1 %2 %3").arg(qq.isMini()).arg(qq.rect().x()).arg(qq.rect().right()));
        qApp->quit();
    }
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

    QPoint mousePos = QCursor::pos(); //event->screenPos().toPoint()反映的是事件发生的状态 QCursor::pos()实时性更高
    QPoint delta = mousePos - curPos;
    QPoint thisPos = this->pos();

    static constexpr qreal SpeedXLimit = 8000; //5000 px/s
    static QTime lastTime = QTime::currentTime(); //press不需要初始化 因为时间长了没事 & lastTime无论如何都可以更新 无需QDateTIme
    QTime now = QTime::currentTime();
    int gap = lastTime.msecsTo(now); //断电后 gap会突变卡顿
    qreal speedX = gap ? 1000.0 * delta.x() / gap : 0; //防止除数为 0
    if (speedX > SpeedXLimit) isStick = false; //突破速度极限
    lastTime = now;

    if (isStick) { //Hard to drag(x方向)
        delta.setX(0);
        if (thisPos.y() <= qqStickRect.top()) //安全距离 防止蹭出去（边界Release）
            delta.setY(2);
        else if (thisPos.y() >= qqStickRect.bottom())
            delta.setY(-2);
    }
    curPos += delta;
    if (isStick)
        QCursor::setPos(curPos); //坚韧不拔(x阻尼)
    else //isNotStick否则上下移动也会干扰setBGColor
        setBGColor(qqStickRect.contains(thisPos) ? preColor : dangerColor);

    QPoint newPos = thisPos + delta;
    move(newPos);
}

void Widget::wheelEvent(QWheelEvent* event) //从全局发送而来(Hook)
{
    static constexpr int Gap = 80;
    static QDateTime lastTime; //QTime无法区分两天 导致差值 < 0
    QDateTime now = QDateTime::currentDateTime();
    if (lastTime.isValid() && lastTime.msecsTo(now) < Gap) return; //限速器，防止滚轮过快导致按键模拟不及时（触摸板）
    lastTime = now;
    qDebug() << "WheelEvent" << now;

    if (!Win::isForeWindow(qq.winId())) {
        Win::getInputFocus(winID()); //自己获得焦点后才能设置其他窗口焦点
        Win::getInputFocus(qq.winId());
    }

    if (event->delta() > 0) { //模拟Ctrl+Shift+Tab向上切换QQ消息窗口 //Ctrl+↑↓会有系统提示音 很烦
        Win::simulateKeyEvent(KeyList({ VK_CONTROL, VK_SHIFT, VK_TAB }));
    } else { //模拟Ctrl+Tab向下切换QQ消息窗口
        Win::simulateKeyEvent(KeyList({ VK_CONTROL, VK_TAB }));

        /*实现非活动窗口的按键模拟
        keybd_event(VK_CONTROL, 0, KEYEVENTF_EXTENDEDKEY, 0);
        QTimer::singleShot(100, [=]() {//延时可以确保 异步的keybd_event执行完毕
            SendMessage(qq.winId(), WM_KEYDOWN, 'A', 0);
            SendMessage(qq.winId(), WM_KEYUP, 'A', 0);
            keybd_event(VK_CONTROL, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
        });
        */
    }
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

    qDebug() << "paint";
}

void Widget::dragEnterEvent(QDragEnterEvent* event) //拖拽时发送EnterEvent事件弹出QQ
{
    qDebug() << "#dragEnter";
    QPoint curPos = QCursor::pos();
    qApp->postEvent(this, new QEnterEvent(event->pos(), curPos, curPos));
    event->accept(); //接收了才能继续发出dragLeave
}

void Widget::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_UNUSED(event)
    qDebug() << "#dragLeave";
    setAttribute(Qt::WA_UnderMouse, false); //underMouse不会在dragDrop时更新，需要手动设置
}

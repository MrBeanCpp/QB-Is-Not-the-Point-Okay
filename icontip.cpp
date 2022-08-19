#include "icontip.h"
#include "ui_icontip.h"
#include <QDebug>
#include <QtWin>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
IconTip::IconTip(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::IconTip)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool); //Dialog可以不被父窗口截断 Tool不抢占焦点
    setAttribute(Qt::WA_TranslucentBackground); //启用半透明背景//也可以实现背景穿透！！！
    setWindowFlag(Qt::WindowStaysOnTopHint);
    ui->label->move(Shadow_R, Shadow_R);

    Hwnd = HWND(this->winId());
    RegisterShellHookWindow(Hwnd); //注册WM_SHELLHOOKMESSAGE

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(Shadow_R);
    shadow->setColor(QColor(20, 20, 20));
    shadow->setOffset(QPoint(0, 0));
    ui->label->setGraphicsEffect(shadow);
}

IconTip::~IconTip()
{
    delete ui;
    DeregisterShellHookWindow(Hwnd);
}

QPixmap IconTip::getWindowICON(HWND hwnd)
{
    HICON icon = reinterpret_cast<HICON>(SendMessageW(hwnd, WM_GETICON, ICON_SMALL, 0));
    return QtWin::fromHICON(icon);
}

void IconTip::showWindowICON(HWND hwnd)
{
    ui->label->setPixmap(getWindowICON(hwnd));
}

void IconTip::setHeight(int y)
{
    if (anima_group == nullptr)
        move(pos().x(), y);
    else {
        QPoint startP = anima_forward->startValue().toPoint();
        QPoint endP = anima_forward->endValue().toPoint();
        startP.setY(y);
        endP.setY(y);
        anima_forward->setStartValue(startP);
        anima_forward->setEndValue(endP);
        anima_backward->setStartValue(anima_forward->endValue());
        anima_backward->setEndValue(anima_forward->startValue());
    }
}

void IconTip::initAnimation(HWND qqHwnd)
{
    if (anima_group) return;

    QSize iconSize = getWindowICON(qqHwnd).size();
    iconSize += QSize(Shadow_R * 2, Shadow_R * 2);
    resize(iconSize);

    anima_forward = new QPropertyAnimation(this, "pos", this);
    anima_forward->setDuration(600);
    anima_forward->setEasingCurve(QEasingCurve::InOutCubic);
    anima_forward->setStartValue(QPoint(-iconSize.width(), pos().y()));
    anima_forward->setEndValue(QPoint(0, pos().y()));

    anima_backward = new QPropertyAnimation(this, "pos", this);
    anima_backward->setDuration(600);
    anima_backward->setEasingCurve(QEasingCurve::InOutCubic);
    anima_backward->setStartValue(anima_forward->endValue());
    anima_backward->setEndValue(anima_forward->startValue());

    anima_group = new QSequentialAnimationGroup(this);
    anima_group->addAnimation(anima_forward);
    anima_group->addPause(1800);
    anima_group->addAnimation(anima_backward);
}

bool IconTip::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    static const UINT WM_SHELLHOOKMESSAGE = RegisterWindowMessage(TEXT("SHELLHOOK"));
    MSG *msg = (MSG *)message;
    if (msg->message == WM_SHELLHOOKMESSAGE && msg->wParam == HSHELL_FLASH) { //来flash触发4次Message 焦点转移至QQ再触发一次
        HWND hwnd = HWND(msg->lParam);
        HWND foreWin = GetForegroundWindow();
        if (hwnd != qq.winId() || hwnd == foreWin) return false; //排除获取焦点的情况

        if (anima_group == nullptr)
            initAnimation(qq.winId()); //not in 构造函数 防止qq.winId() == nullptr
        showWindowICON(hwnd);
        if (anima_group->state() != QAnimationGroup::Running) {
            show();
            anima_group->start(QAbstractAnimation::KeepWhenStopped);
        }
    }
    return false;
} //https://stackoverflow.com/questions/9305646/detect-a-taskbar-icon-flashing

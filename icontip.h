#ifndef ICONTIP_H
#define ICONTIP_H

#include <QWidget>
#include <windows.h>
#include <QTimeLine>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include "QQChatWin.h"
namespace Ui {
class IconTip;
}

class IconTip : public QWidget
{
    Q_OBJECT

public:
    explicit IconTip(QWidget *parent = nullptr);
    ~IconTip();

    QPixmap getWindowICON(HWND hwnd);
    void showWindowICON(HWND hwnd, bool autoScale = true);
    void setHeight(int y);
    void initAnimationRoute(HWND qqHwnd);
    void setAnimationRoute(const QPoint& sPos, const QPoint& ePos);

private:
    Ui::IconTip *ui;

    QQChatWin& qq = QQChatWin::instance();
    HWND Hwnd = NULL;
    //HWND QQHwnd = HWND(198522);
    static constexpr int Shadow_R = 10;
    QSequentialAnimationGroup *anima_group = nullptr;
    QPropertyAnimation *anima_forward = nullptr;
    QPropertyAnimation *anima_backward = nullptr;

    // QWidget interface
protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
};

#endif // ICONTIP_H

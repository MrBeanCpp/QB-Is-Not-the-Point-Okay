#include "systemtray.h"

SystemTray::SystemTray(QWidget* parent)
    : QSystemTrayIcon(parent)
{
    setIcon(QIcon(R"(E:\Qt5.14.2\Projects\QQ_Helper\images\icon.ico)"));
    setMenu(parent);
    setToolTip("QQ Follower");
}

void SystemTray::setMenu(QWidget* parent)
{
    QMenu* menu = new QMenu(parent);
    menu->setStyleSheet("QMenu{background-color:rgb(15,15,15);color:rgb(220,220,220);}"
                        "QMenu:selected{background-color:rgb(60,60,60);}");
    QAction* act_setting = new QAction("Settings", menu);
    QAction* act_entireHide = new QAction("EntireSideHide", menu);
    QAction* act_quit = new QAction("Quit>", menu);

    act_entireHide->setCheckable(true);
    connect(act_entireHide, &QAction::toggled, this, &SystemTray::askForEntireHide);
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
    menu->addActions(QList<QAction*>() << act_setting << act_entireHide << act_quit);
    this->setContextMenu(menu);
}

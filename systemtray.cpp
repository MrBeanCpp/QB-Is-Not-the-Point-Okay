#include "systemtray.h"

SystemTray::SystemTray(QWidget* parent)
    : QSystemTrayIcon(parent)
{
    setIcon(QIcon(":/images/icon.ico"));
    setMenu(parent);
    setToolTip("QQ Follower");
}

void SystemTray::setMenu(QWidget* parent)
{
    QMenu* menu = new QMenu(parent);
    menu->setStyleSheet("QMenu{background-color:rgb(45,45,45);color:rgb(220,220,220);border:1px solid black;}"
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

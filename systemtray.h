#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
class SystemTray : public QSystemTrayIcon
{
    Q_OBJECT
public:
    SystemTray(QWidget* parent = nullptr);

private:
    void setMenu(QWidget* parent = nullptr);
};

#endif // SYSTEMTRAY_H

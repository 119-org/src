#ifndef SKTOOL_H
#define SKTOOL_H

#include <QtGui>
#include "server.h"
#include "client.h"

class Sktool : public QMainWindow
{
    Q_OBJECT

public:
    Sktool();

    void show();

private slots:

private:
    ServerWidget *serverWidget;
    ClientWidget *clientWidget;

    QMainWindow m_wnd;
    QMenu *fileMenu;
    QMenu *helpMenu;

};

#endif

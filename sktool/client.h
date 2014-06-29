#ifndef CLIENT_H
#define CLIENT_H

#include <QtGui>
#include <QWidget>
#include "ui_client.h"

class ClientWidget : public QTabWidget
{
    Q_OBJECT

public:
    ClientWidget(QWidget *parent = 0);
    void initConfig();
    void initHandler();


private slots:

private:
    Ui::ClientWidget m_ui;
 
};

#endif

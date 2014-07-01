#ifndef SERVER_H
#define SERVER_H

#include <QtGui>
#include <QWidget>
#include "ui_server.h"
#include "sktlib.h"

class ServerWidget : public QTabWidget
{
    Q_OBJECT

public:
    ServerWidget(QWidget *parent = 0);
    void initConfig();
    void initHandler();

private slots:
    void trigger(bool start);

private:
    Ui::ServerWidget m_ui;
    TcpServerSkt m_tcp;
    UdpServerSkt m_udp;

};

#endif

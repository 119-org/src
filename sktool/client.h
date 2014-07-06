#ifndef CLIENT_H
#define CLIENT_H

#include <QtGui>
#include <QWidget>
#include "ui_client.h"
#include "sktlib.h"
#include "logger.h"


class ClientWidget : public QTabWidget
{
    Q_OBJECT

public:
    ClientWidget(QWidget *parent = 0);
    void initConfig();
    void initHandler();
    void initLogger(QToolButton* c, QTreeWidget* o, QPlainTextEdit* d);

signals:
    void output(const QString& info);
    void output(const QString& title, const char* buf, quint32 len);


private slots:
    void trigger(bool start);
    void clearMsg();
    void send();

private:
    Ui::ClientWidget m_ui;
    TcpClientSkt m_tcp;
    UdpClientSkt m_udp;
    Logger m_logger;
 
};

#endif

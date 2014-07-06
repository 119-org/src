#ifndef SERVER_H
#define SERVER_H

#include <QtGui>
#include <QWidget>
#include "ui_server.h"
#include "sktlib.h"
#include "logger.h"

class ServerWidget : public QTabWidget
{
    Q_OBJECT

public:
    ServerWidget(QWidget *parent = 0);
    void initConfig();
    void initHandler();
    void initLogger(QToolButton* c, QTreeWidget* o, QPlainTextEdit* d);

signals:
    void output(const QString& info);
    void output(const QString& title, const char* buf, quint32 len);

private slots:
    void trigger(bool start);
    void clearMsg();

private:
    Ui::ServerWidget m_ui;
    TcpServerSkt m_tcp;
    UdpServerSkt m_udp;
    Logger m_logger;

};

#endif

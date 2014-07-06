#include "client.h"
#include "sktlib.h"

ClientWidget::ClientWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    initConfig();
    initHandler();
    initLogger(m_ui.btnClear, m_ui.treeOutput, m_ui.txtOutput);
}

void ClientWidget::initConfig()
{
    SocketLib::initNetwork(m_ui.cmbAddr);
}

void ClientWidget::initHandler()
{
    connect(m_ui.btnTcp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));
    connect(m_ui.btnUdp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));
    connect(&m_tcp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));
    connect(&m_udp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));
    connect(m_ui.btnSend, SIGNAL(released()), this, SLOT(send()));
}

void ClientWidget::initLogger(QToolButton* c, QTreeWidget* o, QPlainTextEdit* d)
{
    m_logger.init(o, d);

    connect(c, SIGNAL(released()), this, SLOT(clearMsg()));
    connect(this, SIGNAL(output(const QString&)), &m_logger, SLOT(output(const QString&)));
    connect(this, SIGNAL(output(const QString&, const char*, quint32)), &m_logger, SLOT(output(const QString&, const char*, quint32)));
}

void ClientWidget::clearMsg()
{
    m_logger.clear();
}

void ClientWidget::send()
{
    ClientSkt *client = (ClientSkt*)&m_tcp;
    client->printMsg(QString("Start to send."));
    client->send(m_ui.edtBuf0->text());
    client = (ClientSkt*)&m_udp;
    client->send(m_ui.edtBuf0->text());
}

void ClientWidget::trigger(bool start)
{
    QString msg;
    QToolButton* btnTrigger = qobject_cast<QToolButton*>(sender());
    if (!btnTrigger) return;
    bool istcp = (btnTrigger == m_ui.btnTcp);
    ClientSkt *client = istcp ? (ClientSkt*)&m_tcp : (ClientSkt*)&m_udp;
    bool res;
    QString ip = m_ui.cmbAddr->currentText().trimmed();
    quint16 port = m_ui.cmbPort->currentText().trimmed().toUShort(&res, 10);

    if (start) {
        start = client->open(ip, port);
        if (start) {
            client->printMsg(QString("Connect %1:%2 success.").arg(ip).arg(port));
        } else {
            client->printMsg(QString("Connect %1:%2 failed.").arg(ip).arg(port));
        }
    } else {
        client->close();
        client->printMsg(QString("Close connection."));
    }
    m_ui.cmbAddr->setDisabled(start);
    m_ui.cmbPort->setDisabled(start);
}

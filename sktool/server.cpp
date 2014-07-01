#include "server.h"
#include "sktlib.h"

ServerWidget::ServerWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    initConfig();
    initHandler();
}

void ServerWidget::initConfig()
{
    SocketLib::initNetwork(m_ui.cmbTcpAddr);
    SocketLib::initNetwork(m_ui.cmbUdpAddr);
}

void ServerWidget::initHandler()
{
    connect(m_ui.btnTcp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));
    connect(m_ui.btnUdp, SIGNAL(clicked(bool)), this, SLOT(trigger(bool)));
    connect(&m_tcp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));

}

void ServerWidget::trigger(bool start)
{
    QToolButton* btnTrigger = qobject_cast<QToolButton*>(sender());
    if (!btnTrigger) return;

    bool istcp = (btnTrigger==m_ui.btnTcp);
    QComboBox* cmbAddr = istcp ? m_ui.cmbTcpAddr : m_ui.cmbUdpAddr;
    QComboBox* cmbPort = istcp ? m_ui.cmbTcpPort : m_ui.cmbUdpPort;
    ServerSkt* server = istcp ? (ServerSkt*)&m_tcp : (ServerSkt*)&m_udp;

    bool res;
    QString ip = cmbAddr->currentText().trimmed();
    quint16 port = cmbPort->currentText().trimmed().toUShort(&res, 10);

    if (start)
        start = server->open(ip, port);
    else
        server->close();

    cmbAddr->setDisabled(start);
    cmbPort->setDisabled(start);
}

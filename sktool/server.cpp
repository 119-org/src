#include "server.h"
#include "sktlib.h"

ServerWidget::ServerWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    initConfig();
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

}

void ServerWidget::trigger(bool start)
{
    QToolButton* btnTrigger = qobject_cast<QToolButton*>(sender());
    if (!btnTrigger) return;

    bool istcp = (btnTrigger==m_ui.btnTcp);
    QComboBox* cmbAddr = istcp ? m_ui.cmbTcpAddr : m_ui.cmbUdpAddr;
    QComboBox* cmbPort = istcp ? m_ui.cmbTcpPort : m_ui.cmbUdpPort;
    ServerSkt* server = istcp ? (ServerSkt*)&m_tcp : (ServerSkt*)&m_udp;

    IPAddr addr;

    if (start)
        start = server->start(addr.ip, addr.port);
    else
        server->stop();

    cmbAddr->setDisabled(start);
    cmbPort->setDisabled(start);
}

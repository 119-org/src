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

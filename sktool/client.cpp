#include "client.h"
#include "sktlib.h"

ClientWidget::ClientWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    initConfig();
}

void ClientWidget::initConfig()
{
    SocketLib::initNetwork(m_ui.cmbAddr);
}

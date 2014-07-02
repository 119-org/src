#include "server.h"
#include "sktlib.h"

ServerWidget::ServerWidget(QWidget *parent)
    : QTabWidget(parent)
{
    m_ui.setupUi(this);
    initConfig();
    initHandler();
    initLogger(m_ui.btnClear, m_ui.treeOutput, m_ui.txtOutput);
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
    connect(&m_udp, SIGNAL(message(const QString&)), this, SIGNAL(output(const QString&)));

}

void ServerWidget::initLogger(QToolButton* c, QTreeWidget* o, QPlainTextEdit* d)
{
    m_logger.init(o, d);

    connect(c, SIGNAL(released()), this, SLOT(clearMsg()));
    connect(this, SIGNAL(output(const QString&)), &m_logger, SLOT(output(const QString&)));
    connect(this, SIGNAL(output(const QString&, const char*, quint32)), &m_logger, SLOT(output(const QString&, const char*, quint32)));
}

void ServerWidget::clearMsg()
{
    m_logger.clear();
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

    QString tmpPort;
    tmpPort.setNum(server->port(), 10);
    cmbPort->clear();
    cmbPort->addItem(tr(qPrintable(tmpPort)).trimmed(), 1);
    cmbPort->setCurrentIndex(0);
    cmbAddr->setDisabled(start);
    cmbPort->setDisabled(start);
}

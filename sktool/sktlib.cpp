#include <QComboBox>
#include <QToolButton>
#include <QCoreApplication>
#include <QAction>

#include "sktlib.h"
#include "../libraries/libskt/libskt.h"

void SocketLib::initNetwork(QComboBox *box)
{
    skt_addr_list_t *sal, *p;
    char str[MAX_ADDR_STRING];
    QString def;

    if (-1 == skt_get_local_list(&sal, 1)) {
        return;
    }

    for (p = sal; p; p = p->next) {
        skt_addr_ntop(str, p->addr.ip);
        if (-1 == box->findText(str))
            box->insertItem(0, str);
    }

    def = box->currentText();
    box->clearEditText();

    if (!def.isEmpty() && (-1 != box->findText(def))) {
        box->setEditText(def);
    }
}

ServerSkt::ServerSkt(QObject *parent)
: QObject(parent),
  m_ip("127.0.0.1"),
  m_port(0)
{
}

ServerSkt::~ServerSkt()
{
}

void ServerSkt::printMsg(const QString &msg)
{
    emit message(msg);
}


TcpServerSkt::TcpServerSkt(QObject *parent)
:ServerSkt(parent)
{
}

TcpServerSkt::~TcpServerSkt()
{
}

bool TcpServerSkt::open(QString ip, quint16 port)
{
    QString msg("xxx");
    m_fd = skt_tcp_bind_listen(qPrintable(ip), port);
    if (m_fd == -1) {
	printMsg(msg);
        return false;
    }
    printMsg(msg);
    return true;
}

void TcpServerSkt::close()
{
    skt_close(m_fd);
}

UdpServerSkt::UdpServerSkt(QObject *parent)
:ServerSkt(parent)
{
}

UdpServerSkt::~UdpServerSkt()
{
}

bool UdpServerSkt::open(QString ip, quint16 port)
{
    m_fd = skt_tcp_bind_listen(qPrintable(ip), port);
    if (m_fd == -1) {
        return false;
    }
    return true;
}

void UdpServerSkt::close()
{
    skt_close(m_fd);
}

#include <QComboBox>
#include <QToolButton>
#include <QCoreApplication>
#include <QAction>

#include "sktlib.h"
#include "../libraries/libskt/libskt.h"

ServerSkt::ServerSkt(QObject *parent)
: QObject(parent)
{
}

ServerSkt::~ServerSkt()
{
}

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

bool ServerSkt::start(const QString ip, quint16 port)
{
    return true;
}

void ServerSkt::stop()
{
}



TcpServerSkt::TcpServerSkt(QObject *parent)
:ServerSkt(parent)
{
}

TcpServerSkt::~TcpServerSkt()
{
}

UdpServerSkt::UdpServerSkt(QObject *parent)
:ServerSkt(parent)
{
}

UdpServerSkt::~UdpServerSkt()
{
}


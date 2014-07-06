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

void recv_msg(void *args)
{
    TcpServerSkt *tcp = (TcpServerSkt *)args;
    int fd = tcp->m_afd;
    char buf[20];
    skt_recv(fd, buf, 20);
    tcp->printMsg(QString(buf));
}
void handle_new_conn(void *args)
{
    TcpServerSkt *tcp = (TcpServerSkt *)args;
    int fd;
    uint32_t ip;
    uint16_t port;
    fd = skt_accept(tcp->m_fd, &ip, &port);
    if (fd == -1) {
//        err("errno=%d %s\n", errno, strerror(errno));
        return;
    };
    tcp->m_afd = fd;
    struct skt_ev_cbs *evcb = (struct skt_ev_cbs *)calloc(1, sizeof(struct skt_ev_cbs));
    evcb->ev_in = recv_msg;
    evcb->ev_out = NULL;
    evcb->ev_err = NULL;
    struct skt_ev *e = skt_ev_create(fd, EVENT_READ, evcb, (void *)tcp);
    if (-1 == skt_ev_add(e)) {
//        err("event_add failed!\n");
    }
}

void *tcp_server(void *args)
{
    TcpServerSkt *tcp = (TcpServerSkt *)args;
    if (-1 == skt_ev_init()) {
        return NULL;
    }

    struct skt_ev_cbs *evcb = (struct skt_ev_cbs *)calloc(1, sizeof(struct skt_ev_cbs));
    evcb->ev_in = handle_new_conn;
    evcb->ev_out = NULL;
    evcb->ev_err = NULL;
    struct skt_ev *e = skt_ev_create(tcp->m_fd, EVENT_READ, evcb, (void *)tcp);
    if (-1 == skt_ev_add(e)) {
        tcp->printMsg(QString("event_add failed!"));
        return NULL;
    }
    skt_ev_dispatch();
    return NULL;
}

bool TcpServerSkt::open(QString ip, quint16 port)
{
    m_fd = skt_tcp_bind_listen(qPrintable(ip), port);
    if (m_fd == -1) {
        return false;
    }

    m_ip = ip;
    if (port == 0) {
        struct skt_addr addr;
        skt_get_addr_by_fd(&addr, m_fd);
        m_port = addr.port;
    } else {
        m_port = port;
    }
    pthread_t pid;
    if (-1 == pthread_create(&pid, NULL, tcp_server, (void *)this)) {
        printMsg(QString("Create TCP Server thread failed."));
        return false;
    }
    connect(this, SIGNAL(onConnectSignal()), this, SLOT(onConnect()));
    return true;
}

void TcpServerSkt::close()
{
    skt_close(m_fd);
}

int TcpServerSkt::send(const QString& data)
{
    QByteArray bin = data.toAscii();
    const void *buf = bin.constData();
    size_t len = bin.length();
    return skt_send(m_fd, buf, len);

}

void TcpServerSkt::recv()
{

}

void TcpServerSkt::onConnect()
{
    TcpServerSkt *server = qobject_cast<TcpServerSkt*>(sender());
    if (!server) return;
    printMsg(QString("new connect incoming"));
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
    QString msg("Open UDP Server %1.");
    m_fd = skt_udp_bind(qPrintable(ip), port);
    if (m_fd == -1) {
        msg = msg.arg("failed");
        printMsg(msg);
        return false;
    }
    msg = msg.arg("success");
    printMsg(msg);
    m_ip = ip;
    if (port == 0) {
        struct skt_addr addr;
        skt_get_addr_by_fd(&addr, m_fd);
        m_port = addr.port;
    } else {
        m_port = port;
    }
    return true;
}

void UdpServerSkt::close()
{
    QString msg("Close UDP Server.");
    skt_close(m_fd);
    printMsg(msg);
}

// Below is Client Socket Class

ClientSkt::ClientSkt(QObject *parent)
: QObject(parent),
  m_src_ip("127.0.0.1"),
  m_src_port(0)
{
}

ClientSkt::~ClientSkt()
{
}

TcpClientSkt::TcpClientSkt(QObject *parent)
:ClientSkt(parent)
{
}

TcpClientSkt::~TcpClientSkt()
{
}

bool TcpClientSkt::open(QString ip, quint16 port)
{
    m_fd = skt_tcp_conn(qPrintable(ip), port);
    if (m_fd == -1) {
        return false;
    }

    m_dst_ip = ip;
    m_dst_port = port;

    struct skt_addr addr;
    if (-1 == skt_get_addr_by_fd(&addr, m_fd)) {
        return false;
    }
    m_src_port = addr.port;
    char str[MAX_ADDR_STRING];
    skt_addr_ntop(str, addr.ip);
    m_src_ip = str;
    return true;
}

void TcpClientSkt::close()
{
    skt_close(m_fd);
}

int TcpClientSkt::send(const QString& data)
{
    QByteArray bin = data.toAscii();
    const void *buf = bin.constData();
    size_t len = bin.length();
    return skt_send(m_fd, buf, len);
}

void TcpClientSkt::recv()
{

}


UdpClientSkt::UdpClientSkt(QObject *parent)
:ClientSkt(parent)
{
}

void ClientSkt::printMsg(const QString &msg)
{
    emit message(msg);
}


UdpClientSkt::~UdpClientSkt()
{
}

bool UdpClientSkt::open(QString ip, quint16 port)
{
    m_fd = skt_udp_bind(NULL, 0);
    if (m_fd == -1) {
        return false;
    }

    m_dst_ip = ip;
    m_dst_port = port;

    struct skt_addr addr;
    if (-1 == skt_get_addr_by_fd(&addr, m_fd)) {
        return false;
    }
    m_src_port = addr.port;
    char str[MAX_ADDR_STRING];
    skt_addr_ntop(str, addr.ip);
    m_src_ip = str;

    return true;
}
void UdpClientSkt::close()
{
    skt_close(m_fd);
}

int UdpClientSkt::send(const QString& data)
{
    QByteArray bin = data.toAscii();
    const void *buf = bin.constData();
    size_t len = bin.length();
    return skt_sendto(m_fd, qPrintable(m_dst_ip), m_dst_port, buf, len);
}

void UdpClientSkt::recv()
{

}

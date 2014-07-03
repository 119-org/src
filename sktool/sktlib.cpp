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
//    printf("recv buf = %s\n", buf);
    QString msg(buf);
    tcp->printMsg(msg);
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
    int ret;
    TcpServerSkt *tcp = (TcpServerSkt *)args;
    ret = skt_ev_init();
    if (ret == -1) {
        return NULL;
    }

    struct skt_ev_cbs *evcb = (struct skt_ev_cbs *)calloc(1, sizeof(struct skt_ev_cbs));
    evcb->ev_in = handle_new_conn;
    evcb->ev_out = NULL;
    evcb->ev_err = NULL;
    struct skt_ev *e = skt_ev_create(tcp->m_fd, EVENT_READ, evcb, (void *)tcp);
    if (-1 == skt_ev_add(e)) {
        QString msg("event_add failed!");
        tcp->printMsg(msg);
//        err("event_add failed!\n");
    }
    QString msg("event_add success.");
    tcp->printMsg(msg);

    skt_ev_dispatch();
}

bool TcpServerSkt::open(QString ip, quint16 port)
{
    int ret;
    QString msg("Open TCP Server %1.");
    m_fd = skt_tcp_bind_listen(qPrintable(ip), port);
    if (m_fd == -1) {
        msg = msg.arg("failed");
        printMsg(msg);
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
        msg = msg.arg("failed");
        printMsg(msg);
        return false;
    }
    connect(this, SIGNAL(onConnectSignal()), this, SLOT(onConnect()));
    msg = msg.arg("success");
    printMsg(msg);
    return true;
}

void TcpServerSkt::close()
{
    QString msg("Close TCP Server.");
    skt_close(m_fd);
    printMsg(msg);
}

void TcpServerSkt::send(void* cookie, const QByteArray& bin)
{
}

void TcpServerSkt::recv()
{

}

void TcpServerSkt::onConnect()
{
    TcpServerSkt *server = qobject_cast<TcpServerSkt*>(sender());
    if (!server) return;
    QString msg("new connect incoming");

    printMsg(msg);

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

#ifndef SKTLIB_H
#define SKTLIB_H

#include <QObject>
#include <QGlobalStatic>
#include <QVector>
#include <QComboBox>

typedef struct IPAddr {
    QString ip;
    quint16 port;
} IPAddr;

class SocketLib
{
public:
    static void initNetwork(QComboBox *box);

};

class ServerSkt : public QObject
{
    Q_OBJECT

public:
    ServerSkt(QObject *parent=0);
    virtual ~ServerSkt();

    const QString addr() const {return m_ip;};
    quint16 port() {return m_port;};

    void printMsg(const QString &msg);
    virtual bool open(QString ip, quint16 port) = 0;
    virtual void close() = 0;

    QString m_ip;
    quint16 m_port;

signals:
    void message(const QString &msg);

private:
    bool m_started;

};

class TcpServerSkt : public ServerSkt
{
    Q_OBJECT

public:
    TcpServerSkt(QObject *parent=0);
    virtual ~TcpServerSkt();
    int m_fd;
    int m_afd;

protected:
    virtual bool open(QString ip, quint16 port);
    virtual void close();
    virtual int send(const QString& data);

signals:
    void onConnectSignal();


private slots:
    void onConnect();
    void recv();

private:
    QString m_error;
};

class UdpServerSkt : public ServerSkt
{
    Q_OBJECT

public:
    UdpServerSkt(QObject *parent=0);
    virtual ~UdpServerSkt();

protected:
    virtual bool open(QString ip, quint16 port);
    virtual void close();

private:
    int m_fd;
};

class ClientSkt : public QObject
{
    Q_OBJECT

public:
    ClientSkt(QObject *parent=0);
    virtual ~ClientSkt();

    const QString srcAddr() const {return m_src_ip;};
    quint16 srcPort() {return m_src_port;};
    const QString dstAddr() const {return m_dst_ip;};
    quint16 dstPort() {return m_dst_port;};

    void printMsg(const QString &msg);
    virtual bool open(QString ip, quint16 port) = 0;
    virtual void close() = 0;
    virtual int send(const QString &data) = 0;

    QString m_src_ip;
    quint16 m_src_port;
    QString m_dst_ip;
    quint16 m_dst_port;

signals:
    void message(const QString &msg);

private:
    bool m_started;

};

class TcpClientSkt : public ClientSkt
{
    Q_OBJECT

public:
    TcpClientSkt(QObject *parent=0);
    virtual ~TcpClientSkt();
    int m_fd;

protected:
    virtual bool open(QString ip, quint16 port);
    virtual void close();
    virtual int send(const QString& data);

signals:
    void onConnectSignal();


private slots:
//    void onConnect();
    void recv();

private:
    QString m_error;
};

class UdpClientSkt : public ClientSkt
{
    Q_OBJECT

public:
    UdpClientSkt(QObject *parent=0);
    virtual ~UdpClientSkt();

protected:
    virtual bool open(QString ip, quint16 port);
    virtual void close();
//    virtual bool open();
//    virtual bool close(void* cookie);
    virtual int send(const QString& data);
//    virtual void close();

private slots:
    void recv();
private:
    int m_fd;
};



#endif

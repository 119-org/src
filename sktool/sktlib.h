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

    bool start(const QString ip, quint16 port);
    void stop();

};

class TcpServerSkt : public ServerSkt
{
    Q_OBJECT

public:
    TcpServerSkt(QObject *parent=0);
    virtual ~TcpServerSkt();

protected:
//    virtual bool open();
//    virtual bool close(void* cookie);
//    virtual void send(void* cookie, const QByteArray& bin);
//    virtual void close();
};

class UdpServerSkt : public ServerSkt
{
    Q_OBJECT

public:
    UdpServerSkt(QObject *parent=0);
    virtual ~UdpServerSkt();

protected:
//    virtual bool open();
//    virtual bool close(void* cookie);
//    virtual void send(void* cookie, const QByteArray& bin);
//    virtual void close();
};
#endif

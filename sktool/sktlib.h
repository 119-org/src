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
    void setAddr(const QString ip) { m_ip = ip;};
    void setPort(quint16 port) {m_port = port;};

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
    virtual void send(void* cookie, const QByteArray& bin);

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
//    virtual bool open();
//    virtual bool close(void* cookie);
//    virtual void send(void* cookie, const QByteArray& bin);
//    virtual void close();
private:
    int m_fd;
};
#endif

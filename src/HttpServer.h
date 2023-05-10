#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>

class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(const QString &chatServerAddress, quint16 chatServerPort, QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest();
    void sendResponse(QTcpSocket *socket, const QByteArray &status, const QByteArray &contentType, const QByteArray &body);
    void serveFile(QTcpSocket *socket, const QString &fileName, const QString &contentType);

private:
    QString m_chatServerAddress = "";
    quint16 m_chatServerPort = 0;
};

#endif // HTTPSERVER_H

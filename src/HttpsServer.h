#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "HttpServer.h"

#include <QSslConfiguration>

class HttpsServer : public HttpServer
{
    Q_OBJECT

public:
    // purposefully passing QSslConfiguration by copy
    explicit HttpsServer(const QString &chatServerAddress, quint16 chatServerPort, QSslConfiguration sslConfiguration, QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QSslConfiguration m_sslConfiguration;
};

#endif // HTTPSERVER_H

#include "HttpsServer.h"

#include <QTcpSocket>
#include <QDateTime>
#include <QResource>
#include <QTextCodec>
#include <QFileInfo>
#include <QMetaEnum>

HttpsServer::HttpsServer(const QString &chatServerAddress, quint16 chatServerPort, QSslConfiguration sslConfiguration, QObject *parent)
    : HttpServer(chatServerAddress, chatServerPort, parent)
    , m_sslConfiguration(sslConfiguration)
{
    setChatServerProtocol("wss");
}

void HttpsServer::incomingConnection(qintptr socketDescriptor)
{
    if (!m_sslConfiguration.isNull()) {
        QSslSocket *sslSocket = new QSslSocket(this);

        if (sslSocket->setSocketDescriptor(socketDescriptor)) {
            connect(sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                    [this, sslSocket](const QList<QSslError> &errors) {
                        for (auto &err: errors) {
                            qCritical() << err;
                        }
                    });

            sslSocket->setSslConfiguration(m_sslConfiguration);

            connect(sslSocket, &QSslSocket::encrypted, this, [this, sslSocket]() {
                addPendingConnection(sslSocket);
                emit newConnection();
            });

            sslSocket->startServerEncryption();
        }
    } else {
        qCritical() << "No valid SSL configuration assigned to HTTPS server, not accepting connection.";
    }
}

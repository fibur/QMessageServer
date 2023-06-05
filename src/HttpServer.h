#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QSslConfiguration>
#include <QTcpServer>

class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    enum Requests {
        LoginRequest,
        RegisterRequest,
        LogoutRequest,
        MessageRequest,
        AuthorizeRequest
    };

    enum Responses {
        AuthorizationEvent,
        LoginEvent,
        MessageEvent,
        InvalidUserEvent,
        UserlistChangeEvent
    };

    Q_ENUM(Requests)
    Q_ENUM(Responses)

    explicit HttpServer(const QString &chatServerAddress, quint16 chatServerPort, QObject *parent = nullptr);

    bool isValueInEnumRange(int value, const QString& enumName);

    // purposefully passing by copy
    void setSslConfiguration(QSslConfiguration sslConfiguration);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void handleRequest();
    void sendResponse(QTcpSocket *socket, const QByteArray &status, const QByteArray &contentType, const QByteArray &body);
    void serveFile(QTcpSocket *socket, const QString &fileName, const QString &contentType);
    void generateEnumsFile();
    QString convertEnumToJs(const QString &enumName);

private:
    QString m_chatServerAddress = "";
    quint16 m_chatServerPort = 0;
    QString m_enumsJsFile = "";

    QSslConfiguration m_sslConfiguration;
};

#endif // HTTPSERVER_H

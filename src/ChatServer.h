#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QDateTime>
#include <QObject>
#include <QSslConfiguration>

class User;
class UserManager;
class HttpServer;
class QWebSocket;
class QWebSocketServer;

class ChatServer : public QObject {

    Q_OBJECT

public:
    explicit ChatServer(QObject *parent = nullptr);

    void setupSSL(const QString &sslCertificate, const QString &sslPrivateKey);

public slots:
    void start(const QString &ip, int httpPort, quint16 port = 12345);

private slots:
    void onNewConnection();
    void handleMessage(const QString &message, QWebSocket *socket);
    void sendUserListChange();

private:
    HttpServer *m_httpServer = nullptr;
    QJsonArray getUserListAsJsonObject(const QList<User *> &list);

    QWebSocketServer *m_webSocketServer = nullptr;
    UserManager *m_userManager = nullptr;
    QSslConfiguration m_sslConfiguration;
};


#endif // CHATSERVER_H

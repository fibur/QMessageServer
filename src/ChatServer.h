#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QDateTime>
#include <QObject>
#include <QWebSocketServer>

class User;
class UserManager;
class HttpServer;
class QWebSocket;

class ChatServer : public QObject {

    Q_OBJECT

public:
    explicit ChatServer(QObject *parent = nullptr);

public slots:
    void start(const QString &ip, int httpPort, quint16 port = 12345);

private slots:
    void onNewConnection();
    void handleMessage(const QString &message, QWebSocket *socket);

private:
    HttpServer *m_httpServer = nullptr;
    QJsonArray getUserListAsJsonObject(const QList<User *> &list);

    void sendUserListChange();

    QWebSocketServer m_webSocketServer{ QStringLiteral("Chat Server"), QWebSocketServer::NonSecureMode };
    UserManager *m_userManager = nullptr;
};


#endif // CHATSERVER_H

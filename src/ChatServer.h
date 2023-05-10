#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QDateTime>
#include <QObject>
#include <QWebSocketServer>

class HttpServer;
class QWebSocket;

class ChatServer : public QObject {

    Q_OBJECT

public:
    explicit ChatServer(QObject *parent = nullptr);

public slots:
    void start(quint16 port = 12345);

private slots:
    void onNewConnection();

private:
    struct UserData {
        QString name;
        QWebSocket *socket;
        QString token;
        QDateTime lastActive;

        bool operator == (const UserData &other);
    };

    HttpServer *m_httpServer = nullptr;

    UserData *findUserByToken(const QString &token);

    UserData *findOfflineUserByToken(const QString &token);

    QJsonArray getUserListAsJsonObject();

    void sendUserListChangeAndClear();

    QWebSocketServer m_webSocketServer{ QStringLiteral("Chat Server"), QWebSocketServer::NonSecureMode };
    QMap<QString, UserData> m_users;
    QList<UserData> m_offlineUsers;
};


#endif // CHATSERVER_H

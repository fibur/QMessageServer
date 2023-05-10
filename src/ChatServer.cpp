#include "ChatServer.h"
#include "HttpServer.h"

#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
{
}

void ChatServer::start(quint16 port)
{
    connect(&m_webSocketServer, &QWebSocketServer::newConnection, this, &ChatServer::onNewConnection);
    qDebug() << "Initiating chat server on port" << port;
    if (m_webSocketServer.listen(QHostAddress::Any, port)) {
        qDebug() << "Chat server started, listening on" << m_webSocketServer.serverAddress() << ":" << m_webSocketServer.serverPort();
        qDebug() << "Initiating HTTP server on port 8080...";

        if (m_httpServer) {
            m_httpServer->deleteLater();
        }

        m_httpServer = new HttpServer(QStringLiteral("77.237.28.186"), m_webSocketServer.serverPort(), this);

        if (m_httpServer->listen(QHostAddress::Any, 8080)) {
            qDebug() << "HTTP server started, listening on" << m_httpServer->serverAddress() << ":" << m_httpServer->serverPort();
        } else {
            qCritical() << "Couldn't start HTTP server on port" << port;
            throw std::runtime_error(m_httpServer->errorString().toStdString());
        }
    } else {
        qCritical() << "Couldn't start chat server on port" << port;
        throw std::runtime_error(m_webSocketServer.errorString().toStdString());
    }
}

void ChatServer::onNewConnection() {
    auto socket = m_webSocketServer.nextPendingConnection();
    connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString &message) {
        const auto request = QJsonDocument::fromJson(message.toUtf8()).object();
        const auto action = request["action"].toString();
        QJsonObject response;
        response["valid"] = true;

        if (action == "authenticate") {
            const auto token = request["token"].toString();
            response["event"] = "authentication";
            auto user = findUserByToken(token);

            if (!user) {
                user = findOfflineUserByToken(token);
            }

            response["valid"] = (user != nullptr);
            if (user) {
                user->socket = socket;
                m_users[user->name] = *user;
                m_offlineUsers.removeOne(*user);
            }

            socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));

            sendUserListChangeAndClear();
        } else if (action == "logout") {
            const auto token = request["token"].toString();
            const auto user = findUserByToken(token);
            if (user) {
                const auto name = user->name;
                user->socket->close();
                m_users.remove(name);
                sendUserListChangeAndClear();
            }
        } else if (action == "login") {
            const auto name = request["name"].toString();
            response["event"] = "login";
            if (!m_users.contains(name)) {
                const auto token = QUuid::createUuid().toString();
                auto &userData = m_users[name];
                userData.socket = socket;
                userData.token = token;
                userData.name = name;
                userData.lastActive = QDateTime::currentDateTime();

                connect(socket, &QWebSocket::disconnected, this, [this, name] {
                    if (m_users.contains(name)) {
                        m_offlineUsers << m_users[name];
                        m_users.remove(name);

                        sendUserListChangeAndClear();
                    }
                });

                response["token"] = token;
                response["username"] = name;

                sendUserListChangeAndClear();
                response["users"] = getUserListAsJsonObject();

                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            } else {
                response["valid"] = false;
                response["error"] = "User already logged in";
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            }
        } else {
            const auto token = request["token"].toString();
            const auto user = findUserByToken(token);
            if (user) {
                user->lastActive = QDateTime::currentDateTime();
                if (action == "message") {
                    const auto target = request["target"].toString();
                    const auto message = request["message"].toString();
                    if (m_users.contains(target)) {
                        response["event"] = "messageEvent";
                        response["sender"] = user->name;
                        response["message"] = message;

                        m_users[target].socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
                    }
                }
            } else {
                response["event"] = "userInvalid";
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            }
        }
    });
}

ChatServer::UserData *ChatServer::findUserByToken(const QString &token) {
    const auto now = QDateTime::currentDateTime();
    for (auto &userData : m_users) {
        if (userData.token == token) {
            if (now.toSecsSinceEpoch() - userData.lastActive.toSecsSinceEpoch() <= 600) {
                return &userData;
            } else {
                userData.socket->close();
                m_users.remove(userData.name);
                break;
            }
        }
    }
    return nullptr;
}

ChatServer::UserData *ChatServer::findOfflineUserByToken(const QString &token) {
    const auto now = QDateTime::currentDateTime();
    for (int i = 0; i < m_offlineUsers.length(); i++) {
        auto &userData = m_offlineUsers[i];
        if (userData.token == token) {
            if (now.toSecsSinceEpoch() - userData.lastActive.toSecsSinceEpoch() <= 600) {
                return &userData;
            } else {
                m_offlineUsers.removeAt(i);
                break;
            }
        }
    }
    return nullptr;
}

QJsonArray ChatServer::getUserListAsJsonObject() {
    QJsonArray userArray;
    const auto now = QDateTime::currentDateTime();
    QStringList toRemove;

    for (const auto &userdata : qAsConst(m_users)) {
        if (now.toSecsSinceEpoch() - userdata.lastActive.toSecsSinceEpoch() <= 600) {
            userArray.append(userdata.name);
        } else {
            if (userdata.socket) {
                QJsonObject response;
                response["event"] = "userInvalid";
                userdata.socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
                userdata.socket->close();
            }
            toRemove << userdata.name;
        }
    }

    for (const auto &user : qAsConst(toRemove)) {
        m_users.remove(user);
    }

    return userArray;
}

void ChatServer::sendUserListChangeAndClear() {
    QJsonArray userArray = getUserListAsJsonObject();

    QJsonObject response;
    response["event"] = "userlistChange";
    response["users"] = userArray;

    const auto &values = m_users.values();
    for (const auto &user : values) {
        if (user.socket) {
            user.socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
        }
    }

    const auto now = QDateTime::currentDateTime();
    for (int i = 0; i < m_offlineUsers.length(); i++) {
        if (now.toSecsSinceEpoch() - m_offlineUsers[i].lastActive.toSecsSinceEpoch() > 600) {
            m_offlineUsers.removeAt(i);
            i--;
        }
    }
}

bool ChatServer::UserData::operator ==(const UserData &other) {
    return other.name == name;
}

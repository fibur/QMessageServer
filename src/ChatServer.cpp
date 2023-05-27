#include "ChatServer.h"
#include "HttpServer.h"
#include "User.h"
#include "UserManager.h"

#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , m_userManager(new UserManager(this))
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

    m_userManager->loadUsers();
    connect(m_userManager, &UserManager::activeUsersChanged, this, &ChatServer::sendUserListChange);
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
            auto user = m_userManager->findUserByToken(token);

            response["valid"] = (user != nullptr);
            if (user) {
                m_userManager->authorizeUser(user, socket);
            }

            socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));

            sendUserListChange();
        } else if (action == "logout") {
            const auto token = request["token"].toString();
            const auto user = m_userManager->findUserByToken(token);
            if (user) {
                m_userManager->deauthorizeUser(user);
            }
        } else if (action == "login") {
            const auto name = request["name"].toString();
            const auto passphrase = ""; // = request["passphrase"].toString(); // TODO: implement this
            response["event"] = "login";

            User* user = m_userManager->authenticateUser(name, passphrase);
            if (user) {
                m_userManager->authorizeUser(user, socket);
                response["token"] = user->token();
                response["username"] = user->name();

                sendUserListChange();
                response["users"] = getUserListAsJsonObject(m_userManager->activeUsers());

                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            } else {
                response["valid"] = false;
                response["error"] = "User or password is invalid.";
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            }
        } else {
            const auto token = request["token"].toString();
            const auto user = m_userManager->findUserByToken(token);
            if (user) {
                m_userManager->authorizeUser(user);
                if (action == "message") {
                    const auto targetName = request["target"].toString();
                    const auto message = request["message"].toString();

                    User* targetUser = m_userManager->findActiveUserByName(targetName);
                    if (targetUser) {
                        response["event"] = "messageEvent";
                        response["sender"] = user->name();
                        response["message"] = message;

                        targetUser->socket()->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
                    }
                }
            } else {
                response["event"] = "userInvalid";
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            }
        }
    });
}

QJsonArray ChatServer::getUserListAsJsonObject(const QList<User*>& list) {
    QJsonArray userArray;

    for (const auto &user : list) {
        userArray.append(user->name());
    }

    return userArray;
}

void ChatServer::sendUserListChange() {
    const auto &activeUsers = m_userManager->activeUsers();
    QJsonArray userArray = getUserListAsJsonObject(activeUsers);

    QJsonObject response;
    response["event"] = "userlistChange";
    response["users"] = userArray;

    for (const auto &user : activeUsers) {
        if (user->socket()) {
            user->socket()->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
        }
    }
}

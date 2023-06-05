#include "ChatServer.h"
#include "HttpServer.h"
#include "User.h"
#include "UserManager.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>
#include <QFile>
#include <QSslKey>

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , m_userManager(new UserManager(this))
{
}

void ChatServer::setupSSL(const QString &sslCertificate, const QString &sslPrivateKey)
{
    QFile certificateFile(sslCertificate);
    if (certificateFile.open(QFile::OpenModeFlag::ReadOnly)) {
        QSslCertificate certificate(&certificateFile);

        QFile privateKeyFile(sslPrivateKey);
        if (privateKeyFile.open(QFile::OpenModeFlag::ReadOnly)) {
            QSslKey privateKey(&privateKeyFile, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);

            m_sslConfiguration = QSslConfiguration();
            m_sslConfiguration.setLocalCertificate(certificate);
            m_sslConfiguration.setPrivateKey(privateKey);

            privateKeyFile.close();

            qDebug() << "Cert file valid:" << !certificate.isNull();
            qDebug() << "Private key valid:" << !privateKey.isNull();
        } else {
            qWarning() << "Couldn't load the SSH certificate file on path" << sslCertificate;
            qWarning() << "Reason:" << privateKeyFile.errorString();
        }

        certificateFile.close();
    } else {
        qWarning() << "Couldn't load the SSH certificate file on path" << sslCertificate;
        qWarning() << "Reason:" << certificateFile.errorString();
    }
}

void ChatServer::start(const QString &ip, int httpPort, quint16 port)
{

    qDebug() << "Initiating chat server on port" << port;
    if (!m_sslConfiguration.isNull()) {
        m_webSocketServer = new QWebSocketServer(QStringLiteral("Chat Server"), QWebSocketServer::SecureMode, this);
        qDebug() << "SSL configuration loaded, running on secure connection";

        m_sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
        m_sslConfiguration.setProtocol(QSsl::TlsV1SslV3);

        m_webSocketServer->setSslConfiguration(m_sslConfiguration);
        connect(m_webSocketServer, &QWebSocketServer::sslErrors, this, [this](const QList<QSslError> &errors) {
            for (const auto &error : errors) {
                qWarning() << error;
            }
        });
    } else {
        m_webSocketServer = new QWebSocketServer(QStringLiteral("Chat Server"), QWebSocketServer::NonSecureMode, this);
    }

    connect(m_webSocketServer, &QWebSocketServer::newConnection, this, &ChatServer::onNewConnection);
    if (m_webSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "Chat server started, listening on" << ip << ":" << m_webSocketServer->serverPort();
        qDebug() << QString("Initiating HTTP server on port %1...").arg(httpPort);

        if (m_httpServer) {
            m_httpServer->deleteLater();
        }

        m_httpServer = new HttpServer(ip, m_webSocketServer->serverPort(), this);
        if (!m_sslConfiguration.isNull()) {
            m_httpServer->setSslConfiguration(m_sslConfiguration);
        }

        if (m_httpServer->listen(QHostAddress::Any, httpPort)) {
            qDebug() << "HTTP server started, listening on" << ip << ":" << m_httpServer->serverPort();
        } else {
            qCritical() << "Couldn't start HTTP server on port" << port;
            throw std::runtime_error(m_httpServer->errorString().toStdString());
        }
    } else {
        qCritical() << "Couldn't start chat server on port" << port;
        throw std::runtime_error(m_webSocketServer->errorString().toStdString());
    }

    m_userManager->loadUsers();
    connect(m_userManager, &UserManager::activeUsersChanged, this, &ChatServer::sendUserListChange);
}

void ChatServer::onNewConnection() {
    auto socket = m_webSocketServer->nextPendingConnection();

    connect(socket, &QWebSocket::textMessageReceived, this, [this, socket](const QString &message) {
        handleMessage(message, socket);
    });
}

void ChatServer::handleMessage(const QString &message, QWebSocket* socket)
{
    const auto request = QJsonDocument::fromJson(message.toUtf8()).object();
    const auto action = request["action"].toInt();
    QJsonObject response;
    response["valid"] = true;

    if (!m_httpServer->isValueInEnumRange(action, "Requests")) {
        response["valid"] = false;
        socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
        return;
    }

    HttpServer::Requests requestType = static_cast<HttpServer::Requests>(action);

    switch (requestType) {

    case HttpServer::LoginRequest:
    case HttpServer::RegisterRequest: {
        const auto name = request["name"].toString();
        const auto password = request["password"].toString();
        response["event"] = HttpServer::Responses::LoginEvent;

        if (name.isEmpty() || password.isEmpty()) {
            response["valid"] = false;
            response["error"] = "Please fill all fields.";
            socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));

            return;
        }

        User* user = nullptr;
        if (requestType == HttpServer::RegisterRequest) {
            if (!m_userManager->saveUser(name, password)) {
                response["valid"] = false;
                response["error"] = "Username already exists.";
                socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));

                return;
            } else {
                user = m_userManager->users().last();
            }
        } else {
            user = m_userManager->authenticateUser(name, password);
        }

        if (user) {
            if (!user->token().isEmpty()) {
                user->setToken("");
                if (user->socket()) {
                    user->socket()->close();
                }
            }

            m_userManager->authorizeUser(user, socket);
            user->setPublicKey(request["pubKey"].toString());

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

        break;
    }
    case HttpServer::LogoutRequest: {
        const auto token = request["token"].toString();
        const auto user = m_userManager->findUserByToken(token);
        if (user) {
            m_userManager->deauthorizeUser(user);
        }

        break;
    }

    case HttpServer::MessageRequest: {
        const auto token = request["token"].toString();
        const auto user = m_userManager->findUserByToken(token);
        if (user) {
            m_userManager->authorizeUser(user);
            const auto targetId = request["target"].toString();
            const auto message = request["message"].toString();

            User* targetUser = m_userManager->findActiveUserById(targetId);
            if (targetUser) {
                response["event"] = HttpServer::Responses::MessageEvent;
                response["sender"] = user->id();
                response["message"] = message;

                targetUser->socket()->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
            }
        }

        break;
    }
    case HttpServer::AuthorizeRequest:{
        const auto token = request["token"].toString();
        response["event"] = HttpServer::Responses::AuthorizationEvent;
        auto user = m_userManager->findUserByToken(token);

        response["valid"] = (user != nullptr);
        if (user) {
            m_userManager->authorizeUser(user, socket);
        } else {
            response["event"] = HttpServer::Responses::InvalidUserEvent;
            socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
        }

        socket->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));

        sendUserListChange();

        break;
    }
    }
}

QJsonArray ChatServer::getUserListAsJsonObject(const QList<User*>& list) {
    QJsonArray userArray;

    for (const auto &user : list) {
        QJsonObject userObj;

        userObj["id"] = user->id();
        userObj["name"] = user->name();
        userObj["publicKey"] = user->publicKey();

        userArray.append(userObj);
    }

    return userArray;
}

void ChatServer::sendUserListChange() {
    const auto &activeUsers = m_userManager->activeUsers();
    QJsonArray userArray = getUserListAsJsonObject(activeUsers);

    QJsonObject response;
    response["event"] = HttpServer::Responses::UserlistChangeEvent;
    response["users"] = userArray;

    for (const auto &user : activeUsers) {
        if (user->socket()) {
            user->socket()->sendTextMessage(QString::fromUtf8(QJsonDocument(response).toJson()));
        }
    }
}

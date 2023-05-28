#include "User.h"
#include <QWebSocket>

User::User(const QString &id, const QString &name, const QString &password, QObject *parent)
    : QObject{parent}
    , m_id(id)
    , m_name(name)
    , m_password(password)
{

}

QString User::id() const
{
    return m_id;
}

QString User::name() const
{
    return m_name;
}

QString User::publicKey() const
{
    return m_publicKey;
}

void User::setPublicKey(const QString &publicKey)
{
    m_publicKey = publicKey;
}

QString User::token() const
{
    return m_token;
}

void User::setToken(const QString &token)
{
    m_token = token;
}

QDateTime User::lastActive() const
{
    return m_lastActive;
}

void User::setLastActive(const QDateTime &lastActive)
{
    m_lastActive = lastActive;
}

QWebSocket *User::socket() const
{
    return m_socket;
}

void User::setSocket(QWebSocket *socket)
{
    if (m_socket) {
        disconnect(m_socket, nullptr, this, nullptr);
        m_socket->close();
        emit userDisconnected();
    }

    if (socket) {
        connect(socket, &QWebSocket::disconnected, this, &User::onSocketDisconnected);
    }
    m_socket = socket;
}

void User::onSocketDisconnected()
{
    m_socket = nullptr;
    emit userDisconnected();
}

QString User::password() const
{
    return m_password;
}

void User::setPassword(const QString &password)
{
    m_password = password;
}

void User::setName(const QString &name)
{
    m_name = name;
}

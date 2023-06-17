#include "User.h"
#include "UserManager.h"
#include "quuid.h"

#include <QCryptographicHash>
#include <QSqlQuery>
#include <QVariant>
#include <QRandomGenerator>
#include <QSqlError>

UserManager::UserManager(QObject *parent) : QObject(parent) {
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName("users.db");

    if (!m_database.open()) {
        qFatal("Failed to connect to database");
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS users (id TEXT PRIMARY KEY, name TEXT, password TEXT)");
}

void UserManager::loadUsers() {
    qDeleteAll(m_users);
    m_users.clear();

    QSqlQuery query("SELECT * FROM users");
    while (query.next()) {
        addUser(query.value("id").toString(),
                query.value("name").toString(),
                query.value("password").toString());
    }
}

bool UserManager::saveUser(const QString &name, const QString &password) {
    User *alreadyExisting = findUserByName(name);

    if (alreadyExisting) {
        return false;
    }

    QString id = generateUniqueID();

    QSqlQuery query;
    query.prepare("INSERT INTO users (id, name, password) VALUES (:id, :name, :password)");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":password", password);
    query.exec();

    if (query.lastError().type() == QSqlError::NoError) {
        addUser(id, name, password);

        return true;
    }

    return false;
}

User *UserManager::authenticateUser(const QString &name, const QString &password) {
    for (User* user : qAsConst(m_users)) {
        if (user->name().toLower() == name.toLower() && user->password() == password) {
            return user;
        }
    }
    return nullptr;
}

User *UserManager::findUserByToken(const QString &token)
{
    deauthorizeInactiveUsers();
    const auto result = std::find_if(m_users.begin(), m_users.end(), [&token](User* user){
        if (user->token() == token) {
            return true;
        }

        return false;
    });

    if (result == m_users.end()) {
        return nullptr;
    }

    return *result;
}

User *UserManager::findUserByName(const QString &name)
{
    const auto result = std::find_if(m_users.begin(), m_users.end(), [&name](User* user){
        if (user->name().toLower() == name.toLower()) {
            return true;
        }

        return false;
    });

    if (result == m_users.end()) {
        return nullptr;
    }

    return *result;
}

User *UserManager::findActiveUserById(const QString &id)
{
    const auto &activeUserList = activeUsers();

    const auto result = std::find_if(activeUserList.begin(), activeUserList.end(), [&id](User* user){
        if (user->id() == id) {
            return true;
        }

        return false;
    });

    if (result == activeUserList.end()) {
        return nullptr;
    }

    return *result;
}

void UserManager::deauthorizeUser(User *user)
{
    if (user) {
        user->setToken("");
        user->setSocket(nullptr);
        emit activeUsersChanged();
    }
}

void UserManager::authorizeUser(User *user, QWebSocket *socket)
{
    if (user) {
        if (socket) {
            user->setSocket(socket);
            emit activeUsersChanged();
        }

        if (user->token().isEmpty()) {
            const auto token = QUuid::createUuid().toString();
            user->setToken(token);
        }

        user->setLastActive(QDateTime::currentDateTime());
    }
}

QString UserManager::generateUniqueID() {
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int idLength = 6;

    QString id;
    for (int i = 0; i < idLength; ++i) {
        int index = QRandomGenerator::global()->bounded(static_cast<int>(chars.length()));
        id.append(chars.at(index));
    }

    for (User* user : qAsConst(m_users)) {
        if (user->id() == id) {
            return generateUniqueID();
        }
    }

    return id;
}

QByteArray UserManager::hashPassword(const QString &password) {
    QByteArray passwordBytes = password.toUtf8();
    QByteArray hashedBytes = QCryptographicHash::hash(passwordBytes, QCryptographicHash::Sha256);
    return hashedBytes.toHex();
}

QString UserManager::generatePublicKey(const QString &username, const QString &hashedPassword)
{
    QString data = username + hashedPassword;
    QByteArray dataBytes = data.toUtf8();

    QByteArray publicKey = QCryptographicHash::hash(dataBytes, QCryptographicHash::Sha256);
    QString publicKeyHex = publicKey.toHex();

    return publicKeyHex;
}

void UserManager::deauthorizeInactiveUsers()
{
    QDateTime now = QDateTime::currentDateTime();
    std::for_each(m_users.begin(), m_users.end(), [&now, this](User* user){
        if (!user->token().isEmpty() && now.toSecsSinceEpoch() - user->lastActive().toSecsSinceEpoch() > 600) {
            deauthorizeUser(user);
        }
    });
}

void UserManager::addUser(const QString &id, const QString &name, const QString &publicKey)
{
    User* user = new User(id,
                          name,
                          publicKey,
                          this);
    m_users.append(user);

    connect(user, &User::userDisconnected, this, &UserManager::activeUsersChanged);
}

const QList<User *> &UserManager::users() const
{
    return m_users;
}

QList<User *> UserManager::activeUsers()
{
    deauthorizeInactiveUsers();

    QList<User*> result;
    std::copy_if(m_users.begin(), m_users.end(), std::back_inserter(result), [](User* user){
        return user->socket();
    });

    return result;
}

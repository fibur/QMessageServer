#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QSqlDatabase>
#include <QObject>
#include <QDateTime>

class QWebSocket;
class User;

class UserManager : public QObject {

    Q_OBJECT

public:
    explicit UserManager(QObject *parent = nullptr);

    void loadUsers();
    bool saveUser(const QString& name, const QString& password);

    User *authenticateUser(const QString& name, const QString& password);
    User *findUserByToken(const QString& token);
    User *findUserByName(const QString& name);

    User *findActiveUserById(const QString& id);

    void deauthorizeUser(User *user);
    void authorizeUser(User *user, QWebSocket* socket = nullptr);

    const QList<User *>& users() const;
    QList<User *> activeUsers();

signals:
    void activeUsersChanged();

private:
    QString generateUniqueID();
    QByteArray hashPassword(const QString& password);

    QString generatePublicKey(const QString& username, const QString& hashedPassword);

    void deauthorizeInactiveUsers();
    void addUser(const QString &id, const QString& name, const QString& publicKey);

private:
    QSqlDatabase m_database;
    QList<User*> m_users;
};

#endif // USERMANAGER_h

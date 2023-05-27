#ifndef USER_H
#define USER_H

#include <QDateTime>
#include <QObject>

class QWebSocket;

class User : public QObject {
    Q_OBJECT

public:
    explicit User(const QString &id, const QString &name, const QString &passphrase, QObject* parent = nullptr);

    QString id() const;

    QString name() const;
    void setName(const QString &name);

    QString password() const;
    void setPassword(const QString &password);

    QString token() const;
    void setToken(const QString &token);

    QDateTime lastActive() const;
    void setLastActive(const QDateTime &lastActive);

    QWebSocket *socket() const;
    void setSocket(QWebSocket *socket);

signals:
    void userDisconnected();

private slots:
    void onSocketDisconnected();

private:
    QString m_id = "";
    QString m_name = "";
    QString m_password = "";

    QString m_token = "";
    QDateTime m_lastActive;

    QWebSocket* m_socket = nullptr;
};

#endif // USER_H

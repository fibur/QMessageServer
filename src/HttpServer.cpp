#include "HttpServer.h"

#include <QTcpSocket>
#include <QDateTime>
#include <QResource>
#include <QTextCodec>
#include <QFileInfo>
#include <QMetaEnum>
#include <QMimeDatabase>

HttpServer::HttpServer(const QString &chatServerAddress, quint16 chatServerPort, QObject *parent)
    : QTcpServer(parent)
    , m_chatServerAddress(chatServerAddress)
    , m_chatServerPort(chatServerPort)
{
    generateEnumsFile();

    connect(this, &QTcpServer::newConnection, this, &HttpServer::setupPendingSocket);
}

bool HttpServer::isValueInEnumRange(int value, const QString &enumName)
{
    const QMetaObject* metaObj = &HttpServer::staticMetaObject;

    int enumIndex = metaObj->indexOfEnumerator(enumName.toLatin1());
    if (enumIndex == -1) {
        return false;
    }

    QMetaEnum metaEnum = metaObj->enumerator(enumIndex);
    int minValue = metaEnum.keyToValue(metaEnum.key(0));
    int maxValue = metaEnum.keyToValue(metaEnum.key(metaEnum.keyCount() - 1));

    return (value >= minValue && value <= maxValue);
}

void HttpServer::setChatServerProtocol(const QString &protocolString)
{
    m_chatServerProtocol = protocolString;
}

void HttpServer::handleRequest()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    if (!m_redirectTo.isEmpty()) {
        static const QString &redirectResponse = QStringLiteral("HTTP/1.1 301 Moved Permanently\r\n"
                                                                "Location: %1\r\n"
                                                                "Connection: close\r\n\r\n");

        socket->write(redirectResponse.arg(m_redirectTo).toUtf8());

        socket->waitForBytesWritten();
    } else {
        QByteArray requestData = socket->readAll();
        QList<QByteArray> requestLines = requestData.split('\n');
        if (requestLines.isEmpty()) {
            return;
        }

        QList<QByteArray> requestParts = requestLines.first().split(' ');
        if (requestParts.length() != 3) {
            return;
        }

        QByteArray method = requestParts[0];
        QByteArray path = requestParts[1];


        if (method == "GET") {
            if (path == "/enums.mjs") {
                sendResponse(socket, "200 OK", "text/javascript", m_enumsJsFile.toUtf8());
            } else {
                if (path == "/") {
                    path = "/index.html";
                }

                serveFile(socket, path, QMimeDatabase().mimeTypeForFile(path).name());
            }

            return;
        } else {
            sendResponse(socket, "404 Not Found", "text/plain", "Page not found");
        }
    }

    socket->disconnectFromHost();
}

void HttpServer::sendResponse(QTcpSocket *socket, const QByteArray &status, const QByteArray &contentType, const QByteArray &body)
{
    QByteArray response;
    response.append("HTTP/1.1 " + status + "\r\n");
    response.append("Content-Type: " + contentType + "\r\n");
    response.append("Content-Length: " + QByteArray::number(body.length()) + "\r\n");
    response.append("Date: " + QDateTime::currentDateTime().toString(Qt::RFC2822Date) + "\r\n");
    response.append("\r\n");
    response.append(body);

    socket->write(response);
    socket->waitForBytesWritten();
}

void HttpServer::serveFile(QTcpSocket *socket, const QString &fileName, const QString &contentType)
{
    QFile file("html" + fileName);

    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sendResponse(socket, "404 Not Found", "text/plain", "File not found");
        return;
    }

    QString fileContent = file.readAll();

    fileContent.replace("%SERVER_PROTOCOL%", m_chatServerProtocol);
    fileContent.replace("%SERVER_ADDRESS%", m_chatServerAddress);
    fileContent.replace("%SERVER_PORT%", QString::number(m_chatServerPort));

    sendResponse(socket, "200 OK", contentType.toUtf8(), fileContent.toUtf8());

    file.close();
}

void HttpServer::generateEnumsFile()
{
    m_enumsJsFile += convertEnumToJs("Requests");
    m_enumsJsFile += convertEnumToJs("Responses");
}

QString HttpServer::convertEnumToJs(const QString &enumName)
{
    QString enumContents = "export const %1 = {";

    enumContents = enumContents.arg(enumName);
    enumContents += "\n";

    const QMetaObject &metaObj = HttpServer::staticMetaObject;
    int enumIndex = metaObj.indexOfEnumerator(enumName.toStdString().c_str());
    if (enumIndex != -1) {
        QMetaEnum metaEnum = metaObj.enumerator(enumIndex);
        for (int i = 0; i < metaEnum.keyCount(); ++i) {
            QString enumValue = metaEnum.key(i);
            enumContents += QString("%1: %2,\n").arg(enumValue).arg(i);
        }
    }

    enumContents += "};\n";

    return enumContents;
}

void HttpServer::setupPendingSocket()
{
    QTcpSocket *socket = nextPendingConnection();
    if (!socket) {
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::handleRequest);
}

void HttpServer::setRedirectTo(const QString &redirectTo)
{
    m_redirectTo = redirectTo;
}

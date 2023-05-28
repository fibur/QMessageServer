#include "HttpServer.h"

#include <QTcpSocket>
#include <QDateTime>
#include <QResource>
#include <QTextCodec>
#include <QFileInfo>
#include <QMetaEnum>

HttpServer::HttpServer(const QString &chatServerAddress, quint16 chatServerPort, QObject *parent)
    : QTcpServer(parent)
    , m_chatServerAddress(chatServerAddress)
    , m_chatServerPort(chatServerPort)
{
    generateEnumsFile();
}

bool HttpServer::isValueInEnumRange(int value, const QString &enumName)
{
    const QMetaObject* metaObj = &HttpServer::staticMetaObject;  // Replace "YourClass" with the appropriate class name

    int enumIndex = metaObj->indexOfEnumerator(enumName.toLatin1());
    if (enumIndex == -1) {
        // Enum not found
        return false;
    }

    QMetaEnum metaEnum = metaObj->enumerator(enumIndex);
    int minValue = metaEnum.keyToValue(metaEnum.key(0));
    int maxValue = metaEnum.keyToValue(metaEnum.key(metaEnum.keyCount() - 1));

    return (value >= minValue && value <= maxValue);
}

void HttpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &HttpServer::handleRequest);
}

void HttpServer::handleRequest()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket) {
        return;
    }

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

    if (method == "GET" && (path == "/" || path == "/index.html")) {
        serveFile(socket, ":/index.html", "text/html");
    } else if (method == "GET" && path == "/script.js") {
        serveFile(socket, ":/script.js", "text/javascript");
    } else if (method == "GET" && path == "/style.css") {
        serveFile(socket, ":/style.css", "text/css");
    } else if (method == "GET" && path == "/jsrsasign-latest-all-min.js") {
        serveFile(socket, ":/jsrsasign-latest-all-min.js", "text/css");
    } else if (method == "GET" && path == "/enums.js") {
        sendResponse(socket, "200 OK", "text/javascript", m_enumsJsFile.toUtf8());
    } else {
        sendResponse(socket, "404 Not Found", "text/plain", "Page not found");
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
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        sendResponse(socket, "404 Not Found", "text/plain", "File not found");
        return;
    }

    QString fileContents = file.readAll();

    fileContents.replace("%SERVER_ADDRESS%", m_chatServerAddress);

    fileContents.replace("%SERVER_PORT%", QString::number(m_chatServerPort));
    sendResponse(socket, "200 OK", contentType.toUtf8(), fileContents.toUtf8());
}

void HttpServer::generateEnumsFile()
{
    m_enumsJsFile += convertEnumToJs("Requests");
    m_enumsJsFile += convertEnumToJs("Responses");
}

QString HttpServer::convertEnumToJs(const QString &enumName)
{
    QString enumContents = "const %1 = {";

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

#include <QCommandLineParser>
#include <QCoreApplication>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;

    parser.setApplicationDescription("QMessageServer serves as both websocket chat server, and http server for user interface.");
    parser.addHelpOption();

    QCommandLineOption chatServerPortOption(QStringList() << "chatServerPort" << "csp" << "cp",
                                            "Set the chat port.", "port", "12345");
    parser.addOption(chatServerPortOption);

    QCommandLineOption httpServerPortOption(QStringList() << "httpServerPort" << "httpPort" << "hp",
                                            "Set the HTTP server port.", "port", "8080");
    parser.addOption(httpServerPortOption);

    QCommandLineOption httpsServerPortOption(QStringList() << "httpsServerPort" << "httpsPort" << "hsp",
                                             "Set the HTTPS server port.", "port", "8443");
    parser.addOption(httpsServerPortOption);

    QCommandLineOption serverIpOption(QStringList() << "serverIp" << "ip",
                                      "Set the server IP address.", "ip", "localhost");

    parser.addOption(serverIpOption);
    QCommandLineOption sslCertificateOption(QStringList() << "sslCertificate" << "sslcert",
                                            "Set the server's SSL certificate,", "sslCertificate file", "");

    parser.addOption(sslCertificateOption);
    QCommandLineOption sslPrivateKeyOption(QStringList() << "sslPrivateKey" << "sslprvkey",
                                           "Set the server IP address.", "sslPrivateKey file", "");

    QCommandLineOption disableHttpsOption(QStringList() << "disableHttps",
                                         "Disable HTTPS encryption.");
    parser.addOption(disableHttpsOption);

    QCommandLineOption disableWssOption(QStringList() << "disableWss",
                                       "Disable WSS (WebSocket over HTTPS).");
    parser.addOption(disableWssOption);

    parser.addOption(sslPrivateKeyOption);

    parser.process(app);

    QString chatServerPort = parser.value(chatServerPortOption);
    QString httpServerPort = parser.value(httpServerPortOption);
    QString httpsServerPort = parser.value(httpsServerPortOption);
    QString serverIp = parser.value(serverIpOption);

    QString sslCertificate = parser.value(sslCertificateOption);
    QString sslPrivateKey = parser.value(sslPrivateKeyOption);

    bool disableHttps = parser.isSet(disableHttpsOption);
    bool disableWss = parser.isSet(disableWssOption);

    ChatServer server;
    if (!sslCertificate.isEmpty() && !sslPrivateKey.isEmpty()) {
        server.setupSSL(sslCertificate, sslPrivateKey);
    }

    qDebug() << "test" << disableHttps << disableWss;

    server.start(serverIp, httpServerPort.toInt(), httpsServerPort.toInt(), chatServerPort.toInt(),
                 disableHttps, disableWss);

    return app.exec();
}

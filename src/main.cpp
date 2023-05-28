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

    QCommandLineOption httpServerPortOption(QStringList() << "httpServerPort" << "hsp" << "hp",
                                            "Set the HTTP server port.", "port", "8080");
    parser.addOption(httpServerPortOption);

    QCommandLineOption serverIpOption(QStringList() << "serverIp" << "ip",
                                      "Set the server IP address.", "ip", "localhost");
    parser.addOption(serverIpOption);
    parser.process(app);

    QString chatServerPort = parser.value(chatServerPortOption);
    QString httpServerPort = parser.value(httpServerPortOption);
    QString serverIp = parser.value(serverIpOption);

    ChatServer server;
    server.start(serverIp, httpServerPort.toInt(), chatServerPort.toInt());

    return app.exec();
}

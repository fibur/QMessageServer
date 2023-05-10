#include <QCoreApplication>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    ChatServer server;
    server.start();

    return app.exec();
}

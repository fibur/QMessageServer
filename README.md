# QMessageServer

QMessageServer is a chat server application that serves as both a WebSocket and HTTP/HTTPS server. The application provides access to a simple chat server with a login/register system and end-to-end encryption on the client-side. It is built using the Qt framework.

## Features

- WebSocket chat server
- HTTP and HTTPS server for user interface
- User registration and login system
- End-to-End Encryption
- Server-side SSL configuration

## Dependencies

- Qt 5.15 or later

## Installation

Clone the repository:

```
git clone https://github.com/fibur/QMessageServer.git
cd QMessageServer
```

Build the project using qmake and make:

```
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

After building the project, you can start the server by executing the built binary. The server supports command-line options to configure different aspects such as ports, server IP, and SSL certificates:

```
./QMessageServer -chatServerPort 12345 -httpServerPort 8080 -httpsServerPort 8443 -serverIp localhost -sslCertificate path/to/certificate -sslPrivateKey path/to/privatekey
```

### Command Line Options

- `-chatServerPort`, `-csp`, `-cp`: Set the chat server port (default: 12345).
- `-httpServerPort`, `-httpPort`, `-hp`: Set the HTTP server port (default: 8080).
- `-httpsServerPort`, `-httpsPort`, `-hsp`: Set the HTTPS server port (default: 8443).
- `-serverIp`, `-ip`: Set the server IP address (default: localhost).
- `-sslCertificate`, `-sslcert`: Set the server's SSL certificate file path.
- `-sslPrivateKey`, `-sslprvkey`: Set the server's SSL private key file path.

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](https://choosealicense.com/licenses/mit/)

# QMessageServer

QMessageServer is a chat server application that serves as both a WebSocket and HTTP/HTTPS server. The application provides access to a simple chat server with a login/register system and end-to-end encryption on the client-side. It is built using the Qt framework.

## Overview
### Features

- WebSocket chat server
- HTTP and HTTPS server for user interface
- User registration and login system
- End-to-End Encryption
- Server-side SSL configuration

### Dependencies

- Qt 5.15 or later

### Installation

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

### Usage

#### Backend
After building the project, you can start the server by executing the built binary. The server supports command-line options to configure different aspects such as ports, server IP, and SSL certificates:

```
./QMessageServer -chatServerPort 12345 -httpServerPort 8080 -httpsServerPort 8443 -serverIp localhost -sslCertificate path/to/certificate -sslPrivateKey path/to/privatekey
```
#### Server file Command Line Options

- `-chatServerPort`, `-csp`, `-cp`: Set the chat server port (default: 12345).
- `-httpServerPort`, `-httpPort`, `-hp`: Set the HTTP server port (default: 8080).
- `-httpsServerPort`, `-httpsPort`, `-hsp`: Set the HTTPS server port (default: 8443).
- `-serverIp`, `-ip`: Set the server IP address (default: localhost).
- `-sslCertificate`, `-sslcert`: Set the server's SSL certificate file path.
- `-sslPrivateKey`, `-sslprvkey`: Set the server's SSL private key file path.

#### Frontend
Server loads HTML dynamically, from `{workinkg-directory}`/html folder. You have to provide frontend by your own, or use content from the `exampleHTML` folder, which provides full functionality, with simple UI. If you want to create it by your own, then below you can find basic informations about communication workflow.


## User Management Workflow

The `UserManager` class is responsible for managing the users of the chat application. This class takes care of creating, loading, saving, authorizing, and de-authorizing users. It also handles user authentication.

### Initialization

When a `UserManager` object is created, it connects to an SQLite database (`users.db`), and if the database doesn't exist, it creates one. It also creates a table named `users` if it doesn't already exist.

### Loading Users

The `loadUsers` method is responsible for loading the users from the database into memory.

### Saving Users

The `saveUser` method saves a new user to the database if it does not exist already.

### User Authentication

The `authenticateUser` method checks if a user with the given name and password exists in memory.

### User Authorization

`UserManager` also handles user authorization by generating a unique token for an authenticated user and setting the last active time.

### User Deauthorization

It also deauthorizes inactive users by setting their token to an empty string and their socket to null.

### Generating Unique IDs

Unique IDs are generated for new users with `generateUniqueID`.

### Password Hashing

Passwords are hashed using SHA-256 before being stored in the database.

### User Lookup

Users can be looked up by their token, name, or ID using the respective methods.

### Active Users

The class provides a method to get a list of currently active users.

## Communication with the WebSocket Server

This document describes the communication process with a WebSocket server which uses the provided QWebSocket message handling method.

### Communication Overview

1. Client sends a JSON request to the server via WebSocket.
2. Server processes the request and sends back a JSON response.

### Request Format

The request sent by the client should be a JSON object containing at least an `action` field. The `action` field should be an integer corresponding to the type of request.

```json
{
    "action": 1,
    "name": "john_doe",
    "password": "password123"
}
```

### Supported Actions

The `action` field in the request can have one of the following values:

- `0` or `Requests.LoginRequest`: Login Request
- `1` or `Requests.RegisterRequest`: Register Request
- `2` or `Requests.LogoutRequest`: Logout Request
- `3` or `Requests.MessageRequest`: Message Request
- `4` or `Requests.AuthorizeRequest`: Authorize Request

The server utilizes an internal enum, `HttpServer::Requests`, to map these values to the request types.

#### 1. Login Request (`action` = `0` or `Requests.LoginRequest`)

Client sends their username and password to login.

Fields:
- `action`: 0
- `name`: username
- `password`: password

#### 2. Register Request (`action` = 1 or `Requests.RegisterRequest`)

Client sends their desired username and password to register a new account.

Fields:
- `action`: 1
- `name`: desired username
- `password`: desired password

#### 3. Logout Request (`action` = 2 or `Requests.LogoutRequest`)

Client sends a request to log out.

Fields:
- `action`: 2
- `token`: authentication token

#### 4. Message Request (`action` = 3 or `Requests.MessageRequest`)

Client sends a message to another user.

Fields:
- `action`: 3
- `token`: authentication token
- `target`: recipient user ID
- `message`: message content

#### 5. Authorize Request (`action` = 4 or `Requests.AuthorizeRequest`)

Client sends a token to authorize itself.

Fields:
- `action`: 4
- `token`: authentication token

### Response Format

The server responds with a JSON object. The object always contains a `valid` field which indicates whether the request was processed successfully or not.

Example response JSON for successful login:

```json
{
    "valid": true,
    "event": 1, // HttpServer::Responses::LoginEvent or Responses.LoginEvent from enums.js
    "token": "auth_token",
    "username": "john_doe",
    "users": [ /* list of users */ ]
}
```

Example response JSON for failed login:

```json
{
    "valid": false,
    "error": "User or password is invalid."
}
```

### Error Handling

If an error occurs during the processing of the request, the response JSON will contain a `valid` field set to `false`, and an `error` field containing the error message.

### Connection Closing

The connection may be closed by the server if a user logs out or if a user logs in from another device. The client should be prepared to handle the connection being closed by the server.

## HTTP Server Additional Features

## Server files
All server files are provided dynamically from html folder from working dir. Additionally, server provides virtual file named `enum.js` in root location, providing all needed enum values from C++ dynamically to avoid usage of magic numbers.

### Replacing Variables in JavaScript Files

The HTTP server processes JavaScript files and replaces the following placeholders with actual values:

- `%SERVER_ADDRESS%`: Replaced with the WebSocket chat server's address.
- `%SERVER_PORT%`: Replaced with the WebSocket chat server's port.
- `%SERVER_PROTOCOL%`: Replaced with either `ws` (WebSocket) or `wss` (WebSocket Secure) based on the server's configuration.

### Providing Action Type Enumerations

The HTTP server provides a list of action type enumerations at the URL `http(s)://<address>/enums.js`. This is a static constant list of values behaving as an enum, providing all message action types.

Example usage:

```javascript
// Load the enums from the server
fetch('http(s)://<address>/enums.js')
  .then(response => response.json())
  .then(enums => {
      console.log(enums); // Logs the list of action types
  });
```

## Examples
Folder `exampleHTML` contains fully functional HTML content example, which is compatible with server implementation. Feel free to adjust it to your needs.
## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[MIT](https://choosealicense.com/licenses/mit/)

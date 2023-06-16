const serverAddress = "%SERVER_PROTOCOL%://%SERVER_ADDRESS%:%SERVER_PORT%";

let publicKey = "";
let privateKey = "";
let myName = null;
let socket = null;
let token = null;
let currentUser = null;
let users = {};
let messageHistory = {};
let unreadMessages = {};

init();

function login() {
  const formError = document.getElementById("formError");
  formError.textContent = "";
  
  const username = document.getElementById("username").value.trim();
  const password = hashPassword(document.getElementById("password").value.trim());
  const register = document.getElementById("registerCheckbox").checked;
  const confirmPassword = hashPassword(document.getElementById("confirmPassword").value.trim());

  if (!username || !password || (register && !confirmPassword)) {
    formError.textContent = "Please fill in all fields.";
    return;
  }

  if (register && password !== confirmPassword) {
    formError.textContent = "Passwords do not match.";
    return;
  }

  showLoadingIndicator();

  const handleKeys = (keypair) => {
    privateKey = keypair ? keypair.prvKeyObj : privateKey;
    publicKey = keypair ? keypair.pubKeyObj : publicKey;
    if (socket) {
      socket.send(JSON.stringify({ action: register ? Requests.RegisterRequest : Requests.LoginRequest, name: username, password, pubKey: KEYUTIL.getPEM(publicKey) }));
    }
  };

  if (!privateKey || !publicKey) {
    generateRSAKeys(username, password).then(handleKeys).catch(console.error).finally(hideLoadingIndicator);
  } else {
    handleKeys();
  }
}

function displayUsers() {
  const userList = document.getElementById("userList");
  userList.innerHTML = "";
  
  Object.values(users).forEach(user => {
    if (user.name !== myName) {
      const li = document.createElement("li");
      li.textContent = user.name + (unreadMessages[user.id] ? ` (${unreadMessages[user.id]})` : "");
      li.classList.add("user-item");

      if (currentUser && user.id === currentUser.id) {
        li.classList.add("selected");
      }

      li.onmouseover = () => li.classList.add("hovered");
      li.onmouseout = () => li.classList.remove("hovered");

      li.onclick = () => handleUserClick(user);

      userList.appendChild(li);
    }
  });

  if (!Object.values(users).some(user => user === currentUser)) {
    currentUser = null;
    document.getElementById("messageHistory").innerHTML = "";
    document.getElementById("messages").querySelector("h2").textContent = "Messages";
  }
}

function handleUserClick(user) {
  currentUser = user;
  unreadMessages[user.id] = 0;

  const messageContainer = document.getElementById("messageHistory");
  messageContainer.innerHTML = "";

  const messages = messageHistory[user.id] || [];
  messages.forEach(msg => {
    const isSentByCurrentUser = msg.sender === myName;
    addMessageToHistoryDiv(msg.message, ["message", isSentByCurrentUser ? "sent" : "received"]);
  });

  document.getElementById("messages").querySelector("h2").textContent = `Messages - ${currentUser.name}`;
  displayUsers();
}

function addMessageToHistoryDiv(message, messageTypes) {
  const messageContainer = document.getElementById("messageHistory");
  const lastMessage = messageContainer.lastElementChild;

  if (!lastMessage) {
    const messageDiv = document.createElement("div");
    for (let classToAdd of messageTypes) {
      messageDiv.classList.add(classToAdd);
    }

    messageDiv.textContent = message;
    messageContainer.appendChild(messageDiv);
    return
  }

  for (let className of messageTypes) {
      if (!lastMessage.classList.contains(className)) {
          const messageDiv = document.createElement("div");
          for (let classToAdd of messageTypes) {
            messageDiv.classList.add(classToAdd);
          }

          messageDiv.textContent = message;
          messageContainer.appendChild(messageDiv);
          return;
      }
  }

  lastMessage.innerHTML += "<br>" + message;
}

function sendMessage() {
  const messageValue = document.getElementById("message").value;

  if (socket && token && messageValue) {
    if (!currentUser) {
      alert("Please select an user to chat with.");
      return;
    }

    var message = "Couldn't encrypt this message, reason:";
    let messageType = ["message", "sent"];

    try {
      message = encryptMessage(currentUser.publicKey, messageValue); 
      socket.send(JSON.stringify({ action: Requests.MessageRequest, token: token, target: currentUser.id, message: message }));

      if (!messageHistory[currentUser.id]) {
        messageHistory[currentUser.id] = [];
      }
  
      messageHistory[currentUser.id].push({ sender: myName, message: messageValue });
      message = messageValue;
    } catch (exception) {
      message += exception;
      messageType.append("error");
    }

    addMessageToHistoryDiv(message, messageType);
    document.getElementById("message").value = "";
  }
}

function logout() {
  if (socket && token) {
    socket.send(JSON.stringify({ action: Requests.LogoutRequests, token: token }));
    sessionStorage.clear();
    document.getElementById("loginForm").classList.remove("hidden");
    document.getElementById("chat").classList.add("hidden");
    socket = null
    token = null;
    currentUser = null;
    myName = null;
    location.reload();
  }
}

function findUserById(id) {
  for (const user of users) {
    if (user.id === id) {
      return user;
    }
  }

  return null;
}

function handleMessage(data) {
  if (data.sender && data.message) {
    if (!messageHistory[data.sender]) {
      messageHistory[data.sender] = [];
    }

    const user = findUserById(data.sender);
    if (!user) {
      return;
    }

    let decryptedMessage = "Couldn't decrypt this message, reason:";
    let error = false;
    try {
      decryptedMessage = decryptMessage(privateKey, data.message);
      messageHistory[data.sender].push({ sender: user.name, message: decryptedMessage });
    } catch (exception) {
      decryptedMessage += exception;
      error = true;
    }
    
    if (!!currentUser && currentUser.id === data.sender) {
        let messageType = ["message", "received"];
        if (error) {
          messageType.append("error");
        }

        addMessageToHistoryDiv(decryptedMessage, messageType);
    } else {
      unreadMessages[data.sender]++;
      displayUsers();
    }
  }
}

function init() {
  document.getElementById("username").addEventListener("keyup", (event) => {
                                                         if (event.key === "Enter") {
                                                           event.preventDefault();
                                                           login();
                                                         }
                                                       });

  document.getElementById("message").addEventListener("keyup", (event) => {
                                                        if (event.key === "Enter") {
                                                          event.preventDefault();
                                                          sendMessage();
                                                        }
                                                      });

  socket = new WebSocket(serverAddress);
  socket.onmessage = handleServerMessage;

  const storedToken = sessionStorage.getItem("token");
  if (!!storedToken) {
    myName = sessionStorage.getItem("name");
    publicKey = KEYUTIL.getKey(sessionStorage.getItem("pubKey"));
    privateKey = KEYUTIL.getKey(sessionStorage.getItem("prvKey"));
    token = storedToken;
    socket.onopen = () => {
      socket.send(JSON.stringify({ action: Requests.AuthorizeRequest, token: token }));
    };
  }
  
  socket.onerror = () => {
    alert("An error occurred. Please try again later.");
    location.reload();
  };

  socket.onclose = () => {
    location.reload();
  };
}

function handleAuth(data) {
  if (data.valid) {
    document.getElementById("loginForm").classList.add("hidden");
    document.getElementById("chat").classList.remove("hidden");
  } else {
    logout();
  }
}

function handleLogin(data) {
  if (data.valid) {
    token = data.token;
    myName = data.username;

    sessionStorage.setItem("token", token);
    sessionStorage.setItem("name", myName);
    sessionStorage.setItem("pubKey", KEYUTIL.getPEM(publicKey));
    sessionStorage.setItem("prvKey", KEYUTIL.getPEM(privateKey, "PKCS8PRV"));

    document.getElementById("loginForm").classList.add("hidden");
    document.getElementById("chat").classList.remove("hidden");
  } else {
    document.getElementById("formError").textContent = data.error;
  }
}

function handleUserlistChange(data) {
  if (data.users) {
    let found = false;
    for (const user in messageHistory) {
      for (const userData in data.users) {
        if (userData.id === user) {
          break
        }
      }

      if (!found) {
        delete messageHistory[user];
        delete unreadMessages[user];
      }
    }

    users = data.users;
    for (const user of users) {
      const userId = user.id;

      if (!unreadMessages[userId]) {
        unreadMessages[userId] = 0;
      }
    }

    displayUsers();
  }
}

function handleServerMessage(event) {
  const data = JSON.parse(event.data);
  switch(data.event) {
  case Responses.InvalidUserEvent:
    logout();
    break;
  case Responses.AuthorizationEvent:
    handleAuth(data);
    handleUserlistChange(data);
    break;
  case Responses.LoginEvent:
    handleLogin(data);
    handleUserlistChange(data);
    break;
  case Responses.MessageEvent:
    handleMessage(data);
    break;
  case Responses.UserlistChangeEvent:
    handleUserlistChange(data);
    break;
  default:
    console.warn("Unknown message type", data.event, data);
    break;
  }
}

function hashPassword(password) {
  var sha256 = new KJUR.crypto.MessageDigest({ alg: "sha256", prov: "cryptojs" });
  sha256.updateString(password);
  var hashValue = sha256.digest();
  return hashValue;
}

function generateRSAKeys(userName, hashedPassword) {
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      showLoadingIndicator();
      setTimeout(() => {
      var hash = KJUR.crypto.Util.hashString(userName + hashedPassword, "sha256");

      var rsaKeypair = KEYUTIL.generateKeypair("RSA", 2048, hash);

      resolve(rsaKeypair);
      }, 10);
    }, 0);
  });
}

function showLoadingIndicator() {
  document.getElementById("indicatorContainer").classList.remove("hidden");
  document.getElementById('formControls').classList.add("hidden");
}

function hideLoadingIndicator() {
  document.getElementById("indicatorContainer").classList.add("hidden");
  document.getElementById('formControls').classList.remove("hidden");
}

function toggleRegisterFields() {
  const registerFields = document.getElementById("registerFields");
    if (document.getElementById("registerCheckbox").checked) {
        registerFields.classList.remove("hidden");
        document.getElementById('formHeader').textContent = "Register";
        document.getElementById('formButton').textContent = "Register";
    } else {
        registerFields.classList.add("hidden");
        document.getElementById('formHeader').textContent = "Login";
        document.getElementById('formButton').textContent = "Login";
    }
}

function decryptMessage(privateKey, plaintext) {
  return privateKey.decrypt(plaintext);
}

function encryptMessage(publicKey, ciphertext) {
  var rsaPublicKey = KEYUTIL.getKey(publicKey);
  var encryptedMessage = rsaPublicKey.encrypt(ciphertext);
  return encryptedMessage;
}

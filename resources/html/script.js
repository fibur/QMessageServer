const serverAddress = "ws://%SERVER_ADDRESS%:%SERVER_PORT%";

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
  document.getElementById("formError").innerHTML = "";
  const username = document.getElementById("username").value;
  const password = hashPassword(document.getElementById("password").value);
  const register = document.getElementById("registerCheckbox").checked;
  const confirmPassword = hashPassword(document.getElementById("confirmPassword").value);

  if (username === "" || password === "" || (register && confirmPassword === "")) {
      document.getElementById("formError").innerHTML = "Please fill in all fields.";
      return;
  }

  if (register && password !== confirmPassword) {
      document.getElementById("formError").innerHTML = "Passwords do not match.";
      return;
  }

  showLoadingIndicator();

  generateRSAKeys(username, password).then((keypair) => {
    if (username && socket) {
      privateKey = keypair.prvKeyObj;
      publicKey = keypair.pubKeyObj;
      socket.send(JSON.stringify({ action: register ? Requests.RegisterRequest : Requests.LoginRequest, name: username , password: password, pubKey: publicKey}));
    }
  }).catch((error) => {
    console.error(error);
  }).finally(() => {
    hideLoadingIndicator();
  });
}

function displayUsers() {
  const userList = document.getElementById("userList");
  userList.innerHTML = "";
  for (const user of users) {
    if (user !== myName) {
      const li = document.createElement("li");
      li.textContent = user + (unreadMessages[user] > 0 ? " (" + unreadMessages[user] + ")" : "");
      li.classList.add("user-item");
      if (user === currentUser) {
        li.classList.add("selected");
      }

      li.onmouseover = () => {
        li.style.backgroundColor = "#ddd";
      };
      li.onmouseout = () => {
        li.style.backgroundColor = "#f0f0f0";
      };
      li.onclick = () => {
        currentUser = user;
        unreadMessages[user] = 0;


        const messageContainer = document.getElementById("messageHistory");
        messageContainer.innerHTML = "";
        if (messageHistory[user]) {
          for (const msg of messageHistory[user]) {
            const messageDiv = document.createElement("div");
            messageDiv.textContent = msg.sender === myName ? "You: " + msg.message : msg.sender + ": " + msg.message;
            if (msg.sender === myName) {
              messageDiv.classList.add("message", "sent");
            } else {
              messageDiv.classList.add("message");
            }
            messageContainer.appendChild(messageDiv);
          }
        }
        document.getElementById("messages").querySelector("h2").textContent = "Messages - " + currentUser;
        displayUsers();
      };

      userList.appendChild(li);
    }
  }

  if (!users.includes(currentUser)) {
    currentUser = null;
    document.getElementById("messageHistory").innerHTML = "";
    document.getElementById("messages").querySelector("h2").textContent = "Messages";
  }
}

function sendMessage() {
  const message = document.getElementById("message").value;
  if (socket && token && currentUser && message) {
    if (!currentUser) {
      alert("Please select a user to chat with.");
      return;
    }

    socket.send(JSON.stringify({ action: Requests.MessageRequest, token: token, target: currentUser, message: message }));
    const messageHistoryDiv = document.getElementById("messageHistory");
    const messageDiv = document.createElement("div");
    messageDiv.textContent = "You: " + message;
    messageDiv.classList.add("message", "sent");

    messageHistoryDiv.appendChild(messageDiv);
    document.getElementById("message").value = "";

    if (!messageHistory[currentUser]) {
      messageHistory[currentUser] = [];
    }

    messageHistory[currentUser].push({ sender: myName, message: message });
  }
}

function logout() {
  if (socket && token) {
    socket.send(JSON.stringify({ action: Requests.LogoutRequests, token: token }));
    sessionStorage.clear();
    document.getElementById("loginForm").style.display = "block";
    document.getElementById("chat").style.display = "none";
    socket = null
    token = null;
    currentUser = null;
    myName = null;
    location.reload();
  }
}

function handleMessage(data) {
  if (data.sender && data.message) {
    if (!messageHistory[data.sender]) {
      messageHistory[data.sender] = [];
    }
    messageHistory[data.sender].push({ sender: data.sender, message: data.message });
    if (currentUser === data.sender) {
      const messageHistory = document.getElementById("messageHistory");
      const messageDiv = document.createElement("div");
      messageDiv.textContent = data.sender + ": " + data.message;
      messageDiv.classList.add("message");
      messageHistory.appendChild(messageDiv);
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
    publicKey = sessionStorage.getItem("pubKey");
    privateKey = sessionStorage.getItem("prvKey");
    token = storedToken;
    socket.onopen = () => {
      socket.send(JSON.stringify({ action: Requests.AuthorizeRequest, token: token }));
    };

    socket.onerror = () => {
      alert("An error occurred. Please try again later.");
      location.reload();
    };
  }
}

function handleAuth(data) {
  if (data.valid) {
    document.getElementById("loginForm").style.display = "none";
    document.getElementById("chat").style.display = "block";
  } else {
    logout();
  }
}

function handleLogin(data) {
  if (data.valid) {
    token = data.token;
    myName = data.username;

    sessionStorage.setItem("token", token);
    sessionStorage.setItem("name", token);
    sessionStorage.setItem("pubKey", publicKey);
    sessionStorage.setItem("prvKey", privateKey);

    document.getElementById("loginForm").style.display = "none";
    document.getElementById("chat").style.display = "block";
  } else {
    document.getElementById("formError").textContent = data.error;
  }
}

function handleUserlistChange(data) {
  if (data.users) {
    for (const user in messageHistory) {
      if (!data.users.includes(user)) {
        delete messageHistory[user];
        delete unreadMessages[user];
      }
    }

    users = data.users;
    for (const user of users) {
      if (!unreadMessages[user]) {
        unreadMessages[user] = 0;
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
  document.getElementById('indicatorContainer').style.display = 'block';
  document.getElementById('formControls').style.display = 'none';
}

function hideLoadingIndicator() {
  document.getElementById('indicatorContainer').style.display = 'none';
  document.getElementById('formControls').style.display = 'block';
}

function toggleRegisterFields() {
  var registerCheckbox = document.getElementById("registerCheckbox");
  var registerFields = document.getElementById("registerFields");
  var formButton = document.getElementById("formButton");
  if (registerCheckbox.checked) {
      registerFields.style.display = "block";
      formButton.innerHTML = "Register";
  } else {
    registerFields.style.display = "none";
    formButton.innerHTML = "Login";
  }
}
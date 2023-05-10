const serverAddress = "ws://%SERVER_ADDRESS%:%SERVER_PORT%";

let myName = null;
let socket = null;
let token = null;
let currentUser = null;
let users = {};
let messageHistory = {};
let unreadMessages = {};

init();

function login() {
  const username = document.getElementById("username").value;
  if (username && socket) {
    socket.send(JSON.stringify({ action: "login", name: username }));
  }
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

    socket.send(JSON.stringify({ action: "message", token: token, target: currentUser, message: message }));
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
    socket.send(JSON.stringify({ action: "logout", token: token }));
    document.cookie = "token=; expires=Thu, 01 Jan 1970 00:00:00 UTC";
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

  const storedToken = document.cookie.replace(/(?:(?:^|.*;\s*)token\s*\=\s*([^;]*).*$)|^.*$/, "$1");
  if (storedToken) {
    myName = document.cookie.replace(/(?:(?:^|.*;\s*)name\s*\=\s*([^;]*).*$)|^.*$/, "$1");
    token = storedToken;
    socket.onopen = () => {
      socket.send(JSON.stringify({ action: "authenticate", token: token }));
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
    document.cookie = "token=" + token;
    document.cookie = "name=" + myName;
    document.getElementById("loginForm").style.display = "none";
    document.getElementById("chat").style.display = "block";
  } else {
    document.getElementById("loginError").textContent = data.error;
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
  case "userInvalid":
    logout();
    break;
  case "authentication":
    handleAuth(data);
    handleUserlistChange(data);
    break;
  case "login":
    handleLogin(data);
    handleUserlistChange(data);
    break;
  case "messageEvent":
    handleMessage(data);
    break;
  case "userlistChange":
    handleUserlistChange(data);
    break;
  default:
    console.warn("Unknown message type", data.event, data);
    break;
  }
}

/* Ajax context */
const xhr = new XMLHttpRequest();
const socket = new WebSocket("/ws");

socket.onopen = function (event) {
  console.log("Connected to WebSocket server");
  socket.send("start");
};

socket.onmessage = function (event) {
  console.log("Message from server:", event.data);
  // HANDLE MESSAGE
};

socket.onclose = function (event) {
  console.log("Disconnected from WebSocket server");
};

socket.onerror = function (error) {
  console.error("WebSocket error:", error);
};

// Send a message after 1 second
setTimeout(() => {
  socket.send("Another message from the client!");
}, 1000);
// Function to show the loader and freeze the UI
function showLoader() {
  document.getElementById("loader-overlay").style.display = "flex";
  return setTimeout(() => {
    hideLoader();
    alert("Request timed out!");
  }, timeoutDuration);
}

// Function to hide the loader and unfreeze the UI
function hideLoader() {
  document.getElementById("loader-overlay").style.display = "none";
}

// Open the first tab by default
document.querySelector(".tab button").click();

// Function to authenticate the user
function authenticateUser() {
  const username = document.getElementById("authUsername").value;
  const password = document.getElementById("authPassword").value;
  const url = "/auth"; // Replace with your ESP32 authentication endpoint

  // Set up Basic Auth header
  const auth = "Basic " + btoa(username + ":" + password);
  const xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.setRequestHeader("Authorization", auth);

  showLoader(); // Show loader while waiting for authentication response

  xhr.onload = function () {
    hideLoader(); // Hide loader after receiving the response
    if (xhr.status === 200) {
      // Hide the authentication modal and show the main content
      document.getElementById("authModal").style.display = "none";
      document.getElementById("mainContent").style.display = "block";
    } else {
      document.getElementById("errorMessage").innerText =
        "Authentication failed. Please try again.";
    }
  };

  xhr.onerror = function () {
    hideLoader(); // Hide loader in case of an error
    document.getElementById("errorMessage").innerText =
      "Network error. Please check your connection.";
  };

  xhr.send();
}

// Wrapping functions to handle loader and requests
function wrappedRequest(requestFunction) {
  return function () {
    showLoader();
    setTimeout(() => {
      hideLoader();
      alert("Request timed out!");
    }, 10000); // Set a timeout for response
    requestFunction(); // Call the original request function
  };
}

// Example: wrapping makeRequest function with loader
function makeRequest() {
  const xhr = new XMLHttpRequest();
  xhr.open("GET", "/some-endpoint", true); // Replace with actual endpoint
  xhr.onload = function () {
    hideLoader();
    document.getElementById("result").innerText = xhr.responseText;
  };
  xhr.onerror = function () {
    hideLoader();
    document.getElementById("result").innerText = "Network Error";
  };
  xhr.send();
}

// Tab switching functionality
function openTab(evt, tabName) {
  document
    .querySelectorAll(".tabcontent")
    .forEach((tab) => (tab.style.display = "none"));
  document
    .querySelectorAll(".tablinks")
    .forEach((btn) => (btn.className = btn.className.replace(" active", "")));
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}

// Set new values and update previous values on success
function setNewValue(type) {
  if (type === "number") {
    const newNumber = document.getElementById("inputNumber").value;
    if (newNumber === "") {
      alert("Please enter a valid number.");
      return;
    }
    document.getElementById("previousNumber").innerText =
      "Previous Number: " + newNumber;
    document.getElementById("headerNumber").innerText =
      "Number Output: " + newNumber;
  } else if (type === "text") {
    const newText = document.getElementById("inputText").value;
    if (newText === "") {
      alert("Please enter a valid text.");
      return;
    }
    document.getElementById("previousText").innerText =
      "Previous Text: " + newText;
    document.getElementById("headerText").innerText = "Text Output: " + newText;
  }
}

// Dummy scan list data for Provisioning Tab
const dummyNetworks = [
  { ssid: "Rahul", rssi: -50, protected: true },
  { ssid: "Rajesh", rssi: -70, protected: false },
  { ssid: "Priya", rssi: -65, protected: true },
  { ssid: "Mayank", rssi: -80, protected: false },
  { ssid: "Sandeep", rssi: -90, protected: true },
];

// Function to calculate signal strength based on RSSI
function getSignalClass(rssi) {
  if (rssi > -60) return "strong-signal";
  else if (rssi > -80) return "moderate-signal";
  else return "weak-signal";
}

// Calculate width percentage for the RSSI bar based on RSSI value
function calculateRSSIWidth(rssi) {
  return Math.max(0, Math.min(100, ((rssi + 100) * 100) / 60)) + "%";
}

// Show dummy scan list on button press
function startScan() {
  const scanList = document.getElementById("scanlist");
  scanList.innerHTML = ""; // Clear the previous scan list

  dummyNetworks.forEach((network) => {

    const listItem = document.createElement("li");
    listItem.className = "network-item";

    const icon = document.createElement("span");
    icon.className = "network-icon";
    icon.innerHTML = "&#128274;"; // Unicode for the lock icon 🔒

    const info = document.createElement("div");
    info.className = "network-info";
    info.innerHTML = `<span class="network-ssid">${network.ssid}</span>`;

    const rssiBarContainer = document.createElement("div");
    rssiBarContainer.className = "rssi-bar-container";

    const rssiBar = document.createElement("div");
    rssiBar.className = `rssi-bar ${getSignalClass(network.rssi)}`;
    rssiBar.style.width = calculateRSSIWidth(network.rssi);

    // Add onclick event to the list item
    listItem.onclick = function () {
      selectNetwork(network.ssid);
    };

    // Append elements to the list item
    rssiBarContainer.appendChild(rssiBar);
    listItem.appendChild(info);
    if (network.protected) {
      listItem.appendChild(icon);
    }
    listItem.appendChild(rssiBarContainer);
    scanList.appendChild(listItem);
  });
}

// Populate the SSID field when a network is selected
function selectNetwork(ssid) {
  document.getElementById("ssid").value = ssid;
}

// Connect to the selected Wi-Fi network
function connectToNetwork() {
  const ssid = document.getElementById("ssid").value;
  const password = document.getElementById("password").value;
  if (!ssid) {
    alert("Please select a network from the scan list.");
    return;
  }

  if (
    password === "" &&
    confirm("No password provided. Continue with open connection?")
  ) {
    return;
  }

  // Dummy connection process
  const isConnected = ssid === "Network_1" && password === "correct_password";
  updateConnectionStatus(isConnected);
  alert(
    isConnected
      ? "Connection successful!"
      : "Connection failed. Check credentials."
  );
}

// Start OTA update process
function startOTA() {
  console.log("OTA request");

  // const otaFile = document.getElementById("ota-file").files[0];
  // if (!otaFile) {
  //   alert("Please select a firmware file to upload.");
  //   return;
  // }
  // // Simulate OTA update status
  // document.getElementById("ota-status").innerText = "Uploading firmware...";
  // setTimeout(() => {
  //   document.getElementById("ota-status").innerText =
  //     "Firmware uploaded successfully!";
  // }, 2000);
}

function sendRequest(type, data) {
  const message = JSON.stringify({ type, data });
  const timeoutId = startLoader();

  // Send message through WebSocket
  socket.send(message);
}

window.onload = function () {
  // Show the authentication modal on page load
  document.getElementById("authModal").style.display = "flex";
  document.getElementById("authUsername").focus();
};

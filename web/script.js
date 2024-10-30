// Open first tab by default
document.querySelector(".tab button").click();

// Open websocket connection.
const socket = new WebSocket('ws:/');


// Function to show the loader and freeze the UI
function showLoader() {
  document.getElementById("loader-overlay").style.display = "flex";
  // Simulate a process (e.g., waiting for a response)
  const timeout = setTimeout(() => {
    hideLoader(); // Hide loader after timeout
    alert("Request timed out!");
  }, 10000); // Timeout duration (5 seconds)
}

// Function to hide the loader and unfreeze the UI
function hideLoader() {
  document.getElementById("loader-overlay").style.display = "none";
}


// Call simulateRequest on page load
window.onload = simulateRequest;

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
function startDummyScan() {
  const scanList = document.getElementById("scanlist");
  scanList.innerHTML = ""; // Clear the previous scan list

  dummyNetworks.forEach((network) => {
    const listItem = document.createElement("li");
    listItem.className = "network-item";

    const icon = document.createElement("span");
    icon.className = "network-icon";
    //icon.innerHTML = "🔒"; // Network icon
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
    !confirm("No password provided. Continue with open connection?")
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
  const otaFile = document.getElementById("ota-file").files[0];
  if (!otaFile) {
    alert("Please select a firmware file to upload.");
    return;
  }
  // Simulate OTA update status
  document.getElementById("ota-status").innerText = "Uploading firmware...";
  setTimeout(() => {
    document.getElementById("ota-status").innerText =
      "Firmware uploaded successfully!";
  }, 2000);
}

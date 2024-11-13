const COLOR_OK = '#5adb69'
const COLOR_ERR = '#e36a64'

const loader = document.getElementById('loader-overlay')
/* Ajax context */
const xhr = new XMLHttpRequest()
/* Websocket context */
const ws = new WebSocket('/ws')

/* Timer handle */
let timer = null

/* Mqtt tls state */
let mqttTls = false

/* Open default tab */
document.querySelector('.tab button').click()

ws.onopen = function (evt) {
  console.log('ws server connected')
}

ws.onmessage = function (evt) {
  console.log('Message from server:', evt.data)
  if (timer !== null) clearTimeout(timer) // stop timer
  hideLoader()
  let data
  try {
    data = JSON.parse(evt.data)
  } catch (e) {
    console.error('invalid JSON received')
    return
  }

  switch (data.type) {
    case 'scan':
      handleScanResp(data)
      break
    case 'provsn':
      // handleConnResp(data)
      break
    case 'wifi_conn':
      handleWifiConn(data.connected)
      break
    default:
      console.log('invalid responce')
  }
}

ws.onclose = function (evt) {
  console.log('Disconnected from WebSocket server')
}

ws.onerror = function (err) {
  console.error('WebSocket error:', error)
}

/* -------------Loader----------- */
function showLoader() {
  loader.style.display = 'flex'
}

function hideLoader() {
  loader.style.display = 'none'
}
/* ------------------------------ */

/* ------------Tab Opening------- */
function openTab(evt, name) {
  document
    .querySelectorAll('.tabcontent')
    .forEach((tab) => (tab.style.display = 'none'))
  document
    .querySelectorAll('.tablinks')
    .forEach((btn) => (btn.className = btn.className.replace(' active', '')))
  document.getElementById(name).style.display = 'block'
  evt.currentTarget.className += ' active'
}
/* ----------------------------- */

/*---------------RSSI Bar----------*/

function getSigClass(rssi) {
  if (rssi > -60) return 'sig-strong'
  else if (rssi > -80) return 'sig-moderate'
  else return 'sig-weak'
}

function calculateWidth(rssi) {
  return Math.max(0, Math.min(100, ((rssi + 100) * 100) / 60)) + '%'
}

/*---------------------------------*/

/* Handles user authentication */
function authenticateUser() {
  const user = document.getElementById('auth-user').value
  const pass = document.getElementById('auth-pass').value

  const auth = 'Basic ' + btoa(user + ':' + pass)
  xhr.open('GET', '/auth', true)
  xhr.setRequestHeader('Authorization', auth)
  showLoader()

  xhr.onload = function () {
    hideLoader() // Hide loader after receiving the response
    if (xhr.status === 200) {
      // Hide the authentication modal and show the main content
      document.getElementById('auth-model').style.display = 'none'
      document.getElementById('root-container').style.display = 'block'
    } else {
      confirm('Authentication failed.')
    }
  }

  xhr.onerror = function () {
    hideLoader()
    alert('Network error.')
  }
  xhr.send()
}

/* Send scan request */
function startScan() {
  const req = createRequest('scan')
  sendMessage(req, 30)
}

/* Send provisioning requset */
function startProvsn(network) {
  console.log('provision called')
  const pass = document.getElementById('pass-container').value
  const ssid = document.getElementById('ssid-container').value
  if (!ssid) {
    alert('Enter valid ssid')
    return
  }
  if (
    pass == '' &&
    !confirm('No password provided. Continue with open connection?')
  ) {
    return
  }
  const req = createRequest('prvsn', { ssid: ssid, pass: pass })
  sendMessage(req, 5)
}

function enableTls() {
  mqttTls = !mqttTls
  const elm = document.getElementById('mqtt-files')
  if (mqttTls) elm.style.display = 'block'
  else elm.style.display = 'none'
}

/* Send websocket message with timeout */
function sendMessage(message, timeout) {
  if (ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(message))
    showLoader() // Show loader when message is sent

    // Set up the timeout
    timer = setTimeout(() => {
      hideLoader() // Hide loader after timeout
      alert('Request timed out. No response from server.')
    }, timeout * 1000)
  } else {
    alert('Disconnected from server')
  }
  return timer
}

/* Create request from type */
function createRequest(type, data) {
  let req = { type }

  switch (type) {
    case 'scan':
      break
    case 'prvsn':
      req.ssid = data.ssid
      req.pass = data.pass
      break
    default:
      console.warn("invalid 'type'")
  }

  console.log(req) // TODO remove
  return req
}

/*-----------------Responce Handlers--------------*/

function handleScanResp(data) {
  const listElm = document.getElementById('scanlist')

  listElm.innerHTML = ''
  data.networks.forEach((network) => {
    const item = document.createElement('li')
    item.className = 'network-item'
    item.innerHTML = `
    <div class="network-info">
    <span class="network-ssid">${network.ssid}</span>
    </div>
    <span class="network-icon">${network.open ? '' : '🔒'}</span>
    <div class="rssi-bar-container">
    <div class="rssi-bar ${getSigClass(
      network.rssi
    )}" style="width: ${calculateWidth(network.rssi)}"></div>
    </div>`

    item.onclick = function () {
      document.getElementById('ssid-container').value = network.ssid
    }

    listElm.appendChild(item)
  })
}

function handleWifiConn(connected) {
  let style = document.getElementById('status-conn').style
  if (connected) style.backgroundColor = COLOR_OK
  else style.backgroundColor = COLOR_ERR
}

/*------------------------------------------------*/

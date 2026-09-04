
#include <WiFi.h>
#include <WebServer.h>

// ============================================================
// RELAYDROP
// AUTOMATIC NODE UID + E22 RADIO ADDRESS
//
// ESP32 hardware UID
//        ↓
// RelayDrop Node UID
//        ↓
// E22 radio address
//
// NO NODE ID NEEDS TO BE HARD-CODED.
//
// Each ESP32 automatically generates its own identity.
//
// Example:
//
// ESP32 MAC:
// 24:6F:28:A1:B2:C3
//
// RelayDrop UID:
// RD-A1B2C3
//
// E22 Address:
// 0xA1B2
// ============================================================


// ============================================================
// WIFI
// ============================================================

WebServer server(80);

const char* WIFI_PASSWORD = "relief123";

String wifiSSID;


// ============================================================
// E22 UART
// ============================================================

HardwareSerial E22Serial(2);

const int E22_RX = 16;
const int E22_TX = 17;
const int E22_M1 = 21;


// ============================================================
// AUTOMATIC NODE ID
// ============================================================

String NODE_UID;


// ============================================================
// AUTOMATIC E22 ADDRESS
// ============================================================

uint16_t E22_ADDRESS;


// ============================================================
// MESSAGE NUMBER
// ============================================================

unsigned long messageNumber = 0;


// ============================================================
// CHAT STORAGE
// ============================================================

struct ChatMessage {

  String from;
  String radioAddress;
  String type;
  String message;

  bool outgoing;
};


const int MAX_MESSAGES = 30;

ChatMessage chatHistory[MAX_MESSAGES];

int messageCount = 0;


// ============================================================
// GENERATE NODE UID FROM ESP32 MAC
// ============================================================

void generateNodeIdentity() {

  uint64_t chipID = ESP.getEfuseMac();


  // ----------------------------------------------------------
  // Extract the last 6 hexadecimal digits
  // ----------------------------------------------------------

  uint32_t uniquePart =
    (uint32_t)(chipID & 0xFFFFFF);


  char uidBuffer[20];


  sprintf(
    uidBuffer,
    "RD-%06X",
    uniquePart
  );


  NODE_UID =
    String(uidBuffer);


  // ----------------------------------------------------------
  // Generate 16-bit E22 address
  //
  // We use bits from the ESP32 hardware ID.
  // ----------------------------------------------------------

  E22_ADDRESS =
    (uint16_t)(
      (chipID ^
       (chipID >> 16) ^
       (chipID >> 32)) & 0xFFFF
    );


  // ----------------------------------------------------------
  // Prevent address 0x0000
  // ----------------------------------------------------------

  if (E22_ADDRESS == 0x0000) {

    E22_ADDRESS = 0x0001;
  }


  // ----------------------------------------------------------
  // Wi-Fi network name
  // ----------------------------------------------------------

  wifiSSID =
    "RelayDrop-" +
    NODE_UID;


  // ----------------------------------------------------------
  // Display identity
  // ----------------------------------------------------------

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "     AUTOMATIC NODE IDENTITY"
  );

  Serial.println(
    "================================"
  );


  Serial.print(
    "ESP32 MAC: "
  );

  Serial.println(
    String((uint32_t)(chipID >> 32), HEX)
    + ":" +
    String((uint32_t)(chipID & 0xFFFFFFFF), HEX)
  );


  Serial.print(
    "RelayDrop UID: "
  );

  Serial.println(
    NODE_UID
  );


  Serial.print(
    "E22 Address: 0x"
  );

  Serial.println(
    E22_ADDRESS,
    HEX
  );


  Serial.println(
    "================================"
  );

  Serial.println();
}


// ============================================================
// ADD CHAT MESSAGE
// ============================================================

void addChatMessage(
  String from,
  String radioAddress,
  String type,
  String message,
  bool outgoing
) {

  if (messageCount >= MAX_MESSAGES) {

    for (
      int i = 1;
      i < MAX_MESSAGES;
      i++
    ) {

      chatHistory[i - 1] =
        chatHistory[i];
    }

    messageCount =
      MAX_MESSAGES - 1;
  }


  chatHistory[messageCount].from =
    from;

  chatHistory[messageCount].radioAddress =
    radioAddress;

  chatHistory[messageCount].type =
    type;

  chatHistory[messageCount].message =
    message;

  chatHistory[messageCount].outgoing =
    outgoing;

  messageCount++;
}


// ============================================================
// ESCAPE JSON
// ============================================================

String escapeJSON(String text) {

  text.replace(
    "\\",
    "\\\\"
  );

  text.replace(
    "\"",
    "\\\""
  );

  text.replace(
    "\n",
    "\\n"
  );

  text.replace(
    "\r",
    "\\r"
  );

  return text;
}


// ============================================================
// ESCAPE HTML
// ============================================================

String escapeHTML(String text) {

  text.replace(
    "&",
    "&amp;"
  );

  text.replace(
    "<",
    "&lt;"
  );

  text.replace(
    ">",
    "&gt;"
  );

  text.replace(
    "\"",
    "&quot;"
  );

  text.replace(
    "'",
    "&#39;"
  );

  return text;
}


// ============================================================
// HOME PAGE
// ============================================================

void handleHome() {

  String page = R"rawliteral(
<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
      content="width=device-width, initial-scale=1, viewport-fit=cover">

<title>RelayDrop</title>

<style>

* {
  box-sizing: border-box;
}

html,
body {
  width: 100%;
  height: 100%;
}

body {

  margin: 0;

  font-family:
    Arial,
    Helvetica,
    sans-serif;

  background: #d9dbd5;

  min-height: 100vh;

  min-height: 100dvh;

  overflow: hidden;
}


.app {

  width: 100%;

  height: 100vh;

  height: 100dvh;

  display: flex;

  flex-direction: column;

  background: #efeae2;
}


/* ==========================================================
   HEADER
   ========================================================== */

.header {

  height: 75px;

  flex: 0 0 75px;

  background: #075e54;

  color: white;

  display: flex;

  align-items: center;

  padding: 10px 18px;

  box-shadow:
    0 2px 5px rgba(0,0,0,0.25);
}


.logo {

  width: 45px;

  height: 45px;

  border-radius: 50%;

  background: #128c7e;

  display: flex;

  align-items: center;

  justify-content: center;

  margin-right: 12px;

  overflow: hidden;
}


.logo svg {

  width: 30px;

  height: 30px;

  fill: rgba(255,255,255,0.95);
}


.header-info {

  display: flex;

  flex-direction: column;
}


.header-title {

  font-size: 19px;

  font-weight: bold;
}


.header-status {

  font-size: 12px;

  opacity: 0.9;

  margin-top: 3px;
}


/* ==========================================================
   CHAT
   ========================================================== */

.chat {

  flex: 1;

  min-height: 0;

  overflow-y: auto;

  overscroll-behavior-y: contain;

  scrollbar-gutter: stable;

  -webkit-overflow-scrolling: touch;

  padding: 18px 12px;

  background-color: #efeae2;

  background-image:
    radial-gradient(
      rgba(0,0,0,0.035) 1px,
      transparent 1px
    );

  background-size: 20px 20px;
}


.message-row {

  display: flex;

  width: 100%;

  margin-bottom: 10px;
}


.message-row.sent {

  justify-content: flex-end;
}


.message-row.received {

  justify-content: flex-start;
}


.bubble {

  max-width: 75%;

  padding: 9px 12px;

  border-radius: 10px;

  box-shadow:
    0 1px 1px rgba(0,0,0,0.12);
}


.sent .bubble {

  background: #dcf8c6;

  border-top-right-radius: 3px;
}


.received .bubble {

  background: white;

  border-top-left-radius: 3px;
}


.sender {

  font-size: 12px;

  font-weight: bold;

  color: #075e54;

  margin-bottom: 3px;
}


.address {

  font-size: 9px;

  color: #999;

  margin-bottom: 4px;
}


.message-type {

  font-size: 10px;

  font-weight: bold;

  color: #777;

  margin-bottom: 4px;

  text-transform: uppercase;
}


.message-text {

  font-size: 15px;

  line-height: 1.4;

  white-space: pre-wrap;

  word-wrap: break-word;
}


.message-meta {

  font-size: 10px;

  color: #888;

  text-align: right;

  margin-top: 4px;
}


/* ==========================================================
   EMPTY CHAT
   ========================================================== */

.empty {

  height: 100%;

  display: flex;

  align-items: center;

  justify-content: center;

  text-align: center;

  color: #777;
}


.empty-box {

  background:
    rgba(255,255,255,0.8);

  padding: 25px;

  border-radius: 15px;

  box-shadow:
    0 2px 5px rgba(0,0,0,0.08);
}


.empty-title {

  font-size: 20px;

  font-weight: bold;

  margin-bottom: 8px;
}


/* ==========================================================
   COMPOSER
   ========================================================== */

.composer {

  flex: 0 0 auto;

  background: #f0f0f0;

  padding: 10px;

  border-top:
    1px solid #ddd;
}


.composer-form {

  display: flex;

  gap: 8px;

  align-items: flex-end;
}


.controls {

  flex: 1;

  display: flex;

  flex-direction: column;

  gap: 6px;
}


select {

  border: none;

  border-radius: 8px;

  padding: 7px 10px;

  background: white;

  font-size: 13px;

  outline: none;
}


textarea {

  width: 100%;

  resize: none;

  border: none;

  border-radius: 20px;

  padding: 10px 15px;

  font-size: 15px;

  font-family: inherit;

  min-height: 44px;

  outline: none;

  background: white;
}


.send-button {

  width: 48px;

  height: 48px;

  border-radius: 50%;

  border: none;

  background: #128c7e;

  color: white;

  cursor: pointer;

  display: flex;

  align-items: center;

  justify-content: center;

  flex: 0 0 48px;
}


.send-button svg {

  width: 25px;

  height: 25px;

  fill: white;

  transform: translate(-1px, 1px);
}


.node-info {

  text-align: center;

  font-size: 10px;

  color: #777;

  padding-top: 4px;
}


@media (max-width: 600px) {

  .bubble {

    max-width: 85%;
  }

}

</style>

</head>


<body>

<div class="app">


<!-- ========================================================
     HEADER
     ======================================================== -->

<div class="header">

  <div class="logo" aria-label="Default profile picture">
    <svg viewBox="0 0 24 24" aria-hidden="true">
      <circle cx="12" cy="8" r="4"></circle>
      <path d="M4.5 21c.5-5 3-7.5 7.5-7.5s7 2.5 7.5 7.5z"></path>
    </svg>
  </div>

  <div class="header-info">

    <div class="header-title">
      RelayDrop
    </div>

    <div class="header-status">

      Node: )rawliteral";

  page += NODE_UID;

  page += R"rawliteral(

      &bull; Radio: 0x)rawliteral";

  page += String(
    E22_ADDRESS,
    HEX
  );

  page += R"rawliteral(

    </div>

  </div>

</div>


<!-- ========================================================
     CHAT
     ======================================================== -->

<div
  class="chat"
  id="chat">

  <div
    class="empty"
    id="empty">

    <div class="empty-box">

      <div class="empty-title">
        RelayDrop Chat
      </div>

      <div>
        Off-grid emergency
        communication.
      </div>

    </div>

  </div>

</div>


<!-- ========================================================
     COMPOSER
     ======================================================== -->

<div class="composer">

  <form
  id="messageForm"
  class="composer-form"
  onsubmit="sendMessage(event)">

    <div class="controls">

      <select name="type">

        <option value="MEDICAL">
          Medical Assistance
        </option>

        <option value="TRAPPED">
          Trapped Person
        </option>

        <option value="HAZARD">
          Report Hazard
        </option>

        <option value="SAFE">
          I Am Safe
        </option>

        <option value="GENERAL">
          General Message
        </option>

      </select>


      <textarea
        name="message"
        rows="1"
        placeholder="Type a message..."
        required></textarea>

    </div>


    <button
      class="send-button"
      type="submit"
      aria-label="Send message">

      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path d="M21.7 2.3a1 1 0 0 0-1-.22L2.6 9.05a1 1 0 0 0 .08 1.9l7.53 2.37 2.37 7.53a1 1 0 0 0 .91.7h.04a1 1 0 0 0 .92-.62l7.47-17.56a1 1 0 0 0-.22-1.07zM11.3 12.04 6.06 10.4l11.28-4.34-6.04 5.98zm2.3 5.92-1.64-5.24 5.98-6.04-4.34 11.28z"></path>
      </svg>
    </button>

  </form>


  <div class="node-info">

    )rawliteral";

  page += NODE_UID;

  page += R"rawliteral(
    &bull; E22 address 0x)rawliteral";

  page += String(
    E22_ADDRESS,
    HEX
  );

  page += R"rawliteral(

  </div>

</div>


</div>

<script>

// ==========================================================
// LOCAL MESSAGE CACHE
// ==========================================================

let displayedMessages = 0;


// ==========================================================
// LOAD MESSAGES
//
// This does NOT reload the webpage.
//
// It only asks the ESP32 for the latest messages.
// ==========================================================

function loadMessages() {

  fetch("/messages")

    .then(function(response) {

      return response.json();

    })

    .then(function(messages) {

      const chat =
        document.getElementById("chat");

      const empty =
        document.getElementById("empty");

      // Keep following the conversation only when the viewer
      // is already close to the newest message. This prevents
      // each background refresh from pulling the view downward
      // while someone is reading older messages.
      const isInitialLoad =
        displayedMessages === 0;

      const wasNearBottom =
        chat.scrollHeight -
        chat.scrollTop -
        chat.clientHeight < 80;

      let addedMessages = false;


      // ------------------------------------------------------
      // No messages
      // ------------------------------------------------------

      if (messages.length === 0) {

        empty.style.display = "flex";

        return;
      }


      empty.style.display = "none";


      // ------------------------------------------------------
      // Only add NEW messages
      // ------------------------------------------------------

      while (
        displayedMessages < messages.length
      ) {

        const msg =
          messages[displayedMessages];


        const row =
          document.createElement("div");


        // ----------------------------------------------------
        // Sent / received
        // ----------------------------------------------------

        if (msg.outgoing) {

          row.className =
            "message-row sent";

        } else {

          row.className =
            "message-row received";
        }


        // ----------------------------------------------------
        // Bubble
        // ----------------------------------------------------

        const bubble =
          document.createElement("div");

        bubble.className =
          "bubble";


        // ----------------------------------------------------
        // Sender
        // ----------------------------------------------------

        const sender =
          document.createElement("div");

        sender.className =
          "sender";

        sender.textContent =
          msg.from;


        // ----------------------------------------------------
        // Radio address
        // ----------------------------------------------------

        const address =
          document.createElement("div");

        address.className =
          "address";

        address.textContent =
          "E22: 0x" +
          msg.radioAddress;


        // ----------------------------------------------------
        // Message type
        // ----------------------------------------------------

        const type =
          document.createElement("div");

        type.className =
          "message-type";

        type.textContent =
          msg.type;


        // ----------------------------------------------------
        // Message
        // ----------------------------------------------------

        const text =
          document.createElement("div");

        text.className =
          "message-text";

        text.textContent =
          msg.message;


        // ----------------------------------------------------
        // Status
        // ----------------------------------------------------

        const meta =
          document.createElement("div");

        meta.className =
          "message-meta";


        if (msg.outgoing) {

          meta.textContent =
            "\u2713 Sent";

        } else {

          meta.textContent =
            "\u2193 Received";
        }


        // ----------------------------------------------------
        // Build bubble
        // ----------------------------------------------------

        bubble.appendChild(sender);

        bubble.appendChild(address);

        bubble.appendChild(type);

        bubble.appendChild(text);

        bubble.appendChild(meta);


        row.appendChild(bubble);

        chat.appendChild(row);

        addedMessages = true;


        // ----------------------------------------------------
        // Increase counter
        // ----------------------------------------------------

        displayedMessages++;
      }


      // ------------------------------------------------------
      // Scroll only when new content was added and the viewer
      // has not intentionally moved up through the history.
      // ------------------------------------------------------

      if (
        addedMessages &&
        (isInitialLoad || wasNearBottom)
      ) {

        chat.scrollTo({

          top: chat.scrollHeight,

          behavior:
            isInitialLoad ? "auto" : "smooth"

        });
      }

    })

    .catch(function(error) {

      console.log(
        "Message update error:",
        error
      );

    });
}


// ==========================================================
// SEND MESSAGE
//
// IMPORTANT:
//
// The form is intercepted here.
//
// The browser does NOT navigate to /send.
//
// Instead JavaScript sends the request in the background.
// ==========================================================

function sendMessage(event) {

  event.preventDefault();


  const form =
    document.getElementById("messageForm");


  const type =
    form.querySelector(
      'select[name="type"]'
    ).value;


  const messageBox =
    form.querySelector(
      'textarea[name="message"]'
    );


  const message =
    messageBox.value.trim();


  if (message.length === 0) {

    return false;
  }


  // --------------------------------------------------------
  // Disable send button temporarily
  // --------------------------------------------------------

  const sendButton =
    form.querySelector(
      ".send-button"
    );


  sendButton.disabled = true;


  sendButton.style.opacity =
    "0.6";


  // --------------------------------------------------------
  // Build request
  // --------------------------------------------------------

  const params =
    new URLSearchParams();


  params.append(
    "type",
    type
  );


  params.append(
    "message",
    message
  );


  // --------------------------------------------------------
  // Send to ESP32 WITHOUT page reload
  // --------------------------------------------------------

  fetch(
    "/send?" +
    params.toString()
  )

  .then(function(response) {

    if (!response.ok) {

      throw new Error(
        "Send failed"
      );
    }

    return response.json();

  })

  .then(function(result) {

    console.log(
      "Message sent:",
      result
    );


    // ------------------------------------------------------
    // Clear message box
    // ------------------------------------------------------

    messageBox.value = "";


    // ------------------------------------------------------
    // Immediately update chat
    // ------------------------------------------------------

    loadMessages();

  })

  .catch(function(error) {

    console.error(
      "Transmission error:",
      error
    );


    alert(
      "Unable to send message."
    );

  })

  .finally(function() {

    sendButton.disabled = false;

    sendButton.style.opacity =
      "1";

  });


  return false;
}


// ==========================================================
// CHECK FOR NEW MESSAGES
//
// Every 500 ms.
//
// This is a silent background request.
// The webpage itself never reloads.
// ==========================================================

setInterval(
  loadMessages,
  500
);


// ==========================================================
// INITIAL LOAD
// ==========================================================

loadMessages();

</script>


</body>

</html>
)rawliteral";


  server.send(
    200,
    "text/html",
    page
  );
}


// ============================================================
// SEND MESSAGE
// ============================================================

void handleSend() {

  String type = server.arg("type");
  String message = server.arg("message");

  message.trim();

  if (message.length() == 0) {
    server.send(
      400,
      "text/plain",
      "Message cannot be empty."
    );
    return;
  }

  messageNumber++;

  // ----------------------------------------------------------
  // Create radio packet
  // ----------------------------------------------------------

  String radioMessage =
    "FROM=" +
    NODE_UID +
    "|ADDR=" +
    String(E22_ADDRESS, HEX) +
    "|TYPE=" +
    type +
    "|MSG=" +
    String(messageNumber) +
    "|MESSAGE=" +
    message;


  // ----------------------------------------------------------
  // Serial Monitor
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("============================");
  Serial.println("       OUTGOING MESSAGE");
  Serial.println("============================");

  Serial.print("FROM: ");
  Serial.println(NODE_UID);

  Serial.print("E22 ADDRESS: 0x");
  Serial.println(E22_ADDRESS, HEX);

  Serial.print("TYPE: ");
  Serial.println(type);

  Serial.print("MESSAGE ID: ");
  Serial.println(messageNumber);

  Serial.print("MESSAGE: ");
  Serial.println(message);

  Serial.println();

  // ----------------------------------------------------------
  // TRANSMIT THROUGH E22
  // ----------------------------------------------------------

  Serial.println("[E22] Transmitting...");

  E22Serial.println(radioMessage);

  Serial.println("[E22] Transmission complete");


  // ----------------------------------------------------------
  // Add message to local chat
  // ----------------------------------------------------------

  addChatMessage(
    NODE_UID,
    String(E22_ADDRESS, HEX),
    type,
    message,
    true
  );


  // ----------------------------------------------------------
  // IMPORTANT:
  // Return JSON instead of redirecting the browser.
  //
  // This prevents the webpage from reloading.
  // ----------------------------------------------------------

  String json = "{";

  json += "\"success\":true,";

  json += "\"from\":\"";
  json += escapeJSON(NODE_UID);
  json += "\",";

  json += "\"radioAddress\":\"";
  json += escapeJSON(
    String(E22_ADDRESS, HEX)
  );
  json += "\",";

  json += "\"type\":\"";
  json += escapeJSON(type);
  json += "\",";

  json += "\"message\":\"";
  json += escapeJSON(message);
  json += "\"";

  json += "}";


  server.send(
    200,
    "application/json",
    json
  );
}


// ============================================================
// GET PACKET FIELD
// ============================================================

String getField(
  String data,
  String field
) {

  String search =
    field + "=";


  int start =
    data.indexOf(search);


  if (
    start < 0
  ) {

    return "";
  }


  start +=
    search.length();


  int end =
    data.indexOf(
      "|",
      start
    );


  if (
    end < 0
  ) {

    return data.substring(
      start
    );
  }


  return data.substring(
    start,
    end
  );
}


// ============================================================
// RECEIVE E22
// ============================================================

void checkE22Messages() {

  while (
    E22Serial.available()
  ) {

    String receivedMessage =
      E22Serial.readStringUntil(
        '\n'
      );


    receivedMessage.trim();


    if (
      receivedMessage.length() == 0
    ) {

      return;
    }


    // --------------------------------------------------------
    // Extract packet fields
    // --------------------------------------------------------

    String from =
      getField(
        receivedMessage,
        "FROM"
      );


    String address =
      getField(
        receivedMessage,
        "ADDR"
      );


    String type =
      getField(
        receivedMessage,
        "TYPE"
      );


    String msgID =
      getField(
        receivedMessage,
        "MSG"
      );


    String message =
      getField(
        receivedMessage,
        "MESSAGE"
      );


    // --------------------------------------------------------
    // Validate
    // --------------------------------------------------------

    if (
      from.length() == 0 ||
      message.length() == 0
    ) {

      Serial.println();

      Serial.println(
        "[E22] Unknown packet:"
      );

      Serial.println(
        receivedMessage
      );

      return;
    }


    // --------------------------------------------------------
    // Ignore our own packet
    // --------------------------------------------------------

    if (
      from == NODE_UID
    ) {

      return;
    }


    // --------------------------------------------------------
    // Display incoming message
    // --------------------------------------------------------

    Serial.println();

    Serial.println(
      "============================"
    );

    Serial.println(
      "       MESSAGE RECEIVED"
    );

    Serial.println(
      "============================"
    );


    Serial.print(
      "FROM: "
    );

    Serial.println(
      from
    );


    Serial.print(
      "E22 ADDRESS: 0x"
    );

    Serial.println(
      address
    );


    Serial.print(
      "TYPE: "
    );

    Serial.println(
      type
    );


    Serial.print(
      "MESSAGE ID: "
    );

    Serial.println(
      msgID
    );


    Serial.print(
      "MESSAGE: "
    );

    Serial.println(
      message
    );


    Serial.println(
      "============================"
    );

    Serial.println();


    // --------------------------------------------------------
    // Add to chat
    // --------------------------------------------------------

    addChatMessage(
      from,
      address,
      type,
      message,
      false
    );
  }
}


// ============================================================
// MESSAGE JSON API
// ============================================================

void handleMessages() {

  String json =
    "[";


  for (
    int i = 0;
    i < messageCount;
    i++
  ) {

    if (
      i > 0
    ) {

      json += ",";
    }


    json += "{";


    json +=
      "\"from\":\"";

    json +=
      escapeJSON(
        chatHistory[i].from
      );

    json +=
      "\",";


    json +=
      "\"radioAddress\":\"";

    json +=
      escapeJSON(
        chatHistory[i].radioAddress
      );

    json +=
      "\",";


    json +=
      "\"type\":\"";

    json +=
      escapeJSON(
        chatHistory[i].type
      );

    json +=
      "\",";


    json +=
      "\"message\":\"";

    json +=
      escapeJSON(
        chatHistory[i].message
      );

    json +=
      "\",";


    json +=
      "\"outgoing\":";


    if (
      chatHistory[i].outgoing
    ) {

      json +=
        "true";

    } else {

      json +=
        "false";
    }


    json +=
      "}";
  }


  json +=
    "]";


  server.send(
    200,
    "application/json",
    json
  );
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // USB SERIAL
  // ----------------------------------------------------------

  Serial.begin(
    115200
  );


  delay(
    1000
  );


  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "       RELAYDROP NODE"
  );

  Serial.println(
    "================================"
  );


  // ----------------------------------------------------------
  // AUTOMATIC IDENTITY
  // ----------------------------------------------------------

  generateNodeIdentity();


  // ----------------------------------------------------------
  // E22 UART
  // ----------------------------------------------------------

  E22Serial.begin(
    9600,
    SERIAL_8N1,
    E22_RX,
    E22_TX
  );


  // ----------------------------------------------------------
  // E22 NORMAL MODE
  // ----------------------------------------------------------

  pinMode(
    E22_M1,
    OUTPUT
  );


  digitalWrite(
    E22_M1,
    LOW
  );


  delay(
    200
  );


  Serial.println(
    "[E22] M0 = LOW"
  );

  Serial.println(
    "[E22] M1 = LOW"
  );

  Serial.println(
    "[E22] Normal / Transparent Mode"
  );


  // ----------------------------------------------------------
  // WIFI ACCESS POINT
  // ----------------------------------------------------------

  WiFi.mode(
    WIFI_AP
  );


  WiFi.softAP(
    wifiSSID.c_str(),
    WIFI_PASSWORD
  );


  Serial.println();

  Serial.println(
    "[WiFi] Access Point Created"
  );


  Serial.print(
    "[WiFi] SSID: "
  );

  Serial.println(
    wifiSSID
  );


  Serial.print(
    "[WiFi] IP: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // ----------------------------------------------------------
  // WEB ROUTES
  // ----------------------------------------------------------

  server.on(
    "/",
    handleHome
  );


  server.on(
    "/send",
    handleSend
  );


  server.on(
    "/messages",
    handleMessages
  );


  // ----------------------------------------------------------
  // START WEB SERVER
  // ----------------------------------------------------------

  server.begin();


  Serial.println(
    "[Web] Server started"
  );


  // ----------------------------------------------------------
  // READY
  // ----------------------------------------------------------

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "          READY"
  );

  Serial.println(
    "================================"
  );


  Serial.print(
    "RelayDrop UID: "
  );

  Serial.println(
    NODE_UID
  );


  Serial.print(
    "E22 Address: 0x"
  );

  Serial.println(
    E22_ADDRESS,
    HEX
  );


  Serial.print(
    "Wi-Fi: "
  );

  Serial.println(
    wifiSSID
  );


  Serial.println();

  Serial.println(
    "TX + RX enabled."
  );

  Serial.println();
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // Web server
  server.handleClient();


  // E22 receiver
  checkE22Messages();
}

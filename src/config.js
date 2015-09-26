var msg_queue = [];

function msg_ack(e) {
  console.log("Sent msg.");  
  msg_queue.shift();
  sendMessages();
}

function msg_nak(e) {
  console.log("Failed msg. resending");
  sendMessages();
}

function sendMessages() {
  if(msg_queue.length > 0) {
    Pebble.sendAppMessage(msg_queue[0], msg_ack, msg_nak);
  }
}

function updateMenu(conf) {
  var colors = JSON.parse(conf);

  var msg = {
    "secondColor": colors[0],
    "1": colors[1],
    "2": colors[2]
  };
  msg_queue.push(msg);  
  sendMessages();
}

Pebble.addEventListener('ready', function(e) {
    console.log('JavaScript ready.');
});

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  var loc = 'http://phytomine.github.io#' +
    encodeURIComponent(localStorage.getItem("config"));
  Pebble.openURL(loc);  
});

Pebble.addEventListener("appmessage", function(e) {
  updateMenu(localStorage.getItem("config"));
});


Pebble.addEventListener("webviewclosed", function(e) {
  if(e.response && e.response.length) {
    var config = decodeURIComponent(e.response);
    localStorage.setItem("config", config);
    updateMenu(config);
  }
});
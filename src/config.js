function updateMenu(conf){
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  dict['KEY_MESSAGE_TYPE'] = 1;
  if(configData['hide_seconds'] === true) {
    dict['KEY_HIDE_SECONDS'] = configData['hide_seconds'];
  }
  dict['KEY_HOUR_COLOR'] = configData['hour_color'];
  dict['KEY_SECOND_COLOR'] = configData['second_color'];
  dict['KEY_MINUTE_COLOR'] = configData['minute_color'];
  dict['KEY_TEMP_SCALE'] = configData['temp_scale'];  

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
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
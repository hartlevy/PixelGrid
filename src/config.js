function updateMenu(conf){
  var configData = JSON.parse(conf);
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  /*if(configData['hide_seconds'] === true) {
    dict['KEY_HIDE_SECONDS'] = configData['hide_seconds'];
  }*/
  dict['isWeather'] = 0;
  dict['KEY_HOUR_COLOR'] = parseInt(configData['hour_color']);
  dict['KEY_SECOND_COLOR'] = parseInt(configData['second_color']);
  dict['KEY_MINUTE_COLOR'] = parseInt(configData['minute_color']);
  dict['KEY_TEMP_SCALE'] = parseInt(configData['temp_scale']);  
  dict['KEY_BT_LOGO_TYPE'] = parseInt(configData['bt_logo']);
  dict['KEY_SHOW_ANIMATION'] = parseInt(configData['show_animation']);
  dict['KEY_HIDE_SECONDS'] = parseInt(configData['hide_seconds']);
  

  console.log('Dict: ' + JSON.stringify(dict));
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
  var loc = 'http://phytomine.github.io';
  console.log('Showing configuration page: ' + loc);
  Pebble.openURL(loc);  
});


Pebble.addEventListener("webviewclosed", function(e) {
  if(e.response && e.response.length) {
    var config = decodeURIComponent(e.response);
    updateMenu(config);
  }
});
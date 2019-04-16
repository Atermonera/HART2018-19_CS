// import dependencies
const express = require('express');
const app = express();
const ws = require('./websocket')

// constants
const port = 1500;

// setup express server
app.use(express.static('public'));
app.set('view engine', 'ejs');

// main (desktop) endpoint
app.get('/', function(req, res) {
  res.render('index');
});

// mobile endpoint
app.get('/mobile', function(req, res) {
  res.render('mobile');
});

// listen on 1500
app.listen(port, function() {
  console.log(new Date().toISOString() + ' - HART Groundstation listening on port ' + port + '!');
});
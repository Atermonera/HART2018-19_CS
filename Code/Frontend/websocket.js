// import dependencies
const WebSocket = require('ws');
const fs = require('fs');
const Papa = require('papaparse');

// globals
const WebSocketServer = WebSocket.Server;
const wss = new WebSocketServer({port: 1501});
// 0         1         2         3         4         5         6         7         8         9         10          11        12        13        14        15        16        17        18        19        20        21        22        23         24
// clock,    x_dist,   x_vel,    x_accel,  y_dist,   y_vel,    y_accel,  z_dist,   z_vel,    z_accel,  temp,       gyro_x,   gyro_y,   gyro_z,   unk,      unk,      unk,      unk,      unk,      unk,      unk,      unk,      unk,      lat,       lon
// 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 311.000000, 1.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 32.990281, -106.975009

const csvInputFile = 'data/clean_2019-04-13-13h-34m-24s.csv';
const bst_csvInputFile  = 'data/bst_clean_2019-04-14-15h-25m-54s.csv';//'data/clean_2019-04-13-13h-34m-24s.csv';
const sst_csvInputFile  = 'data/sst_clean_2019-04-14-15h-25m-54s.csv';
//const blockElements = 24;
const clockIndex    = 0;
const x_dstIndex    = 1;
const x_velIndex    = 2;
const x_accIndex    = 3;
const y_dstIndex    = 4;
const y_velIndex    = 5;
const y_accIndex    = 6;
const z_dstIndex    = 7;
const z_velIndex    = 8;
const z_accIndex    = 9;
const s_tmpIndex    = 10;
//const x_gyrIndex    = 11;
//const y_gyrIndex    = 12;
//const z_gyrIndex    = 13;
const s_latIndex    = 23;
const s_lonIndex    = 24;
var dataArr = [];
var bstDataArr = [];
var susDataArr = [];
var broadcastDataArr = [];
var dataLen = 0;
var idx = 1;      // skip the header

var test = 0;
var initBlock = 1;
var lastBlock = 0;


// open the data file
const file = fs.createReadStream(csvInputFile);
// parse the data
Papa.parse(file, {
  complete: function(results) {
    dataArr = results.data;
    //console.log("combo data size: " + dataArr.length);
    //lastBlock = results.data.length - 1;
    //initBlock = lastBlock;
  }
});


// open the data file
const bstFile = fs.createReadStream(bst_csvInputFile);
// parse the data
Papa.parse(bstFile, {
  complete: function(results) {
    bstDataArr = results.data;
    //console.log("booster data size: " + bstDataArr.length);
    //lastBlock = results.data.length - 1;
    //initBlock = lastBlock;
  }
});

// open the data file
const susFile = fs.createReadStream(sst_csvInputFile);
// parse the data
Papa.parse(susFile, {
  complete: function(results) {
    susDataArr = results.data;
    //console.log("sustainer data size: " + susDataArr.length);
    //lastBlock = results.data.length - 1;
    //initBlock = lastBlock;
  }
});

//function processData() {
//bstDataArr.length > susDataArr.length ? dataLen = susDataArr.length : dataLen = bstDataArr.length;

function processData() {
  
  dataLen = Math.max(bstDataArr.length, susDataArr.length);
  //console.log(susDataArr)
  for (var i = 0; i < dataLen; i++) {
    var line = [];
    line.push(i); // index = 0
    line.push(susDataArr[i][clockIndex]) // clock = 1
    line.push(bstDataArr[i][x_dstIndex]) // bst x pos = 2
    line.push(bstDataArr[i][y_dstIndex]) // bst y pos = 3
    line.push(bstDataArr[i][z_dstIndex]) // bst z pos = 4
    line.push(susDataArr[i][x_dstIndex]) // sus x pos = 5
    line.push(susDataArr[i][y_dstIndex]) // sus y pos = 6
    line.push(susDataArr[i][z_dstIndex]) // sus z pos = 7
    line.push(bstDataArr[i][x_velIndex]) // bst x vel = 8
    line.push(bstDataArr[i][y_velIndex]) // bst x vel = 9
    line.push(bstDataArr[i][z_velIndex]) // bst x vel = 10
    line.push(susDataArr[i][x_velIndex]) // sus x vel = 11
    line.push(susDataArr[i][y_velIndex]) // sus y vel = 12
    line.push(susDataArr[i][z_velIndex]) // sus z vel = 13
    line.push(bstDataArr[i][x_accIndex]) // bst x acc = 14
    line.push(bstDataArr[i][y_accIndex]) // bst y acc = 15
    line.push(bstDataArr[i][z_accIndex]) // bst z acc = 16
    line.push(susDataArr[i][x_accIndex]) // sus x acc = 17
    line.push(susDataArr[i][y_accIndex]) // sus y acc = 18
    line.push(susDataArr[i][z_accIndex]) // sus z acc = 19
    line.push(bstDataArr[i][s_tmpIndex]) // bst temp = 20
    line.push(susDataArr[i][s_tmpIndex]) // sus temp = 21
    line.push(bstDataArr[i][s_latIndex]) // bst lat = 22
    line.push(bstDataArr[i][s_lonIndex]) // bst lon = 23
    line.push(susDataArr[i][s_latIndex]) // sus lat = 24
    line.push(susDataArr[i][s_lonIndex]) // sus lon = 25
    broadcastDataArr.push(line);
  }
}
//processData();

// Broadcast to all.
wss.broadcast = function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};

wss.on('connection', function(ws) {
  processData();
  ws.on('message', function(message) {
    console.log('Received: ' + message);
    // Broadcast to everyone else.
    wss.clients.forEach(function(client) {
      //if (client !== ws && client.readyState === WebSocket.OPEN) {
        setInterval(() => idx < broadcastDataArr.length ? wss.broadcast(broadcastDataArr[idx++].toString()) : wss.broadcast(broadcastDataArr[idx-1].toString()), 25);
        //setInterval(() => idx < dataArr.length ? wss.broadcast(dataArr[idx++].toString()) : wss.broadcast(dataArr[idx-1].toString()), 25);
      //}
    });
  });
});

function sumXYZVectors(x_component, y_component, z_component) {
  return Math.sqrt(x_component*x_component + y_component*y_component + z_component+z_component)
} 











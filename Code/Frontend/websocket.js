// import dependencies
const WebSocket = require('ws');
const fs = require('fs');
const Papa = require('papaparse');

// globals
const WebSocketServer = WebSocket.Server;
const wss = new WebSocketServer({port: 1501});

// NEW DATA FORMAT
// 0            1         2         3            4         5         6             7         8         9            10              11        12          13              14        15         16           17        18  
// clock,       x_dist,   x_vel,    x_accel,     y_dist,   y_vel,    y_accel,      z_dist,   z_vel,    z_accel,     temp,           gyro_yaw, gyro_pitch, gyro_roll,      rot_yaw,  rot_pitch  rot_roll     lat,      lon
// 0.000000,    0.000000, 0.000000, 0.000000,    0.000000, 0.000000, 0.000000,     0.000000, 0.000000, 0.000000,    311.000000,     1.000000, 0.000000,   0.000000,       0.000000, 1.000000,  0.000000,    32.990281, -106.975009

const bst_csvInputFile_SIM  = 'data/bst_clean_2019-04-14-15h-25m-54s.csv';
const sst_csvInputFile_SIM  = 'data/sst_clean_2019-04-14-15h-25m-54s.csv';
const bst_csvInputFile  = '../Groundstation/bst_out.csv';
const sst_csvInputFile  = '../Groundstation/sst_out.csv';

/*
 * NOTE: these indices are for the NEW data format
 */
// mission clock
const clockIndex    = 0;
// stage x distance/velocity/acceleration
const x_dstIndex    = 1;
const x_velIndex    = 2;
const x_accIndex    = 3;
// stage y distance/velocity/acceleration
const y_dstIndex    = 4;
const y_velIndex    = 5;
const y_accIndex    = 6;
// stage z distance/velocity/acceleration
const z_dstIndex    = 7;
const z_velIndex    = 8;
const z_accIndex    = 9;
// stage temperature
const s_tmpIndex    = 10;
// stage gyro yaw/pitch/roll
const s_gyYIndex    = 11;
const s_gyPIndex    = 12;
const S_gyRIndex    = 13;
// stage rotation yaw/pitch/roll
const s_roYIndex    = 14;
const s_roPIndex    = 15;
const S_roRIndex    = 16;
// stage latitude/longitude
const s_latIndex    = 17;
const s_lonIndex    = 18;

var dataArr = [];
var bstDataArr = [];
var susDataArr = [];
var broadcastDataArr = [];
var dataLen = 0;
var idx = 1, idxB = 0, idxS = 0;      // skip the header

var test = 0;
var initBlock = 1;
var lastBlock = 0;

/*
 * file realtime read vars
 */
var bite_size = 300;//could probably be lowered to 283 or use separate for b/s
var readbytesB = 0, readbytesS = 0;
var lastLineFeedB, lastLineFeedS;
var lineArrayB, lineArrayS;
var lineCountB = 0, lineCountS = 0;
var valueArrayB = [], valueArrayS = [];

/*
 * booster file stuff
 */
// fs.open(path[, flags[, mode]], callback)
// REAL //fs.open(bst_csvInputFile, 'r', function(err, fd) { fileB = fd; readsomeB(); }); // REAL         <------- uncomment for live
fs.open(bst_csvInputFile_SIM, 'r', function(err, fd) { fileB = fd; readsomeB(); }); // SIMULATED

function readsomeB() {
    //console.log("****new test B*****");
    var stats = fs.fstatSync(fileB); // yes sometimes async does not make sense!
    if(stats.size < readbytesB+1) {
        setTimeout(readsomeB, 25);
    }
    else {
        // fs.read(fd, buffer, offset, length, position, callback)
        fs.read(fileB, new Buffer(bite_size), 0, bite_size, readbytesB, processsomeB);
    }
}

function processsomeB(err, bytecount, buff) {
    lastLineFeedB = buff.toString('utf-8', 0, bytecount).lastIndexOf('\n');

    if(lastLineFeedB > -1){

        // Split the buffer by line
        lineArrayB = buff.toString('utf-8', 0, bytecount).slice(0,lastLineFeedB).split('\n');
        //console.log(lineArrayB);
        lineCountB += lineArrayB.length;
        //console.log(lineCountB);
        // Then split each line by comma
        for(i=0;i<lineArrayB.length;i++){
            // Add read rows to an array for use elsewhere
            valueArrayB.push(lineArrayB[i].split(','));
            //console.log("** valueArrayB: " + valueArrayB.length);
        }   

        // Set a new position to read from
        readbytesB+=lastLineFeedB+1;
    } else {
        // No complete lines were read
        readbytesB+=bytecount;
    }
    process.nextTick(readsomeB);
}

/*
 * sustainer file stuff
 */
// fs.open(path[, flags[, mode]], callback)
// REAL //fs.open(sst_csvInputFile, 'r', function(err, fd) { fileS = fd; readsomeS(); }); // REAL         <------- uncomment for live
fs.open(sst_csvInputFile_SIM, 'r', function(err, fd) { fileS = fd; readsomeS(); }); // SIMULATED

function readsomeS() {
    //console.log("-----new test S----");
    var stats = fs.fstatSync(fileS); // yes sometimes async does not make sense!
    if(stats.size < readbytesS+1) {
        setTimeout(readsomeS, 25);
    }
    else {
        // fs.read(fd, buffer, offset, length, position, callback)
        fs.read(fileS, new Buffer(bite_size), 0, bite_size, readbytesS, processsomeS);
    }
}

function processsomeS(err, bytecount, buff) {
    lastLineFeedS = buff.toString('utf-8', 0, bytecount).lastIndexOf('\n');

    if(lastLineFeedS > -1){

        // Split the buffer by line
        lineArrayS = buff.toString('utf-8', 0, bytecount).slice(0,lastLineFeedS).split('\n');
        //console.log(lineArrayB);
        lineCountS += lineArrayS.length;
        //console.log(lineCountS);
        // Then split each line by comma
        for(i=0;i<lineArrayS.length;i++){
            // Add read rows to an array for use elsewhere
            valueArrayS.push(lineArrayS[i].split(','));
            //console.log("-- valueArrayS: " + valueArrayS.length);
        }   

        // Set a new position to read from
        readbytesS+=lastLineFeedS+1;
    } else {
        // No complete lines were read
        readbytesS+=bytecount;
    }
    process.nextTick(readsomeS);
}

/*
 * broadcast setup
 */
// Broadcast to all.
wss.broadcast = function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
};

wss.on('connection', function(ws) {

  console.log('Connected!');
  ws.on('message', function(message) {
    console.log('Received: ' + message);
    // Broadcast to everyone else.
    wss.clients.forEach(function(client) {
      //setInterval(function, milliseconds, param1, param2, ...) params are optional additional parameters to pass to the function
      setInterval(() => sendData(), 25);
    });
  });
});

/*
 * helper fns
 */
function sendData() {
  var line = [];
  var idxM = 0;

  if (valueArrayB.length == 0 || valueArrayS.length == 0) {
    console.log("Error: empty arrays!");
    return;
    
  } else if (idxB < valueArrayB.length - 1 && idxS < valueArrayS.length - 1) {
    // we have data from both stages
    // move to next index
    idxB += 1;
    idxS += 1;
    idxM = Math.max(idxB, idxS);

  } else if(idxB >= valueArrayB.length - 1 && idxS < valueArrayS.length - 1) {
    // we have data from sustainer only - NOTE: this is an anticipated case so it has precedence
    // set missing stage (booster) index to array max and increment active stage (sustainer)
    idxB = valueArrayB.length - 1;
    idxS += 1;
    idxM = Math.max(idxB, idxS);
    
  } else if(idxB < valueArrayB.length - 1 && idxS >= valueArrayS.length - 1) {
    // we have data from booster only
    // set missing stage (sustainer) index to array max and increment active stage (booster)
    idxB += 1;
    idxS = valueArrayS.length - 1;
    idxM = Math.max(idxB, idxS);
    
  } else {
    // we have no stage data
    // set missing stages (booster/sustainer) indexes to array maxes
    idxB = valueArrayB.length - 1;
    idxS = valueArrayS.length - 1;
    idxM = Math.max(idxB, idxS);
    
  }
  
  line.push(idxM);                          // index = 0
  // push the mission clock
  line.push(valueArrayS[idxM][clockIndex]); // clock = 1
  // push booster x,y,z distance
  line.push(valueArrayB[idxB][x_dstIndex]); // bst x pos = 2
  line.push(valueArrayB[idxB][y_dstIndex]); // bst y pos = 3
  line.push(valueArrayB[idxB][z_dstIndex]); // bst z pos = 4
  // push sustainer x,y,z distance
  line.push(valueArrayS[idxS][x_dstIndex]); // sus x pos = 5
  line.push(valueArrayS[idxS][y_dstIndex]); // sus y pos = 6
  line.push(valueArrayS[idxS][z_dstIndex]); // sus z pos = 7
  // push booster x,y,z velocity
  line.push(valueArrayB[idxB][x_velIndex]); // bst x vel = 8
  line.push(valueArrayB[idxB][y_velIndex]); // bst x vel = 9
  line.push(valueArrayB[idxB][z_velIndex]); // bst x vel = 10
  // push sustainer x,y,z velocity
  line.push(valueArrayS[idxS][x_velIndex]); // sus x vel = 11
  line.push(valueArrayS[idxS][y_velIndex]); // sus y vel = 12
  line.push(valueArrayS[idxS][z_velIndex]); // sus z vel = 13
  // push booster x,y,z acceleration
  line.push(valueArrayB[idxB][x_accIndex]); // bst x acc = 14
  line.push(valueArrayB[idxB][y_accIndex]); // bst y acc = 15
  line.push(valueArrayB[idxB][z_accIndex]); // bst z acc = 16
  // push sustainer x,y,z acceleration
  line.push(valueArrayS[idxS][x_accIndex]); // sus x acc = 17
  line.push(valueArrayS[idxS][y_accIndex]); // sus y acc = 18
  line.push(valueArrayS[idxS][z_accIndex]); // sus z acc = 19
  // push booster temperature
  line.push(valueArrayB[idxB][s_tmpIndex]); // bst temp = 20
  // push sustainer temperature
  line.push(valueArrayS[idxM][s_tmpIndex]); // sus temp = 21
  // push booster lat, lon
  line.push(valueArrayB[idxB][s_latIndex]); // bst lat = 22
  line.push(valueArrayB[idxB][s_lonIndex]); // bst lon = 23
  // push sustainer lat, lon
  line.push(valueArrayS[idxS][s_latIndex]); // sus lat = 24
  line.push(valueArrayS[idxS][s_lonIndex]); // sus lon = 25
    
  //console.log(line);
  wss.broadcast(line.toString());
  
  // old broadcast reference:
  //idx < valueArrayB.length ? wss.broadcast(valueArrayB[idx++].toString()) : wss.broadcast(valueArrayB[idx-1].toString())
}

function sumXYZVectors(x_component, y_component, z_component) {
  return Math.sqrt(x_component*x_component + y_component*y_component + z_component+z_component)
} 
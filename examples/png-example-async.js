var fs  = require('fs');
var Png = require('../build/Release/png').Png;
var Buffer = require('buffer').Buffer;

// the rgba-terminal.dat file is 1152000 bytes long.
var rgba = fs.readFileSync('./rgba-terminal.dat');

var png = new Png(rgba, 720, 400, 'rgba');
png.encode(function (data, error) {
    if (error) {
        console.log('Error: ' + error.toString());
        process.exit(1);
    }
    fs.writeFileSync('./png-async.png', data.toString('binary'), 'binary');
});


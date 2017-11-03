const express = require('express');
const SerialPort = require('serialport');
const Readline = SerialPort.parsers.Readline;

const PORT = 3000;
const SERIAL_DEVICE = process.argv[2];
const SERIAL_BAUD_RATE = 115200;

const app = express();
const port = new SerialPort(SERIAL_DEVICE, { baudRate: SERIAL_BAUD_RATE }, false);
const parser = port.pipe(new Readline());

const jobQueue = [];

function writeToSerial(command, callback) {
  port.write(command, 'ascii', err => {
    if (err) {
      console.log('Error writing to serial port.');
      return;
    }
    console.log(command, 'command written to serial port.');
    if (callback) callback();
  })
}

app.get('/status', (request, response) => {
  writeToSerial('STATUS', () => {
    jobQueue.unshift(status => {
      response.json({ status })
    });
  });
});

app.post('/on', (request, response) => {
  writeToSerial('ON');
  response.send('Command sent.');
});

app.post('/off', (request, response) => {
  writeToSerial('OFF');
  response.send('Command sent.');
});

app.listen(PORT, err => {
  if (err) {
    console.log('Something went wrong initializing the server.');
    return;
  }
  console.log(`Server is listening on port ${PORT}.`);
})

port.on('data', data => {
  const callback = jobQueue.pop();
  const dataString = data.toString('ascii').trim();
  callback(dataString);
  console.log('Arduino output:', dataString);
});

const express = require('express');
const bodyParser = require('body-parser');
const _ = require('lodash');
const SerialPort = require('serialport');
const ByteLength = SerialPort.parsers.ByteLength;

const PORT = 3000;
const SERIAL_DEVICE = process.argv[2];
const SERIAL_BAUD_RATE = 115200;

const app = express();
app.use(bodyParser.json());
const port = new SerialPort(SERIAL_DEVICE, { baudRate: SERIAL_BAUD_RATE }, false);
const parser = port.pipe(new ByteLength({ length: 1 }));

const jobQueue = [];

const lanternOpts = {
  lanternMode: ['off', 'on', 'pulse', 'sensory'],
  lightMode: ['off', 'on', 'pulse', 'mirror'],
  pulseSpeed: [1, 2, 3, 4],
};

function lanternsToBitField(flags) {
  return flags.reduce((bitField, flag) => bitField | 1 << flag, 0);
}

function optionsToBitField(requestOpts) {
  const { lanternMode, lightMode, lightColor } = _.mapValues(
    _.pick(requestOpts, _.keys(lanternOpts)),
    (value, key) => lanternOpts[key].indexOf(value)
  );
  const optArray = [lanternMode, lightMode, lightColor];
  if (optArray.includes(-1)) {
    throw new Error('Unknown option.');
  }
  return optArray.reduce((bitField, opt, index) => bitField | opt << 2 * index, 0);
}

function bitFieldToStatus(bitField) {
  return ['lanternMode', 'lightMode', 'pulseSpeed'].reduce(
    (opts, opt, idx) => {
      const optIndex = (bitField >> (2 * idx)) & 0x3;
      opts[opt] = lanternOpts[opt][optIndex];
      return opts; 
    }, {}
  );
}

function writeToSerial(command, callback) {
  if (callback) {
    jobQueue.unshift(callback);
  }
  port.write(command, 'binary', err => {
    if (err) {
      console.log('Error writing to serial port.');
      return;
    }
    console.log('Command written to serial port.');
  });
}

function writeLanternUpdate(opts, callback) {
  if (!opts) {
    throw new Error();
  }
  const command = Buffer.from([optionsToBitField(opts)]);
  writeToSerial(command, callback);
}

app.get('/status', (request, response) => {
  console.log('Status');
  writeToSerial(Buffer.from([255]), status => {
    response.json(bitFieldToStatus(status));
  });
});

app.post('/update', (request, response) => {
  let success;
  try {
    writeLanternUpdate(request.body, status => {
      response.json(bitFieldToStatus(status));
    });
  } catch (e) {
    console.log(e);
    response.status(400).send('Malformed lantern command.');
  }
});

app.listen(PORT, err => {
  if (err) {
    console.log('Something went wrong initializing the server.');
    return;
  }
  console.log(`Server is listening on port ${PORT}.`);
})

port.on('data', buf => {
  const callback = jobQueue.pop();
  const data = buf.readInt8(0);
  if (callback) callback(data);
  console.log('Arduino output:', data);
});

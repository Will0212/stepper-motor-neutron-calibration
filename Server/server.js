const express = require('express'); // Import for express
const { SerialPort } = require('serialport'); // Import for SerialPort
const { ReadlineParser } = require('@serialport/parser-readline'); // Import for ReadlineParser


// Initialize a new Express application instance
const app = express();

// Create a file system module instance
const fs = require('fs');

 
// Initialize SerialPort
const port = new SerialPort({ path: 'COM6', baudRate: 9600 });

// Initialize Parser
const parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

// Assign all initial values of motor
let currentStepCount = 0;
let currentSourcePosition = 0;
let currentDirection = 'CounterClockWise'; 
let currentOverrideStatus = 'OFF'; 
let onOffValue = 'OFF';
let currentDelayValue = 1000;

 
// Serve static files from 'public' folder
app.use(express.static('public'));
 
// Route to handle motor control commands
app.get('/control_motor', (req, res) => {
    const command = req.query.command;
    let arduinoCommand = command;

    // If the command is start set onOffValue to ON;
    if(command == "start"){
        onOffValue = 'ON';
    }
    // If the command is stop set the onOffValue to OFF;
    else if(command == 'stop'){
        onOffValue = 'OFF';
    }

    // Check if a value is provided for delay
    if (command === "setDelay" && req.query.value) {
        arduinoCommand += " " + req.query.value;
        currentDelayValue = parseInt(req.query.value, 10);
    }
 
    port.write(arduinoCommand + '\n', (err) => {
        if (err) {
            return res.status(500).send('Error on write: ' + err.message);
        }
        res.send('Command sent to Arduino: ' + arduinoCommand);
    });
});
 

// Route to get the source postion stored on the arduino to use in index.html
app.get('/get_source_position', (req, res) => {
    res.send({ sourcePosition: currentSourcePosition });
});


// Route to get the direction status stored on the arduino to use in index.html 
app.get('/get_direction_status', (req, res) => {
    res.send({ direction: currentDirection });
});


// Route to get the override status stored on the arduino to use in index.html  
app.get('/get_override_status', (req, res) => {
    res.send({ overrideOn: currentOverrideStatus }); 
});


// Route to get the delay value stored on the arduino to use in index.html 
app.get('/get_delay_value', (req, res) => {
    res.send({ delayValue: currentDelayValue }); 
});

 
// Open server on port 3000
app.listen(3000, () => {
    console.log('Server started on http://localhost:3000');
});


// Function to write the current source postion to a txt file in this folder, will create a new file if one is not already created
function logData(sourcePosition, direction, delayValue, onOffValue) 
{
    const filePath = './DataLog.txt';
    const now = new Date();
    const logEntry = `${now.toISOString()}: sourcePosition=${sourcePosition}, direction=${direction}, delayValue=${delayValue}, onOffValue=${onOffValue}\n`;

    fs.appendFile(filePath, logEntry, (err) => {
        if (err) {
            console.error('Error writing to file', err);
        } else {
        }
    });
}


// Parses data depending on the incoming data line
parser.on('data', line => {
    // Handle step count
    if (line.startsWith("stepCount:")) {
        currentStepCount = parseInt(line.split("stepCount:")[1]);
    }
 
    // Handle sourcePosition
    if (line.startsWith("sourcePosition:")) {
        if(onOffValue == 'ON'){ // Only log the data when the motor is OFF
            currentSourcePosition = parseFloat(line.split("sourcePosition:")[1]);
            logData(currentSourcePosition, currentDirection, currentDelayValue, onOffValue); // Log the updated sourcePosition
        }
        else if(onOffValue == 'OFF'){
            currentSourcePosition = parseFloat(line.split("sourcePosition:")[1]);
        }
    }
        // Handle direction status
    if (line.startsWith("direction:")) {
        currentDirection = line.split("direction:")[1].trim(); // This will be either 'ClockWise' or 'CounterClockWise'
    }
 
    // Handle override status
    if (line.startsWith("overrideOn:")) {
        currentOverrideStatus = line.split("overrideOn:")[1].trim(); // This will be either 'ON' or 'OFF'
        //console.log(currentOverrideStatus);
    }
    // ... handle other data as needed
});
import wasmEngineFactory from "../build/engine.js";
import downloadjs from 'downloadjs';
import nipplejs from 'nipplejs';

const fbCanvas = document.getElementById('framebuffer');
const fbContext = fbCanvas.getContext('2d');
const fbImage = fbContext.createImageData(320, 200);

function fbCanvasStatusMessage(text, color)
{
    // Blank canvas, including a 1-pixel black border for consistent edge blending
    fbContext.fillStyle = '#000';
    fbContext.fillRect(0, 0, 322, 202);

    fbContext.fillStyle = color;
    fbContext.font = '18px sans-serif';
    fbContext.fillText(text, 10, 20);
}

// Canvas resize handler
const resize = () => {
    const aspect = 4/3.0;
    const max_w = 960;
    const w = Math.min(Math.min(max_w, window.innerWidth),
        aspect * window.innerHeight)|0;
    const h = (w / aspect)|0;
    fbCanvas.style.width = w + 'px';
    fbCanvas.style.height = h + 'px';
};
window.addEventListener('resize', resize);
resize();

// Joystick is created immediately, but callbacks aren't hooked up until the engine loads
const joystick = nipplejs.create({
    zone: document.getElementById('joystick_xy'),
    mode: 'static',
    size: '140',
    threshold: 0.001,
    position: { left: '50%', top: '50%' }
});

// Make it easy to hide the joystick on desktop
document.getElementById('joystick_hide').addEventListener('click', (e) => {
    document.getElementById('joystick').style.display = 'none';
});

var asm = null;
fbCanvasStatusMessage("Loading", '#555');
try {
    asm = wasmEngineFactory();
    // For console debugging later
    window.ROEngine = asm;
} catch (e) {
    fbCanvasStatusMessage("Fail. No WebAssembly?", '#ddd');
    throw e;
}

asm.onSaveFileWrite = () => {
    const saveData = asm.getSaveFile();
    downloadjs(saveData, filenameForSaveData(saveData), 'application/octet-stream');

    // work in progress
    console.log(asm.packSaveFile());
};

asm.onRenderFrame = (rgbData) => {
    fbImage.data.set(rgbData);
    fbContext.putImageData(fbImage, 1, 1);
};

asm.audioSetup = () => {
    // Called via an event callback; necessary to unlock audio on iOS.

    if (asm.audioContext === undefined) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        asm.audioContext = new AudioContext();
        if (!asm.audioContext) {
            return false;
        }
    }

    if (asm.audioContext.state == 'suspended') {
        asm.audioContext.resume();
        if (asm.audioContext.state == 'suspended') {
            return false;
        }
    }

    return true;
};

asm.onRenderSound = (pcmData, rate) => {
    // TO DO: Volume control
    const volume = 0.1;

    if (volume > 0.0 && asm.audioSetup()) {
        const buffer = asm.audioContext.createBuffer(1, pcmData.length, rate);
        const channelData = buffer.getChannelData(0);
        for (var i = 0; i < pcmData.length; i++) {
            channelData[i] = pcmData[i] * volume;
        }

        const source = asm.audioContext.createBufferSource();
        source.buffer = buffer;
        source.connect(asm.audioContext.destination);
        source.start();
    }
};

function keycode(ascii, scancode)
{
    if (typeof(ascii) != typeof(0)) {
        ascii = ascii.length == 1 ? ascii.charCodeAt(0) : parseInt(ascii, 0);
    }
    asm.pressKey(ascii, scancode);
}

function controlCode(key)
{
    return String.fromCharCode(key.toUpperCase().charCodeAt(0) - 'A'.charCodeAt(0) + 1)
}

function onKeydown(e)
{
    if (e.code == "ArrowUp" && e.shiftKey == false) keycode(0, 0x48);
    else if (e.code == "ArrowUp" && e.shiftKey == true) keycode('8', 0x48);
    else if (e.code == "ArrowDown" && e.shiftKey == false) keycode(0, 0x50);
    else if (e.code == "ArrowDown" && e.shiftKey == true) keycode('2', 0x50);
    else if (e.code == "ArrowLeft" && e.shiftKey == false) keycode(0, 0x4B);
    else if (e.code == "ArrowLeft" && e.shiftKey == true) keycode('4', 0x4B);
    else if (e.code == "ArrowRight" && e.shiftKey == false) keycode(0, 0x4D);
    else if (e.code == "ArrowRight" && e.shiftKey == true) keycode('6', 0x4D);
    else if (e.code == "Backspace" && !e.ctrlKey && !e.altKey && !e.metaKey) keycode('\x08', 0);
    else if (e.code == "Enter" && !e.ctrlKey && !e.altKey && !e.metaKey) keycode('\x0D', 0x1C);
    else if (e.code == "Escape" && !e.ctrlKey && !e.altKey && !e.metaKey) keycode('\x1b', 0x01);
    else if (e.key.length == 1 && !e.ctrlKey && !e.altKey && !e.metaKey) keycode(e.key.toUpperCase(), 0);
    else if (e.key.length == 1 && e.ctrlKey && !e.altKey && !e.metaKey) keycode(controlCode(e.key), 0);
    else return;
    e.preventDefault();
}

function refocus(e)
{
    e.target.blur();
    document.getElementById('framebuffer').focus();    
}

const LAB_WORLD = 30;

function worldIdFromSaveData(bytes)
{
    if (bytes.length == 24389) {
        return bytes[bytes.length - 1];
    }
    return null;
}

function chipNameFromSaveData(bytes)
{
    if (bytes.length == 1333) {
        var name = '';
        const nameField = bytes.slice(0x40A, 0x41C);
        for (let byte of nameField) {
            if (byte < 0x20 || byte > 0x7F) {
                break;
            }
            name += String.fromCharCode(byte);
        }
        return name.trim() || "untitled";
    }
    return null;
}

function filenameForSaveData(bytes)
{
    const world = worldIdFromSaveData(bytes);
    const chip = chipNameFromSaveData(bytes);
    const now = new Date();

    if (world !== null) {
        if (world == LAB_WORLD) {
            return "robotodyssey-lab-" + now.toISOString() + ".lsv";
        } else {
            return "robotodyssey-world" + (world+1) + "-" + now.toISOString() + ".gsv";
        }
    }
    if (chip !== null) {
        return "robotodyssey-chip-" + chip + "-" + now.toISOString() + ".csv";
    }
    return "robotodyssey-" + now.toISOString() + ".bin";
}


asm.then(() =>
{
    document.body.addEventListener('keydown', onKeydown);

    joystick.on('move', (e, data) => {
        const scale = 8.0;
        const x = scale * data.force * Math.cos(data.angle.radian);
        const y = scale * data.force * Math.sin(data.angle.radian);
        asm.setJoystickAxes(x, -y);
        asm.audioSetup();
    });
    joystick.on('end', (e) => {
        asm.setJoystickAxes(0, 0);
        asm.audioSetup();
    });

    for (let button of document.getElementsByClassName('joystick_btn')) {
        button.addEventListener('mousedown', (e) => {
            asm.setJoystickButton(true);
            refocus(e);
            asm.audioSetup();
       });
        button.addEventListener('mouseup', (e) => {
            asm.setJoystickButton(false);
            refocus(e);
            asm.audioSetup();
        });
        button.addEventListener('touchstart', (e) => {
            e.preventDefault();
            asm.setJoystickButton(true);
            asm.audioSetup();
        });
        button.addEventListener('touchend', (e) => {
            e.preventDefault();
            asm.setJoystickButton(false);
            asm.audioSetup();
        });
    }

    for (let button of document.getElementsByClassName('exec_btn')) {
        button.addEventListener('click', (e) => {
            asm.exec(button.dataset.program, button.dataset.args);
            refocus(e);
            asm.audioSetup();
        });
    }

    for (let button of document.getElementsByClassName('keyboard_btn')) {
        const key = () => keycode(button.dataset.ascii, parseInt(button.dataset.scancode, 0));
        button.addEventListener('click', (e) => {
            key();
            asm.audioSetup();
            refocus(e);
        });
        button.addEventListener('touchend', (e) => {
            e.preventDefault();
            key();
            asm.audioSetup();
            refocus(e);
        });
    }

    for (let button of document.getElementsByClassName('setspeed_btn')) {
        button.addEventListener('click', (e) => {
            asm.setSpeed(parseFloat(button.dataset.speed));
            refocus(e);
        });
    }

    document.getElementById('savefile_btn').addEventListener('click', (e) => {
        document.getElementById('savefile_input').click();
        refocus(e);
        asm.audioSetup();
    });

    document.getElementById('savefile_input').addEventListener('change', (e) => {
        const files = e.target.files;
        if (files.length == 1 && files[0].size <= asm.MAX_FILESIZE) {
            const reader = new FileReader();
            reader.onload = (e) => asm.loadSaveFile(reader.result);
            reader.readAsArrayBuffer(files[0]);
        }
    });

    asm.loadSaveFile = (array) => {
        const bytes = new Uint8Array(array);
        const world = worldIdFromSaveData(bytes);

        asm.setSaveFile(bytes);

        if (world == LAB_WORLD) {
            asm.exec("lab.exe", "99");
        } else if (world !== null) {
            asm.exec("game.exe", "99");
        }
    };

    asm.downloadRAMSnapshot = (filename) => {
        downloadjs(asm.getMemory(), filename || 'ram-snapshot.bin', 'application/octet-stream');
    };
});


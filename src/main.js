import engine from "./engine.cpp";
import downloadjs from 'downloadjs';
import nipplejs from 'nipplejs';
import './main.css';

const asm = engine();
console.log("Loading wasm");

// For console debugging later
window.ROEngine = asm;

const exec = asm.cwrap('exec', 'number', ['string', 'string'])

function keycode(str, scancode)
{
    asm._pressKey((str || '\0').charCodeAt(0), scancode);
}

const fbCanvas = document.getElementById('framebuffer');
const fbContext = fbCanvas.getContext('2d');
const fbImage = fbContext.createImageData(320, 200);

asm.onRenderFrame = (rgbData) => {
    fbImage.data.set(rgbData);
    fbContext.putImageData(fbImage, 1, 1);
};

// Blank canvas, including a 1-pixel black border for consistent edge blending
fbContext.fillStyle = '#000';
fbContext.fillRect(0, 0, 322, 202);

// Temporary loading message
fbContext.fillStyle = '#555';
fbContext.font = '18px sans-serif';
fbContext.fillText('Loading', 40, 20);

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
    else if (e.code == "Enter") keycode('\x0D', 0x1C);
    else if (e.code == "Escape") keycode('\x1b', 0x01);
    else if (e.key.length == 1) keycode(e.key.toUpperCase(), 0);
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
        const nameField = bytes.slice(0x40A, 0x41C);
        console.log(nameField);
        var name = '';
        for (let byte of nameField) {
            if (byte >= 0x20 && byte <= 0x7F) {
                name += String.fromCharCode(byte);
            } else {
                break;
            }
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

asm.onSaveFileWrite = (saveData) => {
    downloadjs(saveData, filenameForSaveData(saveData), 'application/octet-stream');
};


asm.then(() =>
{
    document.body.addEventListener('keydown', onKeydown);

    joystick.on('move', (e, data) => {
        const scale = 8.0;
        const x = scale * data.force * Math.cos(data.angle.radian);
        const y = scale * data.force * Math.sin(data.angle.radian);
        asm._setJoystick(x, -y, false);
    });
    joystick.on('end', (e) => {
        asm._setJoystick(0, 0, false);
    });

    for (let button of document.getElementsByClassName('joystick_btn')) {
        button.addEventListener('mousedown', (e) => {
            asm._setJoystick(0, 0, true);
            refocus(e);
        });
        button.addEventListener('mouseup', (e) => {
            asm._setJoystick(0, 0, false);
            refocus(e);
        });
        button.addEventListener('touchstart', (e) => {
            e.preventDefault();
            asm._setJoystick(0, 0, true);
        });
        button.addEventListener('touchend', (e) => {
            e.preventDefault();
            asm._setJoystick(0, 0, false);
        });
    }

    for (let button of document.getElementsByClassName('exec_btn')) {
        button.addEventListener('click', (e) => {
            exec(button.dataset.program, button.dataset.args);
            refocus(e);
        });
    }

    for (let button of document.getElementsByClassName('keyboard_btn')) {
        const key = () => keycode(button.dataset.ascii, parseInt(button.dataset.scancode, 0));
        button.addEventListener('click', (e) => {
            key();
            refocus(e);
        });
        button.addEventListener('touchend', (e) => {
            e.preventDefault();
            key();
        });
    }

    for (let button of document.getElementsByClassName('setspeed_btn')) {
        button.addEventListener('click', (e) => {
            asm._setSpeed(parseFloat(button.dataset.speed));
            refocus(e);
        });
    }

    document.getElementById('savefile_btn').addEventListener('click', (e) => {
        document.getElementById('savefile_input').click();
        refocus(e);
    });

    document.getElementById('savefile_input').addEventListener('change', (e) => {
        const files = e.target.files;
        if (files.length == 1 && files[0].size <= 0x10000) {
            const reader = new FileReader();
            reader.onload = (e) => asm.loadSaveFile(reader.result);
            reader.readAsArrayBuffer(files[0]);
        }
    });

    asm.loadSaveFile = (array) => {
        const bytes = new Uint8Array(array);
        asm._setSaveFileSize(bytes.length);
        asm.HEAPU8.set(bytes, asm._saveFilePointer());

        if (worldIdFromSaveData(bytes) == LAB_WORLD) {
            exec("lab.exe", "99");
        } else {
            exec("game.exe", "99");
        }
    };

    asm.downloadRAMSnapshot = (filename) => {
        const ptr = asm._memPointer();
        const size = asm._memSize();
        const ram = asm.HEAPU8.subarray(ptr, ptr+size);
        downloadjs(ram, filename || 'ram-snapshot.bin', 'application/octet-stream');
    };

    asm._start();
});
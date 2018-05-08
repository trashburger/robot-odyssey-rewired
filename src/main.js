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
}

function filenameForSaveData(bytes)
{
    const world = worldIdFromSaveData(bytes);
    const now = new Date();
    if (world == LAB_WORLD) {
        return "robotodyssey-lab-" + now.toISOString() + ".lsv";
    } else {
        return "robotodyssey-world" + (world+1) + "-" + now.toISOString() + ".gsv";
    }
}

asm.then(() =>
{
    const fbCanvas = document.getElementById('framebuffer');
    const fbContext = fbCanvas.getContext('2d');
    const fbImage = fbContext.createImageData(320, 200);

    // Blank canvas, including a 1-pixel black border for consistent edge blending
    fbContext.fillStyle = '#000000';
    fbContext.fillRect(0, 0, 322, 202);

    document.body.addEventListener('keydown', onKeydown);

    asm.onRenderFrame = (rgbData) => {
        fbImage.data.set(rgbData);
        fbContext.putImageData(fbImage, 1, 1);
    };

    asm.onSaveFileWrite = (saveData) => {
        downloadjs(saveData, filenameForSaveData(saveData), 'application/octet-stream');
    };

    const joystick = nipplejs.create({
        zone: document.getElementById('joystick_zone'),
        maxNumberOfNipples: 1,
    });
    joystick.on('move', (e, data) => {
        const scale = 4.0;
        const x = scale * data.force * Math.cos(data.angle.radian);
        const y = scale * data.force * Math.sin(data.angle.radian);
        asm._setJoystick(x, -y, 0);
    });
    joystick.on('end', (e) => {
        asm._setJoystick(0, 0, 0);
    });

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

    document.getElementById('ram_snapshot_btn').addEventListener('click', (e) => {
        const ptr = asm._memPointer();
        const size = asm._memSize();
        const ram = asm.HEAPU8.subarray(ptr, ptr+size);
        downloadjs(ram, 'ram-snapshot.bin', 'application/octet-stream');
        refocus(e);
    });

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

    asm._start();
});
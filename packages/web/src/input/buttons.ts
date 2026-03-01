// import { audioContextSetup } from '../sound.ts';
// import { mouseTrackingEnd } from './mouse.ts';
// import * as GameMenu from '../gameMenu.ts';
// import * as FileManager from '../files/fileManager.ts';
// import * as AutoSave from '../files/autoSave.ts';
// import * as EngineLoader from '../engineLoader.ts';
//
// const canvas = document.getElementById('framebuffer');
// const speed_selector = document.getElementById('speed_selector');
// const gamepad_button_mappings = [];
//
// export function updateMappedGamepadButton(pressed, index) {
//     const handler = gamepad_button_mappings[index];
//     if (handler) {
//         handler(pressed, index);
//     }
// }
//
// export function addButtonClick(button_element, click) {
//     addButtonEvents(
//         button_element,
//         () => {
//             button_element.classList.add('active_btn');
//         },
//         () => {
//             button_element.classList.remove('active_btn');
//         },
//         click,
//     );
// }
//
// export function addButtonEvents(button_element, down, up, click) {
//     const down_wrapper = function (e) {
//         mouseTrackingEnd();
//         audioContextSetup();
//         if (!click) {
//             e.preventDefault();
//         }
//         if (down) {
//             down(e);
//         }
//     };
//
//     const up_wrapper = function (e) {
//         if (!click) {
//             e.preventDefault();
//         }
//         if (up) {
//             up(e);
//         }
//         button_element.blur();
//         canvas.focus();
//     };
//
//     const options = {
//         passive: !!click,
//     };
//
//     button_element.addEventListener('mousedown', down_wrapper, options);
//     button_element.addEventListener('mouseup', up_wrapper, options);
//     button_element.addEventListener('mouseleave', up_wrapper, options);
//
//     button_element.addEventListener('touchstart', down_wrapper, options);
//     button_element.addEventListener('touchend', up_wrapper, options);
//     button_element.addEventListener('touchcancel', up_wrapper, options);
//
//     if (click) {
//         button_element.addEventListener('click', click);
//     }
//
//     const gamepad_button = parseInt(button_element.dataset.gamepad);
//     if (gamepad_button >= 0) {
//         gamepad_button_mappings[gamepad_button] = (pressed) => {
//             if (pressed) {
//                 down();
//             } else {
//                 if (click) {
//                     click();
//                 }
//                 up();
//             }
//         };
//     }
// }
//
//
// export function init() {
//     document.body.addEventListener('keydown', function (e) {
//         const press = GameMenu.pressKey;

//         audioContextSetup();
//
//         if (code.includes('Arrow')) {
//             // Most keys can coexist with mouse tracking, but arrow keys will take over.
//             mouseTrackingEnd();
//         }
//
//         // Eat events for any recognized keys
//         e.preventDefault();
//     });
//
//     let delay = null;
//     let repeater = null;
//     function stopRepeat() {
//         if (delay !== null) {
//             clearTimeout(delay);
//             delay = null;
//         }
//         if (repeater !== null) {
//             clearInterval(repeater);
//             repeater = null;
//         }
//     }
//
//     for (let button of document.getElementsByClassName('keyboard_btn')) {
//         const press = () => {
//             delay = null;
//             GameMenu.pressKey(
//                 button.dataset.ascii || '0x00',
//                 parseInt(button.dataset.scancode || '0', 0),
//             );
//         };
//
//         addButtonEvents(
//             button,
//             () => {
//                 button.classList.add('active_btn');
//                 press();
//                 stopRepeat();
//                 if (button.dataset.rdelay && button.dataset.rrate) {
//                     delay = setTimeout(() => {
//                         stopRepeat();
//                         repeater = setInterval(
//                             press,
//                             parseInt(button.dataset.rrate),
//                         );
//                     }, parseInt(button.dataset.rdelay));
//                 }
//             },
//             () => {
//                 button.classList.remove('active_btn');
//                 stopRepeat();
//             },
//         );
//     }
//
//     speed_selector.addEventListener('change', async (e) => {
//         if (e && e.target) {
//             e.target.blur();
//             canvas.focus();
//         }
//         const engine = await EngineLoader.complete;
//         engine.setSpeed(parseFloat(speed_selector.value));
//     });
//
//     for (let button of document.getElementsByClassName('speed_adjust_btn')) {
//         addButtonEvents(
//             button,
//             async () => {
//                 let i =
//                     speed_selector.selectedIndex +
//                     parseInt(button.dataset.value);
//                 if (i >= 0 && i < speed_selector.length) {
//                     speed_selector.selectedIndex = i;
//                     const engine = await EngineLoader.complete;
//                     engine.setSpeed(parseFloat(speed_selector.value));
//                     button.classList.add('active_btn');
//                 }
//             },
//             () => {
//                 button.classList.remove('active_btn');
//             },
//         );
//     }
//
//     for (let button of document.getElementsByClassName('savezip_btn')) {
//         addButtonClick(button, async () => {
//             const engine = await EngineLoader.complete;
//             engine.downloadArchive();
//         });
//     }
//
//     for (let button of document.getElementsByClassName('filepicker_btn')) {
//         addButtonClick(button, async () => {
//             const engine = await EngineLoader.complete;
//             engine.filePicker();
//         });
//     }
//
//     for (let button of document.getElementsByClassName('copy_url_btn')) {
//         addButtonClick(button, async () => {
//             const engine = await EngineLoader.complete;
//             const save_status = await AutoSave.doAutoSave();
//             const href = window.location.href;
//             switch (save_status) {
//                 case engine.SaveStatus.OK:
//                     await window.navigator.clipboard.writeText(href);
//                     GameMenu.modal('Ok!\n\nGame copied to clipboard');
//                     break;
//                 case engine.SaveStatus.BLOCKED:
//                     break;
//                 case engine.SaveStatus.NOT_SUPPORTED:
//                     await window.navigator.clipboard.writeText(href);
//                     GameMenu.modal("Can't save game here, copied\n" + href);
//                     break;
//             }
//         });
//     }
//
//     for (let button of document.getElementsByClassName('fullscreen_btn')) {
//         addButtonClick(button, async () => {
//             if (!document.fullscreenElement) {
//                 document.body.requestFullscreen();
//             } else if (document.exitFullscreen) {
//                 document.exitFullscreen();
//             }
//         });
//     }
//
//     for (let button of document.getElementsByClassName('exec_btn')) {
//         addButtonClick(button, async () => {
//             // Go to the EXEC_LAUNCHING state, either via closing
//             // the file manager (if applicable) or waiting for the
//             // engine to finish loading.
//             if (!FileManager.close(GameMenu.States.EXEC_LAUNCHING)) {
//                 await GameMenu.afterLoadingState();
//                 GameMenu.setState(GameMenu.States.EXEC_LAUNCHING);
//             }
//             const args = button.dataset.exec.split(' ');
//             EngineLoader.instance.exec(args[0], args[1] || '');
//         });
//     }
// }

// const zone = document.getElementById('joystick_zone');
// const center = document.getElementById('joystick_center');
// const thumb = document.getElementById('joystick_thumb');
//
// function joystickAxes(x, y) {
//     const engine = EngineLoader.instance;
//
//     if (zone && thumb) {
//         const zone_rect = zone.getBoundingClientRect();
//         const scale = 0.5 * zone_rect.height;
//         thumb.style.transform =
//             'translate(' + scale * x + 'px, ' + scale * y + 'px)';
//     }
//
/// ...
//     if (engine.calledRun) {
//         engine.setJoystickAxes(cx, cy);
//         engine.autoSave();
//     }
//
// function joystickButton(b) {
//     const engine = EngineLoader.instance;
//
//     if (engine.calledRun) {
//         engine.setJoystickButton(b);
//         engine.autoSave();
//     }
//
//     GameMenu.setJoystickButton(b);
// }
//
// function onMove(e) {
//     if (center && zone) {
//         const zone_rect = zone.getBoundingClientRect();
//         const center_rect = center.getBoundingClientRect();
//         const scale = 2.0 / zone_rect.height;
//         const x = (e.clientX - center_rect.x) * scale;
//         const y = (e.clientY - center_rect.y) * scale;
//         joystickAxes(
//             Math.max(-1, Math.min(1, x)),
//             Math.max(-1, Math.min(1, y)),
//         );
//         e.preventDefault();
//     }
// }
//
// function onBegin(e) {
//     mouseTrackingEnd();
//     audioContextSetup();
//     if (thumb && zone) {
//         thumb.classList.add('active_btn');
//         zone.onpointermove = onMove;
//         zone.setPointerCapture(e.pointerId);
//     }
//     onMove(e);
// }
//
// function onEnd(e) {
//     joystickAxes(0, 0);
//     if (thumb && zone) {
//         thumb.classList.remove('active_btn');
//         zone.onpointermove = null;
//         zone.releasePointerCapture(e.pointerId);
//     }
//     e.preventDefault();
// }
//
// export function init() {
//     const engine = EngineLoader.instance;
//
//     if (thumb && zone) {
//         zone.addEventListener('pointerdown', onBegin);
//         zone.addEventListener('pointerup', onEnd);
//         zone.addEventListener('pointercancel', onEnd);
//     }
//
//     for (let button of document.getElementsByClassName('joystick_btn')) {
//         Buttons.addButtonEvents(
//             button,
//             () => {
//                 button.classList.add('active_btn');
//                 joystickButton(true);
//             },
//             () => {
//                 button.classList.remove('active_btn');
//                 joystickButton(false);
//             },
//         );
//     }
//

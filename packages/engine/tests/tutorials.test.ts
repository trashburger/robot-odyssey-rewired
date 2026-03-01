import { expect, test } from 'vitest';
import { testArrowKeyMovement, testRawJoystickMovement, testRelativePixelJoystickMovement } from './movement.ts';
import { engineOutputSummary } from './summarize.ts';
import { runEngineUntilFrame } from './run_engine.ts';
import { loadEngine } from './loader.ts';

test('robot anatomy tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '21')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('robot anatomy tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '21')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('robot anatomy tutorial movement with single joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '21')).toBeTruthy();
    testRawJoystickMovement(engine);
});

test('robot anatomy tutorial movement with relative pixel joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '21')).toBeTruthy();
    testRelativePixelJoystickMovement(engine);
});

test('robot wiring tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '22')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('robot wiring tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '22')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('sensors tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '23')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('sensors tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '23')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('toolkit tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '24')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('toolkit tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '24')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('robot circuits tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '25')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('robot circuits tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '25')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('robot teamwork tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '26')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('robot teamwork tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('tut.exe', '26')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('chip design tutorial engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '27')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('chip design tutorial movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '27')).toBeTruthy();
    testArrowKeyMovement(engine);
});

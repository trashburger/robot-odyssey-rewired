import { expect, test } from 'vitest';
import { testArrowKeyMovement, testRawJoystickMovement, testRelativePixelJoystickMovement } from './movement.ts';
import { engineOutputSummary } from './summarize.ts';
import { runEngineUntilFrame } from './run_engine.ts';
import { loadEngine } from './loader.ts';

test('innovation lab engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '30')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('innovation lab movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '30')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('innovation lab movement with single joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '30')).toBeTruthy();
    testRawJoystickMovement(engine);
});

test('innovation lab movement with relative pixel joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('lab.exe', '30')).toBeTruthy();
    testRelativePixelJoystickMovement(engine);
});

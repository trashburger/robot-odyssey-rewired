import { expect, test } from 'vitest';
import { testArrowKeyMovement, testRawJoystickMovement, testRelativePixelJoystickMovement } from './movement.ts';
import { engineOutputSummary } from './summarize.ts';
import { runEngineUntilFrame } from './run_engine.ts';
import { loadEngine } from './loader.ts';

test('robotropolis engine output until frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('game.exe')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilFrame(engine))).toMatchSnapshot();
});

test('robotropolis movement with arrow keys', async () => {
    const engine = await loadEngine();
    expect(engine.exec('game.exe')).toBeTruthy();
    testArrowKeyMovement(engine);
});

test('robotropolis movement with single joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('game.exe')).toBeTruthy();
    testRawJoystickMovement(engine);
});

test('robotropolis movement with relative pixel joystick events', async () => {
    const engine = await loadEngine();
    expect(engine.exec('game.exe')).toBeTruthy();
    testRelativePixelJoystickMovement(engine);
});

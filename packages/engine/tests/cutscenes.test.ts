import { expect, test } from 'vitest';
import { runEngineUntilDone, collectSingleFrame } from './run_engine.ts';
import { engineOutputSummary, engineFrameSummary } from './summarize.ts';
import { loadEngine } from './loader.ts';

test('new game cutscene engine output', async () => {
    const engine = await loadEngine();
    expect(engine.exec('show.exe')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilDone(engine))).toMatchSnapshot();
});

test('new game cutscene collected as single frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('show.exe')).toBeTruthy();
    expect(engineFrameSummary(collectSingleFrame(engine, 30))).toMatchSnapshot();
});

test('end game cutscene engine output', async () => {
    const engine = await loadEngine();
    expect(engine.exec('show2.exe')).toBeTruthy();
    expect(engineOutputSummary(runEngineUntilDone(engine))).toMatchSnapshot();
});

test('end game cutscene collected as single frame', async () => {
    const engine = await loadEngine();
    expect(engine.exec('show2.exe')).toBeTruthy();
    expect(engineFrameSummary(collectSingleFrame(engine, 10))).toMatchSnapshot();
});

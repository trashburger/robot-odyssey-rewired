import { expect, test } from 'vitest';
import { ExecError } from '@robot-odyssey-rewired/engine';
import { EngineLoader } from '@robot-odyssey-rewired/engine/dist/node/loader.js';
import { fileInfoSummary } from './summarize.ts';
import { loadEngine } from './loader.ts';

test('wasm engine instance can be loaded', async () => {
    await loadEngine();
});

test('multiple wasm instances can be loaded', async () => {
    await loadEngine();
    await loadEngine();
});

test('engine can allocate memory', async () => {
    const engine = await loadEngine();
    const a = engine.alloc(1000);
    const b = engine.alloc(1);
    expect(a).toBeGreaterThanOrEqual(100000);
    expect(b).toBeGreaterThanOrEqual(a + 1000);
});

test('engine provides memory access', async () => {
    const engine = await loadEngine();
    expect(engine.ioBuffer.length).toBeGreaterThanOrEqual(10000);
    expect(engine.mem.length).toBeGreaterThanOrEqual(100000);
});

test('engine decompresses and reports bundled files', async () => {
    const engine = await loadEngine();
    expect(fileInfoSummary(engine.staticFiles)).toMatchSnapshot();
});

test('run engine with no process active', async () => {
    const engine = await loadEngine();
    expect(engine.run()).toBe(null);
    expect(engine.run()).toBe(null);
    expect(engine.run()).toBe(null);
});

test('exec fails if the process is not recognized', async () => {
    const engine = await loadEngine();
    expect(engine.exec('nope.exe')).toBeFalsy();
    expect(engine.exec('game')).toBeFalsy();
    expect(engine.exec('lab')).toBeFalsy();
    expect(engine.exec('lab.exe ')).toBeFalsy();
    expect(engine.exec('')).toBeFalsy();
});

test('exec accepts all expected file names', async () => {
    const engine = await loadEngine();
    expect(engine.exec('game.EXE')).toBeTruthy();
    expect(engine.exec('GaMe.ExE')).toBeTruthy();
    expect(engine.exec('lab.exe')).toBeTruthy();
    expect(engine.exec('tut.exe')).toBeTruthy();
    expect(engine.exec('SHOW.EXE')).toBeTruthy();
    expect(engine.exec('SHOW2.EXE')).toBeTruthy();
});

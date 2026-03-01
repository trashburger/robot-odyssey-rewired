import { type Engine } from '@robot-odyssey-rewired/engine';
import { NodeEngineLoader } from '@robot-odyssey-rewired/engine/dist/node/loader.js';

export function loadEngine(): Promise<Engine> {
    return new NodeEngineLoader().promise;
}

import * as process from 'node:process';

import { type Engine, type EngineOutput } from '@robot-odyssey-rewired/engine';
import { loadEngine } from '@robot-odyssey-rewired/engine/dist/node/loader.js';

async function exec(engine: Engine, exe: string, arg: string): Promise<void> {
    console.log(`exec('${exe}', '${arg}')`);
    if (!engine.exec(exe, arg)) {
        throw 'exec failed';
    }

    let output;
    while ((output = engine.run())) {
        for (var item of output) {
            console.log(item);
            if (item[0] === 'error') {
                throw item[1];
            }
        }
    }
}

async function main() {
    await exec(await loadEngine(), process.argv[2] || '', process.argv[3] || '');
}

main().catch((error) => {
    console.warn(error);
});

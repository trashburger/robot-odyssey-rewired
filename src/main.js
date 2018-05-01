import engine from "./engine.cpp";

const engine_inst = engine();
const start = engine_inst.cwrap('start');

engine_inst.then(() => {

	start();

});


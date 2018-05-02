import engine from "./engine.cpp";

const engine_inst = engine();
console.log("Loading wasm");

engine_inst.then(() => {
	console.log("Engine ready");
	engine_inst._start();
	console.log("Started game");
});


import engine from "./engine.cpp";

const engine_inst = engine();
const start = engine_inst.cwrap('start');
const tick = engine_inst.cwrap('tick');

console.log("Loading wasm");

engine_inst.then(() => {
	console.log("Engine ready");

	document.getElementById('start_btn').onclick = () => {
		console.log("starting");
		start();
		console.log("back from starting");
	}

	document.getElementById('tick_btn').onclick = () => {
		console.log("tick");
		tick();
		console.log("return from tick");
	}

});


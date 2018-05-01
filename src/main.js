import engine from "./engine.cpp";

const engine_inst = engine();
const increment = engine_inst.cwrap('increment');

engine_inst.then(() => {

	alert(increment(1));

});


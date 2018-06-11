import './game_menu.css'

export const States = {
	SPLASH: 0,
	MENU: 1,
	EXEC: 2,
	LOADING: 3,
	ERROR: 4,
};

const game_menu = document.getElementById('game_menu');
const splash = document.getElementById('splash');

var timer = null;
var current_state = States.S_SPLASH;
var menu_choice = 0;
var loaded_engine = null;


export function showError(err)
{
	setState(States.S_ERROR);
    const el = document.getElementById("error");

    err = err.toString();
    if (err.includes("no binaryen method succeeded")) {
        // This is obtuse; we only build for wasm, so really this means the device doesn't support wasm
        err = "No WebAssembly?\n\nSorry, this browser might not be supported.";
    } else {
        // Something else went wrong.
        err = "Fail.\n\n" + err;
    }

    el.innerText = err;
    el.style.display = "block";
}

export function engineLoaded(engine)
{
	loaded_engine = engine;
	if (current_state == States.S_LOADING) {
		setState(States.S_EXEC);
	}
}

export function init(engine)
{
	// Event handlers during splashscreen
	splash.addEventListener('mousedown', function () {
		setState(States.MENU);
	});
	splash.addEventListener('touchstart', function () {
		setState(States.MENU);
	});
	splash.addEventListener('keydown', function (e) {
		if (!e.ctrlKey && !e.altKey && !e.metaKey && !e.shiftKey) {
			setState(States.MENU);
		}
	});

	getLastSplashImage().addEventListener('animationend', function () {
		setStateWithDelay(States.MENU, 2000);
	});

	engine.onProcessExit = function () {
		setState(States.MENU);
	};

	setState(States.SPLASH);
}

function getLastSplashImage()
{
	var result = null;
	for (let child of splash.children) {
		if (child.nodeName == 'IMG') {
			result = child;
		}
	}
	return result;
}

function setStateWithDelay(s, timeout)
{
	if (timer) {
		clearTimeout(timer);
	}
	timer = setTimeout(function () {
		setState(s);
	}, timeout);
}

function setVisFocus(element_id, vis, focus)
{
	const element = document.getElementById(element_id);

	if (vis) {
		element.classList.remove("hidden");
	} else {
		element.classList.add("hidden");
	}

	if (focus === true) {
		element.focus();
	} else if (focus === false) {
		element.blur();
	}
}

export function setState(s)
{
	current_state = s;
	if (timer) {
		clearTimeout(timer);
		timer = null;
	}

	if (s == States.MENU) {
		menu_choice = 0;
	}

	setVisFocus('splash', s == States.SPLASH || s == States.MENU, s == States.SPLASH);
	setVisFocus('game_menu', s == States.MENU, s == States.MENU);
	setVisFocus('framebuffer', s == States.EXEC, s == States.EXEC);
	setVisFocus('engine_controls', s == States.EXEC);

	if (s == States.EXEC) {
		loaded_engine.exec("game.exe", "");
	}
}

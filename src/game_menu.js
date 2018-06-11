import './game_menu.css'

export const States = {
	// Same order as Z stacking and transitions
	SPLASH: 0,
	MENU: 1,
	EXEC: 2,
};

const game_menu = document.getElementById('game_menu');
const splash = document.getElementById('splash');
const engine_area = document.getElementById('engine_area');
var timer = null;

export function init(engine)
{
	splash.addEventListener('mousedown', function () {
		setState(States.MENU);
	});

	splash.addEventListener('touchstart', function () {
		setState(States.MENU);
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

function setVisibility(element, visible)
{
	if (visible) {
		element.classList.remove("hidden");
	} else {
		element.classList.add("hidden");		
	}
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

export function setState(s)
{
	if (timer) {
		clearTimeout(timer);
		timer = null;
	}

	setVisibility(splash, s >= States.SPLASH);
	setVisibility(game_menu, s >= States.MENU);
	setVisibility(engine_area, s >= States.EXEC);
}

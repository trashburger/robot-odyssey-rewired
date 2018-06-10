import './game_menu.css'

export function init(engine)
{
	const game_menu = document.getElementById('game_menu');
	const splash = document.getElementById('splash');
	const engine_area = document.getElementById('engine_area');

	game_menu.addEventListener('animationend', function () {

		game_menu.classList.add('hidden');
		splash.classList.add('hidden');
		engine_area.classList.remove('hidden');

		// temp
        engine.exec("show.exe", "");
		engine.onProcessExit = function () {
			engine.exec("game.exe", "");
			engine.onProcessExit = null;
		};

	});
}

export function engineLoaded(engine)
{

}

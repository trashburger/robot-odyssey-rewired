export function loadingBegin()
{
    document.getElementById("engine_loading").style.display = "block";
}

export function loadingError(err)
{
    document.getElementById("engine_loading").style.display = "none";
    const el = document.getElementById("engine_error");
    el.innerText = "Fail.\n\n" + err;
    el.style.display = "block";
}

export function loadingDone()
{
    document.getElementById("engine_loading").style.display = "none";
    document.getElementById("controls").className = "controls_visible";
}

use std::process::Command;
use std::env;

fn python(args: &Vec<String>) {
    let status = Command::new("python3").args(args).status();
    if !status.expect("Failed to execute python subprocess").success() {
        panic!("Error from Python process while running {:?}", args);
    }
}

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let original_dir = env::var("ORIGINAL_DIR").unwrap_or("../original".to_string());

    python(&vec!["scripts/check-originals.py".to_string(),
            original_dir.clone(), out_dir.clone()]);

    let game_scripts = [
        "bt_game.py", "bt_lab.py", "bt_tutorial.py",
        "bt_menu.py", "bt_renderer.py"
    ];

    for script in game_scripts.iter() {
        python(&vec![format!("scripts/{}", script), out_dir.clone()]);
    }
}

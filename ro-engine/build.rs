use std::env;
use std::ffi::OsStr;
use std::fmt::Debug;
use std::process::Command;

const GAME_SCRIPTS: [&str; 5] = [
    "bt_game.py",
    "bt_lab.py",
    "bt_tutorial.py",
    "bt_menu.py",
    "bt_renderer.py",
];

fn python<S: AsRef<OsStr> + Debug>(args: &[S]) {
    let status = Command::new("python3")
        .args(args)
        .status()
        .expect("Failed to execute python subprocess");
    if !status.success() {
        panic!("Error from Python process while running {:?}", args);
    }
}

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let original_dir = env::var("ORIGINAL_DIR").unwrap_or("../original".to_string());

    python(&["scripts/check-originals.py", &original_dir, &out_dir]);

    for script in GAME_SCRIPTS.iter() {
        let script = format!("scripts/{}", script);
        python(&[&script, &out_dir]);
    }
}

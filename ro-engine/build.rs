use std::process::Command;
use std::env;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let original_dir = env::var("ORIGINAL_DIR").unwrap_or("../original".to_string());

    Command::new("python3").args(&["scripts/check-originals.py",
        &original_dir, &out_dir]).status().unwrap();
}

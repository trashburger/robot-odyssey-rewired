#![feature(proc_macro)]

#[macro_use]
extern crate stdweb;

use stdweb::js_export;

#[js_export]
fn increment( i: u32 ) -> u32 {
	i + 1
}

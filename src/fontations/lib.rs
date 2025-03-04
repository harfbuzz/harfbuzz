#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::os::raw::{c_void};
use std::ptr::{null_mut};

// If you want to parse TTF/OTF with read-fonts, etc. import them:
//extern crate read_fonts;
//use read_fonts::FontRef;
// use skrifa::{...};

// A struct for storing your “fontations” data
#[repr(C)]
struct FontationsData {
    // e.g. storing an owned font buffer, or read-fonts handles:
    // font_bytes: Vec<u8>,
    // font_ref: Option<FontRef<'static>>,
    pub offset: i32,
}

// A destructor for the user_data
extern "C" fn _hb_fontations_data_destroy(ptr: *mut c_void) {
    if !ptr.is_null() {
        unsafe { let _ = Box::from_raw(ptr as *mut FontationsData); }
    }
}

// Our callback: get glyph horizontal advance
extern "C" fn _hb_fontations_get_glyph_h_advance(
    _font: *mut hb_font_t,
    font_data: *mut c_void,
    glyph: hb_codepoint_t,
    _user_data: *mut c_void
) -> hb_position_t {
    let data = unsafe {
        assert!(!font_data.is_null());
        &*(font_data as *const FontationsData)
    };
    // Just a dummy formula:
    let advance = (glyph as i32) + data.offset;
    advance
}

fn _hb_fontations_font_funcs_create() -> *mut hb_font_funcs_t {
    let ffuncs = unsafe { hb_font_funcs_create() };

    unsafe {
        hb_font_funcs_set_glyph_h_advance_func(
            ffuncs,
            Some(_hb_fontations_get_glyph_h_advance),
            null_mut(),
            None
        );
    }

    ffuncs
}

// A helper to attach these funcs to a hb_font_t
#[no_mangle]
pub extern "C" fn hb_fontations_font_set_funcs(
    font: *mut hb_font_t,
) {
    let ffuncs = _hb_fontations_font_funcs_create ();

    // Set up some data for the callbacks to use:
    let data = FontationsData { offset: 100 };
    let data_ptr = Box::into_raw(Box::new(data)) as *mut c_void;

    unsafe {
        hb_font_set_funcs (font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
    }

    unsafe { hb_font_funcs_destroy (ffuncs); }
}

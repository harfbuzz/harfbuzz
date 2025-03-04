//
// lib.rs — Rust code that sets up HarfBuzz font funcs
//

use std::os::raw::{c_void, c_char};
use std::ptr::{null_mut};

// If you want to parse TTF/OTF with read-fonts, etc. import them:
// use read_fonts::FontRef;
// use skrifa::{...};

// Minimal HarfBuzz FFI
// Typically you'd generate these with bindgen from <hb.h>, but we'll do a small subset:

pub type hb_bool_t = i32;
pub type hb_position_t = i32;
pub type hb_codepoint_t = u32;
pub type hb_destroy_func_t = Option<extern "C" fn(user_data: *mut c_void)>;

#[repr(C)]
pub struct hb_font_t { _unused: [u8; 0], }

#[repr(C)]
pub struct hb_font_funcs_t { _unused: [u8; 0], }

extern "C" {
    fn hb_font_funcs_create() -> *mut hb_font_funcs_t;

    fn hb_font_funcs_set_glyph_h_advance_func(
        ffuncs: *mut hb_font_funcs_t,
        func: Option<hb_font_get_glyph_advance_func_t>,
        user_data: *mut c_void,
        destroy: hb_destroy_func_t
    );

    fn hb_font_set_funcs(
        font: *mut hb_font_t,
        klass: *mut hb_font_funcs_t,
        font_data: *mut c_void,
        destroy: hb_destroy_func_t
    );

    fn hb_font_funcs_destroy (
        ffuncs: *mut hb_font_funcs_t
    );

    // If you need more sets or functions, declare them here
}

// Callback type: e.g. horizontal advance
pub type hb_font_get_glyph_advance_func_t = extern "C" fn(
    font: *mut hb_font_t,
    font_data: *mut c_void,
    glyph: hb_codepoint_t,
    user_data: *mut c_void
) -> hb_position_t;

// A struct for storing your “fontations” data
#[repr(C)]
pub struct FontationsData {
    // e.g. storing an owned font buffer, or read-fonts handles:
    // font_bytes: Vec<u8>,
    // font_ref: Option<FontRef<'static>>,
    pub offset: i32,
}

// A destructor for the user_data
#[no_mangle]
pub extern "C" fn fontations_data_destroy(ptr: *mut c_void) {
    if !ptr.is_null() {
        unsafe { Box::from_raw(ptr as *mut FontationsData); }
    }
}

// Our callback: get glyph horizontal advance
#[no_mangle]
pub extern "C" fn hb_fontations_get_glyph_h_advance(
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

#[no_mangle]
pub extern "C" fn _hb_fontations_font_funcs_create() -> *mut hb_font_funcs_t {
    let ffuncs = unsafe { hb_font_funcs_create() };

    unsafe {
        hb_font_funcs_set_glyph_h_advance_func(
            ffuncs,
            Some(hb_fontations_get_glyph_h_advance),
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
        hb_font_set_funcs (font, ffuncs, data_ptr, Some(fontations_data_destroy));
    }

    unsafe { hb_font_funcs_destroy (ffuncs); }
}

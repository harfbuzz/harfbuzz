#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::os::raw::{c_void};
use std::ptr::{null_mut};

use skrifa::{MetadataProvider, GlyphId};
use skrifa::font::FontRef;
use skrifa::instance::{Location, Size};

// A struct for storing your “fontations” data
#[repr(C)]
struct FontationsData {
    font_ref: FontRef<'static>,
}

extern "C" fn _hb_fontations_data_destroy(ptr: *mut c_void) {
    if !ptr.is_null() {
        unsafe { let _ = Box::from_raw(ptr as *mut FontationsData); }
    }
    // TODO
}

extern "C" fn _hb_fontations_get_glyph_h_advances(
    font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    count: ::std::os::raw::c_uint,
    first_glyph: *const hb_codepoint_t,
    glyph_stride: ::std::os::raw::c_uint,
    first_advance: *mut hb_position_t,
    advance_stride: ::std::os::raw::c_uint,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe {
        assert!(!font_data.is_null());
        &*(font_data as *const FontationsData)
    };
    let mut x_scale : i32 = 0;
    let mut y_scale : i32 = 0;
    unsafe { hb_font_get_scale(font, &mut x_scale, &mut y_scale); };
    let font_ref = &data.font_ref;
    let size = Size::new(x_scale as f32);
    let location = Location::default(); // TODO
    let glyph_metrics = font_ref.glyph_metrics(size, &location);

    for i in 0..count {
        let glyph = unsafe { *first_glyph.offset((i * glyph_stride) as isize) };
        let glyph_id = GlyphId::new(glyph);
        let advance = glyph_metrics.advance_width(glyph_id).unwrap_or_default();
        unsafe { *first_advance.offset((i * advance_stride) as isize) = advance as hb_position_t; }
    }
}

fn _hb_fontations_font_funcs_create() -> *mut hb_font_funcs_t {
    let ffuncs = unsafe { hb_font_funcs_create() };

    unsafe {
        hb_font_funcs_set_glyph_h_advances_func(ffuncs, Some(_hb_fontations_get_glyph_h_advances), null_mut(), None);
    }

    ffuncs
}

// A helper to attach these funcs to a hb_font_t
#[no_mangle]
pub extern "C" fn hb_fontations_font_set_funcs(
    font: *mut hb_font_t,
) {
    let ffuncs = _hb_fontations_font_funcs_create ();

    let face_index = unsafe { hb_face_get_index(hb_font_get_face(font)) };
    let face_blob = unsafe { hb_face_reference_blob(hb_font_get_face(font)) };
    let blob_length = unsafe { hb_blob_get_length(face_blob) };
    let blob_data = unsafe { hb_blob_get_data(face_blob, null_mut()) };
    let face_data = unsafe { std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize) };

    let font_ref = FontRef::from_index(face_data, face_index).unwrap();
    // Set up some data for the callbacks to use:
    let data = FontationsData { font_ref };
    let data_ptr = Box::into_raw(Box::new(data)) as *mut c_void;

    unsafe {
        hb_font_set_funcs (font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
    }

    unsafe { hb_font_funcs_destroy (ffuncs); }
}

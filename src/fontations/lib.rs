#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::os::raw::{c_void};
use std::ptr::{null_mut};
use std::sync::atomic::{AtomicPtr, Ordering};

use skrifa::{charmap, GlyphId, MetadataProvider};
use skrifa::font::FontRef;
use skrifa::instance::{Location, Size};

// A struct for storing your “fontations” data
#[repr(C)]
struct FontationsData
{
    face_blob: *mut hb_blob_t,
    font_ref: FontRef<'static>,
    char_map : charmap::Charmap<'static>,
}

extern "C" fn _hb_fontations_data_destroy(font_data: *mut c_void)
{
    let data = unsafe { Box::from_raw(font_data as *mut FontationsData) };

    unsafe { hb_blob_destroy(data.face_blob); }
}

extern "C" fn _hb_fontations_get_nominal_glyphs(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    count: ::std::os::raw::c_uint,
    first_unicode: *const hb_codepoint_t,
    unicode_stride: ::std::os::raw::c_uint,
    first_glyph: *mut hb_codepoint_t,
    glyph_stride: ::std::os::raw::c_uint,
    _user_data: *mut ::std::os::raw::c_void,
) -> ::std::os::raw::c_uint
{
    let data = unsafe { &*(font_data as *const FontationsData) };
    let char_map = &data.char_map;

    for i in 0..count {
        let unicode = unsafe { *(first_unicode as *const u8).offset((i * unicode_stride) as isize) as *const hb_codepoint_t };
        let glyph = char_map.map(unicode as u32);
        if glyph.is_none()
        { return i; }
        let glyph_id = u32::from(glyph.unwrap()) as hb_codepoint_t;
        unsafe { *((first_glyph as *mut u8).offset((i * glyph_stride) as isize) as *mut hb_codepoint_t) = glyph_id; }
    }

    count
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
)
{
    let data = unsafe { &*(font_data as *const FontationsData) };
    let mut x_scale : i32 = 0;
    let mut y_scale : i32 = 0;
    unsafe { hb_font_get_scale(font, &mut x_scale, &mut y_scale); };
    let font_ref = &data.font_ref;
    let size = Size::new(x_scale as f32);
    let location = Location::default(); // TODO
    let glyph_metrics = font_ref.glyph_metrics(size, &location);

    for i in 0..count {
        let glyph = unsafe { *(first_glyph as *const u8).offset((i * glyph_stride) as isize) as *const hb_codepoint_t };
        let glyph_id = GlyphId::new(glyph as u32);
        let advance = glyph_metrics.advance_width(glyph_id).unwrap_or_default().round() as i32;
        unsafe { *((first_advance as *mut u8).offset((i * advance_stride) as isize) as *mut hb_position_t) = advance as hb_position_t; }
    }
}


fn _hb_fontations_font_funcs_create() -> *mut hb_font_funcs_t
{
    static static_ffuncs: AtomicPtr<hb_font_funcs_t> = AtomicPtr::new(null_mut());

    loop
    {
        let mut ffuncs = static_ffuncs.load(Ordering::Acquire);

        if !ffuncs.is_null() {
            return ffuncs;
        }

        ffuncs = unsafe { hb_font_funcs_create() };

        unsafe {
            hb_font_funcs_set_nominal_glyphs_func(ffuncs, Some(_hb_fontations_get_nominal_glyphs), null_mut(), None);
            hb_font_funcs_set_glyph_h_advances_func(ffuncs, Some(_hb_fontations_get_glyph_h_advances), null_mut(), None);
        }

        if (static_ffuncs.compare_exchange(null_mut(), ffuncs, Ordering::SeqCst, Ordering::Relaxed)) == Ok(null_mut()) {
            return ffuncs;
        } else {
            unsafe { hb_font_funcs_destroy(ffuncs); }
        }
    }
}

// A helper to attach these funcs to a hb_font_t
#[no_mangle]
pub extern "C" fn hb_fontations_font_set_funcs(
    font: *mut hb_font_t,
)
{
    let ffuncs = _hb_fontations_font_funcs_create ();

    let face_index = unsafe { hb_face_get_index(hb_font_get_face(font)) };
    let face_blob = unsafe { hb_face_reference_blob(hb_font_get_face(font)) };
    let blob_length = unsafe { hb_blob_get_length(face_blob) };
    let blob_data = unsafe { hb_blob_get_data(face_blob, null_mut()) };
    let face_data = unsafe { std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize) };

    let font_ref = FontRef::from_index(face_data, face_index).unwrap();
    let char_map = charmap::Charmap::new(&font_ref);

    let data = Box::new(FontationsData {
        face_blob,
        font_ref,
        char_map,
    });
    let data_ptr = Box::into_raw(data) as *mut c_void;

    unsafe {
        hb_font_set_funcs (font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
    }

    unsafe { hb_font_funcs_destroy (ffuncs); }
}

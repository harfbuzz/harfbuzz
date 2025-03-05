#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::os::raw::c_void;
use std::ptr::null_mut;
use std::sync::atomic::{AtomicPtr, Ordering};

use skrifa::charmap::Charmap;
use skrifa::charmap::MapVariant::Variant;
use skrifa::font::FontRef;
use skrifa::instance::{Location, NormalizedCoord, Size};
use skrifa::outline::pen::OutlinePen;
use skrifa::outline::DrawSettings;
use skrifa::{GlyphId, MetadataProvider};

// A struct for storing your “fontations” data
#[repr(C)]
struct FontationsData {
    face_blob: *mut hb_blob_t,
    font_ref: FontRef<'static>,
    char_map: Charmap<'static>,
    x_size: Size,
    y_size: Size,
    location: Location,
}

extern "C" fn _hb_fontations_data_destroy(font_data: *mut c_void) {
    let data = unsafe { Box::from_raw(font_data as *mut FontationsData) };

    unsafe {
        hb_blob_destroy(data.face_blob);
    }
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
) -> ::std::os::raw::c_uint {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let char_map = &data.char_map;

    for i in 0..count {
        let unicode = unsafe {
            *(first_unicode as *const u8).offset((i * unicode_stride) as isize)
                as *const hb_codepoint_t
        };
        let glyph = char_map.map(unicode as u32);
        if glyph.is_none() {
            return i;
        }
        let glyph_id = u32::from(glyph.unwrap()) as hb_codepoint_t;
        unsafe {
            *((first_glyph as *mut u8).offset((i * glyph_stride) as isize)
                as *mut hb_codepoint_t) = glyph_id;
        }
    }

    count
}
extern "C" fn _hb_fontations_get_variation_glyph(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    unicode: hb_codepoint_t,
    variation_selector: hb_codepoint_t,
    glyph: *mut hb_codepoint_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_bool_t {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let char_map = &data.char_map;

    let variant = char_map.map_variant(unicode as u32, variation_selector as u32);
    match variant {
        Some(Variant(glyph_id)) => {
            unsafe { *glyph = u32::from(glyph_id) as hb_codepoint_t };
            true as hb_bool_t
        }
        _ => false as hb_bool_t,
    }
}

extern "C" fn _hb_fontations_get_glyph_h_advances(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    count: ::std::os::raw::c_uint,
    first_glyph: *const hb_codepoint_t,
    glyph_stride: ::std::os::raw::c_uint,
    first_advance: *mut hb_position_t,
    advance_stride: ::std::os::raw::c_uint,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let font_ref = &data.font_ref;
    let size = &data.x_size;
    let location = &data.location;
    let glyph_metrics = font_ref.glyph_metrics(*size, location);

    for i in 0..count {
        let glyph = unsafe {
            *(first_glyph as *const u8).offset((i * glyph_stride) as isize) as *const hb_codepoint_t
        };
        let glyph_id = GlyphId::new(glyph as u32);
        let advance = glyph_metrics
            .advance_width(glyph_id)
            .unwrap_or_default()
            .round() as i32;
        unsafe {
            *((first_advance as *mut u8).offset((i * advance_stride) as isize)
                as *mut hb_position_t) = advance as hb_position_t;
        }
    }
}
extern "C" fn _hb_fontations_get_glyph_extents(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    glyph: hb_codepoint_t,
    extents: *mut hb_glyph_extents_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_bool_t {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let font_ref = &data.font_ref;
    let x_size = &data.x_size;
    let y_size = &data.y_size;
    let location = &data.location;
    let x_glyph_metrics = font_ref.glyph_metrics(*x_size, location);
    let y_glyph_metrics = font_ref.glyph_metrics(*y_size, location);

    let glyph_id = GlyphId::new(glyph as u32);
    let x_extents = x_glyph_metrics.bounds(glyph_id);
    let y_extents = y_glyph_metrics.bounds(glyph_id);
    if x_extents.is_none() || y_extents.is_none() {
        return false as hb_bool_t;
    }
    let x_extents = x_extents.unwrap();
    let y_extents = y_extents.unwrap();
    unsafe {
        (*extents).x_bearing = x_extents.x_min as hb_position_t;
    }
    unsafe {
        (*extents).width = (x_extents.x_max - x_extents.x_min) as hb_position_t;
    }
    unsafe {
        (*extents).y_bearing = y_extents.y_max as hb_position_t;
    }
    unsafe {
        (*extents).height = (y_extents.y_min - y_extents.y_max) as hb_position_t;
    }

    true as hb_bool_t
}

extern "C" fn _hb_fontations_get_font_h_extents(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    extents: *mut hb_font_extents_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_bool_t {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let font_ref = &data.font_ref;
    let size = &data.y_size;
    let location = &data.location;
    let metrics = font_ref.metrics(*size, location);

    unsafe {
        (*extents).ascender = metrics.ascent as hb_position_t;
    }
    unsafe {
        (*extents).descender = metrics.descent as hb_position_t;
    }
    unsafe {
        (*extents).line_gap = metrics.leading as hb_position_t;
    }

    true as hb_bool_t
}

struct HbPen
where
    Self: OutlinePen,
{
    draw_state: *mut hb_draw_state_t,
    draw_funcs: *mut hb_draw_funcs_t,
    draw_data: *mut c_void,
}

impl OutlinePen for HbPen {
    fn move_to(&mut self, x: f32, y: f32) {
        unsafe {
            hb_draw_move_to(self.draw_funcs, self.draw_data, self.draw_state, x, y);
        }
    }
    fn line_to(&mut self, x: f32, y: f32) {
        unsafe {
            hb_draw_line_to(self.draw_funcs, self.draw_data, self.draw_state, x, y);
        }
    }
    fn quad_to(&mut self, x1: f32, y1: f32, x: f32, y: f32) {
        unsafe {
            hb_draw_quadratic_to(
                self.draw_funcs,
                self.draw_data,
                self.draw_state,
                x1,
                y1,
                x,
                y,
            );
        }
    }
    fn curve_to(&mut self, x1: f32, y1: f32, x2: f32, y2: f32, x: f32, y: f32) {
        unsafe {
            hb_draw_cubic_to(
                self.draw_funcs,
                self.draw_data,
                self.draw_state,
                x1,
                y1,
                x2,
                y2,
                x,
                y,
            );
        }
    }
    fn close(&mut self) {
        unsafe {
            hb_draw_close_path(self.draw_funcs, self.draw_data, self.draw_state);
        }
    }
}

extern "C" fn _hb_fontations_draw_glyph(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    glyph: hb_codepoint_t,
    draw_funcs: *mut hb_draw_funcs_t,
    draw_data: *mut ::std::os::raw::c_void,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let font_ref = &data.font_ref;
    let x_size = &data.x_size;
    let location = &data.location;

    // Create an outline-glyph
    let glyph_id = GlyphId::new(glyph as u32);
    let outline_glyphs = font_ref.outline_glyphs();
    let outline_glyph = outline_glyphs.get(glyph_id);
    if outline_glyph.is_none() {
        return;
    }
    let outline_glyph = outline_glyph.unwrap();
    let draw_settings = DrawSettings::unhinted(*x_size, location);
    // Allocate zero bytes for the draw_state_t on the stack.
    let mut draw_state: hb_draw_state_t = unsafe { std::mem::zeroed::<hb_draw_state_t>() };

    let mut pen = HbPen {
        draw_state: &mut draw_state,
        draw_funcs,
        draw_data,
    };
    let _ = outline_glyph.draw(draw_settings, &mut pen);
}

fn _hb_fontations_font_funcs_get() -> *mut hb_font_funcs_t {
    static static_ffuncs: AtomicPtr<hb_font_funcs_t> = AtomicPtr::new(null_mut());

    loop {
        let mut ffuncs = static_ffuncs.load(Ordering::Acquire);

        if !ffuncs.is_null() {
            return ffuncs;
        }

        ffuncs = unsafe { hb_font_funcs_create() };

        unsafe {
            hb_font_funcs_set_nominal_glyphs_func(
                ffuncs,
                Some(_hb_fontations_get_nominal_glyphs),
                null_mut(),
                None,
            );
            hb_font_funcs_set_variation_glyph_func(
                ffuncs,
                Some(_hb_fontations_get_variation_glyph),
                null_mut(),
                None,
            );
            hb_font_funcs_set_glyph_h_advances_func(
                ffuncs,
                Some(_hb_fontations_get_glyph_h_advances),
                null_mut(),
                None,
            );
            hb_font_funcs_set_glyph_extents_func(
                ffuncs,
                Some(_hb_fontations_get_glyph_extents),
                null_mut(),
                None,
            );
            hb_font_funcs_set_font_h_extents_func(
                ffuncs,
                Some(_hb_fontations_get_font_h_extents),
                null_mut(),
                None,
            );
            hb_font_funcs_set_draw_glyph_func(
                ffuncs,
                Some(_hb_fontations_draw_glyph),
                null_mut(),
                None,
            );
        }

        if (static_ffuncs.compare_exchange(null_mut(), ffuncs, Ordering::SeqCst, Ordering::Relaxed))
            == Ok(null_mut())
        {
            return ffuncs;
        } else {
            unsafe {
                hb_font_funcs_destroy(ffuncs);
            }
        }
    }
}

// A helper to attach these funcs to a hb_font_t
#[no_mangle]
pub extern "C" fn hb_fontations_font_set_funcs(font: *mut hb_font_t) {
    let ffuncs = _hb_fontations_font_funcs_get();

    let face_index = unsafe { hb_face_get_index(hb_font_get_face(font)) };
    let face_blob = unsafe { hb_face_reference_blob(hb_font_get_face(font)) };
    let blob_length = unsafe { hb_blob_get_length(face_blob) };
    let blob_data = unsafe { hb_blob_get_data(face_blob, null_mut()) };
    let face_data =
        unsafe { std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize) };

    let font_ref = FontRef::from_index(face_data, face_index).unwrap();

    let char_map = Charmap::new(&font_ref);

    let mut num_coords: u32 = 0;
    let coords = unsafe { hb_font_get_var_coords_normalized(font, &mut num_coords) };
    let mut location = Location::new(num_coords as usize);
    let coords_mut = location.coords_mut();
    for i in 0..num_coords as usize {
        coords_mut[i] = NormalizedCoord::from_bits(unsafe { *coords.offset(i as isize) } as i16);
    }

    let mut x_scale: i32 = 0;
    let mut y_scale: i32 = 0;
    unsafe {
        hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    };
    let x_size = Size::new(x_scale as f32);
    let y_size = Size::new(y_scale as f32);

    let data = Box::new(FontationsData {
        face_blob,
        font_ref,
        char_map,
        x_size,
        y_size,
        location,
    });
    let data_ptr = Box::into_raw(data) as *mut c_void;

    unsafe {
        hb_font_set_funcs(font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
    }
}

#![allow(non_upper_case_globals)]

use super::hb::*;

use std::os::raw::c_void;
use std::ptr::null_mut;
use std::str::FromStr;

use font_types::Tag;
use harfruzz::{Direction, Face, FontRef, Language, Script, ShaperFont};

pub struct HBHarfRuzzFaceData<'a> {
    face_blob: *mut hb_blob_t,
    face: Face<'a>,
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_face_data_create_rs(
    face: *mut hb_face_t,
) -> *mut c_void {
    let face_index = hb_face_get_index(face);
    let face_blob = hb_face_reference_blob(face);
    let blob_length = hb_blob_get_length(face_blob);
    let blob_data = hb_blob_get_data(face_blob, null_mut());
    let face_data = std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize);

    let font_ref = FontRef::from_index(face_data, face_index)
        .expect("FontRef::from_index should succeed on valid HarfBuzz face data");
    let shaper_font = ShaperFont::new(&font_ref);
    let face = shaper_font.shaper(&font_ref, &[]);

    let hr_face_data = HBHarfRuzzFaceData { face_blob, face };
    let hr_face_data_ptr = Box::into_raw(Box::new(hr_face_data));
    hr_face_data_ptr as *mut c_void
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_face_data_destroy_rs(data: *mut c_void) {
    // The data pointer is expected to be a pointer to HBHarfRuzzFaceData
    let data = data as *mut HBHarfRuzzFaceData;
    let hr_face_data = Box::from_raw(data);
    let blob = hr_face_data.face_blob;
    hb_blob_destroy(blob);
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shape_rs(
    data: *const c_void,
    font: *mut hb_font_t,
    buffer: *mut hb_buffer_t,
    _features: *const hb_feature_t,
    _num_features: u32,
) -> hb_bool_t {
    // The data pointer is expected to be a pointer to HBHarfRuzzFaceData
    let data = data as *const HBHarfRuzzFaceData;

    let mut hr_buffer = harfruzz::UnicodeBuffer::new();

    // Set buffer properties
    // XXX
    //let flags = hb_buffer_get_flags(buffer);
    //hr_buffer.set_flags(harfruzz::BufferFlags::from_bits_truncate(flags));

    // Segment properties:
    let script = hb_buffer_get_script(buffer);
    let language = hb_buffer_get_language(buffer);
    let direction = hb_buffer_get_direction(buffer);
    // Convert to harfRuzz types
    let script =
        Script::from_iso15924_tag(Tag::from_u32(script)).unwrap_or(harfruzz::script::UNKNOWN);
    let language_str = hb_language_to_string(language);
    let language_str = std::ffi::CStr::from_ptr(language_str);
    let language_str = language_str.to_str().unwrap_or("");
    let language = Language::from_str(language_str).unwrap();
    let direction = match direction {
        hb_direction_t_HB_DIRECTION_LTR => Direction::LeftToRight,
        hb_direction_t_HB_DIRECTION_RTL => Direction::RightToLeft,
        hb_direction_t_HB_DIRECTION_TTB => Direction::TopToBottom,
        hb_direction_t_HB_DIRECTION_BTT => Direction::BottomToTop,
        _ => Direction::Invalid,
    };
    // Set properties on the buffer
    hr_buffer.set_script(script);
    hr_buffer.set_language(language);
    hr_buffer.set_direction(direction);

    // Populate buffer
    let count = hb_buffer_get_length(buffer);
    let infos = hb_buffer_get_glyph_infos(buffer, null_mut());

    for i in 0..count {
        let info = infos.add(i as usize);
        let unicode = (*info).codepoint;
        let cluster = (*info).cluster;
        hr_buffer.add(char::from_u32_unchecked(unicode), cluster);
    }

    let face = &(*data).face;

    let glyphs = harfruzz::shape(face, &[], hr_buffer);

    let count = glyphs.len();
    hb_buffer_set_content_type(
        buffer,
        hb_buffer_content_type_t_HB_BUFFER_CONTENT_TYPE_GLYPHS,
    );
    hb_buffer_set_length(buffer, count as u32);
    if hb_buffer_get_length(buffer) != count as u32 {
        return false as hb_bool_t;
    }
    let infos = hb_buffer_get_glyph_infos(buffer, null_mut());
    let positions = hb_buffer_get_glyph_positions(buffer, null_mut());

    let mut x_scale: i32 = 0;
    let mut y_scale: i32 = 0;
    hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    let upem = face.units_per_em();
    let x_scale = x_scale as f32 / upem as f32;
    let y_scale = y_scale as f32 / upem as f32;

    for (i, (hr_info, hr_pos)) in glyphs
        .glyph_infos()
        .iter()
        .zip(glyphs.glyph_positions())
        .enumerate()
    {
        let info = infos.add(i);
        let pos = positions.add(i);
        (*info).codepoint = hr_info.glyph_id;
        (*info).cluster = hr_info.cluster;
        (*pos).x_advance = (hr_pos.x_advance as f32 * x_scale).round() as hb_position_t;
        (*pos).y_advance = (hr_pos.y_advance as f32 * y_scale).round() as hb_position_t;
        (*pos).x_offset = (hr_pos.x_offset as f32 * x_scale).round() as hb_position_t;
        (*pos).y_offset = (hr_pos.y_offset as f32 * y_scale).round() as hb_position_t;
    }

    true as hb_bool_t
}

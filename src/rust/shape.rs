#![allow(non_upper_case_globals)]

use super::hb::*;

use std::ffi::c_void;
use std::ptr::null_mut;
use std::str::FromStr;

use harfruzz::{F2Dot14, Face, FontRef, ShaperFont, Tag};

pub struct HBHarfRuzzFaceData<'a> {
    face_blob: *mut hb_blob_t,
    font_ref: FontRef<'a>,
    shaper_font: ShaperFont,
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_face_data_create_rs(
    face: *mut hb_face_t,
) -> *mut c_void {
    let face_index = hb_face_get_index(face);
    let face_blob = hb_face_reference_blob(face);
    let blob_length = hb_blob_get_length(face_blob);
    let blob_data = hb_blob_get_data(face_blob, null_mut());
    if blob_data.is_null() {
        return null_mut();
    }
    let face_data = std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize);

    let font_ref = match FontRef::from_index(face_data, face_index) {
        Ok(f) => f,
        Err(_) => return null_mut(),
    };
    let shaper_font = ShaperFont::new(&font_ref);

    let hr_face_data = Box::new(HBHarfRuzzFaceData {
        face_blob,
        font_ref,
        shaper_font,
    });

    Box::into_raw(hr_face_data) as *mut c_void
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_face_data_destroy_rs(data: *mut c_void) {
    let data = data as *mut HBHarfRuzzFaceData;
    let hr_face_data = Box::from_raw(data);
    let blob = hr_face_data.face_blob;
    hb_blob_destroy(blob);
}

pub struct HBHarfRuzzFontData<'a> {
    coords: Vec<F2Dot14>,
    face: Option<Face<'a>>,
}

fn font_coords_to_f2dot14(font: *mut hb_font_t) -> Vec<F2Dot14> {
    let mut num_coords: u32 = 0;
    let coords = unsafe { hb_font_get_var_coords_normalized(font, &mut num_coords) };
    let coords = if coords.is_null() {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(coords, num_coords as usize) }
    };
    coords
        .iter()
        .map(|&v| F2Dot14::from_bits(v as i16))
        .collect::<Vec<_>>()
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_font_data_create_rs(
    font: *mut hb_font_t,
    face_data: *const c_void,
) -> *mut c_void {
    let face_data = face_data as *const HBHarfRuzzFaceData;

    let coords = font_coords_to_f2dot14(font);

    let hr_font_data = Box::new(HBHarfRuzzFontData { coords, face: None });
    let hr_font_data_ptr = Box::into_raw(hr_font_data);
    (*hr_font_data_ptr).face = Some(
        (*face_data)
            .shaper_font
            .shaper(&(*face_data).font_ref, &(*hr_font_data_ptr).coords),
    );

    hr_font_data_ptr as *mut c_void
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shaper_font_data_destroy_rs(data: *mut c_void) {
    let data = data as *mut HBHarfRuzzFontData;
    let _hr_font_data = Box::from_raw(data);
}

fn hb_language_to_hr_language(language: hb_language_t) -> Option<harfruzz::Language> {
    let language_str = unsafe { hb_language_to_string(language) };
    if language_str.is_null() {
        return None;
    }
    let language_str = unsafe { std::ffi::CStr::from_ptr(language_str) };
    let language_str = language_str.to_str().unwrap_or_default();
    Some(harfruzz::Language::from_str(language_str).unwrap())
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shape_plan_create_rs(
    font_data: *const c_void,
    script: hb_script_t,
    language: hb_language_t,
    direction: hb_direction_t,
) -> *mut c_void {
    let font_data = font_data as *const HBHarfRuzzFontData;

    let script = harfruzz::Script::from_iso15924_tag(Tag::from_u32(script));
    let language = hb_language_to_hr_language(language);
    let direction = match direction {
        hb_direction_t_HB_DIRECTION_LTR => harfruzz::Direction::LeftToRight,
        hb_direction_t_HB_DIRECTION_RTL => harfruzz::Direction::RightToLeft,
        hb_direction_t_HB_DIRECTION_TTB => harfruzz::Direction::TopToBottom,
        hb_direction_t_HB_DIRECTION_BTT => harfruzz::Direction::BottomToTop,
        _ => harfruzz::Direction::Invalid,
    };

    let hr_shape_plan = harfruzz::ShapePlan::new(
        (*font_data).face.as_ref().unwrap(),
        direction,
        script,
        language.as_ref(),
        &[],
    );
    let hr_shape_plan = Box::new(hr_shape_plan);
    Box::into_raw(hr_shape_plan) as *mut c_void
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shape_plan_destroy_rs(data: *mut c_void) {
    let data = data as *mut harfruzz::ShapePlan;
    let _hr_shape_plan = Box::from_raw(data);
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shape_rs(
    font_data: *const c_void,
    shape_plan: *const c_void,
    font: *mut hb_font_t,
    buffer: *mut hb_buffer_t,
    features: *const hb_feature_t,
    num_features: u32,
) -> hb_bool_t {
    let font_data = font_data as *const HBHarfRuzzFontData;

    let mut hr_buffer = harfruzz::UnicodeBuffer::new();

    // Set buffer properties
    let cluster_level = hb_buffer_get_cluster_level(buffer);
    let cluster_level = match cluster_level {
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES => {
            harfruzz::BufferClusterLevel::MonotoneGraphemes
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS => {
            harfruzz::BufferClusterLevel::MonotoneCharacters
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_CHARACTERS => {
            harfruzz::BufferClusterLevel::Characters
        }
        /* TODO: Enable when added to HarfRuzz
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES => {
            harfruzz::BufferClusterLevel::Graphemes
        }
        */
        _ => harfruzz::BufferClusterLevel::default(),
    };
    hr_buffer.set_cluster_level(cluster_level);
    let flags = hb_buffer_get_flags(buffer);
    hr_buffer.set_flags(harfruzz::BufferFlags::from_bits_truncate(flags));
    let not_found_variation_selector_glyph =
        hb_buffer_get_not_found_variation_selector_glyph(buffer);
    if not_found_variation_selector_glyph != u32::MAX {
        hr_buffer.set_not_found_variation_selector_glyph(not_found_variation_selector_glyph);
    }

    // Segment properties:
    let script = hb_buffer_get_script(buffer);
    let language = hb_buffer_get_language(buffer);
    let direction = hb_buffer_get_direction(buffer);
    // Convert to HarfRuzz types
    let script = harfruzz::Script::from_iso15924_tag(Tag::from_u32(script))
        .unwrap_or(harfruzz::script::UNKNOWN);
    let language = hb_language_to_hr_language(language);
    let direction = match direction {
        hb_direction_t_HB_DIRECTION_LTR => harfruzz::Direction::LeftToRight,
        hb_direction_t_HB_DIRECTION_RTL => harfruzz::Direction::RightToLeft,
        hb_direction_t_HB_DIRECTION_TTB => harfruzz::Direction::TopToBottom,
        hb_direction_t_HB_DIRECTION_BTT => harfruzz::Direction::BottomToTop,
        _ => harfruzz::Direction::Invalid,
    };
    // Set properties on the buffer
    hr_buffer.set_script(script);
    if let Some(lang) = language {
        hr_buffer.set_language(lang);
    }
    hr_buffer.set_direction(direction);

    // Populate buffer
    let count = hb_buffer_get_length(buffer);
    let infos = hb_buffer_get_glyph_infos(buffer, null_mut());

    for i in 0..count {
        let info = &*infos.add(i as usize);
        let unicode = info.codepoint;
        let cluster = info.cluster;
        hr_buffer.add(char::from_u32_unchecked(unicode), cluster);
    }

    let face = &(*font_data).face.as_ref().unwrap();

    let glyphs = if shape_plan.is_null() {
        let features = if features.is_null() {
            Vec::new()
        } else {
            let features = std::slice::from_raw_parts(features, num_features as usize);
            features
                .iter()
                .map(|f| {
                    let tag = f.tag;
                    let value = f.value;
                    let start = f.start;
                    let end = f.end;
                    harfruzz::Feature {
                        tag: Tag::from_u32(tag),
                        value,
                        start,
                        end,
                    }
                })
                .collect::<Vec<_>>()
        };
        harfruzz::shape(face, &features, hr_buffer)
    } else {
        let shape_plan = shape_plan as *const harfruzz::ShapePlan;
        harfruzz::shape_with_plan(face, shape_plan.as_ref().unwrap(), hr_buffer)
    };

    let count = glyphs.len();
    hb_buffer_set_length(buffer, 0u32);
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
        let info = &mut *infos.add(i);
        let pos = &mut *positions.add(i);
        info.codepoint = hr_info.glyph_id;
        info.cluster = hr_info.cluster;
        if hr_info.unsafe_to_break() {
            info.mask |= hb_glyph_flags_t_HB_GLYPH_FLAG_UNSAFE_TO_BREAK;
        }
        pos.x_advance = (hr_pos.x_advance as f32 * x_scale).round() as hb_position_t;
        pos.y_advance = (hr_pos.y_advance as f32 * y_scale).round() as hb_position_t;
        pos.x_offset = (hr_pos.x_offset as f32 * x_scale).round() as hb_position_t;
        pos.y_offset = (hr_pos.y_offset as f32 * y_scale).round() as hb_position_t;
    }

    true as hb_bool_t
}

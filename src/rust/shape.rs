use super::hb::*;

use std::os::raw::c_void;
use std::ptr::null_mut;

use harfruzz::{FontRef, ShaperFont};

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
    let face_data = std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize);

    let font_ref = FontRef::from_index(face_data, face_index)
        .expect("FontRef::from_index should succeed on valid HarfBuzz face data");
    let shaper_font = ShaperFont::new(&font_ref);

    let hr_face_data = HBHarfRuzzFaceData {
        face_blob,
        font_ref,
        shaper_font,
    };
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
    // The hr_font is automatically cleaned up when it goes out of scope
}

#[no_mangle]
pub unsafe extern "C" fn _hb_harfruzz_shape_rs(
    data: *const c_void,
    _font: *mut hb_font_t,
    buffer: *mut hb_buffer_t,
    _features: *const hb_feature_t,
    _num_features: u32,
) -> hb_bool_t {
    // The data pointer is expected to be a pointer to HBHarfRuzzFaceData
    let data = data as *const HBHarfRuzzFaceData;

    let mut hr_buffer = harfruzz::UnicodeBuffer::new();

    // Set buffer properties
    // XXX

    // Populate buffer
    let count = hb_buffer_get_length(buffer);
    let infos = hb_buffer_get_glyph_infos(buffer, null_mut());

    for i in 0..count {
        let info = infos.add(i as usize);
        let unicode = (*info).codepoint;
        let cluster = (*info).cluster;
        hr_buffer.add(char::from_u32_unchecked(unicode), cluster);
    }

    let font_ref = &(*data).font_ref;
    let shaper_font = &(*data).shaper_font;
    let shaper = shaper_font.shaper(font_ref, &[]);

    let glyphs = harfruzz::shape(&shaper, &[], hr_buffer);

    let count = glyphs.len();
    hb_buffer_set_content_type(
        buffer,
        hb_buffer_content_type_t_HB_BUFFER_CONTENT_TYPE_GLYPHS,
    );
    hb_buffer_set_length(buffer, count as u32);
    let infos = hb_buffer_get_glyph_infos(buffer, null_mut());
    let positions = hb_buffer_get_glyph_positions(buffer, null_mut());

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
        (*pos).x_advance = hr_pos.x_advance;
        (*pos).y_advance = hr_pos.y_advance;
        (*pos).x_offset = hr_pos.x_offset;
        (*pos).y_offset = hr_pos.y_offset;
    }

    true as hb_bool_t
}

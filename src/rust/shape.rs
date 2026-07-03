#![allow(non_upper_case_globals)]
// C enum becomes i32 on some systems (eg. Windows).
#![allow(clippy::unnecessary_cast)]

use super::hb::*;

use std::ffi::c_void;
use std::mem::{align_of, offset_of, size_of};
use std::ptr::null_mut;
use std::sync::Arc;

use harfrust::{
    font::{
        AdvanceWidthBatch, BuiltinFontFuncs, Font, FontBlob, FontFuncs, FontInstance,
        FontTableFunction,
    },
    GlyphExtents, GlyphFlags as HRGlyphFlags, GlyphId, GlyphInfo as HRGlyphInfo,
    GlyphPosition as HRGlyphPosition, NormalizedCoord, ShapeOptions, Tag,
};
use smallvec::SmallVec;

type HRFeatureVec = SmallVec<[harfrust::Feature; 4]>;

const _: () = {
    assert!(size_of::<hb_glyph_info_t>() == size_of::<HRGlyphInfo>());
    assert!(align_of::<hb_glyph_info_t>() == align_of::<HRGlyphInfo>());
    assert!(offset_of!(hb_glyph_info_t, codepoint) == offset_of!(HRGlyphInfo, glyph_id));
    assert!(offset_of!(hb_glyph_info_t, cluster) == offset_of!(HRGlyphInfo, cluster));
    assert!(
        hb_glyph_flags_t_HB_GLYPH_FLAG_UNSAFE_TO_BREAK as u32
            == HRGlyphFlags::UNSAFE_TO_BREAK.to_bits()
    );
    assert!(
        hb_glyph_flags_t_HB_GLYPH_FLAG_UNSAFE_TO_CONCAT as u32
            == HRGlyphFlags::UNSAFE_TO_CONCAT.to_bits()
    );
    assert!(
        hb_glyph_flags_t_HB_GLYPH_FLAG_SAFE_TO_INSERT_TATWEEL as u32
            == HRGlyphFlags::SAFE_TO_INSERT_TATWEEL.to_bits()
    );
    assert!(
        hb_glyph_flags_t_HB_GLYPH_FLAG_DEFINED as u32
            == (HRGlyphFlags::UNSAFE_TO_BREAK.to_bits()
                | HRGlyphFlags::UNSAFE_TO_CONCAT.to_bits()
                | HRGlyphFlags::SAFE_TO_INSERT_TATWEEL.to_bits())
    );

    assert!(size_of::<hb_glyph_position_t>() == size_of::<HRGlyphPosition>());
    assert!(align_of::<hb_glyph_position_t>() == align_of::<HRGlyphPosition>());
    assert!(offset_of!(hb_glyph_position_t, x_advance) == offset_of!(HRGlyphPosition, x_advance));
    assert!(offset_of!(hb_glyph_position_t, y_advance) == offset_of!(HRGlyphPosition, y_advance));
    assert!(offset_of!(hb_glyph_position_t, x_offset) == offset_of!(HRGlyphPosition, x_offset));
    assert!(offset_of!(hb_glyph_position_t, y_offset) == offset_of!(HRGlyphPosition, y_offset));
};

pub struct HBHarfRustFaceData {
    font: Font,
}

struct HbFace(*mut hb_face_t);

impl HbFace {
    unsafe fn new(face: *mut hb_face_t) -> Self {
        Self(face)
    }

    fn reference_table(&self, tag: Tag) -> *mut hb_blob_t {
        unsafe { hb_face_reference_table(self.0, u32::from_be_bytes(tag.to_be_bytes())) }
    }
}

unsafe impl Send for HbFace {}
unsafe impl Sync for HbFace {}

struct HbBlob(*mut hb_blob_t);

impl Drop for HbBlob {
    fn drop(&mut self) {
        unsafe {
            hb_blob_destroy(self.0);
        }
    }
}

impl AsRef<[u8]> for HbBlob {
    fn as_ref(&self) -> &[u8] {
        let mut length = 0;
        let data = unsafe { hb_blob_get_data(self.0, &mut length) };
        if data.is_null() {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(data.cast(), length as usize) }
        }
    }
}

unsafe impl Send for HbBlob {}
unsafe impl Sync for HbBlob {}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shaper_face_data_create_rs(
    face: *mut hb_face_t,
) -> *mut c_void {
    let face_index = hb_face_get_index(face);
    let hb_face = HbFace::new(face);
    let table_fn = FontTableFunction::new(Arc::new(move |tag| {
        let blob = hb_face.reference_table(tag);
        if blob.is_null() {
            return None;
        }
        if unsafe { hb_blob_get_length(blob) } == 0 {
            unsafe {
                hb_blob_destroy(blob);
            }
            return None;
        }
        Some(FontBlob::Shared(Arc::new(HbBlob(blob))))
    }));

    let font = match Font::new(table_fn, face_index) {
        Ok(font) => font,
        Err(_) => return null_mut(),
    };

    let hr_face_data = Box::new(HBHarfRustFaceData { font });

    Box::into_raw(hr_face_data) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shaper_face_data_destroy_rs(data: *mut c_void) {
    let data = data as *mut HBHarfRustFaceData;
    let _hr_face_data = Box::from_raw(data);
}

pub struct HBHarfRustFontData {
    instance: FontInstance,
}

struct HBHarfBuzzFontFuncs {
    font: *mut hb_font_t,
}

impl FontFuncs for HBHarfBuzzFontFuncs {
    fn nominal_glyph(&mut self, _: &BuiltinFontFuncs, c: u32) -> Option<GlyphId> {
        let mut glyph = 0;
        if unsafe { hb_font_get_nominal_glyph(self.font, c, &mut glyph) } != 0 {
            Some(GlyphId::new(glyph))
        } else {
            None
        }
    }

    fn variant_glyph(&mut self, _: &BuiltinFontFuncs, c: u32, vs: u32) -> Option<GlyphId> {
        let mut glyph = 0;
        if unsafe { hb_font_get_variation_glyph(self.font, c, vs, &mut glyph) } != 0 {
            Some(GlyphId::new(glyph))
        } else {
            None
        }
    }

    fn advance_width(&mut self, _: &BuiltinFontFuncs, glyph: GlyphId) -> i32 {
        unsafe { hb_font_get_glyph_h_advance(self.font, glyph.to_u32()) }
    }

    fn populate_advance_widths(&mut self, _: &BuiltinFontFuncs, batch: AdvanceWidthBatch<'_>) {
        let raw = batch.into_raw();
        unsafe {
            hb_font_get_glyph_h_advances(
                self.font,
                raw.len as u32,
                raw.gids,
                raw.gid_stride as u32,
                raw.advances,
                raw.advance_stride as u32,
            );
        }
    }

    fn advance_height(&mut self, _: &BuiltinFontFuncs, glyph: GlyphId) -> i32 {
        unsafe { hb_font_get_glyph_v_advance(self.font, glyph.to_u32()) }
    }

    fn vertical_origin(&mut self, _: &BuiltinFontFuncs, glyph: GlyphId) -> (i32, i32) {
        let mut x = 0;
        let mut y = 0;
        unsafe {
            hb_font_get_glyph_v_origin(self.font, glyph.to_u32(), &mut x, &mut y);
        }
        (x, y)
    }

    fn extents(&mut self, _: &BuiltinFontFuncs, glyph: GlyphId) -> Option<GlyphExtents> {
        let mut extents = hb_glyph_extents_t {
            x_bearing: 0,
            y_bearing: 0,
            width: 0,
            height: 0,
        };
        if unsafe { hb_font_get_glyph_extents(self.font, glyph.to_u32(), &mut extents) } != 0 {
            Some(GlyphExtents {
                x_bearing: extents.x_bearing,
                y_bearing: extents.y_bearing,
                width: extents.width,
                height: extents.height,
            })
        } else {
            None
        }
    }
}

fn font_to_instance(font: *mut hb_font_t, font_ref: &Font) -> FontInstance {
    let mut num_coords: u32 = 0;
    let coords = unsafe { hb_font_get_var_coords_normalized(font, &mut num_coords) };
    let coords = if coords.is_null() {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(coords, num_coords as usize) }
    };
    let coords = coords.iter().map(|&v| NormalizedCoord::from_bits(v as i16));
    FontInstance::builder(font_ref)
        .normalized_coords(coords)
        .build()
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shaper_font_data_create_rs(
    font: *mut hb_font_t,
    face_data: *const c_void,
) -> *mut c_void {
    let face_data = face_data as *const HBHarfRustFaceData;

    let instance = font_to_instance(font, &(*face_data).font);
    let hr_font_data = HBHarfRustFontData { instance };

    let hr_font_data = Box::new(hr_font_data);
    let hr_font_data_ptr = Box::into_raw(hr_font_data);

    hr_font_data_ptr as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shaper_font_data_destroy_rs(data: *mut c_void) {
    let data = data as *mut HBHarfRustFontData;
    let _hr_font_data = Box::from_raw(data);
}

fn hb_language_to_hr_language(language: hb_language_t) -> Option<harfrust::Language> {
    let language_str = unsafe { hb_language_to_string(language) };
    if language_str.is_null() {
        return None;
    }
    let language_str = unsafe { std::ffi::CStr::from_ptr(language_str) };
    harfrust::Language::new(language_str.to_bytes())
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_buffer_create_rs() -> *mut c_void {
    let hr_buffer = Box::new(harfrust::UnicodeBuffer::new());
    Box::into_raw(hr_buffer) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_buffer_destroy_rs(data: *mut c_void) {
    let data = data as *mut harfrust::UnicodeBuffer;
    let _hr_buffer = Box::from_raw(data);
}

fn hb_feature_to_hr_feature(features: *const hb_feature_t, num_features: u32) -> HRFeatureVec {
    if features.is_null() {
        SmallVec::new()
    } else {
        let features = unsafe { std::slice::from_raw_parts(features, num_features as usize) };
        features
            .iter()
            .map(|f| {
                let tag = f.tag;
                let value = f.value;
                let start = f.start;
                let end = f.end;
                harfrust::Feature {
                    tag: Tag::from_u32(tag),
                    value,
                    start,
                    end,
                }
            })
            .collect()
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shape_plan_create_rs(
    font_data: *const c_void,
    script: hb_script_t,
    language: hb_language_t,
    direction: hb_direction_t,
    features: *const hb_feature_t,
    num_features: u32,
) -> *mut c_void {
    let font_data = font_data as *const HBHarfRustFontData;

    let script = harfrust::Script::from_iso15924_tag(Tag::from_u32(script as u32));
    let language = hb_language_to_hr_language(language);
    let direction = match direction {
        hb_direction_t_HB_DIRECTION_LTR => harfrust::Direction::LeftToRight,
        hb_direction_t_HB_DIRECTION_RTL => harfrust::Direction::RightToLeft,
        hb_direction_t_HB_DIRECTION_TTB => harfrust::Direction::TopToBottom,
        hb_direction_t_HB_DIRECTION_BTT => harfrust::Direction::BottomToTop,
        _ => harfrust::Direction::Invalid,
    };
    let features = hb_feature_to_hr_feature(features, num_features);

    let hr_shape_plan = harfrust::ShapePlan::new(
        &(*font_data).instance,
        direction,
        script,
        language.as_ref(),
        &features,
    );
    let hr_shape_plan = Box::new(hr_shape_plan);
    Box::into_raw(hr_shape_plan) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shape_plan_destroy_rs(data: *mut c_void) {
    let data = data as *mut harfrust::ShapePlan;
    let _hr_shape_plan = Box::from_raw(data);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shape_rs(
    font_data: *const c_void,
    shape_plan: *const c_void,
    hr_buffer_box: *const c_void,
    font: *mut hb_font_t,
    buffer: *mut hb_buffer_t,
    pre_context: *const u32,
    pre_context_length: u32,
    post_context: *const u32,
    post_context_length: u32,
    features: *const hb_feature_t,
    num_features: u32,
) -> hb_bool_t {
    let font_data = font_data as *const HBHarfRustFontData;
    let hr_buffer_box = hr_buffer_box as *mut harfrust::UnicodeBuffer;
    let mut hr_buffer_box = Box::from_raw(hr_buffer_box);
    let mut hr_buffer = *hr_buffer_box;

    // Set buffer properties
    let cluster_level = hb_buffer_get_cluster_level(buffer);
    let cluster_level = match cluster_level {
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES => {
            harfrust::BufferClusterLevel::MonotoneGraphemes
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS => {
            harfrust::BufferClusterLevel::MonotoneCharacters
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_CHARACTERS => {
            harfrust::BufferClusterLevel::Characters
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES => {
            harfrust::BufferClusterLevel::Graphemes
        }
        _ => harfrust::BufferClusterLevel::default(),
    };
    hr_buffer.set_cluster_level(cluster_level);
    let flags = hb_buffer_get_flags(buffer);
    hr_buffer.set_flags(harfrust::BufferFlags::from_bits_truncate(flags as u32));
    let not_found_variation_selector_glyph =
        hb_buffer_get_not_found_variation_selector_glyph(buffer);
    if not_found_variation_selector_glyph != u32::MAX {
        hr_buffer.set_not_found_variation_selector_glyph(not_found_variation_selector_glyph);
    }

    // Segment properties:
    let script = hb_buffer_get_script(buffer);
    let language = hb_buffer_get_language(buffer);
    let direction = hb_buffer_get_direction(buffer);
    // Convert to HarfRust types
    let script = harfrust::Script::from_iso15924_tag(Tag::from_u32(script as u32))
        .unwrap_or(harfrust::script::UNKNOWN);
    let language = hb_language_to_hr_language(language);
    let direction = match direction {
        hb_direction_t_HB_DIRECTION_LTR => harfrust::Direction::LeftToRight,
        hb_direction_t_HB_DIRECTION_RTL => harfrust::Direction::RightToLeft,
        hb_direction_t_HB_DIRECTION_TTB => harfrust::Direction::TopToBottom,
        hb_direction_t_HB_DIRECTION_BTT => harfrust::Direction::BottomToTop,
        _ => harfrust::Direction::Invalid,
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
    let infos = std::slice::from_raw_parts(infos.cast(), count as usize);
    if !hr_buffer.push_glyph_infos(infos) {
        return false as hb_bool_t;
    }

    let pre_context = std::slice::from_raw_parts(pre_context, pre_context_length as usize);
    hr_buffer.set_pre_context_codepoints(pre_context);
    let post_context = std::slice::from_raw_parts(post_context, post_context_length as usize);
    hr_buffer.set_post_context_codepoints(post_context);

    let ptem = hb_font_get_ptem(font);
    let ptem = if ptem > 0.0 { Some(ptem) } else { None };

    let features = hb_feature_to_hr_feature(features, num_features);
    let shape_plan = (shape_plan as *const harfrust::ShapePlan).as_ref();
    let mut x_scale = 0;
    let mut y_scale = 0;
    hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    let mut font_funcs = HBHarfBuzzFontFuncs { font };
    let options = ShapeOptions::new()
        .plan(shape_plan)
        .scale_separate(Some((x_scale, y_scale)))
        .point_size(ptem)
        .features(&features)
        .font_funcs(Some(&mut font_funcs));
    let glyphs = harfrust::shape(&(*font_data).instance, hr_buffer, options);

    hb_buffer_set_content_type(
        buffer,
        hb_buffer_content_type_t_HB_BUFFER_CONTENT_TYPE_GLYPHS,
    );
    let count = glyphs.len();
    hb_buffer_set_length(buffer, count as u32);
    let mut count_out: u32 = 0;
    let infos = hb_buffer_get_glyph_infos(buffer, &mut count_out);
    let positions = hb_buffer_get_glyph_positions(buffer, null_mut());
    if count != count_out as usize {
        return false as hb_bool_t;
    }

    std::ptr::copy_nonoverlapping(glyphs.glyph_infos().as_ptr().cast(), infos, count);
    std::ptr::copy_nonoverlapping(glyphs.glyph_positions().as_ptr().cast(), positions, count);

    let hr_buffer = glyphs.clear();
    *hr_buffer_box = hr_buffer; // Move the buffer back into the box
    let _ = Box::into_raw(hr_buffer_box); // Prevent double free

    true as hb_bool_t
}

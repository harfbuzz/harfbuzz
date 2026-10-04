#![allow(non_upper_case_globals)]
// C enum becomes i32 on some systems (eg. Windows).
#![allow(clippy::unnecessary_cast)]

use super::{hb::*, HbBlob};

use std::ffi::c_void;
use std::mem::{align_of, offset_of, size_of};
use std::ptr::null_mut;
use std::sync::Arc;

use harfrust::{
    font::{Blob, Font, NormalizedCoord, TableFunction},
    Advances, FontFuncs, GlyphExtents, GlyphFlags as HRGlyphFlags, GlyphId,
    GlyphInfo as HRGlyphInfo, GlyphPosition as HRGlyphPosition, NominalGlyphs, ShapeOptions,
    ShaperFont, Tag,
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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_shaper_face_data_create_rs(
    face: *mut hb_face_t,
) -> *mut c_void {
    let face_index = hb_face_get_index(face);
    let hb_face = HbFace::new(face);
    let table_fn = TableFunction::new(Arc::new(move |tag| {
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
        Some(Blob::Shared(Arc::new(HbBlob(blob))))
    }));

    let font = match Font::new(table_fn, face_index) {
        Some(font) => font,
        None => return null_mut(),
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
    instance: Font,
    x_scale: i32,
    y_scale: i32,
    ptem: Option<f32>,
}

struct HBHarfBuzzFontFuncs {
    font: *mut hb_font_t,
}

impl FontFuncs for HBHarfBuzzFontFuncs {
    fn nominal_glyph(&self, _: &ShaperFont, c: u32) -> Option<GlyphId> {
        let mut glyph = 0;
        if unsafe { hb_font_get_nominal_glyph(self.font, c, &mut glyph) } != 0 {
            Some(GlyphId::new(glyph))
        } else {
            None
        }
    }

    fn variation_glyph(&self, _: &ShaperFont, c: u32, vs: u32) -> Option<GlyphId> {
        let mut glyph = 0;
        if unsafe { hb_font_get_variation_glyph(self.font, c, vs, &mut glyph) } != 0 {
            Some(GlyphId::new(glyph))
        } else {
            None
        }
    }

    fn glyph_h_advance(&self, _: &ShaperFont, glyph: GlyphId) -> i32 {
        unsafe { hb_font_get_glyph_h_advance(self.font, glyph.to_u32()) }
    }

    fn glyph_h_advances(&self, _: &ShaperFont, batch: Advances<'_>) {
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

    fn nominal_glyphs(&self, _: &ShaperFont, batch: NominalGlyphs<'_>) -> usize {
        let raw = batch.into_raw();
        unsafe {
            hb_font_get_nominal_glyphs(
                self.font,
                raw.len as u32,
                raw.codepoints,
                raw.codepoint_stride as u32,
                raw.glyphs,
                raw.glyph_stride as u32,
            ) as usize
        }
    }

    fn glyph_v_advance(&self, _: &ShaperFont, glyph: GlyphId) -> i32 {
        unsafe { hb_font_get_glyph_v_advance(self.font, glyph.to_u32()) }
    }

    fn glyph_v_origin(&self, _: &ShaperFont, glyph: GlyphId) -> (i32, i32) {
        let mut x = 0;
        let mut y = 0;
        unsafe {
            hb_font_get_glyph_v_origin(self.font, glyph.to_u32(), &mut x, &mut y);
        }
        (x, y)
    }

    fn glyph_extents(&self, _: &ShaperFont, glyph: GlyphId) -> Option<GlyphExtents> {
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

fn font_to_instance(font: *mut hb_font_t, font_ref: &Font) -> Font {
    let mut num_coords: u32 = 0;
    let coords = unsafe { hb_font_get_var_coords_normalized(font, &mut num_coords) };
    let coords = if coords.is_null() {
        &[]
    } else {
        unsafe { std::slice::from_raw_parts(coords, num_coords as usize) }
    };
    let coords = coords.iter().map(|&v| NormalizedCoord::from_bits(v as i16));
    font_ref
        .instance_builder()
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
    let mut x_scale = 0;
    let mut y_scale = 0;
    hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    let ptem = hb_font_get_ptem(font);
    let ptem = (ptem > 0.0).then_some(ptem);
    // HarfBuzz invalidates shaper font data when any of these values change.
    let hr_font_data = HBHarfRustFontData {
        instance,
        x_scale,
        y_scale,
        ptem,
    };

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
    let hr_buffer = Box::new(harfrust::Buffer::new());
    Box::into_raw(hr_buffer) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _hb_harfrust_buffer_destroy_rs(data: *mut c_void) {
    let data = data as *mut harfrust::Buffer;
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
    let hr_buffer = &mut *(hr_buffer_box as *mut harfrust::Buffer);
    hr_buffer.clear();
    let shape_plan = (shape_plan as *const harfrust::ShapePlan).as_ref();

    // Set buffer properties
    let cluster_level = hb_buffer_get_cluster_level(buffer);
    let cluster_level = match cluster_level {
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES => {
            harfrust::ClusterLevel::MonotoneGraphemes
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS => {
            harfrust::ClusterLevel::MonotoneCharacters
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_CHARACTERS => {
            harfrust::ClusterLevel::Characters
        }
        hb_buffer_cluster_level_t_HB_BUFFER_CLUSTER_LEVEL_GRAPHEMES => {
            harfrust::ClusterLevel::Graphemes
        }
        _ => harfrust::ClusterLevel::MonotoneGraphemes,
    };
    hr_buffer.set_cluster_level(cluster_level);
    let flags = hb_buffer_get_flags(buffer);
    hr_buffer.set_flags(harfrust::BufferFlags::from_bits_truncate(flags as u32));
    let not_found_variation_selector_glyph =
        hb_buffer_get_not_found_variation_selector_glyph(buffer);
    hr_buffer.set_not_found_variation_selector_glyph(
        (not_found_variation_selector_glyph != u32::MAX)
            .then_some(not_found_variation_selector_glyph),
    );

    if let Some(plan) = shape_plan {
        // Language is only used to build the plan; shaping with an explicit
        // plan reads its cached segment properties instead of buffer language.
        hr_buffer.set_script(Some(plan.script().unwrap_or(harfrust::Script::UNKNOWN)));
        hr_buffer.set_direction(plan.direction());
    } else {
        // Convert segment properties when shaping without a cached plan.
        let script = hb_buffer_get_script(buffer);
        let language = hb_buffer_get_language(buffer);
        let direction = hb_buffer_get_direction(buffer);
        let script = harfrust::Script::from_iso15924_tag(Tag::from_u32(script as u32))
            .unwrap_or(harfrust::Script::UNKNOWN);
        let language = hb_language_to_hr_language(language);
        let direction = match direction {
            hb_direction_t_HB_DIRECTION_LTR => harfrust::Direction::LeftToRight,
            hb_direction_t_HB_DIRECTION_RTL => harfrust::Direction::RightToLeft,
            hb_direction_t_HB_DIRECTION_TTB => harfrust::Direction::TopToBottom,
            hb_direction_t_HB_DIRECTION_BTT => harfrust::Direction::BottomToTop,
            _ => harfrust::Direction::Invalid,
        };
        hr_buffer.set_script(Some(script));
        if let Some(lang) = language {
            hr_buffer.set_language(Some(lang));
        }
        hr_buffer.set_direction(direction);
    }

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

    let features = hb_feature_to_hr_feature(features, num_features);
    let font_funcs = HBHarfBuzzFontFuncs { font };
    let shaper_font = ShaperFont::new(&(*font_data).instance)
        .with_scale_separate((*font_data).x_scale, (*font_data).y_scale)
        .with_font_funcs(Some(&font_funcs));
    let options = ShapeOptions::new()
        .plan(shape_plan)
        .point_size((*font_data).ptem)
        .features(&features);
    if harfrust::shape(&shaper_font, hr_buffer, options).is_err()
        || !hr_buffer.allocation_successful()
    {
        return false as hb_bool_t;
    }

    hb_buffer_set_content_type(
        buffer,
        hb_buffer_content_type_t_HB_BUFFER_CONTENT_TYPE_GLYPHS,
    );
    let count = hr_buffer.len();
    hb_buffer_set_length(buffer, count as u32);
    let mut count_out: u32 = 0;
    let infos = hb_buffer_get_glyph_infos(buffer, &mut count_out);
    let positions = hb_buffer_get_glyph_positions(buffer, null_mut());
    if count != count_out as usize {
        return false as hb_bool_t;
    }

    std::ptr::copy_nonoverlapping(hr_buffer.glyph_infos().as_ptr().cast(), infos, count);
    std::ptr::copy_nonoverlapping(
        hr_buffer.glyph_positions().as_ptr().cast(),
        positions,
        count,
    );

    hr_buffer.clear();

    true as hb_bool_t
}

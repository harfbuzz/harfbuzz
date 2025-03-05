#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::os::raw::c_void;
use std::ptr::null_mut;
use std::sync::atomic::{AtomicPtr, Ordering};

use read_fonts::tables::cpal::ColorRecord;
use read_fonts::TableProvider;
use skrifa::charmap::Charmap;
use skrifa::charmap::MapVariant::Variant;
use skrifa::color::{
    Brush, ColorGlyphCollection, ColorPainter, ColorStop, CompositeMode, Extend, Transform,
};
use skrifa::font::FontRef;
use skrifa::instance::{Location, NormalizedCoord, Size};
use skrifa::metrics::BoundingBox;
use skrifa::outline::pen::OutlinePen;
use skrifa::outline::DrawSettings;
use skrifa::OutlineGlyphCollection;
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
    outline_glyphs: OutlineGlyphCollection<'static>,
    color_glyphs: ColorGlyphCollection<'static>,
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
            *((first_unicode as *const u8).offset((i * unicode_stride) as isize)
                as *const hb_codepoint_t)
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
            *((first_glyph as *const u8).offset((i * glyph_stride) as isize)
                as *const hb_codepoint_t)
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
    let x_size = &data.x_size;
    let location = &data.location;
    let outline_glyphs = &data.outline_glyphs;

    // Create an outline-glyph
    let glyph_id = GlyphId::new(glyph as u32);
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

struct HbColorPainter {
    font: *mut hb_font_t,
    paint_funcs: *mut hb_paint_funcs_t,
    paint_data: *mut c_void,
    color_records: Box<[ColorRecord]>,
    foreground: hb_color_t,
    composite_mode: Vec<CompositeMode>,
    clip_transform_stack: Vec<bool>,
}

impl HbColorPainter {
    fn push_root_transform(&mut self) {
        let font = self.font;
        let face = unsafe { hb_font_get_face(font) };
        let upem = unsafe { hb_face_get_upem(face) };
        let mut x_scale: i32 = 0;
        let mut y_scale: i32 = 0;
        unsafe {
            hb_font_get_scale(font, &mut x_scale, &mut y_scale);
        }
        let slant = unsafe { hb_font_get_synthetic_slant(font) };
        let slant = if y_scale != 0 {
            slant as f32 * x_scale as f32 / y_scale as f32
        } else {
            0.
        };

        self.push_transform(Transform {
            xx: x_scale as f32 / upem as f32,
            yx: 0.0,
            xy: slant * y_scale as f32 / upem as f32,
            yy: y_scale as f32 / upem as f32,
            dx: 0.0,
            dy: 0.0,
        });
    }

    fn push_inverse_root_transform(&mut self) {
        let font = self.font;
        let face = unsafe { hb_font_get_face(font) };
        let upem = unsafe { hb_face_get_upem(face) };
        let mut x_scale: i32 = 0;
        let mut y_scale: i32 = 0;
        unsafe {
            hb_font_get_scale(font, &mut x_scale, &mut y_scale);
        }
        let slant = unsafe { hb_font_get_synthetic_slant(font) };

        self.push_transform(Transform {
            xx: upem as f32 / x_scale as f32,
            yx: 0.0,
            xy: -slant * upem as f32 / x_scale as f32,
            yy: upem as f32 / y_scale as f32,
            dx: 0.0,
            dy: 0.0,
        });
    }

    fn lookup_color(&self, color_index: u16, alpha: f32) -> hb_color_t {
        if color_index == 0xFFFF {
            self.foreground
        } else {
            let c = self.color_records[color_index as usize]; // TODO: bounds check
            (((c.blue as u32) << 24)
                | ((c.green as u32) << 16)
                | ((c.red as u32) << 8)
                | ((c.alpha as f32 * alpha) as u32)) as hb_color_t
        }
    }

    fn make_color_line(&mut self, color_stops: &ColorLineData) -> hb_color_line_t {
        let mut cl = unsafe { std::mem::zeroed::<hb_color_line_t>() };
        cl.data = self as *mut HbColorPainter as *mut ::std::os::raw::c_void;
        cl.get_color_stops = Some(_hb_fontations_get_color_stops);
        cl.get_color_stops_user_data =
            color_stops as *const ColorLineData as *mut ::std::os::raw::c_void;
        cl.get_extend = Some(_hb_fontations_get_extend);
        cl.get_extend_user_data =
            color_stops as *const ColorLineData as *mut ::std::os::raw::c_void;
        cl
    }
}

struct ColorLineData<'a> {
    color_stops: &'a [ColorStop],
    extend: Extend,
}
extern "C" fn _hb_fontations_get_color_stops(
    _color_line: *mut hb_color_line_t,
    color_line_data: *mut ::std::os::raw::c_void,
    start: ::std::os::raw::c_uint,
    count_out: *mut ::std::os::raw::c_uint,
    color_stops_out: *mut hb_color_stop_t,
    user_data: *mut ::std::os::raw::c_void,
) -> ::std::os::raw::c_uint {
    let color_painter = unsafe { &*(color_line_data as *const HbColorPainter) };
    let color_line_data = unsafe { &*(user_data as *const ColorLineData) };
    let color_stops = &color_line_data.color_stops;
    if count_out.is_null() {
        return color_stops.len() as u32;
    }
    let count = unsafe { *count_out };
    for i in 0..count {
        let stop = color_stops.get(start as usize + i as usize);
        if stop.is_none() {
            unsafe {
                *count_out = i;
            };
            return color_stops.len() as u32;
        }
        let stop = stop.unwrap();
        unsafe {
            *(color_stops_out.offset(i as isize)) = hb_color_stop_t {
                offset: stop.offset,
                color: color_painter.lookup_color(stop.palette_index, stop.alpha),
                is_foreground: (stop.palette_index == 0xFFFF) as hb_bool_t,
            };
        }
    }
    return color_stops.len() as u32;
}
extern "C" fn _hb_fontations_get_extend(
    _color_line: *mut hb_color_line_t,
    color_line_data: *mut ::std::os::raw::c_void,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_paint_extend_t {
    let color_line_data = unsafe { &*(color_line_data as *const ColorLineData) };
    color_line_data.extend as hb_paint_extend_t // They are the same
}

impl ColorPainter for HbColorPainter {
    fn push_transform(&mut self, transform: Transform) {
        unsafe {
            hb_paint_push_transform(
                self.paint_funcs,
                self.paint_data,
                transform.xx,
                transform.yx,
                transform.xy,
                transform.yy,
                transform.dx,
                transform.dy,
            );
        }
    }
    fn pop_transform(&mut self) {
        unsafe {
            hb_paint_pop_transform(self.paint_funcs, self.paint_data);
        }
    }
    fn push_clip_glyph(&mut self, glyph: GlyphId) {
        let gid = u32::from(glyph);
        self.clip_transform_stack.push(true);
        self.push_inverse_root_transform();
        unsafe {
            hb_paint_push_clip_glyph(
                self.paint_funcs,
                self.paint_data,
                gid as hb_codepoint_t,
                self.font,
            );
        }
        self.push_root_transform();
    }
    fn push_clip_box(&mut self, bbox: BoundingBox) {
        self.clip_transform_stack.push(false);
        unsafe {
            hb_paint_push_clip_rectangle(
                self.paint_funcs,
                self.paint_data,
                bbox.x_min,
                bbox.y_min,
                bbox.x_max,
                bbox.y_max,
            );
        }
    }
    fn pop_clip(&mut self) {
        let pop_transforms = self.clip_transform_stack.pop().unwrap_or(false);
        if pop_transforms {
            self.pop_transform();
        }
        unsafe {
            hb_paint_pop_clip(self.paint_funcs, self.paint_data);
        }
        if pop_transforms {
            self.pop_transform();
        }
    }
    fn fill(&mut self, brush: Brush) {
        match brush {
            Brush::Solid {
                palette_index: color_index,
                alpha,
            } => {
                let is_foreground = color_index == 0xFFFF;
                unsafe {
                    hb_paint_color(
                        self.paint_funcs,
                        self.paint_data,
                        is_foreground as hb_bool_t,
                        self.lookup_color(color_index, alpha),
                    );
                }
            }
            Brush::LinearGradient {
                color_stops,
                extend,
                p0,
                p1,
            } => {
                let color_stops = ColorLineData {
                    color_stops: &color_stops,
                    extend,
                };
                let mut color_line = self.make_color_line(&color_stops);
                unsafe {
                    hb_paint_linear_gradient(
                        self.paint_funcs,
                        self.paint_data,
                        &mut color_line,
                        p0.x,
                        p0.y,
                        p1.x,
                        p1.y,
                        p1.x, // TODO: Where's p2?
                        p1.y, // TODO: Where's p2?
                    );
                }
            }
            Brush::RadialGradient {
                color_stops,
                extend,
                c0,
                r0,
                c1,
                r1,
            } => {
                let color_stops = ColorLineData {
                    color_stops: &color_stops,
                    extend,
                };
                let mut color_line = self.make_color_line(&color_stops);
                unsafe {
                    hb_paint_radial_gradient(
                        self.paint_funcs,
                        self.paint_data,
                        &mut color_line,
                        c0.x,
                        c0.y,
                        r0,
                        c1.x,
                        c1.y,
                        r1,
                    );
                }
            }
            Brush::SweepGradient {
                color_stops,
                extend,
                c0,
                start_angle,
                end_angle,
            } => {
                let color_stops = ColorLineData {
                    color_stops: &color_stops,
                    extend,
                };
                let mut color_line = self.make_color_line(&color_stops);
                unsafe {
                    hb_paint_sweep_gradient(
                        self.paint_funcs,
                        self.paint_data,
                        &mut color_line,
                        c0.x,
                        c0.y,
                        start_angle,
                        end_angle,
                    );
                }
            }
        }
    }
    fn push_layer(&mut self, mode: CompositeMode) {
        self.composite_mode.push(mode);
        unsafe {
            hb_paint_push_group(self.paint_funcs, self.paint_data);
        }
    }
    fn pop_layer(&mut self) {
        let mode = self.composite_mode.pop();
        if mode.is_none() {
            return;
        }
        let mode = mode.unwrap() as hb_paint_composite_mode_t; // They are the same
        unsafe {
            hb_paint_pop_group(self.paint_funcs, self.paint_data, mode);
        }
    }
}

extern "C" fn _hb_fontations_paint_glyph(
    font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    glyph: hb_codepoint_t,
    paint_funcs: *mut hb_paint_funcs_t,
    paint_data: *mut ::std::os::raw::c_void,
    _palette_index: ::std::os::raw::c_uint,
    foreground: hb_color_t,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe { &*(font_data as *const FontationsData) };
    let font_ref = &data.font_ref;
    let location = &data.location;
    let color_glyphs = &data.color_glyphs;

    // Create an color-glyph
    let glyph_id = GlyphId::new(glyph as u32);
    let color_glyph = color_glyphs.get(glyph_id);
    if color_glyph.is_none() {
        return;
    }
    let color_glyph = color_glyph.unwrap();

    let cpal = font_ref.cpal();
    let mut color_records = Box::<[ColorRecord]>::default();
    if cpal.is_ok() {
        let cpal = cpal.unwrap();
        // TODO fontations doesn't seem to provide a way to access palettes
        // really. Just assume the first palette starts at the beginning
        // of the color records array.
        let color_records_array = cpal.color_records_array();
        if !color_records_array.is_none() {
            color_records = Box::from(color_records_array.unwrap().unwrap());
        }
    }

    let mut painter = HbColorPainter {
        font: font,
        paint_funcs,
        paint_data,
        color_records,
        foreground,
        composite_mode: Vec::new(),
        clip_transform_stack: Vec::new(),
    };
    painter.push_root_transform();
    let _ = color_glyph.paint(location, &mut painter);
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
            hb_font_funcs_set_paint_glyph_func(
                ffuncs,
                Some(_hb_fontations_paint_glyph),
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

    let mut x_scale: i32 = 0;
    let mut y_scale: i32 = 0;
    unsafe {
        hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    };
    let x_size = Size::new(x_scale as f32);
    let y_size = Size::new(y_scale as f32);

    let mut num_coords: u32 = 0;
    let coords = unsafe { hb_font_get_var_coords_normalized(font, &mut num_coords) };
    let coords = unsafe { std::slice::from_raw_parts(coords, num_coords as usize) };
    let all_zeros = coords.iter().all(|&x| x == 0);
    // if all zeros, use Location::default()
    // otherwise, use the provided coords.
    // This currently doesn't seem to have a perf effect on fontations, but it's a good idea to
    // check if the coords are all zeros before creating a Location.
    let location = if all_zeros {
        Location::default()
    } else {
        let mut location = Location::new(num_coords as usize);
        let coords_mut = location.coords_mut();
        for i in 0..num_coords as usize {
            coords_mut[i] = NormalizedCoord::from_bits(coords[i] as i16);
        }
        location
    };

    let outline_glyphs = font_ref.outline_glyphs();

    let color_glyphs = font_ref.color_glyphs();

    let data = Box::new(FontationsData {
        face_blob,
        font_ref,
        char_map,
        x_size,
        y_size,
        location,
        outline_glyphs,
        color_glyphs,
    });
    let data_ptr = Box::into_raw(data) as *mut c_void;

    unsafe {
        hb_font_set_funcs(font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
    }
}

#![allow(non_camel_case_types)]
#![allow(non_upper_case_globals)]
include!(concat!(env!("OUT_DIR"), "/hb.rs"));

use std::alloc::{GlobalAlloc, Layout};
use std::mem::transmute;
use std::os::raw::c_void;
use std::ptr::null_mut;
use std::sync::atomic::{AtomicPtr, Ordering};
use std::sync::Mutex;

use read_fonts::tables::cpal::ColorRecord;
use read_fonts::TableProvider;
use skrifa::charmap::Charmap;
use skrifa::charmap::MapVariant::Variant;
use skrifa::color::{
    Brush, ColorGlyphCollection, ColorPainter, ColorStop, CompositeMode, Extend, Transform,
};
use skrifa::font::FontRef;
use skrifa::instance::{Location, NormalizedCoord, Size};
use skrifa::metrics::{BoundingBox, GlyphMetrics};
use skrifa::outline::pen::OutlinePen;
use skrifa::outline::DrawSettings;
use skrifa::OutlineGlyphCollection;
use skrifa::{GlyphId, MetadataProvider};

struct MyAllocator;

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_malloc(layout.size()) as *mut u8
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_calloc(layout.size(), 1) as *mut u8
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_realloc(ptr as *mut c_void, new_size) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_free(ptr as *mut c_void);
    }
}

#[global_allocator]
static GLOBAL: MyAllocator = MyAllocator;

// A struct for storing your “fontations” data
#[repr(C)]
struct FontationsData<'a> {
    face_blob: *mut hb_blob_t,
    font: *mut hb_font_t,
    font_ref: FontRef<'a>,
    char_map: Charmap<'a>,
    outline_glyphs: OutlineGlyphCollection<'a>,
    color_glyphs: ColorGlyphCollection<'a>,

    // Mutex for the below
    mutex: Mutex<()>,
    serial: u32,
    x_size: Size,
    y_size: Size,
    location: Location,
    x_glyph_metrics: Option<GlyphMetrics<'a>>,
    y_glyph_metrics: Option<GlyphMetrics<'a>>,
}

impl FontationsData<'_> {
    unsafe fn from_hb_font(font: *mut hb_font_t) -> Self {
        let face_index = hb_face_get_index(hb_font_get_face(font));
        let face_blob = hb_face_reference_blob(hb_font_get_face(font));
        let blob_length = hb_blob_get_length(face_blob);
        let blob_data = hb_blob_get_data(face_blob, null_mut());
        let face_data = std::slice::from_raw_parts(blob_data as *const u8, blob_length as usize);

        let font_ref = FontRef::from_index(face_data, face_index).unwrap();

        let char_map = Charmap::new(&font_ref);

        let outline_glyphs = font_ref.outline_glyphs();

        let color_glyphs = font_ref.color_glyphs();

        let mut data = FontationsData {
            face_blob,
            font,
            font_ref,
            char_map,
            outline_glyphs,
            color_glyphs,
            mutex: Mutex::new(()),
            serial: u32::MAX,
            x_size: Size::new(0.0),
            y_size: Size::new(0.0),
            location: Location::default(),
            x_glyph_metrics: None,
            y_glyph_metrics: None,
        };

        data.check_for_updates();

        data
    }

    unsafe fn _check_for_updates(&mut self) {
        let font_serial = hb_font_get_serial(self.font);
        if self.serial == font_serial {
            return;
        }

        let _lock = self.mutex.lock().unwrap();

        let mut x_scale: i32 = 0;
        let mut y_scale: i32 = 0;
        hb_font_get_scale(self.font, &mut x_scale, &mut y_scale);
        self.x_size = Size::new(x_scale as f32);
        self.y_size = Size::new(y_scale as f32);

        let mut num_coords: u32 = 0;
        let coords = hb_font_get_var_coords_normalized(self.font, &mut num_coords);
        let coords = if coords.is_null() {
            &[]
        } else {
            std::slice::from_raw_parts(coords, num_coords as usize)
        };
        let all_zeros = coords.iter().all(|&x| x == 0);
        // if all zeros, use Location::default()
        // otherwise, use the provided coords.
        // This currently doesn't seem to have a perf effect on fontations, but it's a good idea to
        // check if the coords are all zeros before creating a Location.
        self.location = if all_zeros {
            Location::default()
        } else {
            let mut location = Location::new(num_coords as usize);
            let coords_mut = location.coords_mut();
            coords_mut
                .iter_mut()
                .zip(coords.iter().map(|v| NormalizedCoord::from_bits(*v as i16)))
                .for_each(|(dest, source)| *dest = source);
            location
        };

        let location = transmute::<&Location, &Location>(&self.location);
        self.x_glyph_metrics = Some(self.font_ref.glyph_metrics(self.x_size, location));
        let location = transmute::<&Location, &Location>(&self.location);
        self.y_glyph_metrics = Some(self.font_ref.glyph_metrics(self.y_size, location));

        self.serial = font_serial;
    }
    fn check_for_updates(&mut self) {
        unsafe { self._check_for_updates() }
    }
}

extern "C" fn _hb_fontations_data_destroy(font_data: *mut c_void) {
    let data = unsafe { Box::from_raw(font_data as *mut FontationsData) };

    unsafe {
        hb_blob_destroy(data.face_blob);
    }
}

fn struct_at_offset<T: Copy>(first: *const T, index: u32, stride: u32) -> T {
    unsafe { *((first as *const u8).offset((index * stride) as isize) as *const T) }
}

fn struct_at_offset_mut<T: Copy>(first: *mut T, index: u32, stride: u32) -> &'static mut T {
    unsafe { &mut *((first as *mut u8).offset((index * stride) as isize) as *mut T) }
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
        let unicode = struct_at_offset(first_unicode, i, unicode_stride);
        let Some(glyph) = char_map.map(unicode) else {
            return i;
        };
        let glyph_id = u32::from(glyph) as hb_codepoint_t;
        *struct_at_offset_mut(first_glyph, i, glyph_stride) = glyph_id;
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

    match char_map.map_variant(unicode, variation_selector) {
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
    let data = unsafe { &mut *(font_data as *mut FontationsData) };
    data.check_for_updates();

    let glyph_metrics = &data.y_glyph_metrics.as_ref().unwrap();

    for i in 0..count {
        let glyph = struct_at_offset(first_glyph, i, glyph_stride);
        let glyph_id = GlyphId::new(glyph);
        let advance = glyph_metrics
            .advance_width(glyph_id)
            .unwrap_or_default()
            .round() as i32;
        *struct_at_offset_mut(first_advance, i, advance_stride) = advance as hb_position_t;
    }
}
extern "C" fn _hb_fontations_get_glyph_extents(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    glyph: hb_codepoint_t,
    extents: *mut hb_glyph_extents_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_bool_t {
    let data = unsafe { &mut *(font_data as *mut FontationsData) };
    data.check_for_updates();

    let x_size = &data.x_size;
    let y_size = &data.y_size;
    let x_glyph_metrics = &data.x_glyph_metrics.as_ref().unwrap();
    let y_glyph_metrics = &data.y_glyph_metrics.as_ref().unwrap();

    let glyph_id = GlyphId::new(glyph);
    let x_extents = x_glyph_metrics.bounds(glyph_id);
    let y_extents = if x_size == y_size {
        x_extents
    } else {
        y_glyph_metrics.bounds(glyph_id)
    };
    let (Some(x_extents), Some(y_extents)) = (x_extents, y_extents) else {
        return false as hb_bool_t;
    };

    unsafe {
        *extents = hb_glyph_extents_t {
            x_bearing: x_extents.x_min as hb_position_t,
            width: (x_extents.x_max - x_extents.x_min) as hb_position_t,
            y_bearing: y_extents.y_max as hb_position_t,
            height: (y_extents.y_min - y_extents.y_max) as hb_position_t,
        };
    }

    true as hb_bool_t
}

extern "C" fn _hb_fontations_get_font_h_extents(
    _font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    extents: *mut hb_font_extents_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_bool_t {
    let data = unsafe { &mut *(font_data as *mut FontationsData) };
    data.check_for_updates();

    let font_ref = &data.font_ref;
    let size = &data.y_size;
    let location = &data.location;
    let metrics = font_ref.metrics(*size, location);

    unsafe {
        (*extents).ascender = metrics.ascent as hb_position_t;
        (*extents).descender = metrics.descent as hb_position_t;
        (*extents).line_gap = metrics.leading as hb_position_t;
    }

    true as hb_bool_t
}

struct HbPen {
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
    font: *mut hb_font_t,
    font_data: *mut ::std::os::raw::c_void,
    glyph: hb_codepoint_t,
    draw_funcs: *mut hb_draw_funcs_t,
    draw_data: *mut ::std::os::raw::c_void,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe { &mut *(font_data as *mut FontationsData) };
    data.check_for_updates();

    let x_size = &data.x_size;
    let location = &data.location;
    let outline_glyphs = &data.outline_glyphs;

    // Create an outline-glyph
    let glyph_id = GlyphId::new(glyph);
    let Some(outline_glyph) = outline_glyphs.get(glyph_id) else {
        return;
    };
    let draw_settings = DrawSettings::unhinted(*x_size, location);
    // Allocate zero bytes for the draw_state_t on the stack.
    let mut draw_state: hb_draw_state_t = unsafe { std::mem::zeroed::<hb_draw_state_t>() };

    let slant = unsafe { hb_font_get_synthetic_slant(font) };
    let mut x_scale: i32 = 0;
    let mut y_scale: i32 = 0;
    unsafe {
        hb_font_get_scale(font, &mut x_scale, &mut y_scale);
    }
    let slant = if y_scale != 0 {
        slant as f32 * x_scale as f32 / y_scale as f32
    } else {
        0.
    };
    draw_state.slant_xy = slant;

    let mut pen = HbPen {
        draw_state: &mut draw_state,
        draw_funcs,
        draw_data,
    };

    let _ = outline_glyph.draw(draw_settings, &mut pen);
}

struct HbColorPainter<'a> {
    font: *mut hb_font_t,
    paint_funcs: *mut hb_paint_funcs_t,
    paint_data: *mut c_void,
    color_records: &'a [ColorRecord],
    foreground: hb_color_t,
    composite_mode: Vec<CompositeMode>,
    clip_transform_stack: Vec<bool>,
}

impl HbColorPainter<'_> {
    fn lookup_color(&self, color_index: u16, alpha: f32) -> hb_color_t {
        if color_index == 0xFFFF {
            // Apply alpha to foreground color
            return ((self.foreground & 0xFFFFFF00)
                | (((self.foreground & 0xFF) as f32 * alpha).round() as u32))
                as hb_color_t;
        }

        let c = self.color_records.get(color_index as usize);
        if c.is_some() {
            let c = c.unwrap();
            (((c.blue as u32) << 24)
                | ((c.green as u32) << 16)
                | ((c.red as u32) << 8)
                | ((c.alpha as f32 * alpha).round() as u32)) as hb_color_t
        } else {
            0 as hb_color_t
        }
    }

    fn make_color_line(&self, color_line: &ColorLineData) -> hb_color_line_t {
        let mut cl = unsafe { std::mem::zeroed::<hb_color_line_t>() };
        cl.data = color_line as *const ColorLineData as *mut ::std::os::raw::c_void;
        cl.get_color_stops = Some(_hb_fontations_get_color_stops);
        cl.get_extend = Some(_hb_fontations_get_extend);
        cl
    }
}

struct ColorLineData<'a> {
    painter: &'a HbColorPainter<'a>,
    color_stops: &'a [ColorStop],
    extend: Extend,
}
extern "C" fn _hb_fontations_get_color_stops(
    _color_line: *mut hb_color_line_t,
    color_line_data: *mut ::std::os::raw::c_void,
    start: ::std::os::raw::c_uint,
    count_out: *mut ::std::os::raw::c_uint,
    color_stops_out: *mut hb_color_stop_t,
    _user_data: *mut ::std::os::raw::c_void,
) -> ::std::os::raw::c_uint {
    let color_line_data = unsafe { &*(color_line_data as *const ColorLineData) };
    let color_stops = &color_line_data.color_stops;
    if count_out.is_null() {
        return color_stops.len() as u32;
    }
    let count = unsafe { *count_out };
    for i in 0..count {
        let Some(stop) = color_stops.get(start as usize + i as usize) else {
            unsafe {
                *count_out = i;
            };
            break;
        };
        unsafe {
            *(color_stops_out.offset(i as isize)) = hb_color_stop_t {
                offset: stop.offset,
                color: color_line_data
                    .painter
                    .lookup_color(stop.palette_index, stop.alpha),
                is_foreground: (stop.palette_index == 0xFFFF) as hb_bool_t,
            };
        }
    }
    color_stops.len() as u32
}
extern "C" fn _hb_fontations_get_extend(
    _color_line: *mut hb_color_line_t,
    color_line_data: *mut ::std::os::raw::c_void,
    _user_data: *mut ::std::os::raw::c_void,
) -> hb_paint_extend_t {
    let color_line_data = unsafe { &*(color_line_data as *const ColorLineData) };
    color_line_data.extend as hb_paint_extend_t // They are the same
}

pub fn _hb_fontations_unreduce_anchors(
    x0: f32,
    y0: f32,
    x1: f32,
    y1: f32,
) -> (f32, f32, f32, f32, f32, f32) {
    /* Returns (x0, y0, x1, y1, x2, y2) such that the original
     * `_hb_cairo_reduce_anchors` would produce (xx0, yy0, xx1, yy1)
     * as outputs.
     * The OT spec has the following wording; we just need to
     * invert that operation here:
     *
     * Note: An implementation can derive a single vector, from p₀ to a point p₃, by computing the
     * orthogonal projection of the vector from p₀ to p₁ onto a line perpendicular to line p₀p₂ and
     * passing through p₀ to obtain point p₃. The linear gradient defined using p₀, p₁ and p₂ as
     * described above is functionally equivalent to a linear gradient defined by aligning stop
     * offset 0 to p₀ and aligning stop offset 1.0 to p₃, with each color projecting on either side
     * of that line in a perpendicular direction. This specification uses three points, p₀, p₁ and
     * p₂, as that provides greater flexibility in controlling the placement and rotation of the
     * gradient, as well as variations thereof.
     */

    let dx = x1 - x0;
    let dy = y1 - y0;

    (x0, y0, x1, y1, x0 + dy, y0 - dx)
}

impl ColorPainter for HbColorPainter<'_> {
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
        unsafe {
            hb_paint_push_inverse_font_transform(self.paint_funcs, self.paint_data, self.font);
            hb_paint_push_clip_glyph(
                self.paint_funcs,
                self.paint_data,
                gid as hb_codepoint_t,
                self.font,
            );
            hb_paint_push_font_transform(self.paint_funcs, self.paint_data, self.font);
        }
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
                    painter: self,
                    color_stops,
                    extend,
                };
                let mut color_line = self.make_color_line(&color_stops);

                let (x0, y0, x1, y1, x2, y2) =
                    _hb_fontations_unreduce_anchors(p0.x, p0.y, p1.x, p1.y);

                unsafe {
                    hb_paint_linear_gradient(
                        self.paint_funcs,
                        self.paint_data,
                        &mut color_line,
                        x0,
                        y0,
                        x1,
                        y1,
                        x2,
                        y2,
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
                    painter: self,
                    color_stops,
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
                    painter: self,
                    color_stops,
                    extend,
                };
                let mut color_line = self.make_color_line(&color_stops);
                // Skrifa has this gem, so we swap end_angle and start_angle
                // when passing to our API:
                //
                //  * Convert angles and stops from counter-clockwise to clockwise
                //  * for the shader if the gradient is not already reversed due to
                //  * start angle being larger than end angle.
                //
                //  Undo that.
                let (start_angle, end_angle) = (360. - start_angle, 360. - end_angle);
                let start_angle = start_angle.to_radians();
                let end_angle = end_angle.to_radians();
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
    palette_index: ::std::os::raw::c_uint,
    foreground: hb_color_t,
    _user_data: *mut ::std::os::raw::c_void,
) {
    let data = unsafe { &mut *(font_data as *mut FontationsData) };
    data.check_for_updates();

    let font_ref = &data.font_ref;
    let location = &data.location;
    let color_glyphs = &data.color_glyphs;

    // Create an color-glyph
    let glyph_id = GlyphId::new(glyph);
    let Some(color_glyph) = color_glyphs.get(glyph_id) else {
        return;
    };

    let cpal = font_ref.cpal();
    let color_records = if cpal.is_err() {
        unsafe { std::slice::from_raw_parts(std::ptr::NonNull::dangling().as_ptr(), 0) }
    } else {
        let cpal = cpal.unwrap();
        let num_entries = cpal.num_palette_entries().into();
        let color_records = cpal.color_records_array();
        let start_index = cpal.color_record_indices().get(palette_index as usize);
        let start_index = if start_index.is_some() {
            start_index
        } else {
            // https://github.com/harfbuzz/harfbuzz/issues/5116
            cpal.color_record_indices().first()
        };

        if let (Some(Ok(color_records)), Some(start_index)) = (color_records, start_index) {
            let start_index: usize = start_index.get().into();
            let color_records = &color_records[start_index..start_index + num_entries];
            unsafe { std::slice::from_raw_parts(color_records.as_ptr(), num_entries) }
        } else {
            unsafe { std::slice::from_raw_parts(std::ptr::NonNull::dangling().as_ptr(), 0) }
        }
    };

    let mut painter = HbColorPainter {
        font,
        paint_funcs,
        paint_data,
        color_records,
        foreground,
        composite_mode: Vec::new(),
        clip_transform_stack: Vec::new(),
    };
    unsafe {
        hb_paint_push_font_transform(paint_funcs, paint_data, font);
    }
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

/// # Safety
///
/// This function is unsafe because it connects with the HarfBuzz API.
#[no_mangle]
pub unsafe extern "C" fn hb_fontations_font_set_funcs(font: *mut hb_font_t) {
    let ffuncs = _hb_fontations_font_funcs_get();

    let data = FontationsData::from_hb_font(font);
    let data_ptr = Box::into_raw(Box::new(data)) as *mut c_void;

    hb_font_set_funcs(font, ffuncs, data_ptr, Some(_hb_fontations_data_destroy));
}

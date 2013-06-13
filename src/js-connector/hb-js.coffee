
Module["Bool"] = Bool = "i1"
Module["Char"] = Char = "i8"
Module["Unsigned_Char"] = Unsigned_Char = "i8"
Module["int8_t"] = int8_t = "i8"
Module["uint8_t"] = uint8_t = "i8"

Module["Short"] = Short = "i16"
Module["Unsigned_Short"] = Unsigned_Short = "i16"
Module["int16_t"] = int16_t = "i16"
Module["uint16_t"] = uint16_t = "i16"

Module["Int"] = Int = "i32"
Module["Unsigned_Int"] = Unsigned_Int = "i32"
Module["int32_t"] = int32_t = "i32"
Module["uint32_t"] = uint32_t = "i32"

Module["Long"] = Long = "i64"
Module["Unsigned_Long"] = Unsigned_Long = "i64"
Module["Long_Long"] = Long_Long = "i64"
Module["Unsigned_Long_Long"] = Unsigned_Long_Long = "i64"
Module["int64_t"] = int64_t = "i64"
Module["uint64_t"] = uint64_t = "i64"

Module["Enumeration"] = Enumeration = "i32"


Module["HB_TAG"] = HB_TAG = (tag) -> tag.charCodeAt(0) << 24 | tag.charCodeAt(1) << 16 | tag.charCodeAt(2) << 8 | tag.charCodeAt(3)
Module["HB_UNTAG"] = HB_UNTAG = (tag) -> String.fromCharCode tag >>> 24, (tag >>> 16) & 0xFF, (tag >>> 8) & 0xFF, tag & 0xFF

# hb-common.cc

hb_language_impl_t = typedef "hb_language_impl_t", struct {
  "s": array(Char, 1);
}

#
# hb-common.h
#

Module["hb_bool_t"] = hb_bool_t = Int

Module["hb_codepoint_t"] = hb_codepoint_t = uint32_t
Module["hb_position_t"] = hb_position_t = int32_t
Module["hb_mask_t"] = hb_mask_t = uint32_t

# This should be a union, but we are never going to refer to it ourselves
Module["hb_var_int_t"] = hb_var_int_t = uint32_t

Module["hb_tag_t"] = hb_tag_t = uint32_t

Module["hb_direction_t"] = hb_direction_t = Enumeration

# hb_direction_t
Module["HB_DIRECTION_INVALID"] = HB_DIRECTION_INVALID = 0;
Module["HB_DIRECTION_LTR"] = HB_DIRECTION_LTR = 4;
Module["HB_DIRECTION_RTL"] = HB_DIRECTION_RTL = 5;
Module["HB_DIRECTION_TTB"] = HB_DIRECTION_TTB = 6;
Module["HB_DIRECTION_BTT"] = HB_DIRECTION_BTT = 7;

Module["hb_script_t"] = hb_script_t = Enumeration

hb_user_data_key_t = typedef "hb_user_data_key_t", struct {
	# < private >
	"unused": Char,
}

hb_destroy_func_t = ptr(Int) # void (*hb_destroy_func_t) (void *user_data)

hb_language_t = ptr(hb_language_impl_t);
Module["HB_LANGUAGE_INVALID"] = HB_LANGUAGE_INVALID = null

# len=-1 means str is NUL-terminated
define hb_language_t, "hb_language_from_string", "str": string, "len": Int
define string, "hb_language_to_string", "language": hb_language_t


#
# hb-atomic-private.hh
#

hb_atomic_int_t = Int


#
# hb-mutex-private.hh
#

hb_mutex_impl_t = Int

hb_mutex_t = typedef "hb_mutex_t", struct {
	"m": hb_mutex_impl_t,
}


#
# hb-private.hh
#

hb_prealloced_array_t = (Type, StaticSize) -> typedef "hb_prealloced_array_t<#{Type}, #{StaticSize}>", struct {
	"len": Unsigned_Int,
	"allocated": Unsigned_Int,
	"array": ptr(Type),
	"static_array": array(Type, StaticSize),
}

hb_lockable_set_t = (item_t, lock_t) -> typedef "hb_lockable_set_t<#{item_t}, #{lock_t}>", struct {
	"items": hb_prealloced_array_t(item_t, 2),
}


#
# hb-object-private.hh
#

hb_reference_count_t = typedef "hb_reference_count_t", struct {
	"ref_count": hb_atomic_int_t,
}

hb_user_data_item_t = typedef "hb_user_data_item_t", struct {
	"key": ptr(hb_user_data_key_t),
	"data": ptr(Int), # *void
	"destroy": ptr(Int), # *hb_destroy_func_t
}

hb_user_data_array_t = typedef "hb_user_data_array_t", struct {
	"items": hb_lockable_set_t(hb_user_data_item_t, hb_mutex_t),
}

hb_object_header_t = typedef "hb_object_header_t", struct {
	"ref_count": hb_reference_count_t,
	"mutex": hb_mutex_t,
	"user_data": hb_user_data_array_t,
}


#
# hb-unicode-private.hh
#

HB_UNICODE_CALLBACKS = [
	"combining_class",
	"eastasian_width",
	"general_category",
	"mirroring",
	"script",
	"compose",
	"decompose",
	"decompose_compatibility",
]

hb_unicode_funcs_t = typedef "hb_unicode_funcs_t", struct {
	"header": hb_object_header_t,
	"parent": SelfPtr, # *hb_unicode_funcs_t
	"immutable": Bool,
	"func": array(ptr("i32"), HB_UNICODE_CALLBACKS.length), # This is a struct of funtion pointers
	"user_data": array(ptr("i32"), HB_UNICODE_CALLBACKS.length), # This is a struct of void*
	"destroy": array(ptr("i32"), HB_UNICODE_CALLBACKS.length), # This is a struct of funtion pointers
}


#
# hb-unicode.hh
#

define hb_unicode_funcs_t, "hb_unicode_funcs_get_default"
define hb_unicode_funcs_t, "hb_unicode_funcs_reference", "ufuncs": hb_unicode_funcs_t


#
# hb-buffer.h
#

hb_glyph_info_t = typedef "hb_glyph_info_t", struct {
	"codepoint": hb_codepoint_t,
	"mask": hb_mask_t,
	"cluster": uint32_t,

	# < private >
	"var1": hb_var_int_t,
	"var2": hb_var_int_t,
}

hb_glyph_position_t = typedef "hb_glyph_position_t", struct {
	"x_advance": hb_position_t,
	"y_advance": hb_position_t,
	"x_offset": hb_position_t,
	"y_offset": hb_position_t,

	# < private >
	"var": hb_var_int_t,
}

hb_buffer_content_type_t = Enumeration

#
# hb-buffer-private.hh
#

hb_segment_properties_t = typedef "hb_segment_properties_t", struct {
	"direction": hb_direction_t,
	"script": hb_script_t,
	"language": hb_language_t,
}

hb_buffer_t = typedef "hb_buffer_t", struct {
	"header": hb_object_header_t,
	"unicode": ptr(hb_unicode_funcs_t),
	"props": hb_segment_properties_t,
	"content_type": hb_buffer_content_type_t,
	
	# These three bools are packed into one Int's space
	#"in_error": Bool,
	#"have_output": Bool,
	#"have_positions": Bool,
	#"__filler": Bool,
	"_status": Unsigned_Int,

	"idx": Unsigned_Int,
	"len": Unsigned_Int,
	"out_len": Unsigned_Int,

	"allocated": Unsigned_Int,
	"info": ptr(hb_glyph_info_t),
	"out_info": ptr(hb_glyph_info_t),
	"pos": ptr(hb_glyph_position_t),

	"serial": Unsigned_Int,
	"allocated_var_bytes": array(uint8_t, 8),
	"allocated_var_owner": array(ptr(Char), 8),
}

define hb_buffer_t, "hb_buffer_create"
define hb_buffer_t, "hb_buffer_reference", "buffer": hb_buffer_t
define Void, "hb_buffer_destroy", "buffer": hb_buffer_t
define Void, "hb_buffer_reset", "buffer": hb_buffer_t
define hb_buffer_t, "hb_buffer_get_empty"
define Void, "hb_buffer_set_content_type", "buffer": hb_buffer_t, "content_type": hb_buffer_content_type_t
define Int, "hb_buffer_get_content_type", "buffer": hb_buffer_t
define Unsigned_Int, "hb_buffer_get_length", "buffer": hb_buffer_t
define hb_glyph_info_t, "hb_buffer_get_glyph_infos", "buffer": hb_buffer_t, "length": ptr(Unsigned_Int)
define hb_glyph_position_t, "hb_buffer_get_glyph_positions", "buffer":hb_buffer_t, "length": ptr(Unsigned_Int)
define Void, "hb_buffer_normalize_glyphs", "buffer": hb_buffer_t

define Void, "hb_buffer_add", "buffer": hb_buffer_t, "codepoint": hb_codepoint_t, "mask": hb_mask_t, "cluster": Unsigned_Int
define Void, "hb_buffer_add_utf8", "buffer": hb_buffer_t, "text": string, "text_length": Int, "item_offset": Unsigned_Int, "item_length": Int
define Void, "hb_buffer_add_utf16", "buffer": hb_buffer_t, "text": ptr(uint16_t), "text_length": Int, "item_offset": Unsigned_Int, "item_length": Int
define Void, "hb_buffer_add_utf32", "buffer": hb_buffer_t, "text": ptr(uint32_t), "text_length": Int, "item_offset": Unsigned_Int, "item_length": Int

define Unsigned_Int, "hb_buffer_get_length", "buffer": hb_buffer_t

define Void, "hb_buffer_guess_segment_properties", "buffer": hb_buffer_t

define Void, "hb_buffer_set_direction", "buffer": hb_buffer_t, "direction": hb_direction_t
define hb_direction_t, "hb_buffer_get_direction", "buffer": hb_buffer_t

define Void, "hb_buffer_set_script", "buffer": hb_buffer_t, "script": hb_script_t
define hb_script_t, "hb_buffer_get_script", "buffer": hb_buffer_t

define Void, "hb_buffer_set_language", "buffer": hb_buffer_t, "language": hb_language_t
define hb_language_t, "hb_buffer_get_language", "buffer": hb_buffer_t


#
# hb-blob.h
#

hb_memory_mode_t = Enumeration


#
# hb-blob.cc
#

hb_blob_t = typedef "hb_blob_t", struct {
	"header": hb_object_header_t,
	
	"immutable": Bool,

	"data": ptr(Char),
	"length": Unsigned_Int,
	"mode": hb_memory_mode_t,

	"user_data": ptr(Void),
	"destroy": hb_destroy_func_t,
}

#
# hb-bloh.h again
#

define hb_blob_t, "hb_blob_create", "data": string, "length": Unsigned_Int, "mode": hb_memory_mode_t, "user_data": ptr(Void), "destroy": hb_destroy_func_t
define hb_blob_t, "hb_blob_create_sub_blob", "parent": hb_blob_t, "offset": Unsigned_Int, "length": Unsigned_Int
define hb_blob_t, "hb_blob_get_empty"
define hb_blob_t, "hb_blob_reference", "blob": hb_blob_t
define Void, "hb_blob_destroy", "blob": hb_blob_t


#
# hb-shaper-private.hh
#

HB_SHAPER_LIST = [
	# TODO Not sure what to include here for now?
	"ot",
	"fallback"
]

hb_shaper_data_t = typedef "hb_shaper_data_t", struct {
	"_shapers": array(ptr(Void), HB_SHAPER_LIST.length),
}


#
# hb-shape-plan-private.hh
#

hb_shape_func_t = ptr(Void) # function


#
# hb-shape.h
#

hb_feature_t = typedef "hb_feature_t", struct {
  "tag": hb_tag_t,
  "value": uint32_t,
  "start": Unsigned_Int,
  "end": Unsigned_Int,
}

hb_font_t = typedef "hb_font_t", struct {}


# len=-1 means str is NUL-terminated
define hb_bool_t, "hb_feature_from_string", "str": string, "len": Int, "feature": hb_feature_t
# something like 128 bytes is more than enough nul-terminates
define Void, "hb_feature_to_string", "feature": hb_feature_t, "buf": string, "size": Int

define ptr(ptr(Char)), "hb_shape_list_shapers"
define Void, "hb_shape", "font": hb_font_t, "buffer": hb_buffer_t, "features": hb_feature_t, "num_features": Unsigned_Int
define hb_bool_t, "hb_shape_full", "font": hb_font_t, "buffer": hb_buffer_t, "features": hb_feature_t, "num_features": Unsigned_Int, "shaper_list": ptr(ptr(Char))


#
# hb-font-private.hh
#

HB_FONT_CALLBACKS = [
	"glyph",
	"glyph_h_advance",
	"glyph_v_advance",
	"glyph_h_origin",
	"glyph_v_origin",
	"glyph_h_kerning",
	"glyph_v_kerning",
	"glyph_extents",
	"glyph_contour_point",
	"glyph_name",
	"glyph_from_name",
]

hb_reference_table_func_t = ptr(Void) # function

hb_font_funcs_t = typedef "hb_font_funcs_t", struct {
	"header": hb_object_header_t,

	"immutable": hb_bool_t,

	"get": array(ptr(Void), HB_FONT_CALLBACKS.length),
	"user_data": array(ptr(Void), HB_FONT_CALLBACKS.length),
	"destroy": array(ptr(Void), HB_FONT_CALLBACKS.length),
}

hb_face_t = typedef "hb_face_t", struct {
	"header": hb_object_header_t,

	"immutable": hb_bool_t,

	"reference_table_func": hb_reference_table_func_t,
	"user_data": ptr(Void),
	"destroy": hb_destroy_func_t,

	"index": Unsigned_Int,
	"upem": Unsigned_Int,

	"shaper_data": hb_shaper_data_t,

	"shape_plans": ptr(Void),
}

hb_font_t["redefine"] {
	"header": hb_object_header_t,

	"immutable": hb_bool_t,

	"parent": SelfPtr,
	"face": ptr(hb_face_t),

	"x_scale": Int,
	"y_scale": Int,

	"x_ppem": Unsigned_Int,
	"y_ppem": Unsigned_Int,

	"klass": ptr(hb_font_funcs_t),
	"user_data": ptr(Void),
	"destroy": hb_destroy_func_t,

	"shaper_data": hb_shaper_data_t,
}


#
# hb-font.h
#

define hb_face_t, "hb_face_create", "blob": hb_blob_t, "index": Unsigned_Int
define Void, "hb_face_destroy", "face": hb_face_t

define hb_font_t, "hb_font_create", "face": hb_face_t
define Void, "hb_font_destroy", "font": hb_font_t

define Void, "hb_font_set_scale", "font": hb_font_t, "x_scale": Int, "y_scale": Int
define hb_face_t, "hb_font_get_face", "font": hb_font_t

define hb_font_funcs_t, "hb_font_funcs_create"
for fontCallback in HB_FONT_CALLBACKS
	define Void, "hb_font_funcs_set_#{fontCallback}_func", "ffuncs": hb_font_funcs_t, "func": "i32", "user_data": ptr(Void), "destroy": hb_destroy_func_t
define Void, "hb_font_funcs_destroy", "ffuncs": hb_font_funcs_t

define Void, "hb_font_set_funcs", "font": hb_font_t, "klass": hb_font_funcs_t, "font_data": ptr(Void), "destroy": hb_destroy_func_t

#
# ucdn.h
#

define string, "ucdn_get_unicode_version"
define Int, "ucdn_get_combining_class", "code": uint32_t
define Int, "ucdn_get_east_asian_width", "code": uint32_t
define Int, "ucdn_get_general_category", "code": uint32_t
define Int, "ucdn_get_bidi_class", "code": uint32_t
define Int, "ucdn_get_script", "code": uint32_t
define Int, "ucdn_get_mirrored", "code": uint32_t
define Int, "ucdn_mirror", "code": uint32_t
define Int, "ucdn_decompose", "code": uint32_t, "a": ptr(uint32_t), "b": ptr(uint32_t)
define Int, "ucdn_compose", "code": ptr(uint32_t), "a": uint32_t, "b": uint32_t

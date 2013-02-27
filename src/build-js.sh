#!/bin/bash -x -e

VERSION=`git describe`
EMCC_VERSION=`emcc --version | head -n 1`

{
	echo "Module['version'] = '$VERSION';";
	echo "Module['emccVersion'] = '$EMCC_VERSION';";
	cat "pre.js.in";
} > pre.js

make
coffee -j post.js -cb js-connector

EXPORTED_FUNCTIONS="['ccall', 'cwrap', '_hb_language_from_string', '_hb_language_to_string', '_hb_unicode_funcs_get_default', '_hb_unicode_funcs_reference', '_hb_buffer_create', '_hb_buffer_reference', '_hb_buffer_destroy', '_hb_buffer_reset', '_hb_buffer_get_empty', '_hb_buffer_set_content_type', '_hb_buffer_get_content_type', '_hb_buffer_get_length', '_hb_buffer_get_glyph_infos', '_hb_buffer_get_glyph_positions', '_hb_buffer_normalize_glyphs', '_hb_buffer_add', '_hb_buffer_add_utf8', '_hb_buffer_add_utf16', '_hb_buffer_add_utf32', '_hb_buffer_get_length', '_hb_buffer_guess_segment_properties', '_hb_buffer_set_direction', '_hb_buffer_get_direction', '_hb_buffer_set_script', '_hb_buffer_get_script', '_hb_buffer_set_language', '_hb_buffer_get_language', '_hb_blob_create', '_hb_blob_create_sub_blob', '_hb_blob_get_empty', '_hb_blob_reference', '_hb_blob_destroy', '_hb_feature_from_string', '_hb_feature_to_string', '_hb_shape_list_shapers', '_hb_shape', '_hb_shape_full', '_hb_face_create', '_hb_face_destroy', '_hb_font_create', '_hb_font_destroy', '_hb_font_set_scale', '_hb_font_get_face', '_hb_font_funcs_create', '_hb_font_funcs_destroy', '_hb_font_set_funcs', '_hb_font_funcs_set_glyph_func', '_hb_font_funcs_set_glyph_h_advance_func', '_hb_font_funcs_set_glyph_v_advance_func', '_hb_font_funcs_set_glyph_h_origin_func', '_hb_font_funcs_set_glyph_v_origin_func', '_hb_font_funcs_set_glyph_h_kerning_func', '_hb_font_funcs_set_glyph_v_kerning_func', '_hb_font_funcs_set_glyph_extents_func', '_hb_font_funcs_set_glyph_contour_point_func', '_hb_font_funcs_set_glyph_name_func', '_hb_font_funcs_set_glyph_from_name_func']"

echo "Building debug version $VERSION"
time emcc .libs/libharfbuzz.dylib hb-ucdn/.libs/ucdn.o --post-js post.js -o harfbuzz-unoptimized.js -O2 --closure 0 -s "ALLOW_MEMORY_GROWTH=1" -s "EXPORTED_FUNCTIONS=$EXPORTED_FUNCTIONS"

echo "Building debug version $VERSION, no typed arrays"
time emcc .libs/libharfbuzz.dylib hb-ucdn/.libs/ucdn.o --post-js post.js -o harfbuzz-untyped-unoptimized.js -O2 --closure 0 -s "USE_TYPED_ARRAYS=0" -s "ALLOW_MEMORY_GROWTH=1" -s "EXPORTED_FUNCTIONS=$EXPORTED_FUNCTIONS"

echo "Building production version $VERSION with full optimization"
time emcc .libs/libharfbuzz.dylib hb-ucdn/.libs/ucdn.o --post-js post.js -o harfbuzz.js -O2 --closure 1 -s "ALLOW_MEMORY_GROWTH=1" -s "EXPORTED_FUNCTIONS=$EXPORTED_FUNCTIONS"

echo "Building production version $VERSION with full optimization, no typed arrays"
time emcc .libs/libharfbuzz.dylib hb-ucdn/.libs/ucdn.o --post-js post.js -o harfbuzz-untyped.js -s "USE_TYPED_ARRAYS=0" -O2 --closure 1 -s "ALLOW_MEMORY_GROWTH=1" -s "EXPORTED_FUNCTIONS=$EXPORTED_FUNCTIONS"

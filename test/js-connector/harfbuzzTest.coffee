chai = require "chai"
should = chai.should()
expect = chai.expect
chai.Assertion.includeStack = true

harfbuzz = require "../../build/harfbuzz-test"
fs = require "fs"

using = (buffers..., func) ->
	func()
	for buffer in buffers
		harfbuzz.hb_buffer_destroy buffer
	return

describe "Buffer", ->
	buffer = null

	it "should be created via hb_buffer_create()", ->
		buffer = harfbuzz.hb_buffer_create()
		# console.log "Buffer: #{buffer.$ptr}\n\n#{buffer}"
		buffer.get("header").get("ref_count").get("ref_count").should.equal 1

	it "should add a reference via hb_buffer_reference()", ->
		# console.log "==================== #{buffer}"
		buffer = harfbuzz.hb_buffer_reference buffer
		# console.log "Buffer: #{buffer.$ptr}\n\n#{buffer}"
		buffer.get("header").get("ref_count").get("ref_count").should.equal 2

	it "should remove a reference via hb_buffer_destroy()", ->
		harfbuzz.hb_buffer_destroy buffer
		buffer.get("header").get("ref_count").get("ref_count").should.equal 1

	it "should completely destroy itself via another call to hb_buffer_destroy()", ->
		harfbuzz.hb_buffer_destroy buffer

	it "should allow further calls to hb_buffer_destroy() once destroyed", ->
		harfbuzz.hb_buffer_destroy buffer
		harfbuzz.hb_buffer_destroy buffer


describe "Adding to buffer", ->
	it "should work", ->
		using buffer = harfbuzz.hb_buffer_create(), ->
			harfbuzz.hb_buffer_get_length(buffer).should.equal 0
			harfbuzz.hb_buffer_add buffer, 65, 1, 0
			harfbuzz.hb_buffer_get_length(buffer).should.equal 1


describe "Empty buffer", ->
	buffer = null

	it "should be available", ->
		buffer = harfbuzz.hb_buffer_get_empty()
		buffer.get("header").get("ref_count").get("ref_count").should.equal -1
		# console.log "Buffer #{buffer.$ptr}:\n#{buffer}"


glyph_h_advance_func = harfbuzz.callback harfbuzz.hb_position_t, "glyph_h_advance_func", {
		font: harfbuzz.ptr(harfbuzz.hb_font_t),
		font_data: harfbuzz.ptr(harfbuzz.Void),
		glyph: harfbuzz.hb_codepoint_t,
		user_data: harfbuzz.ptr(harfbuzz.Void)
	}, (font, font_data, glyph, user_data) ->
		hAdvance = switch glyph
			when 0x50 then 10
			when 0x69 then 6
			when 0x6E then 5
			else 9

		console.log "Advance for #{glyph} is #{hAdvance}"

		return hAdvance

glyph_h_kerning_func = harfbuzz.callback harfbuzz.hb_position_t, "glyph_h_kerning_func", {
		font: harfbuzz.ptr(harfbuzz.hb_font_t),
		font_data: harfbuzz.ptr(harfbuzz.Void),
		left: harfbuzz.hb_codepoint_t,
		right: harfbuzz.hb_codepoint_t,
		user_data: harfbuzz.ptr(harfbuzz.Void)
	}, (font, font_data, left, right, user_data) ->
		return 0

glyph_func = harfbuzz.callback harfbuzz.Bool, "glyph_func", {
		font: harfbuzz.ptr(harfbuzz.hb_font_t),
		font_data: harfbuzz.ptr(harfbuzz.Void),
		unicode: harfbuzz.hb_codepoint_t,
		variant_selector: harfbuzz.hb_codepoint_t,
		glyph: harfbuzz.ptr(harfbuzz.hb_codepoint_t),
		user_data: harfbuzz.ptr(harfbuzz.Void)
	}, (font, font_data, unicode, variant_selector, glyph, user_data) ->
		harfbuzz.setValue glyph.address, unicode, "i32"
		console.log "Called with unicode: #{unicode}, glyph is #{glyph.get()}"

		return true

describe "Feature", ->
	it "should be converted to string", ->
		feature = new harfbuzz.hb_feature_t()
		feature.set "tag", harfbuzz.HB_TAG "kern"
		feature.set "value", 0
		feature.set "start", 0
		feature.set "end", 0

		bufPtr = new (harfbuzz.ptr(harfbuzz.Char))()
		bufPtr.address = harfbuzz.allocate 128, "i8", harfbuzz.ALLOC_STACK
		harfbuzz.hb_feature_to_string feature, bufPtr, 128
		console.log "Feature: " + bufPtr

	it "should be creatable from string", ->
		feature = new harfbuzz.hb_feature_t()
		bufPtr = new (harfbuzz.ptr(harfbuzz.Char))()
		bufPtr.address = harfbuzz.allocate harfbuzz.intArrayFromString("kern"), "i8", harfbuzz.ALLOC_STACK
		harfbuzz.hb_feature_from_string bufPtr, 128, feature
		console.log "Feature: " + feature

	it "should be creatable from string (2)", ->
		feature = new harfbuzz.hb_feature_t()
		bufPtr = new harfbuzz.string("kern")
		console.log "Buffer's heap: #{bufPtr.$ptr}, #{bufPtr.$type}, #{typeof bufPtr.$type}"
		harfbuzz.hb_feature_from_string bufPtr, -1, feature
		console.log "Feature: " + feature

describe "Loading a blob", ->
	it "should work", ->
		dataBuffer = fs.readFileSync("test/js-connector/OpenBaskerville-0.0.75.otf", "binary")
		data = harfbuzz.allocate dataBuffer, "i8", harfbuzz.ALLOC_NORMAL
		for i in [0...dataBuffer.length]
			harfbuzz.setValue data + i, dataBuffer[i], "i8"

		blob = harfbuzz.hb_blob_create data, dataBuffer.length, 1, null, null

		face = harfbuzz.hb_face_create blob, 0
		# console.log "Face: #{face}"
		harfbuzz.hb_blob_destroy blob

		font = harfbuzz.hb_font_create face
		# console.log "Face: #{font.face}"
		harfbuzz.hb_face_destroy face
		harfbuzz.hb_font_set_scale font, 10, 10
		# console.log "Font: #{font}"

		ffuncs = harfbuzz.hb_font_funcs_create();
		harfbuzz.hb_font_funcs_set_glyph_h_advance_func ffuncs, glyph_h_advance_func, null, null
		harfbuzz.hb_font_funcs_set_glyph_func ffuncs, glyph_func, null, null
		harfbuzz.hb_font_funcs_set_glyph_h_kerning_func ffuncs, glyph_h_kerning_func, null, null
		harfbuzz.hb_font_set_funcs font, ffuncs, null, null
		# console.log "FFuncs: #{ffuncs}"
		harfbuzz.hb_font_funcs_destroy ffuncs

		buffer = harfbuzz.hb_buffer_create()
		#harfbuzz.hb_buffer_set_content_type buffer, 1 # HB_BUFFER_CONTENT_TYPE_UNICODE
		#harfbuzz.hb_buffer_set_direction buffer, 4 # HB_DIRECTION_LTR

		str = new harfbuzz.string "Bacon", harfbuzz.ALLOC_STACK
		harfbuzz.hb_buffer_add_utf8 buffer, str, -1, 0, -1
		harfbuzz.hb_buffer_guess_segment_properties buffer

		harfbuzz.hb_shape font, buffer, null, 0, null
		len = harfbuzz.hb_buffer_get_length buffer
		console.log "Buffer length: #{len}"

		lenPtr = new (harfbuzz.ptr(harfbuzz.Void))()

		glyph = harfbuzz.hb_buffer_get_glyph_infos buffer, lenPtr
		pos = harfbuzz.hb_buffer_get_glyph_positions buffer, null
		console.log "Ptr'd len: #{lenPtr.get()}"
		for i in [0...len]
			console.log "Glyph ##{i}: #{glyph} at #{pos}"
			glyph = glyph.$next()
			pos = pos.$next()

		harfbuzz.hb_buffer_destroy (buffer)
		harfbuzz.hb_font_destroy (font)


###
describe "Shapers", ->
	it "should be available: ot, fallback", ->
		shapers = harfbuzz.hb_shape_list_shapers()
		console.log "Shapers: #{shapers}"
		console.log "Shaper: #{shapers.toString()}"
		shapers.toString().should.equal "ot"
		shapers.$next().toString().should.equal "fallback"
		shapers.$next().$next().address.should.equal 0


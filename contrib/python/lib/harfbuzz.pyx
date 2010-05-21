cdef extern from "stdlib.h":
    ctypedef int size_t
    void *malloc(size_t size)
    void free(void* ptr)

cdef extern from "ft2build.h" :
    pass

cdef extern from "freetype/freetype.h" :
    ctypedef void *FT_Library
    ctypedef void *FT_Face
    ctypedef int FT_Error

    FT_Error FT_Init_FreeType (FT_Library *alib)
    FT_Error FT_Done_FreeType (FT_Library alib)
    FT_Error FT_Done_Face (FT_Face alib)
    FT_Error FT_New_Face( FT_Library library, char *path, unsigned long index, FT_Face *aface)
    FT_Error FT_Set_Char_Size (FT_Face aFace, unsigned int size_x, unsigned int size_y, unsigned int res_x, unsigned int res_y)

cdef extern from "hb-common.h" :
    cdef enum hb_direction_t :
        HB_DIRECTION_LTR
        HB_DIRECTION_RTL
        HB_DIRECTION_TTB
        HB_DIRECTION_BTT
    ctypedef unsigned long hb_codepoint_t
    ctypedef long hb_position_t
    ctypedef unsigned long hb_mask_t
    ctypedef unsigned long hb_tag_t
    hb_tag_t hb_tag_from_string (char *s)
    ctypedef void (*hb_destroy_func_t) (void *user_data)

cdef extern from "hb-unicode.h" :
# there must be a better way of syncing this list with the true source
    ctypedef enum hb_script_t :
        HB_SCRIPT_INVALID_CODE = -1,
        HB_SCRIPT_COMMON = 0,
        HB_SCRIPT_INHERITED,
        HB_SCRIPT_ARABIC,
        HB_SCRIPT_ARMENIAN,
        HB_SCRIPT_BENGALI,
        HB_SCRIPT_BOPOMOFO,
        HB_SCRIPT_CHEROKEE,
        HB_SCRIPT_DESERET,
        HB_SCRIPT_DEVANAGARI,
        HB_SCRIPT_ETHIOPIC,
        HB_SCRIPT_GEORGIAN,
        HB_SCRIPT_GOTHIC,
        HB_SCRIPT_GREEK,
        HB_SCRIPT_GUJARATI,
        HB_SCRIPT_GURMUKHI,
        HB_SCRIPT_HAN,
        HB_SCRIPT_HANGUL,
        HB_SCRIPT_HEBREW,
        HB_SCRIPT_HIRAGANA,
        HB_SCRIPT_KANNADA,
        HB_SCRIPT_KATAKANA,
        HB_SCRIPT_KHMER,
        HB_SCRIPT_LAO,
        HB_SCRIPT_LATIN,
        HB_SCRIPT_MALAYALAM,
        HB_SCRIPT_MONGOLIAN,
        HB_SCRIPT_MYANMAR,
        HB_SCRIPT_OGHAM,
        HB_SCRIPT_OLD_ITALIC,
        HB_SCRIPT_ORIYA,
        HB_SCRIPT_RUNIC,
        HB_SCRIPT_SINHALA,
        HB_SCRIPT_SYRIAC,
        HB_SCRIPT_TAMIL,
        HB_SCRIPT_TELUGU,
        HB_SCRIPT_THAANA,
        HB_SCRIPT_THAI,
        HB_SCRIPT_TIBETAN,
        HB_SCRIPT_CANADIAN_ABORIGINAL,
        HB_SCRIPT_YI,
        HB_SCRIPT_TAGALOG,
        HB_SCRIPT_HANUNDO,
        HB_SCRIPT_BUHID,
        HB_SCRIPT_TAGBANWA,
# Unicode 4.0
        HB_SCRIPT_BRAILLE,
        HB_SCRIPT_CYPRIOT,
        HB_SCRIPT_LIMBU,
        HB_SCRIPT_OSMANYA,
        HB_SCRIPT_SHAVIAN,
        HB_SCRIPT_LINEAR_B,
        HB_SCRIPT_TAI_LE,
        HB_SCRIPT_UGARITIC,
# Unicode 4.1
        HB_SCRIPT_NEW_TAI_LUE,
        HB_SCRIPT_BUGINESE,
        HB_SCRIPT_GLAGOLITIC,
        HB_SCRIPT_TIFINAGH,
        HB_SCRIPT_SYLOTI_NAGRI,
        HB_SCRIPT_OLD_PERSIAN,
        HB_SCRIPT_KHAROSHTHI,
# Unicode 5.0
        HB_SCRIPT_UNKNOWN,
        HB_SCRIPT_BALINESE,
        HB_SCRIPT_CUNEIFORM,
        HB_SCRIPT_PHOENICIAN,
        HB_SCRIPT_PHAGS_PA,
        HB_SCRIPT_NKO,
# Unicode 5.1
        HB_SCRIPT_KAYAH_LI,
        HB_SCRIPT_LEPCHA,
        HB_SCRIPT_REJANG,
        HB_SCRIPT_SUNDANESE,
        HB_SCRIPT_SAURASHTRA,
        HB_SCRIPT_CHAM,
        HB_SCRIPT_OL_CHIKI,
        HB_SCRIPT_VAI,
        HB_SCRIPT_CARIAN,
        HB_SCRIPT_LYCIAN,
        HB_SCRIPT_LYDIAN
# Unicode-5.2
        HB_SCRIPT_AVESTAN
        HB_SCRIPT_BAMUM
        HB_SCRIPT_EGYPTIAN_HIEROGLYPHS
        HB_SCRIPT_IMPERIAL_ARAMAIC
        HB_SCRIPT_INSCRIPTIONAL_PAHLAVI
        HB_SCRIPT_INSCRIPTIONAL_PARTHIAN
        HB_SCRIPT_JAVANESE
        HB_SCRIPT_KAITHI
        HB_SCRIPT_LISU
        HB_SCRIPT_MEITEI_MAYEK
        HB_SCRIPT_OLD_SOUTH_ARABIAN
        HB_SCRIPT_OLD_TURKIC
        HB_SCRIPT_SAMARITAN
        HB_SCRIPT_TAI_THAM
        HB_SCRIPT_TAI_VIET

cdef extern from "hb-language.h" :
    ctypedef void *hb_language_t
    hb_language_t hb_language_from_string(char *str)
    char * hb_language_to_string(hb_language_t language)

cdef extern from "hb-ot-tag.h" :
    hb_script_t hb_ot_tag_to_script (char *sname)

cdef extern from "hb-buffer.h" :
    ctypedef struct hb_buffer_t :
        pass

    ctypedef struct hb_glyph_info_t :
        hb_codepoint_t codepoint
        hb_mask_t mask
        unsigned long cluster

    ctypedef struct hb_glyph_position_t :
        hb_position_t x_advance
        hb_position_t y_advance
        hb_position_t x_offset
        hb_position_t y_offset
        unsigned long internal

    hb_buffer_t *hb_buffer_create(unsigned int size)
    hb_buffer_t *hb_buffer_reference(hb_buffer_t *buffer)
    unsigned int hb_buffer_get_reference_count(hb_buffer_t *buffer)
    void hb_buffer_destroy(hb_buffer_t *buffer)
    void hb_buffer_set_direction(hb_buffer_t *buffer, hb_direction_t direction)
    hb_direction_t hb_buffer_get_direction(hb_buffer_t *buffer)
    void hb_buffer_set_script(hb_buffer_t *buffer, hb_script_t script)
    hb_script_t hb_buffer_get_script(hb_buffer_t *buffer)
    void hb_buffer_set_language(hb_buffer_t *buffer, hb_language_t language)
    hb_language_t hb_buffer_get_language(hb_buffer_t *buffer)
    void hb_buffer_clear(hb_buffer_t *)
    void hb_buffer_clear_positions(hb_buffer_t *buffer)
    void hb_buffer_ensure(hb_buffer_t *buffer, unsigned int size)
    void hb_buffer_reverse(hb_buffer_t *buffer)
    void hb_buffer_reverse_clusters(hb_buffer_t *buffer)
    void hb_buffer_add_glyph(hb_buffer_t *buffer, hb_codepoint_t codepoint, hb_mask_t mask, unsigned int cluster)
    void hb_buffer_add_utf8(hb_buffer_t *buffer, char *text, unsigned int text_length, unsigned int item_offset, unsigned int item_length)
    unsigned int hb_buffer_get_length(hb_buffer_t *buffer)
    hb_glyph_info_t *hb_buffer_get_glyph_infos(hb_buffer_t *buffer)
    hb_glyph_position_t *hb_buffer_get_glyph_positions(hb_buffer_t *buffer)

cdef extern from "hb-blob.h" :
    cdef struct hb_blob_t :
        pass
# do I need blob functions here?

cdef extern from "hb-font.h" :
    ctypedef struct hb_face_t :
        pass
    ctypedef struct hb_font_t :
        pass

    ctypedef hb_blob_t * (*hb_get_table_func_t) (hb_tag_t tag, void *user_data)
    hb_face_t * hb_face_create_for_data(hb_blob_t *blob, unsigned int index)
    hb_face_t * hb_face_create_for_tables(hb_get_table_func_t get_table, hb_destroy_func_t destroy, void *user_data)
    hb_face_t * hb_face_reference(hb_face_t *face)
    unsigned int hb_face_get_reference_count(hb_face_t *face)
    void hb_face_destroy(hb_face_t *face)
    void hb_font_destroy(hb_font_t *font)
    hb_blob_t * hb_face_get_table(hb_face_t *face, hb_tag_t tag)


cdef extern from "hb-shape.h" :
    ctypedef struct hb_feature_t :
        int tag
        unsigned int value
        unsigned int start
        unsigned int end

    void hb_shape (hb_font_t *font, hb_face_t *face, hb_buffer_t *buffer, hb_feature_t *features, unsigned int num_features)

cdef extern from "hb-ft.h" :
    hb_face_t *hb_ft_face_create (FT_Face ft_face, hb_destroy_func_t destroy)
    hb_font_t *hb_ft_font_create (FT_Face ft_face, hb_destroy_func_t destroy)

class glyphinfo :
 
    def __init__(self, gid, cluster, advance, offset, internal = 0) :
        self.gid = gid
        self.cluster = cluster
        self.advance = advance
        self.offset = offset
        self.internal = internal

    def __repr__(self) :
        res = "{0:d}>{1:d}@({2:.2f},{3:.2f})+({4:.2f},{5:.2f})".format(self.gid, self.cluster, self.offset[0], self.offset[1], self.advance[0], self.advance[1])
        if self.internal : res += "/i=" + str(self.internal)
        return res

cdef class buffer :
    cdef hb_buffer_t *buffer

    def __init__(self, char *text, unsigned int length) :
        """Note text must be a utf-8 string and length is number of chars"""
        self.buffer = hb_buffer_create(length)
        hb_buffer_add_utf8(self.buffer, text, length, 0, len(text))

    def set_scriptlang(self, char *script, char *lang) :
        cdef hb_language_t language
        cdef hb_script_t scriptnum

        language = hb_language_from_string(lang)
        scriptnum = hb_ot_string_to_script(script)
        hb_buffer_set_script(self.buffer, scriptnum)
        hb_buffer_set_language(self.buffer, language)

    def get_info(self, scale = 1) :
        cdef hb_glyph_info_t *infos
        cdef hb_glyph_position_t *positions
        cdef unsigned int num
        cdef unsigned int i
        res = []

        num = hb_buffer_get_length(self.buffer)
        infos = hb_buffer_get_glyph_infos(self.buffer)
        positions = hb_buffer_get_glyph_positions(self.buffer)
        for 0 <= i < num :
            temp = glyphinfo(infos[i].codepoint, infos[i].cluster, (positions[i].x_advance / scale, positions[i].y_advance / scale), (positions[i].x_offset / scale, positions[i].y_offset / scale), positions[i].internal)
            res.append(temp)
        return res

    def get_settings(self) :
        cdef hb_script_t script
        cdef hb_language_t lang

        script = hb_buffer_get_script(self.buffer)
        lang = hb_buffer_get_language(self.buffer)
        return {'script' : script, 'language' : hb_language_to_string(lang)}

    def __del__(self) :
        hb_buffer_destroy(self.buffer)

cdef class ft :
    cdef FT_Library engine
    cdef FT_Face face
    cdef hb_face_t *hbface
    cdef hb_font_t *hbfont

    def __init__(self, char *fname, size) :
        cdef FT_Library engine
        FT_Init_FreeType(&engine)
        self.engine = engine
        cdef FT_Face face
        FT_New_Face(engine, fname, 0, &face)
        FT_Set_Char_Size(face, size << 6, size << 6, 144, 144)
        self.face = face
        self.hbface = hb_ft_face_create(face, <void (*)(void *)>hb_face_destroy)
        self.hbfont = hb_ft_font_create(face, <void (*)(void *)>hb_font_destroy)

    def __del__(self) :
        cdef FT_Library engine
        engine = self.engine
        cdef FT_Face face
        face = self.face
        FT_Done_Face(face)
        FT_Done_FreeType(engine)

    def shape(self, buffer aBuffer, features = {}) :
        cdef hb_feature_t *feats
        cdef hb_feature_t *aFeat
        feats = <hb_feature_t *>malloc(sizeof(hb_feature_t) * len(features))
        aFeat = feats
        for k,v in features.items() :
            aFeat.tag = hb_tag_from_string (k)
            aFeat.value = int(v)
            aFeat.start = 0
            aFeat.end = -1
            aFeat += 1
        hb_shape(self.hbfont, self.hbface, aBuffer.buffer, feats, len(features))




#include <graphite2/Segment.h>
#include <stdio.h>
#include <stdlib.h>
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

const void *getTable(const void *appHandle, unsigned int name, size_t *len)
{
    void *res;
    FT_Face ftface = (FT_Face)appHandle;
    FT_ULong ftlen = 0;
    FT_Load_Sfnt_Table(ftface, name, 0, NULL, &ftlen);     /* find length of table */
    if (!ftlen) return NULL;
    res = malloc(ftlen);                                 /* allocate somewhere to hold it */
    if (!res) return NULL;
    FT_Load_Sfnt_Table(ftface, name, 0, res, &ftlen);      /* copy table into buffer */
    *len = ftlen;
    return res;
}

void releaseTable(const void *appHandle, const void *ptr)
{
    free((void *)ptr);          /* simply free the allocated memory */              /*<1>*/
}

float getAdvance(const void *appFont, unsigned short glyphid)
{
    FT_Face ftface = (FT_Face)appFont;
    if (FT_Load_Glyph(ftface, glyphid, FT_LOAD_DEFAULT)) return -1.;  /* grid fit glyph */
    return ftface->glyph->advance.x;                    /* return grid fit advance */
}

/* usage: ./freetype fontfile.ttf string */
int main(int argc, char **argv)
{
    int rtl = 0;                /* are we rendering right to left? probably not */
    int pointsize = 12;         /* point size in points */
    int dpi = 96;               /* work with this many dots per inch */

    char *pError;               /* location of faulty utf-8 */
    gr_font *font = NULL;
    size_t numCodePoints = 0;
    gr_segment * seg = NULL;
    const gr_slot *s;
    gr_face *face;
    FT_Library ftlib;
    FT_Face ftface;
    gr_face_ops faceops = {sizeof(gr_face_ops), &getTable, &releaseTable};          /*<2>*/
    gr_font_ops fontops = {sizeof(gr_font_ops), &getAdvance, NULL};
    /* Set up freetype font face at given point size */
    if (FT_Init_FreeType(&ftlib)) return -1;
    if (FT_New_Face(ftlib, argv[1], 0, &ftface)) return -2;
    if (FT_Set_Char_Size(ftface, pointsize << 6, pointsize << 6, dpi, dpi)) return -3;

    face = gr_make_face_with_ops(ftface, &faceops, gr_face_preloadAll);             /*<3>*/
    if (!face) return 1;
    font = gr_make_font_with_ops(pointsize * dpi / 72.0f, ftface, &fontops, face);  /*<4>*/
    if (!font) return 2;
    numCodePoints = gr_count_unicode_characters(gr_utf8, argv[2], NULL,
                (const void **)(&pError));
    if (pError) return 3;
    seg = gr_make_seg(font, face, 0, 0, gr_utf8, argv[2], numCodePoints, rtl);
    if (!seg) return 3;

    for (s = gr_seg_first_slot(seg); s; s = gr_slot_next_in_segment(s))
        printf("%d(%f,%f) ", gr_slot_gid(s), gr_slot_origin_X(s) / 64,
                             gr_slot_origin_Y(s) / 64);                             /*<5>*/
    gr_seg_destroy(seg);
    gr_font_destroy(font);
    gr_face_destroy(face);
    /* Release freetype face and library handle */
    FT_Done_Face(ftface);
    FT_Done_FreeType(ftlib);
    return 0;
}

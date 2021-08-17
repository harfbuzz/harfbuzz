
#include <graphite2/Segment.h>
#include <stdio.h>
#include <stdlib.h>

/* calculate the breakweight between two slots */
int breakweight_before(const gr_slot *s, const gr_segment *seg)
{
    int bbefore = gr_cinfo_break_weight(gr_seg_cinfo(seg, gr_slot_after(gr_slot_prev_in_segment(s))));
    int bafter = gr_cinfo_break_weight(gr_seg_cinfo(seg, gr_slot_before(s)));

    if (!gr_slot_can_insert_before(s))
        return 50;
    if (bbefore < 0) bbefore = 0;
    if (bafter > 0) bafter = 0;
    return (bbefore > bafter) ? bbefore : bafter;
}

/* usage: ./linebreak fontfile.ttf width string */
int main(int argc, char **argv)
{
    int rtl = 0;                /* are we rendering right to left? probably not */
    int pointsize = 12;         /* point size in points */
    int dpi = 96;               /* work with this many dots per inch */
    int width = atoi(argv[2]) * dpi / 72;  /* linewidth in points */

    char *pError;               /* location of faulty utf-8 */
    gr_font *font = NULL;
    size_t numCodePoints = 0;
    gr_segment * seg = NULL;
    const gr_slot *s, *sprev;
    int i;
    float lineend = (float)width;
    int numlines = 0;
    const gr_slot **lineslots;
    gr_face *face = gr_make_file_face(argv[1], 0);
    if (!face) return 1;
    font = gr_make_font(pointsize * dpi / 72.0f, face);
    if (!font) return 2;
    numCodePoints = gr_count_unicode_characters(gr_utf8, argv[3], NULL,
                (const void **)(&pError));
    if (pError) return 3;
    seg = gr_make_seg(font, face, 0, 0, gr_utf8, argv[3], numCodePoints, rtl);  /*<1>*/
    if (!seg) return 3;

    lineslots = (const gr_slot **)malloc(numCodePoints * sizeof(gr_slot *));
    lineslots[numlines++] = gr_seg_first_slot(seg);                             /*<2>*/
    for (s = lineslots[0]; s; s = gr_slot_next_in_segment(s))                   /*<3>*/
    {
        sprev = NULL;
        if (gr_slot_origin_X(s) > lineend)                                      /*<4>*/
        {
            while (s)
            {
                if (breakweight_before(s, seg) >= gr_breakWord)                 /*<5>*/
                    break;
                s = gr_slot_prev_in_segment(s);                                 /*<6>*/
            }
            lineslots[numlines++] = s;
            gr_slot_linebreak_before((gr_slot *)s);                             /*<7>*/
            lineend = gr_slot_origin_X(s) + width;                              /*<8>*/
        }
    }

    printf("%d:", width);
    for (i = 0; i < numlines; i++)
    {
        gr_seg_justify(seg, (gr_slot *)lineslots[i], font, width, 0, NULL, NULL); /*<9>*/
        for (s = lineslots[i]; s; s = gr_slot_next_in_segment(s))               /*<10>*/
            printf("%d(%.2f,%.2f@%d) ", gr_slot_gid(s), gr_slot_origin_X(s), gr_slot_origin_Y(s), gr_slot_attr(s, seg, gr_slatJWidth, 0));
        printf("\n");
    }
    free((void*)lineslots);
    gr_seg_destroy(seg);
    gr_font_destroy(font);
    gr_face_destroy(face);
    return 0;
}

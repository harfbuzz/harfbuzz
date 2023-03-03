import gi

from collections import namedtuple

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, HarfBuzz as hb


POOL = {}


def move_to_f(funcs, draw_data, st, to_x, to_y, user_data):
    context = POOL[draw_data]
    context.move_to(to_x, to_y)


def line_to_f(funcs, draw_data, st, to_x, to_y, user_data):
    context = POOL[draw_data]
    context.line_to(to_x, to_y)


def cubic_to_f(
    funcs,
    draw_data,
    st,
    control1_x,
    control1_y,
    control2_x,
    control2_y,
    to_x,
    to_y,
    user_data,
):
    context = POOL[draw_data]
    context.curve_to(control1_x, control1_y, control2_x, control2_y, to_x, to_y)


def close_path_f(funcs, draw_data, st, user_data):
    context = POOL[draw_data]
    context.close_path()


DFUNCS = hb.draw_funcs_create()
hb.draw_funcs_set_move_to_func(DFUNCS, move_to_f, None)
hb.draw_funcs_set_line_to_func(DFUNCS, line_to_f, None)
hb.draw_funcs_set_cubic_to_func(DFUNCS, cubic_to_f, None)
hb.draw_funcs_set_close_path_func(DFUNCS, close_path_f, None)


def push_transform_f(funcs, paint_data, xx, yx, xy, yy, dx, dy, user_data):
    raise NotImplementedError


def pop_transform_f(funcs, paint_data, user_data):
    raise NotImplementedError


def color_f(funcs, paint_data, is_foreground, color, user_data):
    context = POOL[paint_data]
    r = hb.color_get_red(color) / 255
    g = hb.color_get_green(color) / 255
    b = hb.color_get_blue(color) / 255
    a = hb.color_get_alpha(color) / 255
    context.set_source_rgba(r, g, b, a)
    context.paint()


def push_clip_rectangle_f(funcs, paint_data, xmin, ymin, xmax, ymax, user_data):
    context = POOL[paint_data]
    context.save()
    context.rectangle(xmin, ymin, xmax, ymax)
    context.clip()


def push_clip_glyph_f(funcs, paint_data, glyph, font, user_data):
    context = POOL[paint_data]
    context.save()
    context.new_path()
    hb.font_draw_glyph(font, glyph, DFUNCS, paint_data)
    context.close_path()
    context.clip()


def pop_clip_f(funcs, paint_data, user_data):
    context = POOL[paint_data]
    context.restore()


def push_group_f(funcs, paint_data, user_data):
    raise NotImplementedError


def pop_group_f(funcs, paint_data, mode, user_data):
    raise NotImplementedError


PFUNCS = hb.paint_funcs_create()
hb.paint_funcs_set_push_transform_func(PFUNCS, push_transform_f, None)
hb.paint_funcs_set_pop_transform_func(PFUNCS, pop_transform_f, None)
hb.paint_funcs_set_color_func(PFUNCS, color_f, None)
hb.paint_funcs_set_push_clip_glyph_func(PFUNCS, push_clip_glyph_f, None)
hb.paint_funcs_set_push_clip_rectangle_func(PFUNCS, push_clip_rectangle_f, None)
hb.paint_funcs_set_pop_clip_func(PFUNCS, pop_clip_f, None)
hb.paint_funcs_set_push_group_func(PFUNCS, push_group_f, None)
hb.paint_funcs_set_pop_group_func(PFUNCS, pop_group_f, None)


class Word:
    def __init__(self, font, text):
        self._text = text
        self._font = font
        self._glyphs = []
        self._positions = []

    def append(self, info, pos):
        self._glyphs.append(info)
        self._positions.append(pos)

    def draw(self, context, font):
        for info, pos in zip(self._glyphs, self._positions):
            context.translate(-pos.x_advance, pos.y_advance)
            context.save()
            context.translate(pos.x_offset, pos.y_offset)
            hb.font_paint_glyph(font, info.codepoint, PFUNCS, id(context), 0, 0x0000FF)
            context.restore()

    @property
    def advance(self):
        return sum(pos.x_advance for pos in self._positions)

    @property
    def strippedadvance(self):
        w = self.advance
        if len(self) and self._text[self._glyphs[-1].cluster] == " ":
            w -= self._positions[-1].x_advance
        return w

    def __str__(self):
        if not self._glyphs:
            return ""
        first = min(g.cluster for g in self._glyphs)
        last = max(g.cluster for g in self._glyphs)
        return self._text[first : last + 1]

    def __len__(self):
        return len(self._glyphs)

    def __bool__(self):
        return len(self) != 0

    def __repr__(self):
        return f"<Word advance={self.advance} text='{str(self)}'>"


class Line:
    def __init__(self, font, target_advance):
        self._font = font
        self._target_advance = target_advance
        self._words = []
        self._variation = None

    def append(self, word):
        self._words.append(word)

    def pop(self):
        return self._words.pop()

    def justify(self):
        buf, text = makebuffer(str(self))

        wiggle = 5
        advance = self.advance
        shrink = self._target_advance - wiggle < advance
        expand = self._target_advance + wiggle > advance

        ret, advance, tag, value = hb.shape_justify(
            self._font,
            buf,
            None,
            None,
            self._target_advance,
            self._target_advance,
            advance,
        )

        if not ret:
            return False

        self._variation = hb.variation_t()
        self._variation.tag = tag
        self._variation.value = value
        self._words = makewords(buf, self._font, text)

        if shrink and advance > self._target_advance + wiggle:
            return False
        if expand and advance < self._target_advance - wiggle:
            return False

        return True

    def draw(self, context):
        context.save()
        context.move_to(-1600, -200)
        context.set_font_size(130)
        context.set_source_rgb(1, 0, 0)
        if self._variation:
            tag = hb.tag_to_string(self._variation.tag).decode("ascii")
            context.show_text(f" {tag}={self._variation.value:g}")
        context.move_to(-1600, 0)
        context.show_text(f" {self.advance:g}/{self._target_advance:g}")
        context.restore()

        if self._variation:
            hb.font_set_variations(self._font, [self._variation])

        context.scale(1, -1)
        context.translate(self._target_advance, 0)
        for word in self._words:
            word.draw(context, self._font)

    @property
    def advance(self):
        w = sum(word.advance for word in self._words[:-1])
        if len(self):
            w += self._words[-1].strippedadvance
        return w

    def __str__(self):
        return "".join(str(w) for w in self._words)

    def __len__(self):
        return len(self._words)

    def __bool__(self):
        return len(self) != 0

    def __repr__(self):
        return f"<Line advance={self.advance} text='{str(self)}'>"


Configuration = namedtuple(
    "Configuration", ["width", "height", "fontsize", "fontpath", "textpath"]
)


def makebuffer(text):
    buf = hb.buffer_create()
    hb.buffer_set_direction(buf, hb.direction_t.RTL)
    hb.buffer_set_script(buf, hb.script_t.ARABIC)
    hb.buffer_set_language(buf, hb.language_from_string(b"ar"))

    # Strip and remove double spaces.
    text = " ".join(text.split())

    hb.buffer_add_codepoints(buf, [ord(c) for c in text], 0, len(text))

    return buf, text


def makewords(buf, font, text):
    hb.buffer_reverse(buf)
    words = [Word(font, text)]
    infos = hb.buffer_get_glyph_infos(buf)
    positions = hb.buffer_get_glyph_positions(buf)
    for info, pos in zip(infos, positions):
        words[-1].append(info, pos)
        if text[info.cluster] == " ":
            words.append(Word(font, text))
    return words


def typeset(conf):
    blob = hb.blob_create_from_file(conf.fontpath)
    face = hb.face_create(blob, 0)
    font = hb.font_create(face)

    with open(conf.textpath) as f:
        text = f.read()

    margin = conf.fontsize * 2
    scale = conf.fontsize / hb.face_get_upem(face)
    target_advance = (conf.width - (margin * 2)) / scale

    buf, text = makebuffer(text)

    hb.shape(font, buf)

    words = makewords(buf, font, text)

    lines = [Line(font, target_advance)]

    for word in words:
        lines[-1].append(word)
        if lines[-1].advance > target_advance:
            if lines[-1].justify():
                # Shrink
                lines.append(Line(font, target_advance))
            else:
                # Remove last word and expand
                lines[-1].pop()
                lines[-1].justify()
                lines.append(Line(font, target_advance))
                lines[-1].append(word)

    if lines[-1].advance != target_advance:
        lines[-1].justify()

    return lines, font


def render(context, conf):
    lines, font = typeset(conf)

    margin = conf.fontsize * 2
    scale = conf.fontsize / hb.face_get_upem(hb.font_get_face(font))

    _, extents = hb.font_get_h_extents(font)
    lineheight = extents.ascender - extents.descender + extents.line_gap
    lineheight *= scale

    context.save()
    context.translate(0, margin)
    for line in lines:
        context.translate(0, lineheight)

        context.save()
        context.translate(margin, 0)
        context.scale(scale, scale)
        line.draw(context)
        context.restore()
    context.restore()


def main(fontpath, textpath):
    def on_draw(da, context):
        alloc = da.get_allocation()
        conf = Configuration(
            width=alloc.width,
            height=alloc.height,
            fontsize=70,
            fontpath=fontpath,
            textpath=textpath,
        )
        POOL[id(context)] = context
        render(context, conf)
        del POOL[id(context)]

    drawingarea = Gtk.DrawingArea()
    drawingarea.connect("draw", on_draw)

    win = Gtk.Window()
    win.connect("destroy", Gtk.main_quit)
    win.set_default_size(1000, 700)
    win.add(drawingarea)

    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="HarfBuzz justification demo.")
    parser.add_argument("fontfile", help="input Glyphs source file")
    parser.add_argument("textfile", help="font version")
    args = parser.parse_args()
    main(args.fontfile, args.textfile)

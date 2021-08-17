#    Copyright 2012, SIL International
#    All rights reserved.
#
#    This library is free software; you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as published
#    by the Free Software Foundation; either version 2.1 of License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should also have received a copy of the GNU Lesser General Public
#    License along with this library in the file named "LICENSE".
#    If not, write to the Free Software Foundation, 51 Franklin Street,
#    suite 500, Boston, MA 02110-1335, USA or visit their web page on the
#    internet at http://www.fsf.org/licenses/lgpl.html.

import ctypes
import ctypes.util
import errno
import os
import sys
from ctypes import (byref,
                    c_char, c_char_p,
                    c_double, c_float,
                    c_int, c_int16,
                    c_size_t,
                    c_uint,  c_uint8, c_uint16, c_uint32,
                    c_ushort,
                    c_void_p,
                    CFUNCTYPE, POINTER, Structure)


libpath = os.environ.get('PYGRAPHITE2_LIBRARY_PATH',
                         ctypes.util.find_library("graphite2"))
if libpath is None:
    # find wheel's library
    wheel = os.path.dirname(__file__)
    if os.name == 'nt':
        libpath = os.path.join(wheel, 'bin', 'graphite2.dll')
    else:
        libpath = os.path.join(wheel, 'lib', 'libgraphite2.so')
gr2 = ctypes.cdll.LoadLibrary(libpath)


def grversion():
    a = c_int()
    b = c_int()
    c = c_int()
    gr2.gr_engine_version(byref(a), byref(b), byref(c))
    return (a.value, b.value, c.value)


def __check(result, func, args):
    if not result:
        raise RuntimeError(func.__name__ + ': returned ' + repr(result))
    return result


def __idx_error(result, func, args):
    if not result:
        raise IndexError(func.__name__ + ': invalid index ' + repr(args[1]))
    return result


def fn(name, res, *params, **kwds):
    f = getattr(gr2, name)
    f.restype = res
    f.argtypes = params
    errcheck = kwds.get('errcheck')
    if errcheck: f.errcheck = errcheck


class FaceInfo(Structure):
    _fields_ = [("extra_ascent", c_ushort),
                ("extra_descent", c_ushort),
                ("upem", c_ushort)]


tablefn = CFUNCTYPE(c_void_p, c_void_p, c_uint, POINTER(c_size_t))
advfn = CFUNCTYPE(c_float, c_void_p, c_ushort)

fn('gr_engine_version', None, POINTER(c_int), POINTER(c_int), POINTER(c_int))
fn('gr_make_face', c_void_p, c_void_p, tablefn, c_uint, errcheck=__check)
fn('gr_str_to_tag', c_uint32, c_char_p)
fn('gr_tag_to_str', None, c_uint32, POINTER(c_char))
fn('gr_face_featureval_for_lang', c_void_p, c_void_p, c_uint32,
    errcheck=__check)
fn('gr_face_find_fref', c_void_p, c_void_p, c_uint32, errcheck=__idx_error)
fn('gr_face_n_fref', c_uint16, c_void_p)
fn('gr_face_fref', c_void_p, c_void_p, c_uint16, errcheck=__idx_error)
fn('gr_face_n_languages', c_ushort, c_void_p)
fn('gr_face_lang_by_index', c_uint32, c_void_p, c_uint16)
fn('gr_face_destroy', None, c_void_p)
fn('gr_face_n_glyphs', c_ushort, c_void_p)
fn('gr_face_info', POINTER(FaceInfo), c_void_p)
fn('gr_face_is_char_supported', c_int, c_void_p, c_uint32, c_uint32)
fn('gr_make_file_face', c_void_p, c_char_p, c_uint, errcheck=__check)
fn('gr_make_font', c_void_p, c_float, c_void_p, errcheck=__check)
fn('gr_make_font_with_advance_fn', c_void_p,
    c_float, c_void_p, advfn, c_void_p, errcheck=__check)
fn('gr_font_destroy', None, c_void_p)
fn('gr_fref_feature_value', c_uint16, c_void_p, c_void_p)
fn('gr_fref_set_feature_value', c_int, c_void_p, c_uint16, c_void_p)
fn('gr_fref_id', c_uint32, c_void_p)
fn('gr_fref_n_values', c_uint16, c_void_p)
fn('gr_fref_value', c_int16, c_void_p, c_uint16)
fn('gr_fref_label', c_void_p,
    c_void_p, POINTER(c_uint16), c_int, POINTER(c_uint32))
fn('gr_fref_value_label', c_void_p,
    c_void_p, c_uint16, POINTER(c_uint16), c_int, POINTER(c_uint32))
fn('gr_label_destroy', None, c_void_p)
fn('gr_featureval_clone', c_void_p, c_void_p, errcheck=__check)
fn('gr_featureval_destroy', None, c_void_p)

fn('gr_cinfo_unicode_char', c_uint, c_void_p)
fn('gr_cinfo_break_weight', c_int, c_void_p)
fn('gr_cinfo_after', c_int, c_void_p)
fn('gr_cinfo_before', c_int, c_void_p)
fn('gr_cinfo_base', c_size_t, c_void_p)
fn('gr_count_unicode_characters', c_size_t,
    c_int, c_void_p, c_void_p, POINTER(c_void_p))
fn('gr_make_seg', c_void_p,
    c_void_p, c_void_p, c_uint32, c_void_p, c_int, c_void_p, c_size_t, c_int,
    errcheck=__check)
fn('gr_seg_destroy', None, c_void_p)
fn('gr_seg_advance_X', c_float, c_void_p)
fn('gr_seg_advance_Y', c_float, c_void_p)
fn('gr_seg_n_cinfo', c_uint, c_void_p)
fn('gr_seg_cinfo', c_void_p, c_void_p, c_uint)
fn('gr_seg_n_slots', c_uint, c_void_p)
fn('gr_seg_first_slot', c_void_p, c_void_p)
fn('gr_seg_last_slot', c_void_p, c_void_p)
fn('gr_seg_justify', c_float,
    c_void_p, c_void_p, c_void_p, c_double, c_int, c_void_p, c_void_p)
fn('gr_slot_next_in_segment', c_void_p, c_void_p)
fn('gr_slot_prev_in_segment', c_void_p, c_void_p)
fn('gr_slot_attached_to', c_void_p, c_void_p)
fn('gr_slot_first_attachment', c_void_p, c_void_p)
fn('gr_slot_next_sibling_attachment', c_void_p, c_void_p)
fn('gr_slot_gid', c_ushort, c_void_p)
fn('gr_slot_origin_X', c_float, c_void_p)
fn('gr_slot_origin_Y', c_float, c_void_p)
fn('gr_slot_advance_X', c_float, c_void_p)
fn('gr_slot_advance_Y', c_float, c_void_p)
fn('gr_slot_before', c_int, c_void_p)
fn('gr_slot_after', c_int, c_void_p)
fn('gr_slot_index', c_uint, c_void_p)
fn('gr_slot_attr', c_int, c_void_p, c_void_p, c_int, c_uint8)
fn('gr_slot_can_insert_before', c_int, c_void_p)
fn('gr_slot_original', c_int, c_void_p)
fn('gr_slot_linebreak_before', None, c_void_p)

(major, minor, debug) = grversion()
if major > 1 or minor > 1:
    fn('gr_start_logging', c_int, c_void_p, c_char_p)
    fn('gr_stop_logging', None, c_void_p)
else:
    fn('graphite_start_logging', c_int, c_void_p, c_int)
    fn('graphite_stop_logging', None)


def tag_to_str(num):
    s = ctypes.create_string_buffer(b'\000' * 5)
    gr2.gr_tag_to_str(num, s)
    return bytes(s.value)


class Label(str):
    def __new__(typename, ref, size):
        v = ctypes.string_at(ref, size)
        if sys.version_info.major > 2:
            v = v.decode("utf-8")
        return super(Label, typename).__new__(
                        typename, v)

    def __init__(self, ref, size):
        self.ref = ref

    def __del__(self, __gr2=gr2):
        if self.ref: __gr2.gr_label_destroy(self.ref)


class FeatureVal(object):
    def __init__(self, fval):
        self.fval = fval

    def __del__(self, __gr2=gr2):
        __gr2.gr_featureval_destroy(self.fval)

    def get(self, fref):
        return gr2.gr_fref_feature_value(fref.fref, self.fval)

    def set(self, fref, val):
        if not gr2.gr_fref_set_feature_value(fref.fref, val, self.fval):
            raise ValueError('gr_fref_set_feature_value call failed')


class FeatureRef(object):
    def __init__(self, fref, index=0):
        self.fref = fref
        self.index = index

    def num(self):
        return gr2.gr_fref_n_values(self.fref)

    def val(self, ind):
        return gr2.gr_fref_value(self.fref, ind)

    def name(self, langid):
        lngid = c_uint16(langid)
        length = c_uint32(0)
        res = gr2.gr_fref_label(self.fref, byref(lngid), 1, byref(length))
        if res is None:
            return Label("", 0)
        return Label(res, length.value)

    def label(self, ind, langid):
        lngid = c_uint16(langid)
        length = c_uint32(0)
        res = gr2.gr_fref_value_label(self.fref, ind, byref(lngid), 1,
                                      byref(length))
        if res is None:
            return Label("", 0)
        return Label(res, length.value)

    def tag(self):
        return tag_to_str(gr2.gr_fref_id(self.fref))


class Face(object):
    def __init__(self, data, options=0, fn=None):
        self.face = None
        if fn:
            self.face = gr2.gr_make_face(bytes(data), fn, options)
        else:
            if not os.path.isfile(data):
                raise FileNotFoundError(errno.ENOENT,
                                        os.strerror(errno.ENOENT), data)
            if hasattr(data, '__fspath__'): data = os.fspath(data)
            self.face = gr2.gr_make_file_face(data.encode('utf_8'), options)

    def __del__(self, __gr2=gr2):
        __gr2.gr_face_destroy(self.face)

    def get_upem(self):
        finfo = gr2.gr_face_info(self.face)
        return finfo.contents.upem

    def num_glyphs(self):
        return gr2.fr_face_n_glyphs(self.face)

    def get_featureval(self, lang):
        if isinstance(lang, bytes):
            lang = gr2.gr_str_to_tag(lang)
        return FeatureVal(gr2.gr_face_featureval_for_lang(self.face, lang))

    def get_featureref(self, featid):
        if isinstance(featid, bytes):
            featid = gr2.gr_str_to_tag(featid)
        return FeatureRef(gr2.gr_face_find_fref(self.face, featid))

    @property
    def featureRefs(self):
        num = gr2.gr_face_n_fref(self.face)
        for i in range(num):
            yield FeatureRef(gr2.gr_face_fref(self.face, i), index=i)

    @property
    def featureLangs(self):
        num = gr2.gr_face_n_languages(self.face)
        for i in range(num):
            yield gr2.gr_face_lang_by_index(self.face, i)


class Font(object):
    def __init__(self, face, ppm, fn=None, data=None):
        if fn:
            self.font = gr2.gr_make_font_with_advance_fn(ppm, data, fn,
                                                         face.face)
        else:
            self.font = gr2.gr_make_font(ppm, face.face)

    def __del__(self, __gr2=gr2):
        __gr2.gr_font_destroy(self.font)


class CInfo(object):
    def __init__(self, pcinfo):
        self.cinfo = pcinfo

    @property
    def unicode(self):
        return gr2.gr_cinfo_unicode_char(self.cinfo)

    @property
    def breakweight(self):
        return gr2.gr_cinfo_break_weight(self.cinfo)

    @property
    def after(self):
        return gr2.gr_cinfo_after(self.cinfo)

    @property
    def before(self):
        return gr2.gr_cinfo_before(self.cinfo)

    @property
    def base(self):
        return gr2.gr_cinfo_base(self.cinfo)


class Slot(object):
    def __init__(self, s):
        self.slot = s

    def attached_to(self):
        return Slot(gr2.gr_slot_attached_to(self.slot))

    def children(self):
        s = gr2.gr_slot_first_attachment(self.slot)
        while s:
            yield Slot(s)
            s = gr2.gr_slot_next_sibling_attachment(s)

    @property
    def index(self):
        return gr2.gr_slot_index(self.slot)

    @property
    def gid(self):
        return gr2.gr_slot_gid(self.slot)

    @property
    def origin(self):
        return (gr2.gr_slot_origin_X(self.slot),
                gr2.gr_slot_origin_Y(self.slot))

    @property
    def advance(self):
        return (gr2.gr_slot_advance_X(self.slot),
                gr2.gr_slot_advance_Y(self.slot))

    @property
    def before(self):
        return gr2.gr_slot_before(self.slot)

    @property
    def after(self):
        return gr2.gr_slot_after(self.slot)

    @property
    def insert_before(self):
        return gr2.gr_slot_can_insert_before(self.slot)

    @property
    def original(self):
        return gr2.gr_slot_original(self.slot)

    @property
    def linebreak(self):
        gr2.gr_slot_linebreak_before(self.slot)

    def gettattr(self, seg, ind, subindex):
        return gr2.gr_slot_attr(self.slot, seg.seg, ind, subindex)


class Segment(object):
    def __init__(self, font, face,
                 scriptid, string, rtl,
                 length=None, feats=None):
        if not length:
            length = len(string)
        if isinstance(scriptid, bytes):
            scriptid = gr2.gr_str_to_tag(scriptid)
        self.seg = gr2.gr_make_seg(font and font.font,
                                   face.face,
                                   scriptid,
                                   feats and feats.fval,
                                   1,
                                   string.encode('utf_8'), length, rtl)

    def __del__(self, __gr2=gr2):
        __gr2.gr_seg_destroy(self.seg)

    @property
    def advance(self):
        return (gr2.gr_seg_advance_X(self.seg),
                gr2.gr_seg_advance_Y(self.seg))

    @property
    def num_cinfo(self):
        return gr2.gr_seg_n_cinfo(self.seg)

    def cinfo(self, ind):
        return CInfo(gr2.gr_seg_cinfo(self.seg, ind))

    @property
    def num_slots(self):
        return gr2.gr_seg_n_slots(self.seg)

    @property
    def slots(self):
        s = gr2.gr_seg_first_slot(self.seg)
        res = []
        while s:
            res.append(Slot(s))
            s = gr2.gr_slot_next_in_segment(s)
        return res

    @property
    def revslots(self):
        s = gr2.gr_seg_last_slot(self.seg)
        res = []
        while s:
            res.append(Slot(s))
            s = gr2.gr_slot_prev_in_segment(s)
        return res

    def justify(self, start, font, width, flags, first=None, last=None):
        gr2.gr_seg_justify(self.seg,
                           start.slot,
                           font.font,
                           width,
                           flags,
                           first and first.slot,
                           last and last.slot)

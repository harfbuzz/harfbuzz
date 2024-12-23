// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.

#include <cstdio>

#include "graphite2/Log.h"
#include "inc/debug.h"
#include "inc/CharInfo.h"
#include "inc/Slot.h"
#include "inc/Segment.h"
#include "inc/json.h"
#include "inc/Collider.h"

#if defined _WIN32
#include "windows.h"
#endif

using namespace graphite2;

#if !defined GRAPHITE2_NTRACING
json *global_log = 0;
#endif

extern "C" {

bool gr_start_logging(GR_MAYBE_UNUSED gr_face * face, const char *log_path)
{
    if (!log_path)  return false;

#if !defined GRAPHITE2_NTRACING
    gr_stop_logging(face);
#if defined _WIN32
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, log_path, -1, 0, 0);
    if (n == 0 || n > MAX_PATH - 12) return false;

    LPWSTR wlog_path = gralloc<WCHAR>(n);
    if (!wlog_path) return false;
    FILE *log = 0;
    if (wlog_path && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, log_path, -1, wlog_path, n))
        log = _wfopen(wlog_path, L"wt");

    free(wlog_path);
#else   // _WIN32
    FILE *log = fopen(log_path, "wt");
#endif  // _WIN32
    if (!log)   return false;

    if (face)
    {
        face->setLogger(log);
        if (!face->logger()) return false;

        *face->logger() << json::array;
#ifdef GRAPHITE2_TELEMETRY
        *face->logger() << face->tele;
#endif
    }
    else
    {
        global_log = new json(log);
        *global_log << json::array;
    }

    return true;
#else   // GRAPHITE2_NTRACING
    return false;
#endif  // GRAPHITE2_NTRACING
}

bool graphite_start_logging(FILE * /* log */, GrLogMask /* mask */)
{
//#if !defined GRAPHITE2_NTRACING
//  graphite_stop_logging();
//
//    if (!log) return false;
//
//    dbgout = new json(log);
//    if (!dbgout) return false;
//
//    *dbgout << json::array;
//    return true;
//#else
    return false;
//#endif
}

void gr_stop_logging(GR_MAYBE_UNUSED gr_face * face)
{
#if !defined GRAPHITE2_NTRACING
    if (face && face->logger())
    {
        FILE * log = face->logger()->stream();
        face->setLogger(0);
        fclose(log);
    }
    else if (!face && global_log)
    {
        FILE * log = global_log->stream();
        delete global_log;
        global_log = 0;
        fclose(log);
    }
#endif
}

void graphite_stop_logging()
{
//    if (dbgout) delete dbgout;
//    dbgout = 0;
}

} // extern "C"

#ifdef GRAPHITE2_TELEMETRY
size_t   * graphite2::telemetry::_category = 0UL;
#endif

#if !defined GRAPHITE2_NTRACING

#ifdef GRAPHITE2_TELEMETRY

json & graphite2::operator << (json & j, const telemetry & t) throw()
{
    j << json::object
            << "type"   << "telemetry"
            << "silf"   << t.silf
            << "states" << t.states
            << "starts" << t.starts
            << "transitions" << t.transitions
            << "glyphs" << t.glyph
            << "code"   << t.code
            << "misc"   << t.misc
            << "total"  << (t.silf + t.states + t.starts + t.transitions + t.glyph + t.code + t.misc)
        << json::close;
    return j;
}
#else
json & graphite2::operator << (json & j, const telemetry &) throw()
{
    return j;
}
#endif


json & graphite2::operator << (json & j, const CharInfo & ci) throw()
{
    return j << json::object
                << "offset"         << ci.base()
                << "unicode"        << ci.unicodeChar()
                << "break"          << ci.breakWeight()
                << "flags"          << ci.flags()
                << "slot" << json::flat << json::object
                    << "before" << ci.before()
                    << "after"  << ci.after()
                    << json::close
                << json::close;
}


json & graphite2::operator << (json & j, const dslot & ds) throw()
{
    assert(ds.first);
    assert(ds.second);
    const Segment & seg = *ds.first;
    const Slot & s = *ds.second;
    const SlotCollision *cslot = seg.collisionInfo(s);

    j << json::object
        << "id"             << objectid(SlotBuffer::const_iterator::from(ds.second))
        << "gid"            << s.gid()
        << "charinfo" << json::flat << json::object
            << "original"       << s.original()
            << "before"         << s.before()
            << "after"          << s.after()
            << json::close
        << "origin"         << s.origin()
        << "shift"          << Position(float(s.getAttr(seg, gr_slatShiftX, 0)),
                                        float(s.getAttr(seg, gr_slatShiftY, 0)))
        << "advance"        << s.advancePos()
        << "insert"         << s.insertBefore()
        << "break"          << s.getAttr(seg, gr_slatBreak, 0);
    if (s.just() > 0)
        j << "justification"    << s.just();
    if (s.bidiLevel() > 0)
        j << "bidi"     << s.bidiLevel();
    if (!s.isBase())
        j << "parent" << json::flat << json::object
            << "id"             << objectid(SlotBuffer::const_iterator::from(s.attachedTo()))
            << "level"          << s.getAttr(seg, gr_slatAttLevel, 0)
            << "offset"         << s.attachOffset()
            << json::close;
    j << "user" << json::flat << json::array;
    for (size_t n = 0; n != seg.numAttrs(); ++n)
        j   << s.userAttrs()[n];
    j       << json::close;
    if (s.isParent())
    {
        j   << "children" << json::flat << json::array;
        for (auto c = s.children(); c != s.end(); ++c)
            j   << objectid(SlotBuffer::const_iterator::from(&*c));
        j       << json::close;
    }
    if (cslot)
    {
		// Note: the reason for using Positions to lump together related attributes is to make the
		// JSON output slightly more compact.
        j << "collision" << json::flat << json::object
//              << "shift" << cslot->shift() -- not used pass level, only within the collision routine itself
              << "offset" << cslot->offset()
              << "limit" << cslot->limit()
              << "flags" << cslot->flags()
              << "margin" << Position(cslot->margin(), cslot->marginWt())
              << "exclude" << cslot->exclGlyph()
              << "excludeoffset" << cslot->exclOffset();
		if (cslot->seqOrder() != 0)
		{
			j << "seqclass" << Position(cslot->seqClass(), cslot->seqProxClass())
				<< "seqorder" << cslot->seqOrder()
				<< "seqabove" << Position(cslot->seqAboveXoff(), cslot->seqAboveWt())
				<< "seqbelow" << Position(cslot->seqBelowXlim(), cslot->seqBelowWt())
				<< "seqvalign" << Position(cslot->seqValignHt(), cslot->seqValignWt());
		}
        j << json::close;
    }
    return j << json::close;
}

void graphite2::objectid::set_name(void const * addr, uint16_t generation) noexcept
{
    uint32_t const p = uint32_t(reinterpret_cast<size_t>(addr));
    sprintf(name, "%.4x-%.2x-%.4hx", uint16_t(p >> 16), generation, uint16_t(p));
    name[sizeof name-1] = 0;
}


#endif

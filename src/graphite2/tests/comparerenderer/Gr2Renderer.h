// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#pragma once

#include <new>
#include <iostream>
#include <memory>
#include <string>
#include "Renderer.h"
#include "FeatureParser.h"
#include <graphite2/Types.h>
#include <graphite2/Segment.h>
#include <graphite2/Log.h>


using gr_face_ptr = std::unique_ptr<gr_face, decltype(&gr_face_destroy)>;
using gr_font_ptr = std::unique_ptr<gr_font, decltype(&gr_font_destroy)>;
using gr_feature_val_ptr = std::unique_ptr<gr_feature_val, decltype(&gr_featureval_destroy)>;
using gr_segment_ptr = std::unique_ptr<gr_segment, decltype(&gr_seg_destroy)>;

class Gr2Face : public gr_face_ptr
{
public:
	Gr2Face(const char * fontFile, const std::string & logPath, const bool demand_load)
	: gr_face_ptr(gr_make_file_face(fontFile,
			!demand_load ? gr_face_preloadGlyphs : gr_face_default),
		&gr_face_destroy)
	{
		if (*this && logPath.size())	gr_start_logging(get(), logPath.c_str());
	}

	Gr2Face(Gr2Face && f): gr_face_ptr(std::move(f)) {}

	~Gr2Face() throw()
	{
		gr_stop_logging(get());
	}
};


class Gr2Renderer : public Renderer
{
public:
	Gr2Renderer(Gr2Face & face, int fontSize, bool rtl, FeatureParser * features)
	: m_rtl(rtl),
		m_grFace(std::move(face)),
		m_grFont(nullptr, &gr_font_destroy),
		m_grFeatures(nullptr, gr_featureval_destroy),
		m_name("graphite2")
	{
		if (!m_grFace)
			return;

		m_grFont.reset(gr_make_font(static_cast<float>(fontSize), m_grFace.get()));
		if (features)
		{
			m_grFeatures.reset(gr_face_featureval_for_lang(m_grFace.get(), features->langId()));
			for (size_t i = 0; i < features->featureCount(); i++)
			{
				const gr_feature_ref * ref = gr_face_find_fref(m_grFace.get(), features->featureId(i));
				if (ref)
				gr_fref_set_feature_value(ref, features->featureSValue(i), m_grFeatures.get());
			}
		}
		else
		{
			m_grFeatures.reset(gr_face_featureval_for_lang(m_grFace.get(), 0));
		}
	}

	virtual void renderText(const char * utf8, size_t length, RenderedLine * result, FILE *log)
	{
		const void * pError = NULL;
		if (!m_grFace)
		{
			new (result) RenderedLine();
			return;
		}
		size_t numCodePoints = gr_count_unicode_characters(gr_utf8, utf8, utf8 + length, &pError);
		if (pError)
			std::cerr << "Invalid Unicode pos" << int(static_cast<const char*>(pError) - utf8) << std::endl;

		gr_segment_ptr pSeg = gr_segment_ptr(
				gr_make_seg(m_grFont.get(), m_grFace.get(), 0u, m_grFeatures.get(),
										gr_utf8, utf8, numCodePoints, m_rtl),
				&gr_seg_destroy);

		if (!pSeg)
		{
			std::cerr << "Failed to create segment" << std::endl;
			new (result) RenderedLine(0, .0f);
			return;
		}

		RenderedLine * renderedLine = new (result) RenderedLine(std::string(utf8, length), gr_seg_n_slots(pSeg.get()),
																															 gr_seg_advance_X(pSeg.get()));
		const gr_slot * s = gr_seg_first_slot(pSeg.get());
		for (int i = 0; s; ++i)
		{
			(*renderedLine)[i].set(gr_slot_gid(s), gr_slot_origin_X(s),
														gr_slot_origin_Y(s), gr_slot_before(s),
														gr_slot_after(s));
			s = gr_slot_next_in_segment(s);
		}
	}
	virtual const char * name() const { return m_name; }
	private:
		Gr2Face 						m_grFace;
		const char * const 	m_name;
		bool 					 			m_rtl;
		gr_font_ptr 				m_grFont;
		gr_feature_val_ptr	m_grFeatures;
	};

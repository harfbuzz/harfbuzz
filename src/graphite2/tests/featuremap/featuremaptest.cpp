// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <graphite2/Font.h>

#include "inc/Endian.h"
#include "inc/Face.h"
#include "inc/FeatureMap.h"
#include "inc/TtfUtil.h"

using namespace graphite2;


class face_handle : public gr_face_ops
{
public:
  using table_t = std::pair<void const *, size_t>;
  static const table_t no_table;


private:
  std::vector<uint8_t> _ttf;
  void const *    _table_dir;
  void const *    _header_tbl;
  mutable std::map<const TtfUtil::Tag, table_t> _tables;

  static
  decltype(_ttf) buffer_from_file(std::string const & backing_font_path) {
    std::ifstream f(backing_font_path, std::ifstream::binary);
    decltype(_ttf) result(size_t(f.seekg(0, std::ios::end).tellg()));
    f.seekg(0, std::ios::beg);

    result.assign(std::istreambuf_iterator<char>(f),
                  std::istreambuf_iterator<char>());
    return result;
  }

public:
  face_handle() noexcept
  : gr_face_ops({sizeof(gr_face_ops), get_table_fn, nullptr}),
    _table_dir(nullptr), _header_tbl(nullptr) {}

  face_handle(std::string const & backing_font_path)
  : gr_face_ops({sizeof(gr_face_ops), get_table_fn, nullptr}),
    _ttf(buffer_from_file(backing_font_path)),
    _table_dir(nullptr),
    _header_tbl(nullptr)
  {
    size_t tbl_offset, tbl_len;

    // Get the header
    if (!TtfUtil::GetHeaderInfo(tbl_offset, tbl_len)
        || tbl_len > _ttf.size()
        || tbl_offset > _ttf.size() - tbl_len
        || !TtfUtil::CheckHeader(_header_tbl = _ttf.data() + tbl_offset))
      throw std::runtime_error(backing_font_path + ": invalid font.");

      // Get the table directory
    if (!TtfUtil::GetTableDirInfo(_header_tbl, tbl_offset, tbl_len)
        || tbl_len > _ttf.size()
        || tbl_offset > _ttf.size() - tbl_len)
      throw std::runtime_error(backing_font_path + ": invalid font");
    _table_dir = _ttf.data() + tbl_offset;
  }

  inline
  void replace_table(TtfUtil::Tag name, void const * data, size_t len) noexcept
  {
    _tables[name] = {data, len};
  }

  table_t const & operator [] (TtfUtil::Tag name) const noexcept
  {
    assert(_header_tbl);
    assert(_table_dir);
    auto & table = _tables[name];

    if (!table.first)
    {
      size_t tbl_offset, tbl_len;
      if (TtfUtil::GetTableInfo(name, _header_tbl, _table_dir,
            tbl_offset, tbl_len))
          table = {_ttf.data() + tbl_offset, tbl_len};
    }

    return table;
  }

private:
  static const void * get_table_fn(const void *afh, unsigned int name,
                                  size_t *len) {
    assert(afh != nullptr);

    face_handle const & fh = *static_cast<decltype(&fh)>(afh);
    auto & t = fh[name];
    *len = t.second;
    return t.first;
  }
};
const face_handle::table_t face_handle::no_table = {0,0};


#pragma pack(push, 1)

template<typename T> class _be
{
	T _v;

public:
  _be(const T & t) noexcept               {_v = be::swap<T>(t);}
  constexpr operator T () const noexcept  {return be::swap<T>(_v); }
};

struct FeatHeader
{
    _be<uint16_t> m_major;
    _be<uint16_t> m_minor;
    _be<uint16_t> m_numFeat;
    _be<uint16_t> m_reserved1;
    _be<uint32_t> m_reserved2;
};

struct FeatDefn
{
    _be<uint32_t> m_featId;
    _be<uint16_t> m_numFeatSettings;
    _be<uint16_t> m_reserved1;
    _be<uint32_t> m_settingsOffset;
    _be<uint16_t> m_flags;
    _be<uint16_t> m_label;
};

struct FeatSetting
{
    _be<int16_t>	m_value;
    _be<uint16_t>	m_label;
};

struct FeatTableTestA
{
    FeatHeader  m_header;
    FeatDefn    m_defs[1];
    FeatSetting m_settings[2];
};
#pragma pack(pop)

const FeatTableTestA testDataA = {
    { 2, 0, 1, 0, 0},
    {{0x41424344, 2, 0, sizeof(FeatHeader) + sizeof(FeatDefn), 0, 1}},
    {{0,10},{1,11}}
};

struct FeatTableTestB
{
    FeatHeader  m_header;
    FeatDefn    m_defs[2];
    FeatSetting m_settings[4];
};

const FeatTableTestB testDataB = {
    { 2, 0, 2, 0, 0},
    {{0x41424344, 2, 0, sizeof(FeatHeader) + 2 * sizeof(FeatDefn), 0, 1},
     {0x41424345, 2, 0, sizeof(FeatHeader) + 2 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 2}},
    {{0,10},{1,11},{0,12},{1,13}}
};
const FeatTableTestB testDataBunsorted = {
    { 2, 0, 2, 0, 0},
    {{0x41424345, 2, 0, sizeof(FeatHeader) + 2 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 2},
     {0x41424344, 2, 0, sizeof(FeatHeader) + 2 * sizeof(FeatDefn), 0, 1}},
    {{0,10},{1,11},{0,12},{1,13}}
};

struct FeatTableTestC
{
    FeatHeader m_header;
    FeatDefn m_defs[3];
    FeatSetting m_settings[7];
};

const FeatTableTestC testDataCunsorted = {
    { 2, 0, 3, 0, 0},
    {{0x41424343, 3, 0, sizeof(FeatHeader) + 3 * sizeof(FeatDefn) + 4 * sizeof(FeatSetting), 0, 1},
     {0x41424345, 2, 0, sizeof(FeatHeader) + 3 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 3},
     {0x41424344, 2, 0, sizeof(FeatHeader) + 3 * sizeof(FeatDefn), 0, 2}},
    {{0,10},{1,11},{0,12},{1,13},{0,14},{1,15},{2,16}}
};

struct FeatTableTestD
{
    FeatHeader m_header;
    FeatDefn m_defs[4];
    FeatSetting m_settings[9];
};

const FeatTableTestD testDataDunsorted = {
    { 2, 0, 4, 0, 0},
    {{400, 3, 0, sizeof(FeatHeader) + 4 * sizeof(FeatDefn) + 4 * sizeof(FeatSetting), 0, 1},
     {100, 2, 0, sizeof(FeatHeader) + 4 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 3},
     {300, 2, 0, sizeof(FeatHeader) + 4 * sizeof(FeatDefn), 0, 2},
     {200, 2, 0, sizeof(FeatHeader) + 4 * sizeof(FeatDefn) + 7 * sizeof(FeatSetting), 0, 2}
    },
    {{0,10},{1,11},{0,12},{10,13},{0,14},{1,15},{2,16},{2,17},{4,18}}
};

struct FeatTableTestE
{
    FeatHeader m_header;
    FeatDefn m_defs[5];
    FeatSetting m_settings[11];
};
const FeatTableTestE testDataE = {
    { 2, 0, 5, 0, 0},
    {{400, 3, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 4 * sizeof(FeatSetting), 0, 1},
     {100, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 3},
     {500, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 9 * sizeof(FeatSetting), 0, 3},
     {300, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn), 0, 2},
     {200, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 7 * sizeof(FeatSetting), 0, 2}
    },
    {{0,10},{1,11},{0,12},{10,13},{0,14},{1,15},{2,16},{2,17},{4,18},{1,19},{2,20}}
};

const FeatTableTestE testBadOffset = {
    { 2, 0, 5, 0, 0},
    {{400, 3, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 4 * sizeof(FeatSetting), 0, 1},
     {100, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 2 * sizeof(FeatSetting), 0, 3},
     {500, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 9 * sizeof(FeatSetting), 0, 3},
     {300, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn), 0, 2},
     {200, 2, 0, sizeof(FeatHeader) + 5 * sizeof(FeatDefn) + 10 * sizeof(FeatSetting), 0, 2}
    },
    {{0,10},{1,11},{0,12},{10,13},{0,14},{1,15},{2,16},{2,17},{4,18},{1,19},{2,20}}
};

template <typename T>
inline
void testAssert(const char * msg, const T b)
{
    if (!b)
    {
        fprintf(stderr, msg, b);
        exit(1);
    }
}

template <typename T, typename R>
inline
void testAssertEqual(const char * msg, const T a, const R b)
{
    if (a != T(b))
    {
        fprintf(stderr, msg, a, T(b));
        exit(1);
    }
}


face_handle dummyFace;
using face_ptr = std::unique_ptr<gr_face, decltype(&gr_face_destroy)>;

template <class T> void testFeatTable(T const & table, std::string const & testName)
{
    FeatureMap testFeatureMap;
    dummyFace.replace_table(TtfUtil::Tag::Feat, &table, sizeof(T));
    face_ptr face(gr_make_face_with_ops(&dummyFace, &dummyFace, 0),
                  gr_face_destroy);
    if (!face)
      throw std::runtime_error("failed to load font");

    testAssert("readFeats", testFeatureMap.readFeats(*face));

    std::cerr << testName << std::endl;
    testAssertEqual("test num features %hu,%hu\n",
                    testFeatureMap.numFeats(),
                    table.m_header.m_numFeat);

    for (size_t i = 0; i < sizeof(table.m_defs) / sizeof(FeatDefn); i++)
    {
        const FeatureRef * ref = testFeatureMap.findFeatureRef(table.m_defs[i].m_featId);
        testAssert("test feat\n", ref);
        testAssertEqual("test feat settings %hu %hu\n", ref->getNumSettings(), table.m_defs[i].m_numFeatSettings);
        testAssertEqual("test feat label %hu %hu\n", ref->getNameId(), table.m_defs[i].m_label);
        size_t settingsIndex = (table.m_defs[i].m_settingsOffset - sizeof(FeatHeader)
            - (sizeof(FeatDefn) * table.m_header.m_numFeat)) / sizeof(FeatSetting);
        for (uint16_t j = 0; j < table.m_defs[i].m_numFeatSettings; j++)
        {
            testAssertEqual("setting label %hu %hu\n", ref->getSettingName(j),
                       table.m_settings[settingsIndex+j].m_label);
        }
    }
}


int main(int argc, char * argv[])
{
  try
  {
    if (argc != 2)
      throw std::length_error("not enough arguments: need a backing font");

    dummyFace = face_handle(argv[1]);
    testFeatTable<FeatTableTestA>(testDataA, "A\n");
    testFeatTable<FeatTableTestB>(testDataB, "B\n");
    testFeatTable<FeatTableTestB>(testDataBunsorted, "Bu\n");
    testFeatTable<FeatTableTestC>(testDataCunsorted, "C\n");
    testFeatTable<FeatTableTestD>(testDataDunsorted, "D\n");
    testFeatTable<FeatTableTestE>(testDataE, "E\n");

    // test a bad settings offset stradling the end of the table
    FeatureMap testFeatureMap;
    dummyFace.replace_table(TtfUtil::Tag::Feat, &testBadOffset, sizeof testBadOffset);
    face_ptr face(gr_make_face_with_ops(&dummyFace, &dummyFace, 0),
                  gr_face_destroy);
		testAssert("fail gracefully on bad table", !face);
  }
  catch (std::exception & e)
  {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  }

  return 0;
}

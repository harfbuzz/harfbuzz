// SPDX-License-Identifier: MIT
// Copyright 2018, SIL International, All rights reserved.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include <graphite2/Font.h>

#include "inc/TtfUtil.h"

namespace TtfUtil = graphite2::TtfUtil;

class memory_face : public gr_face_ops
{
  const uint8_t * _data;
  size_t          _len;
  const void *    _table_dir;
  const void *    _header_tbl;

public:
  memory_face(const uint8_t *data, size_t size)
  : gr_face_ops({sizeof(gr_face_ops), memory_face::get_table_fn, nullptr}),
    _data(data),
    _len(size),
    _table_dir(nullptr),
    _header_tbl(nullptr)
  {
    size_t tbl_offset, tbl_len;

    // Get the header
    if (!TtfUtil::GetHeaderInfo(tbl_offset, tbl_len)
        || tbl_len > size
        || tbl_offset > size - tbl_len) return;
    _header_tbl = data + tbl_offset;
    if (!TtfUtil::CheckHeader(_header_tbl)) return;

    // Get the table directory
    if (!TtfUtil::GetTableDirInfo(_header_tbl, tbl_offset, tbl_len)
        || tbl_len > size
        || tbl_offset > size - tbl_len) return;
    _table_dir = data + tbl_offset;
  }

  operator bool () const noexcept {
    return _table_dir && _header_tbl;
  }

  const uint8_t * data() const noexcept { return _data; }
  size_t          size() const noexcept { return _len; }

private:
  static const void * get_table_fn(const void* app_fh, unsigned int name, size_t *len)
  {
    if (app_fh == nullptr)
      return nullptr;

    const auto & mf = *static_cast<const memory_face *>(app_fh);
    if (!mf)
      return nullptr;

    size_t tbl_offset, tbl_len;
    if (!TtfUtil::GetTableInfo(name, mf._header_tbl, mf._table_dir, tbl_offset, tbl_len))
        return nullptr;

    if (tbl_len > mf._len
        || tbl_offset > mf._len - tbl_len)
        return nullptr;

    if (len)
      *len = tbl_len;
    return mf._data + tbl_offset;
  }
};

#pragma pack(push, 1)
struct feature
{
  uint32_t id;
  uint16_t value;
  uint16_t lang_id;
};

struct common_parameters
{
  uint32_t    script_tag = 0;
  uint32_t    lang_tag = 0;
  feature     feat  = {0xffffffff,0xffff,0x0409};
  uint8_t     encoding = 1;
};

template <class payload_t>
struct test_case
{
  struct slot_attr_parameters
  {
    uint8_t     index;
    uint8_t     subindex;
  };

  using attr_params = std::unique_ptr<slot_attr_parameters[]>;

  memory_face         ttf;
  payload_t           params;

  test_case(const uint8_t *data, size_t size)
  : ttf(data, size)
  {
    if (size > sizeof params)
      std::memcpy(&params, data + size - sizeof params, sizeof params);
  }

  bool has_feature() const noexcept {
    return params.feat.id != 0xffffffff || params.feat.value != 0xffff;
  }

  attr_params get_slot_attr_parameters(size_t n_slots) const {
    const auto sizeof_attrs = sizeof(slot_attr_parameters)*n_slots;
    attr_params block(nullptr);

    if (ttf.size() > sizeof_attrs)
    {
      block.reset(new slot_attr_parameters[n_slots]);
      std::memcpy(block.get(),
                  ttf.data() + ttf.size() - sizeof_attrs,
                  sizeof_attrs);
    }
    return block;
  }
};
#pragma pack(pop)

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

#include <new>
#include <unordered_map>

#include "hb-private.hh"
#include "hb-object-private.hh"

#include "hb-subset-map.hh"

struct hb_map_t {
  hb_object_header_t header;
  ASSERT_POD();
  std::unordered_map<hb_codepoint_t, hb_codepoint_t> *map;
};

hb_map_t *
hb_map_create_or_fail ()
{
  hb_map_t *map = hb_object_create<hb_map_t> ();
  if (map)
  {
    map->map = new (std::nothrow) std::unordered_map<hb_codepoint_t, hb_codepoint_t>();
  }
  if (!map->map)
  {
    hb_object_destroy (map);
    map = nullptr;
  }
  return map;
}

HB_INTERNAL void
hb_map_put (hb_map_t *map, hb_codepoint_t key, hb_codepoint_t value)
{
  (*map->map)[key] = value;
}

HB_INTERNAL hb_bool_t
hb_map_get (hb_map_t *map, hb_codepoint_t key, hb_codepoint_t *value /* OUT */)
{
  const auto it = map->map->find(key);
  if (it == map->map->end()) return false;
  *value = it->second;
  return true;
}

HB_INTERNAL void
hb_map_destroy (hb_map_t *map)
{
  if (map && map->map)
  {
    delete map->map;
  }
  hb_object_destroy (map);
}
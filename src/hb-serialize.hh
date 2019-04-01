/*
 * Copyright © 2007,2008,2009,2010  Red Hat, Inc.
 * Copyright © 2012,2018  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_SERIALIZE_HH
#define HB_SERIALIZE_HH

#include "hb.hh"
#include "hb-blob.hh"
#include "hb-map.hh"


/*
 * Serialize
 */

struct hb_serialize_context_t
{
  typedef unsigned objidx_t;

  struct range_t
  {
    char *head, *tail;
  };

  struct object_t : range_t
  {
    void fini () { links.fini (); }

    bool operator == (const object_t &o) const
    {
      return (tail - head == o.tail - o.head)
	  && (links.length != o.links.length)
	  && 0 == memcmp (head, o.head, tail - head)
	  && 0 == memcmp (&links, &o.links, links.get_size ());
    }
    uint32_t hash () const
    {
      return hb_bytes_t (head, tail - head).hash () ^
	     links.as_bytes ().hash ();
    }

    struct link_t
    {
      bool wide: 1;
      unsigned position : 31;
      objidx_t objidx;
    };

    hb_vector_t<link_t> links;
  };

  range_t snapshot () { range_t s = {head, tail} ; return s; }


  hb_serialize_context_t (void *start_, unsigned int size)
  {
    this->start = (char *) start_;
    this->end = this->start + size;
    reset ();
  }
  ~hb_serialize_context_t ()
  {
    current.fini_deep ();
    packed.fini_deep ();
    packed_map.fini ();
  }

  bool in_error () const { return !this->successful; }

  void reset ()
  {
    this->successful = true;
    this->ran_out_of_room = false;
    this->head = this->start;
    this->tail = this->end;
    this->debug_depth = 0;

    this->current.reset ();
    this->packed.reset ();
    this->packed.push ()->head = this->end;
    this->packed_map.reset ();
  }

  bool propagate_error (bool e)
  { return this->successful = this->successful && e; }
  template <typename T> bool propagate_error (const T &obj)
  { return this->successful = this->successful && !obj.in_error (); }
  template <typename T> bool propagate_error (const T *obj)
  { return this->successful = this->successful && !obj->in_error (); }
  template <typename T1, typename T2> bool propagate_error (T1 &&o1, T2 &&o2)
  { return propagate_error (o1) && propagate_error (o2); }
  template <typename T1, typename T2, typename T3>
  bool propagate_error (T1 &&o1, T2 &&o2, T3 &&o3)
  { return propagate_error (o1) && propagate_error (o2, o3); }

  /* To be called around main operation. */
  template <typename Type>
  Type *start_serialize ()
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, +1,
		     "start [%p..%p] (%lu bytes)",
		     this->start, this->end,
		     (unsigned long) (this->end - this->start));

    assert (!current.length);
    return push<Type> ();
  }
  void end_serialize ()
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, -1,
		     "end [%p..%p] serialized %u bytes; %s",
		     this->start, this->end,
		     (unsigned) (this->head - this->start),
		     this->successful ? "successful" : "UNSUCCESSFUL");

    /* TODO Propagate errors. */

    assert (current.length == 1);

    /* Only "pack" if there exist other objects... Otherwise, don't bother.
     * Saves a move. */
    if (packed.length == 1)
      return;

    pop_pack ();

    link ();
  }

  template <typename Type>
  Type *push ()
  {
    object_t obj;
    obj.head = head;
    obj.tail = tail;
    current.push (obj);
    return start_embed<Type> ();
  }
  void pop_discard ()
  {
    revert (current.pop ());
  }
  objidx_t pop_pack ()
  {
    object_t obj = current.pop ();
    obj.tail = head;
    unsigned len = obj.tail - obj.head;

    objidx_t objidx = packed_map.get (&obj);
    if (objidx)
    {
      obj.fini ();
      return objidx;
    }

    tail -= len;
    memmove (tail, obj.head, len);
    head = obj.head;

    if (!len)
      return 0;

    obj.head = tail;
    obj.tail = tail + len;

    object_t *key = packed.push (hb_move (obj));

    /* TODO Handle error. */
    if (unlikely (packed.in_error ()))
      return 0;

    objidx = packed.length - 1;

    packed_map.set (key, objidx);

    return objidx;
  }

  void revert (range_t snap)
  {
    assert (snap.head <= head);
    assert (tail <= snap.tail);
    head = snap.head;
    tail = snap.tail;
    discard_stale_objects ();
  }

  void discard_stale_objects ()
  {
    while (packed.length > 1 &&
	   packed.tail ().head < tail)
      packed.pop ();
    assert (packed.tail ().head == tail);
  }

  void link ()
  {
    assert (!current.length);

    for (auto obj_it = packed.iter (); obj_it; ++obj_it)
    {
      const object_t &parent = *obj_it;

      for (auto link_it = parent.links.iter (); link_it; ++link_it)
      {
        const object_t::link_t &link = *link_it;
	const object_t &child = packed[link.objidx];
	unsigned offset = child.head - parent.head;

	if (link.wide)
	{
	  auto &off = * ((BEInt<uint32_t, 4> *) (parent.head + offset));
	  off = offset;
	  propagate_error (off == offset);
	}
	else
	{
	  auto &off = * ((BEInt<uint16_t, 2> *) (parent.head + offset));
	  off = offset;
	  propagate_error (off == offset);
	}
      }
    }
  }

  unsigned int length () const { return this->head - current.tail ().head; }

  void align (unsigned int alignment)
  {
    unsigned int l = length () % alignment;
    if (l)
      allocate_size<void> (alignment - l);
  }

  template <typename Type>
  Type *start_embed (const Type *_ HB_UNUSED = nullptr) const
  {
    Type *ret = reinterpret_cast<Type *> (this->head);
    return ret;
  }

  template <typename Type>
  Type *allocate_size (unsigned int size)
  {
    if (unlikely (!this->successful)) return nullptr;

    if (this->tail - this->head < ptrdiff_t (size))
    {
      this->ran_out_of_room = true;
      this->successful = false;
      return nullptr;
    }
    memset (this->head, 0, size);
    char *ret = this->head;
    this->head += size;
    return reinterpret_cast<Type *> (ret);
  }

  template <typename Type>
  Type *allocate_min ()
  {
    return this->allocate_size<Type> (Type::min_size);
  }

  template <typename Type>
  Type *embed (const Type &obj)
  {
    unsigned int size = obj.get_size ();
    Type *ret = this->allocate_size<Type> (size);
    if (unlikely (!ret)) return nullptr;
    memcpy (ret, &obj, size);
    return ret;
  }
  template <typename Type>
  hb_serialize_context_t &operator << (const Type &obj) { embed (obj); return *this; }

  template <typename Type>
  Type *extend_size (Type &obj, unsigned int size)
  {
    assert (this->start <= (char *) &obj);
    assert ((char *) &obj <= this->head);
    assert ((char *) &obj + size >= this->head);
    if (unlikely (!this->allocate_size<Type> (((char *) &obj) + size - this->head))) return nullptr;
    return reinterpret_cast<Type *> (&obj);
  }

  template <typename Type>
  Type *extend_min (Type &obj) { return extend_size (obj, obj.min_size); }

  template <typename Type>
  Type *extend (Type &obj) { return extend_size (obj, obj.get_size ()); }

  /* Output routines. */
  hb_bytes_t copy_bytes () const
  {
    assert (this->successful);
    /* Copy both items from head side and tail side... */
    unsigned int len = (this->head - this->start)
		     + (this->end  - this->tail);
    char *p = (char *) malloc (len);
    if (p)
    {
      memcpy (p, this->start, this->head - this->start);
      memcpy (p + (this->head - this->start), this->tail, this->end - this->tail);
    }
    else
      return hb_bytes_t ();
    return hb_bytes_t (p, len);
  }
  template <typename Type>
  Type *copy () const
  { return reinterpret_cast<Type *> ((char *) copy_bytes ().arrayZ); }
  hb_blob_t *copy_blob () const
  {
    hb_bytes_t b = copy_bytes ();
    return hb_blob_create (b.arrayZ, b.length,
			   HB_MEMORY_MODE_WRITABLE,
			   (char *) b.arrayZ, free);
  }

  public: /* TODO Make private. */
  char *start, *head, *tail, *end;
  unsigned int debug_depth;
  bool successful;
  bool ran_out_of_room;

  private:

  /* Stack of currently under construction objects. */
  hb_vector_t<object_t> current;

  /* Stack of packed objects.  Object 0 is always nil object. */
  hb_vector_t<object_t> packed;

  /* Map view of packed objects. */
  hb_hashmap_t<const object_t *, objidx_t, nullptr, 0> packed_map;
};


#endif /* HB_SERIALIZE_HH */

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


/*
 * Serialize
 */

struct hb_serialize_context_t
{
  hb_serialize_context_t (void *start_, unsigned int size)
  {
    this->start = (char *) start_;
    this->end = this->start + size;
    reset ();
  }

  bool in_error () const { return !this->successful; }

  void reset ()
  {
    this->successful = true;
    this->head = this->start;
    this->debug_depth = 0;
  }

  bool propagate_error (bool e)
  { return this->successful = this->successful && e; }
  template <typename T> bool propagate_error (const T &obj)
  { return this->successful = this->successful && !obj.in_error (); }
  template <typename T> bool propagate_error (const T *obj)
  { return this->successful = this->successful && !obj->in_error (); }
  template <typename T1, typename T2> bool propagate_error (T1 &o1, T2 &o2)
  { return propagate_error (o1) && propagate_error (o2); }
  template <typename T1, typename T2> bool propagate_error (T1 *o1, T2 *o2)
  { return propagate_error (o1) && propagate_error (o2); }
  template <typename T1, typename T2, typename T3>
  bool propagate_error (T1 &o1, T2 &o2, T3 &o3)
  { return propagate_error (o1) && propagate_error (o2, o3); }
  template <typename T1, typename T2, typename T3>
  bool propagate_error (T1 *o1, T2 *o2, T3 *o3)
  { return propagate_error (o1) && propagate_error (o2, o3); }

  /* To be called around main operation. */
  template <typename Type>
  Type *start_serialize ()
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, +1,
		     "start [%p..%p] (%lu bytes)",
		     this->start, this->end,
		     (unsigned long) (this->end - this->start));

    return start_embed<Type> ();
  }
  void end_serialize ()
  {
    DEBUG_MSG_LEVEL (SERIALIZE, this->start, 0, -1,
		     "end [%p..%p] serialized %d bytes; %s",
		     this->start, this->end,
		     (int) (this->head - this->start),
		     this->successful ? "successful" : "UNSUCCESSFUL");
  }

  unsigned int length () const { return this->head - this->start; }

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
    if (unlikely (!this->successful || this->end - this->head < ptrdiff_t (size))) {
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
  template <typename Type>
  Type *copy () const
  {
    assert (this->successful);
    unsigned int len = this->head - this->start;
    void *p = malloc (len);
    if (p)
      memcpy (p, this->start, len);
    return reinterpret_cast<Type *> (p);
  }
  hb_bytes_t copy_bytes () const
  {
    assert (this->successful);
    unsigned int len = this->head - this->start;
    void *p = malloc (len);
    if (p)
      memcpy (p, this->start, len);
    else
      return hb_bytes_t ();
    return hb_bytes_t ((char *) p, len);
  }
  hb_blob_t *copy_blob () const
  {
    assert (this->successful);
    return hb_blob_create (this->start,
			   this->head - this->start,
			   HB_MEMORY_MODE_DUPLICATE,
			   nullptr, nullptr);
  }

  public:
  unsigned int debug_depth;
  char *start, *end, *head;
  bool successful;
};


#endif /* HB_SERIALIZE_HH */

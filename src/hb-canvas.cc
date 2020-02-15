/*
 * Under the same license as https://github.com/raphlinus/font-rs till we can
 * relicense it for HarfBuzz.
 */

#include "hb.hh"

#ifndef HB_NO_RASTER

struct canvas_point_t
{
  float x, y;
  canvas_point_t (float x_, float y_) { x = x_; y = y_; }
};

struct hb_canvas_t
{
  hb_object_header_t header;
  unsigned w, h;
  float *a;
  canvas_point_t p0;
  canvas_point_t start;

  void close_path () { if (is_open ()) line_to (start); }

  void move_to (const canvas_point_t &p)
  {
    if (is_open ()) close_path ();
    p0 = start = p;
  }

  void line_to (const canvas_point_t &p)
  {
    draw_line (p0, p);
    p0 = p;
  }

  void quadratic_to (const canvas_point_t &p1, const canvas_point_t &p2)
  {
    float devx = p0.x - 2.f * p1.x + p2.x;
    float devy = p0.y - 2.f * p1.y + p2.y;
    float devsq = devx * devx + devy * devy;
    if (devsq < 0.333f) return line_to (p2);
    float tol = 3.f;
    float n = 1 + (unsigned) floor (sqrtf (sqrtf (tol * (devx * devx + devy * devy))));
    canvas_point_t p = p0;
    float nrecip = 1.f / (float) n;
    float t = 0.f;
    for (unsigned i = 0; i < n - 1; ++i)
    {
      t += nrecip;

      /* https://developer.roblox.com/en-us/articles/Bezier-curves */
      float omt = 1.f - t;
      float omt2 = omt * omt;
      float t2 = t * t;
      canvas_point_t pn (omt2 * p0.x + 2.f * omt * t * p1.x + t2 * p2.x,
			 omt2 * p0.y + 2.f * omt * t * p1.y + t2 * p2.y);
      draw_line (p, pn);
      p = pn;
    }
    draw_line (p, p2);
    p0 = p2;
  }

  void cubic_to (const canvas_point_t &p1, const canvas_point_t &p2, const canvas_point_t &p3)
  {
    float devx = p0.x - 2.f * (p1.x + p2.x) / 2.f + p3.x; // just a made one, try to find a better way
    float devy = p0.y - 2.f * (p1.x + p2.x) / 2.f + p3.y;
    float devsq = devx * devx + devy * devy;
    if (devsq < 0.333f) return line_to (p3);
    float tol = 3.0;
    float n = 1 + (unsigned) floor (sqrtf (sqrtf (tol * (devx * devx + devy * devy))));
    canvas_point_t p = p0;
    float nrecip = 1.f / (float) n;
    float t = 0.f;
    for (unsigned i = 0; i < n - 1; ++i)
    {
      t += nrecip;

      /* https://twitter.com/freyaholmer/status/1063633411246628864 */
      float omt = 1.f - t;
      float omt2 = omt * omt;
      float t2 = t * t;
      canvas_point_t pn (omt2 * p0.x + 2.f * omt * t * p1.x + t2 * p2.x,
			 omt2 * p0.y + 2.f * omt * t * p1.y + t2 * p2.y);
      draw_line (p, pn);
      p = pn;
    }
    draw_line (p, p3);
    p0 = p3;
  }

  protected:
  bool is_open () { return (p0.x != start.x) || (p0.y != start.y); }
  void draw_line (const canvas_point_t &from, const canvas_point_t &to)
  {
    hb_array_t<float> a (this->a, w * h);
    if (from.y == to.y) return;
    float dir;
    const canvas_point_t *p0_, *p1_;
    if (from.y < to.y) { dir = 1.f; p0_ = &from; p1_ = &to; }
    else { dir = -1.f; p0_ = &to; p1_ = &from; }
    const float dxdy = (p1_->x - p0_->x) / (p1_->y - p0_->y);
    float x = p0_->x;
    unsigned y0 = (unsigned) hb_max (p0_->y, 0.f);
    if (p0_->y < 0.f) x -= p0_->y * dxdy;
    unsigned yZ = hb_min (this->h, (unsigned) ceil (p1_->y));
    for (unsigned y = y0; y < yZ; ++y)
    {
      unsigned linestart = y * this->w;
      float dy = hb_min ((float) (y + 1), p1_->y) - hb_max ((float) y, p0_->y);
      float xnext = x + dxdy * dy;
      float d = dy * dir;
      float x0, x1;
      if (x < xnext) { x0 = x; x1 = xnext; }
      else { x0 = xnext; x1 = x; }
      float x0floor = floor (x0);
      int x0i = (int) x0floor;
      float x1ceil = ceil (x1);
      int x1i = (int) x1ceil;
      if (x1i <= x0i + 1)
      {
	float xmf = .5f * (x + xnext) - x0floor;
	a[linestart + x0i] += d - d * xmf;
	a[linestart + (x0i + 1)] += d * xmf;
      }
      else
      {
	float s = 1.f / (x1 - x0);
	float x0f = x0 - x0floor;
	float a0 = .5f * s * (1.f - x0f) * (1.f - x0f);
	float x1f = x1 - x1ceil + 1.f;
	float am = .5f * s * x1f * x1f;
	a[linestart + x0i] += d * a0;
	if (x1i == x0i + 2)
	  a[linestart + (x0i + 1)] += d * (1.f - a0 - am);
	else
	{
	  float a1 = s * (1.5f - x0f);
	  a[linestart + (x0i + 1)] += d * (a1 - a0);
	  for (int xi = x0i + 2; xi < x1i - 1; ++xi)
	    a[linestart + xi] += d * s;
	  float a2 = a1 + ((float) (x1i - x0i - 3)) * s;
	  a[linestart + (x1i - 1)] += d * (1.f - a2 - am);
	}
	a[linestart + x1i] += d * am;
      }
      x = xnext;
    }
  }
};

/**
 * hb_canvas_create:
 *
 * Creates a canvas.
 *
 * Since: REPLACEME
 **/
hb_canvas_t *
hb_canvas_create (unsigned w, unsigned h)
{
  hb_canvas_t *canvas = hb_object_create<hb_canvas_t> ();
  float *a = (float *) calloc (w * h + 4, sizeof (float));
  if (unlikely (!canvas || !a))
  {
    free (canvas);
    free (a);
    return const_cast<hb_canvas_t *> (&Null (hb_canvas_t));
  }

  canvas_point_t p (0, 0);
  canvas->move_to (p);
  canvas->w = w;
  canvas->h = h;
  canvas->a = a;
  return canvas;
}

/**
 * hb_canvas_get_bitmap:
 *
 * Writes bitmap result of the canvas to an array of bytes with
 * height and width of used for the creation of the canvas.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_write_bitmap (hb_canvas_t *canvas, uint8_t *bitmap)
{
  canvas->close_path ();
  /* SSE3 version available at:
     https://github.com/raphlinus/font-rs/blob/9a63ddf/src/accumulate.c */
  float acc = 0.f;
  unsigned size = canvas->w * canvas->h;
  for (unsigned i = 0; i < size; ++i)
  {
    acc += canvas->a[i];
    float y = fabs (acc);
    if (unlikely (y > 1.f)) y = 1.f;
    bitmap[i] = (uint8_t) (255.f * y);
  }
}

/**
 * hb_canvas_destroy:
 *
 * Destroys the canvas.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_destroy (hb_canvas_t *canvas)
{
  if (!hb_object_destroy (canvas)) return;

  free (canvas->a);
  free (canvas);
}

/**
 * hb_canvas_move_to:
 *
 * Executes a move-to command.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_move_to (hb_canvas_t *canvas, float x, float y)
{
  canvas_point_t p (x, y);
  canvas->move_to (p);
}

/**
 * hb_canvas_line_to
 *
 * Executes a line-to command.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_line_to (hb_canvas_t *canvas, float x, float y)
{
  canvas_point_t p (x, y);
  canvas->line_to (p);
}

/**
 * hb_canvas_quadratic_to
 *
 * Executes a quadratic-to command.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_quadratic_to (hb_canvas_t *canvas, float x1, float y1, float x2, float y2)
{
  canvas_point_t p1 (x1, y1);
  canvas_point_t p2 (x2, y2);
  canvas->quadratic_to (p1, p2);
}

/**
 * hb_canvas_cubic_to
 *
 * Executes a cubic-to command.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_cubic_to (hb_canvas_t *canvas, float x1, float y1, float x2, float y2, float x3, float y3)
{
  canvas_point_t p1 (x1, y1);
  canvas_point_t p2 (x2, y2);
  canvas_point_t p3 (x3, y3);
  canvas->cubic_to (p1, p2, p3);
}

/**
 * hb_canvas_close_path:
 *
 * Executes a close path command by drawing a line
 * to the point the last path has started.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_close_path (hb_canvas_t *canvas)
{
  canvas->close_path ();
}

/**
 * hb_canvas_clear:
 *
 * Clear a canvas.
 *
 * Since: REPLACEME
 **/
void
hb_canvas_clear (hb_canvas_t *canvas)
{
  canvas_point_t p (0, 0);
  canvas->move_to (p);
  memset (canvas->a, 0, sizeof (float) * canvas->w * canvas->h);
}
#endif

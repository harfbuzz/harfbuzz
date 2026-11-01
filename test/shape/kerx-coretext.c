#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <stdbool.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  if (argc != 3)
    return 2;

  CGDataProviderRef provider = CGDataProviderCreateWithFilename (argv[1]);
  if (!provider)
    return 1;
  CGFontRef cg_font = CGFontCreateWithDataProvider (provider);
  if (!cg_font)
    return 1;
  CTFontRef font = CTFontCreateWithGraphicsFont (cg_font, CGFontGetUnitsPerEm (cg_font), NULL, NULL);
  if (!font)
    return 1;
  CFStringRef text = CFStringCreateWithCString (NULL, argv[2], kCFStringEncodingUTF8);
  if (!text)
    return 1;
  const void *keys[] = {kCTFontAttributeName};
  const void *values[] = {font};
  CFDictionaryRef attributes = CFDictionaryCreate (NULL, keys, values, 1,
                                                   &kCFTypeDictionaryKeyCallBacks,
                                                   &kCFTypeDictionaryValueCallBacks);
  if (!attributes)
    return 1;
  CFAttributedStringRef string = CFAttributedStringCreate (NULL, text, attributes);
  if (!string)
    return 1;
  CTLineRef line = CTLineCreateWithAttributedString (string);
  if (!line)
    return 1;

  CFArrayRef runs = CTLineGetGlyphRuns (line);
  bool first = true;
  putchar ('[');
  for (CFIndex i = 0; i < CFArrayGetCount (runs); i++)
  {
    CTRunRef run = (CTRunRef) CFArrayGetValueAtIndex (runs, i);
    CFIndex count = CTRunGetGlyphCount (run);
    if (count < 0 || count > 256)
      return 1;
    CGGlyph glyphs[256];
    CGPoint positions[256];
    CGSize advances[256];
    CFIndex clusters[256];
    CFRange all = CFRangeMake (0, 0);
    CTRunGetGlyphs (run, all, glyphs);
    CTRunGetPositions (run, all, positions);
    CTRunGetAdvances (run, all, advances);
    CTRunGetStringIndices (run, all, clusters);
    for (CFIndex j = 0; j < count; j++)
    {
      if (!first)
        putchar (',');
      first = false;
      printf ("{\"gid\":%u,\"cluster\":%ld,\"x\":%.9g,\"y\":%.9g,\"advance\":%.9g}",
              (unsigned) glyphs[j], (long) clusters[j], positions[j].x, positions[j].y,
              advances[j].width);
    }
  }
  puts ("]");
  CFRelease (line);
  CFRelease (string);
  CFRelease (attributes);
  CFRelease (text);
  CFRelease (font);
  CGFontRelease (cg_font);
  CGDataProviderRelease (provider);
  return 0;
}

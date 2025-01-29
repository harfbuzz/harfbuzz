/*
 * Copyright © 2010  Behdad Esfahbod
 * Copyright © 2011,2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#include <json_object.h>

#include <glib.h>
#include <stdio.h>

#include <hb.h>
#include <hb-ft.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

std::unordered_map<std::string, hb_font_t*> load_fonts_from_directory(const char* dir_path) {
    std::unordered_map<std::string, hb_font_t*> font_map;

    if (!dir_path) {
        std::cerr << "Error: Directory path is null." << std::endl;
        return font_map; // Return empty map
    }

        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".ttf") {
                std::string filename = entry.path().filename().string();
                std::string filepath = entry.path().string();

                // Extract SHA1 hash (filename without extension)
                std::string sha1_hash = filename.substr(0, filename.find_last_of('.'));

                hb_face_t* face = hb_face_create_from_file_or_fail(filepath.c_str(), 0);

                hb_font_t* hb_font = hb_font_create(face);

                font_map[sha1_hash] = hb_font;

            }
        }

    return font_map;
}


int main(int argc, char *argv[]) {
  gchar *sm3dump_dir = NULL;

  static GOptionEntry entries[] = {
    { "sm3dump", 's', 0, G_OPTION_ARG_STRING, &sm3dump_dir, "Directory for sm3dump", NULL },
    { NULL } // The array must be NULL terminated
  };

  GOptionContext *context = g_option_context_new("- a program to read line by line json arrays and font files to simulate shaping calls from Chrome");
  g_option_context_add_main_entries(context, entries, NULL);

  GError *error = NULL;
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("Option parsing failed: %s\n", error->message);
    g_error_free(error);
    g_option_context_free(context);
    return 1;
  }

  if (sm3dump_dir) {
    g_print("sm3dump directory: %s\n", sm3dump_dir);
    // Use the sm3dump_dir here
    // ... your code to process the directory ...
  } else {
    g_print("sm3dump option not specified.\n");
  }

  std::unordered_map<std::string, hb_font_t*> fonts = load_fonts_from_directory(sm3dump_dir);
}
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

#include <json.h>

#include <glib.h>
#include <stdio.h>
#include <cassert>

#include <hb.h>

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <memory>
#include <vector>

using FontsMap = std::unordered_map<std::string, hb_font_t*>;



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

                std::string sha1_hash = filename.substr(0, filename.find_last_of('.'));
                hb_face_t* face = hb_face_create_from_file_or_fail(filepath.c_str(), 0);
                hb_font_t* hb_font = hb_font_create(face);
                font_map[sha1_hash] = hb_font;

            }
        }

    return font_map;
}


// Structure to hold the data from each line
typedef struct {
    hb_font_t* font;
    hb_buffer_t* buffer;
} BenchData;

bool is_json_string(json_object* object) {
  return json_type::json_type_string == json_object_get_type(object);
}

bool is_json_array(json_object* object) {
 return json_type::json_type_array == json_object_get_type(object);
}

std::vector<BenchData> read_bench_data(std::string filename, const FontsMap& fonts) {
    std::vector<BenchData> font_data_list;

    std::ifstream fp(filename);
    if (!fp.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return font_data_list; // Return empty vector
    }

    std::string line_buffer;
    while (std::getline(fp, line_buffer)) {
        // line_buffer.pop_back();
        json_object* root = json_tokener_parse(line_buffer.c_str()); // Use c_str()
        if (!root) {
            std::cerr << "Error parsing JSON on line: " << line_buffer << std::endl;
            json_object_put(root); // If root is not null, release resources.
            continue; // Skip to the next line
        }

         if (!is_json_array(root)) {
        std::cerr << "Root is not a json array";
        json_object_put(root); // Release the JSON object
        continue;
    }

    for (size_t i = 0; i < json_object_array_length(root); i++) {
        json_object *item = json_object_array_get_idx(root, i);

        if(!item || json_type::json_type_object != json_object_get_type(item)) {
          std::cerr << "Array item is not dict: " << line_buffer << std::endl;
            continue; // Skip to the next line
        }

        json_object* fonthash_obj = json_object_object_get(item, "fonthash");
        json_object* serialized_buffer_obj = json_object_object_get(item, "buffer_serialized");

        if (!fonthash_obj || !serialized_buffer_obj ||
            !is_json_string(fonthash_obj) || !is_json_string(serialized_buffer_obj)) {
            std::cerr << "Invalid JSON format on line: " << line_buffer << std::endl;
            continue; // Skip to the next line
        }

        const char* fonthash = json_object_get_string(fonthash_obj);
        const char* serialized_buffer = json_object_get_string(serialized_buffer_obj);
        const int buffer_len = json_object_get_string_len(serialized_buffer_obj);


        hb_font_t* font = nullptr;
        auto find_result = fonts.find(fonthash);
        if (find_result == fonts.end()) {
          std::cerr << "Unmachted font for hash: " << fonthash;
          continue;
        }
        font = find_result->second;

        hb_buffer_t* buffer = hb_buffer_create();
        bool deser_success = hb_buffer_deserialize_unicode(buffer, serialized_buffer, buffer_len, nullptr, HB_BUFFER_SERIALIZE_FORMAT_JSON );
        if (!deser_success) {
          std::cerr << "Failed to deserialize buffer (len " << buffer_len <<") : " << serialized_buffer << "\n";
          continue;
        }

        font_data_list.push_back({font, buffer});
    }

        json_object_put(root); // Release the JSON object
    }

    fp.close();
    return font_data_list;
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

  if (!sm3dump_dir) {
    g_print("sm3dump option not specified.\n");
    return 1;
  }

  std::string jsonFilePath = sm3dump_dir + std::string("shapes.json");

  FontsMap fonts = load_fonts_from_directory(sm3dump_dir);
  std::vector<BenchData> bench_data = read_bench_data(jsonFilePath, fonts);

}
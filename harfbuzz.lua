project "harfbuzz"

dofile(_BUILD_DIR .. "/static_library.lua")

configuration {"*"}

uuid "4040B363-7AA3-415D-9E58-E0E0A74C2740"

defines {
    "_CRT_SECURE_NO_WARNINGS", 
    "HAVE_OT", 
    "HAVE_UCDN", 
    "_LIB"
}

includedirs {
    "src"
}

files {
    "./src/hb-aat-layout.cc",
    "./src/hb-face.cc",
    "./src/hb-fallback-shape.cc",
    "./src/hb-font.cc",
    "./src/hb-aat-map.cc",
    "./src/hb-number.cc",
    "./src/hb-ot-cff1-table.cc",
    "./src/hb-ot-cff2-table.cc",
    "./src/hb-ot-face.cc",
    "./src/hb-ot-font.cc",
    "./src/hb-ot-layout.cc",
    "./src/hb-ot-map.cc",
    "./src/hb-ot-metrics.cc",
    "./src/hb-ot-shape.cc",
    "./src/hb-ot-shape-complex-arabic.cc",
    "./src/hb-ot-shape-complex-default.cc",
    "./src/hb-ot-shape-complex-hangul.cc",
    "./src/hb-ot-shape-complex-hebrew.cc",
    "./src/hb-ot-shape-complex-indic.cc",
    "./src/hb-ot-shape-complex-indic-table.cc",
    "./src/hb-ot-shape-complex-khmer.cc",
    "./src/hb-buffer.cc",
    "./src/hb-ot-shape-complex-myanmar.cc",
    "./src/hb-ot-shape-complex-syllabic.cc",
    "./src/hb-ot-shape-complex-thai.cc",
    "./src/hb-ot-shape-complex-use.cc",
    "./src/hb-ot-shape-complex-vowel-constraints.cc",
    "./src/hb-ot-shape-fallback.cc",
    "./src/hb-ot-shape-normalize.cc",
    "./src/hb-ot-tag.cc",
    "./src/hb-ot-var.cc",
    "./src/hb-set.cc",
    "./src/hb-shape.cc",
    "./src/hb-shape-plan.cc",
    "./src/hb-shaper.cc",
    "./src/hb-common.cc",
    "./src/hb-ucd.cc",
    "./src/hb-unicode.cc",
    "./src/hb-blob.cc"
}

if (_PLATFORM_ANDROID) then 
    defines {
        "HB_NO_MT"
    } 
end

if (_PLATFORM_COCOA) then
    defines {
        "HAVE_CORETEXT"
    }

    files {
        "src/hb-coretext.cc"
    }
end

if (_PLATFORM_IOS) then
    defines {
        "HAVE_CORETEXT"
    }

    files {
        "src/hb-coretext.cc"
    }
end

if (_PLATFORM_LINUX) then 
    defines {
        "HB_NO_MT"
    } 
end

if (_PLATFORM_MACOS) then
    defines {
        "HAVE_CORETEXT"
    }

    files {
        "src/hb-coretext.cc"
    }
end

if (_PLATFORM_WINDOWS) then 
end

if (_PLATFORM_WINUWP) then 
end

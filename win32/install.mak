# NMake Makefile snippet for copying the built libraries, utilities and headers to
# a path under $(PREFIX).

install: all
	@if not exist $(PREFIX)\bin\ mkdir $(PREFIX)\bin
	@if not exist $(PREFIX)\lib\ mkdir $(PREFIX)\lib
	@if not exist $(PREFIX)\include\harfbuzz\ mkdir $(PREFIX)\include\harfbuzz
	@copy /b $(HARFBUZZ_DLL_FILENAME).dll $(PREFIX)\bin
	@copy /b $(HARFBUZZ_DLL_FILENAME).pdb $(PREFIX)\bin
	@copy /b $(CFG)\$(PLAT)\harfbuzz.lib $(PREFIX)\lib
	@if exist $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll copy /b $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll $(PREFIX)\bin
	@if exist $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll copy /b $(HARFBUZZ_GOBJECT_DLL_FILENAME).pdb $(PREFIX)\bin
	@if exist $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll copy /b $(CFG)\$(PLAT)\harfbuzz-gobject.lib $(PREFIX)\lib
	@if exist $(CFG)\$(PLAT)\hb-view.exe copy /b $(CFG)\$(PLAT)\hb-view.exe $(PREFIX)\bin
	@if exist $(CFG)\$(PLAT)\hb-view.exe copy /b $(CFG)\$(PLAT)\hb-view.pdb $(PREFIX)\bin
	@if exist $(CFG)\$(PLAT)\hb-ot-shape-closure.exe copy /b $(CFG)\$(PLAT)\hb-ot-shape-closure.exe $(PREFIX)\bin
	@if exist $(CFG)\$(PLAT)\hb-ot-shape-closure.exe copy /b $(CFG)\$(PLAT)\hb-ot-shape-closure.pdb $(PREFIX)\bin
	@if exist $(CFG)\$(PLAT)\hb-shape.exe copy /b $(CFG)\$(PLAT)\hb-shape.exe $(PREFIX)\bin
	@if exist $(CFG)\$(PLAT)\hb-shape.exe copy /b $(CFG)\$(PLAT)\hb-shape.pdb $(PREFIX)\bin
	@for %h in ($(HB_ACTUAL_HEADERS)) do @copy %h $(PREFIX)\include\harfbuzz
	@if exist $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll for %h in ($(HB_GOBJECT_headers)) do @copy ..\src\%h $(PREFIX)\include\harfbuzz
	@if exist $(HARFBUZZ_GOBJECT_DLL_FILENAME).dll copy $(CFG)\$(PLAT)\harfbuzz-gobject\hb-gobject-enums.h $(PREFIX)\include\harfbuzz
	@rem Copy the generated introspection files
	@if exist $(CFG)\$(PLAT)\HarfBuzz-0.0.gir copy $(CFG)\$(PLAT)\HarfBuzz-0.0.gir $(PREFIX)\share\gir-1.0
	@if exist $(CFG)\$(PLAT)\HarfBuzz-0.0.typelib copy /b $(CFG)\$(PLAT)\HarfBuzz-0.0.typelib $(PREFIX)\lib\girepository-1.0

# Process this file with automake to produce Makefile.in

manifests:
	@find "$(srcdir)/texts" -type d | while read d; do \
		echo "  GEN    $$d/MANIFEST"; \
	  (cd "$$d" && ls | grep -v MANIFEST | LANG=C sort > MANIFEST.tmp && mv MANIFEST.tmp MANIFEST || ($(RM) MANIFEST.tmp; false)); \
	done; true
.PHONY: manifests

-include $(top_srcdir)/git.mk

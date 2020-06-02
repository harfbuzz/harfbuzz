#!/usr/bin/env -S make -f

all: arabic-table emoji-table indic-table tag-table ucd-table use-table vowel-constraints

arabic-table: gen-arabic-table.py ArabicShaping.txt UnicodeData.txt Blocks.txt
	./$^ > hb-ot-shape-complex-arabic-table.hh || (rm hb-ot-shape-complex-arabic-table.hh; false)
emoji-table: gen-emoji-table.py emoji-data.txt
	./$^ > hb-unicode-emoji-table.hh || (rm hb-unicode-emoji-table.hh; false)
indic-table: gen-indic-table.py IndicSyllabicCategory.txt IndicPositionalCategory.txt Blocks.txt
	./$^ > hb-ot-shape-complex-indic-table.cc || (rm hb-ot-shape-complex-indic-table.cc; false)
tag-table: gen-tag-table.py languagetags language-subtag-registry
	./$^ > hb-ot-tag-table.hh || (rm hb-ot-tag-table.hh; false)
ucd-table: gen-ucd-table.py ucd.nounihan.grouped.zip hb-common.h
	./$^ > hb-ucd-table.hh || (rm hb-ucd-table.hh; false)
use-table: gen-use-table.py IndicSyllabicCategory.txt IndicPositionalCategory.txt UnicodeData.txt Blocks.txt
	./$^ > hb-ot-shape-complex-use-table.cc || (rm hb-ot-shape-complex-use-table.cc; false)
vowel-constraints: gen-vowel-constraints.py ms-use/IndicShapingInvalidCluster.txt Scripts.txt
	./$^ > hb-ot-shape-complex-vowel-constraints.cc || (hb-ot-shape-complex-vowel-constraints.cc; false)

packtab:
	python -c "import packTab" 2>/dev/null || /usr/bin/env pip3 install git+https://github.com/harfbuzz/packtab

.PHONY: packtab arabic-table indic-table tag-table use-table vowel-constraints emoji-table

ArabicShaping.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/ArabicShaping.txt

UnicodeData.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/UnicodeData.txt

Blocks.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/Blocks.txt

emoji-data.txt:
	wget https://www.unicode.org/Public/UCD/latest/ucd/emoji/emoji-data.txt

IndicSyllabicCategory.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/IndicSyllabicCategory.txt

IndicPositionalCategory.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/IndicPositionalCategory.txt

languagetags:
	wget https://docs.microsoft.com/en-us/typography/opentype/spec/languagetags

language-subtag-registry:
	wget https://www.iana.org/assignments/language-subtag-registry/language-subtag-registry

ucd.nounihan.grouped.zip:
	wget https://unicode.org/Public/UCD/latest/ucdxml/ucd.nounihan.grouped.zip

Scripts.txt:
	wget https://unicode.org/Public/UCD/latest/ucd/Scripts.txt

clean:
	rm -f \
		ArabicShaping.txt UnicodeData.txt Blocks.txt emoji-data.txt \
		IndicSyllabicCategory.txt IndicPositionalCategory.txt \
		languagetags language-subtag-registry ucd.nounihan.grouped.zip Scripts.txt

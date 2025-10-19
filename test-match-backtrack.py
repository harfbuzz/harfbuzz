#!/bin/env python3

from fontTools.ttLib import TTFont
import sys

perSyllableFeatures = {
    "abvf",
    "abvs",
    "akhn",
    "blwf",
    "blws",
    #"ccmp",
    "cjct",
    "half",
    "haln",
    "init",
    #"locl",
    "nukt",
    "pref",
    "pres",
    "pstf",
    "psts",
    "rkrf",
    "rphf",
    "vatu",
}


def test_match_backtrack(font):
    errors = set()
    hasLigatureOrMultipleSubst = False

    if "GSUB" not in font:
        return errors
    gsub = font["GSUB"].table
    if not gsub.LookupList:
        return errors
    lookups = gsub.LookupList.Lookup
    if not gsub.FeatureList:
        return errors
    features = gsub.FeatureList.FeatureRecord
    for feature in features:
        if feature.FeatureTag not in perSyllableFeatures:
            continue
        for lookup_index in feature.Feature.LookupListIndex:
            lookup = lookups[lookup_index]

            hasBacktrack = False
            recursedLookups = set()

            for subtable in lookup.SubTable:
                if subtable.__class__.__name__ == "ExtensionSubst":
                    subtable = subtable.ExtSubTable
                if subtable.__class__.__name__ != "ChainContextSubst":
                    continue

                if subtable.Format == 1:
                    for rules in subtable.ChainSubRuleSet:
                        if rules is None:
                            continue
                        for rule in rules.ChainSubRule:
                            if rule is None:
                                continue
                            if rule.Backtrack:
                                hasBacktrack = True
                            for lookupRecord in rule.SubstLookupRecord:
                                recursedLookups.add(lookupRecord.LookupListIndex)

                elif subtable.Format == 2:
                    for rules in subtable.ChainSubClassSet:
                        if rules is None:
                            continue
                        for rule in rules.ChainSubClassRule:
                            if rule is None:
                                continue
                            if rule.Backtrack:
                                hasBacktrack = True
                            for lookupRecord in rule.SubstLookupRecord:
                                recursedLookups.add(lookupRecord.LookupListIndex)

                elif subtable.Format == 3:
                    if subtable.BacktrackCoverage:
                        hasBacktrack = True
                    for lookupRecord in subtable.SubstLookupRecord:
                        recursedLookups.add(lookupRecord.LookupListIndex)

            if not hasBacktrack:
                continue

            for recursed_lookup_index in recursedLookups:
                recursed_lookup = lookups[recursed_lookup_index]
                for subtable in recursed_lookup.SubTable:
                    if subtable.__class__.__name__ == "ExtensionSubst":
                        subtable = subtable.ExtSubTable
                    if subtable.__class__.__name__ in (
                        "LigatureSubst",
                        "MultipleSubst",
                    ):
                        errors.add(feature.FeatureTag)

    return errors


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python test-match-backtrack.py <font-file>...")
        sys.exit(1)

    for font_path in sys.argv[1:]:
        font = TTFont(font_path)
        errors = test_match_backtrack(font)
        if errors:
            print(f"{font_path}: {errors}")

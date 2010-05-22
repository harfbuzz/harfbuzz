cdef extern from "fontconfig/fontconfig.h" :
    ctypedef struct FcPattern :
        pass
    ctypedef struct FcConfig :
        pass
    cdef enum FcResult '_FcResult' :
        FcResultMatch = 0, FcResultNoMatch, FcResultTypeMismatch, FcResultNoId,
        FcResultOutOfMemory

    ctypedef char FcChar8
    FcPattern *FcNameParse(FcChar8 *name)
    FcPattern *FcFontMatch(FcConfig *config, FcPattern *match, FcResult *res)
    FcResult FcPatternGetInteger(FcPattern *pattern, char *typeid, int index, int *res)
    FcResult FcPatternGetString(FcPattern *pattern, char *typeid, int index, FcChar8 **res)
    void FcPatternPrint(FcPattern *pattern)
    void FcPatternDestroy(FcPattern *pattern)

    FcConfig *FcConfigGetCurrent()

cdef class fcPattern :
    cdef FcPattern *_pattern

    def __init__(self, char *name) :
        cdef FcPattern *temp
        cdef FcResult res

        temp = FcNameParse(<FcChar8 *>name)
        self._pattern = FcFontMatch(FcConfigGetCurrent(), temp, &res)
        if res != FcResultMatch :
            print "Failed to match" + str(res)
            self._pattern = <FcPattern *>0

    def __destroy__(self) :
        FcPatternDestroy(self._pattern)

    def getInteger(self, char *typeid, int index) :
        cdef int res
        if self._pattern == <FcPattern *>0 or FcPatternGetInteger(self._pattern, typeid, index, &res) != FcResultMatch : return None
        return res

    def getString(self, char *typeid, int index) :
        cdef FcChar8 *res
        if self._pattern == <FcPattern *>0 or FcPatternGetString(self._pattern, typeid, index, &res) != FcResultMatch : return None
        return <char *>res

    def debugPrint(self) :
        FcPatternPrint(self._pattern)

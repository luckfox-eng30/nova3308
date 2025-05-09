cmake_minimum_required( VERSION 2.8.8 )

set(
    LIB_UCHARDET_HEADERS
    uchardet.h
)

include_directories(src)

set(SOURCE_FILES_UCHARDET
    src/CharDistribution.cpp
    src/JpCntx.cpp
    src/LangModels/LangArabicModel.cpp
    src/LangModels/LangBulgarianModel.cpp
    src/LangModels/LangCroatianModel.cpp
    src/LangModels/LangCzechModel.cpp
    src/LangModels/LangEsperantoModel.cpp
    src/LangModels/LangEstonianModel.cpp
    src/LangModels/LangFinnishModel.cpp
    src/LangModels/LangFrenchModel.cpp
    src/LangModels/LangDanishModel.cpp
    src/LangModels/LangGermanModel.cpp
    src/LangModels/LangGreekModel.cpp
    src/LangModels/LangHungarianModel.cpp
    src/LangModels/LangHebrewModel.cpp
    src/LangModels/LangIrishModel.cpp
    src/LangModels/LangItalianModel.cpp
    src/LangModels/LangLithuanianModel.cpp
    src/LangModels/LangLatvianModel.cpp
    src/LangModels/LangMalteseModel.cpp
    src/LangModels/LangPolishModel.cpp
    src/LangModels/LangPortugueseModel.cpp
    src/LangModels/LangRomanianModel.cpp
    src/LangModels/LangRussianModel.cpp
    src/LangModels/LangSlovakModel.cpp
    src/LangModels/LangSloveneModel.cpp
    src/LangModels/LangSwedishModel.cpp
    src/LangModels/LangSpanishModel.cpp
    src/LangModels/LangThaiModel.cpp
    src/LangModels/LangTurkishModel.cpp
    src/LangModels/LangVietnameseModel.cpp
    src/nsHebrewProber.cpp
    src/nsCharSetProber.cpp
    src/nsBig5Prober.cpp
    src/nsEUCJPProber.cpp
    src/nsEUCKRProber.cpp
    src/nsEUCTWProber.cpp
    src/nsEscCharsetProber.cpp
    src/nsEscSM.cpp
    src/nsGB2312Prober.cpp
    src/nsMBCSGroupProber.cpp
    src/nsMBCSSM.cpp
    src/nsSBCSGroupProber.cpp
    src/nsSBCharSetProber.cpp
    src/nsSJISProber.cpp
    src/nsUTF8Prober.cpp
    src/nsLatin1Prober.cpp
    src/nsUniversalDetector.cpp
    uchardet.cpp
)

add_library(${LIB_STATIC_UCHARDET} STATIC ${SOURCE_FILES_UCHARDET})
add_definitions(-std=c++11 -Wall -O0 -g3)
set_target_properties(${LIB_STATIC_UCHARDET} PROPERTIES FOLDER "third-party/uchardet")

message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/Uchardet")





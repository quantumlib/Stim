#include "stim/util_top/missing_detectors.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/util_bot/test_util.test.h"
#include "stim/util_top/has_flow.h"

using namespace stim;

TEST(missing_detectors, circuit) {
    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        R 0
        M 0
        DETECTOR rec[-1]
        DETECTOR rec[-1]
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        R 0
        M 0
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-1]
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        M 0
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-1]
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        M 0
    )CIRCUIT"),
            true),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        R 0 1
        M 0 1
        DETECTOR rec[-1]
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-2]
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        MPP Z0*Z1 X0*X1
        TICK
        MPP Z0*Z1 X0*X1
        DETECTOR rec[-1] rec[-3]
        DETECTOR rec[-2] rec[-4]
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-4]
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        MPP Z0*Z1 X0*X1
        TICK
        MPP Z0*Z1 X0*X1
        DETECTOR rec[-1] rec[-3]
        DETECTOR rec[-2] rec[-4]
        DETECTOR rec[-1] rec[-3] rec[-2] rec[-4]
    )CIRCUIT"),
            false),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-3] rec[-2] rec[-1]
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        MPP Z0*Z1 X0*X1
        TICK
        MPP Z0*Z1 X0*X1
        DETECTOR rec[-1] rec[-3]
        DETECTOR rec[-2] rec[-4]
    )CIRCUIT"),
            true),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        MPP Z0*Z1 X0*X1
        TICK
        MPP Z0*Z1 X0*X1
        OBSERVABLE_INCLUDE(0) rec[-1]
        DETECTOR rec[-2] rec[-4]
        OBSERVABLE_INCLUDE(0) rec[-3]
    )CIRCUIT"),
            true),
        Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        missing_detectors(
            Circuit(R"CIRCUIT(
        OBSERVABLE_INCLUDE(0) Z0 Z1
        MPP Z0*Z1 X0*X1
        TICK
        MPP Z0*Z1 X0*X1
        OBSERVABLE_INCLUDE(0) Z0 Z1
        OBSERVABLE_INCLUDE(0) rec[-1]
        DETECTOR rec[-2] rec[-4]
        OBSERVABLE_INCLUDE(0) rec[-3]
    )CIRCUIT"),
            true),
        Circuit(R"CIRCUIT(
        DETECTOR rec[-3] rec[-1]
    )CIRCUIT"));
}

TEST(missing_detectors, big_case_honeycomb_code) {
    Circuit c = Circuit(R"CIRCUIT(
        R 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 48 49 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44
        DETECTOR rec[-32] rec[-33] rec[-49]
        DETECTOR rec[-29] rec[-30] rec[-40]
        DETECTOR rec[-25] rec[-27] rec[-55]
        DETECTOR rec[-21] rec[-22] rec[-23] rec[-34] rec[-41]
        DETECTOR rec[-20] rec[-28] rec[-31] rec[-37] rec[-60]
        DETECTOR rec[-9] rec[-24] rec[-26] rec[-46] rec[-50]
        DETECTOR rec[-7] rec[-8] rec[-15] rec[-39] rec[-44] rec[-45]
        DETECTOR rec[-5] rec[-6] rec[-14] rec[-38] rec[-42] rec[-43]
        DETECTOR rec[-4] rec[-12] rec[-13] rec[-48] rec[-53] rec[-54]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-47] rec[-51] rec[-52]
        DETECTOR rec[-2] rec[-18] rec[-19] rec[-36] rec[-58] rec[-59]
        DETECTOR rec[-1] rec[-16] rec[-17] rec[-35] rec[-56] rec[-57]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-26] rec[-27] rec[-30] rec[-34] rec[-39] rec[-108] rec[-119] rec[-120]
        DETECTOR rec[-14] rec[-24] rec[-25] rec[-29] rec[-37] rec[-38] rec[-107] rec[-117] rec[-118]
        DETECTOR rec[-13] rec[-22] rec[-23] rec[-28] rec[-35] rec[-36] rec[-106] rec[-115] rec[-116]
        DETECTOR rec[-4] rec[-7] rec[-12] rec[-41] rec[-42] rec[-44] rec[-97] rec[-100] rec[-105]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-43] rec[-47] rec[-48] rec[-96] rec[-103] rec[-104]
        DETECTOR rec[-2] rec[-8] rec[-9] rec[-40] rec[-45] rec[-46] rec[-95] rec[-101] rec[-102]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        DETECTOR rec[-26] rec[-27] rec[-86] rec[-87]
        DETECTOR rec[-24] rec[-25] rec[-84] rec[-85]
        DETECTOR rec[-22] rec[-23] rec[-82] rec[-83]
        DETECTOR rec[-16] rec[-20] rec[-21] rec[-76] rec[-80] rec[-81]
        DETECTOR rec[-14] rec[-15] rec[-17] rec[-74] rec[-75] rec[-77]
        DETECTOR rec[-13] rec[-18] rec[-19] rec[-73] rec[-78] rec[-79]
        DETECTOR rec[-6] rec[-30] rec[-31] rec[-66] rec[-90] rec[-91]
        DETECTOR rec[-5] rec[-32] rec[-33] rec[-65] rec[-92] rec[-93]
        DETECTOR rec[-4] rec[-28] rec[-29] rec[-64] rec[-88] rec[-89]
        DETECTOR rec[-3] rec[-7] rec[-12] rec[-63] rec[-67] rec[-72]
        DETECTOR rec[-2] rec[-10] rec[-11] rec[-62] rec[-70] rec[-71]
        DETECTOR rec[-1] rec[-8] rec[-9] rec[-61] rec[-68] rec[-69]
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 48 49 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44
        DETECTOR rec[-30] rec[-31] rec[-47] rec[-107] rec[-156] rec[-157]
        DETECTOR rec[-26] rec[-27] rec[-41] rec[-101] rec[-152] rec[-153]
        DETECTOR rec[-16] rec[-23] rec[-25] rec[-34] rec[-51] rec[-94] rec[-111] rec[-142] rec[-149] rec[-151]
        DETECTOR rec[-15] rec[-19] rec[-20] rec[-36] rec[-50] rec[-54] rec[-96] rec[-110] rec[-114] rec[-141] rec[-145] rec[-146]
        DETECTOR rec[-14] rec[-17] rec[-18] rec[-35] rec[-52] rec[-53] rec[-95] rec[-112] rec[-113] rec[-140] rec[-143] rec[-144]
        DETECTOR rec[-13] rec[-28] rec[-32] rec[-38] rec[-40] rec[-98] rec[-100] rec[-139] rec[-154] rec[-158]
        DETECTOR rec[-2] rec[-11] rec[-12] rec[-39] rec[-44] rec[-45] rec[-99] rec[-104] rec[-105] rec[-128] rec[-137] rec[-138]
        DETECTOR rec[-1] rec[-9] rec[-10] rec[-37] rec[-42] rec[-43] rec[-97] rec[-102] rec[-103] rec[-127] rec[-135] rec[-136]
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-20] rec[-21] rec[-31] rec[-39] rec[-40] rec[-157] rec[-165] rec[-166] rec[-201] rec[-206] rec[-207]
        DETECTOR rec[-14] rec[-18] rec[-19] rec[-30] rec[-37] rec[-38] rec[-156] rec[-163] rec[-164] rec[-200] rec[-204] rec[-205]
        DETECTOR rec[-6] rec[-11] rec[-12] rec[-34] rec[-35] rec[-42] rec[-160] rec[-161] rec[-168] rec[-192] rec[-197] rec[-198]
        DETECTOR rec[-5] rec[-9] rec[-10] rec[-32] rec[-33] rec[-41] rec[-158] rec[-159] rec[-167] rec[-191] rec[-195] rec[-196]
        DETECTOR rec[-3] rec[-25] rec[-26] rec[-29] rec[-45] rec[-46] rec[-155] rec[-171] rec[-172] rec[-189] rec[-211] rec[-212]
        DETECTOR rec[-2] rec[-23] rec[-24] rec[-28] rec[-43] rec[-44] rec[-154] rec[-169] rec[-170] rec[-188] rec[-209] rec[-210]
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 48 49 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44
        DETECTOR rec[-32] rec[-33] rec[-92] rec[-93]
        DETECTOR rec[-29] rec[-30] rec[-89] rec[-90]
        DETECTOR rec[-25] rec[-27] rec[-85] rec[-87]
        DETECTOR rec[-21] rec[-22] rec[-23] rec[-81] rec[-82] rec[-83]
        DETECTOR rec[-20] rec[-28] rec[-31] rec[-80] rec[-88] rec[-91]
        DETECTOR rec[-9] rec[-24] rec[-26] rec[-69] rec[-84] rec[-86]
        DETECTOR rec[-7] rec[-8] rec[-15] rec[-67] rec[-68] rec[-75]
        DETECTOR rec[-5] rec[-6] rec[-14] rec[-65] rec[-66] rec[-74]
        DETECTOR rec[-4] rec[-12] rec[-13] rec[-64] rec[-72] rec[-73]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-63] rec[-70] rec[-71]
        DETECTOR rec[-2] rec[-18] rec[-19] rec[-62] rec[-78] rec[-79]
        DETECTOR rec[-1] rec[-16] rec[-17] rec[-61] rec[-76] rec[-77]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        DETECTOR rec[-31] rec[-32] rec[-37] rec[-97] rec[-157] rec[-158]
        DETECTOR rec[-29] rec[-30] rec[-36] rec[-96] rec[-155] rec[-156]
        DETECTOR rec[-16] rec[-25] rec[-26] rec[-39] rec[-40] rec[-99] rec[-100] rec[-142] rec[-151] rec[-152]
        DETECTOR rec[-13] rec[-23] rec[-24] rec[-38] rec[-54] rec[-98] rec[-114] rec[-139] rec[-149] rec[-150]
        DETECTOR rec[-6] rec[-11] rec[-12] rec[-35] rec[-44] rec[-45] rec[-95] rec[-104] rec[-105] rec[-132] rec[-137] rec[-138]
        DETECTOR rec[-4] rec[-9] rec[-10] rec[-34] rec[-42] rec[-43] rec[-94] rec[-102] rec[-103] rec[-130] rec[-135] rec[-136]
        DETECTOR rec[-3] rec[-17] rec[-21] rec[-48] rec[-52] rec[-53] rec[-108] rec[-112] rec[-113] rec[-129] rec[-143] rec[-147]
        DETECTOR rec[-2] rec[-19] rec[-20] rec[-47] rec[-50] rec[-51] rec[-107] rec[-110] rec[-111] rec[-128] rec[-145] rec[-146]
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-26] rec[-27] rec[-30] rec[-34] rec[-39] rec[-156] rec[-160] rec[-165] rec[-201] rec[-212] rec[-213]
        DETECTOR rec[-14] rec[-24] rec[-25] rec[-29] rec[-37] rec[-38] rec[-155] rec[-163] rec[-164] rec[-200] rec[-210] rec[-211]
        DETECTOR rec[-13] rec[-22] rec[-23] rec[-28] rec[-35] rec[-36] rec[-154] rec[-161] rec[-162] rec[-199] rec[-208] rec[-209]
        DETECTOR rec[-4] rec[-7] rec[-12] rec[-41] rec[-42] rec[-44] rec[-167] rec[-168] rec[-170] rec[-190] rec[-193] rec[-198]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-43] rec[-47] rec[-48] rec[-169] rec[-173] rec[-174] rec[-189] rec[-196] rec[-197]
        DETECTOR rec[-2] rec[-8] rec[-9] rec[-40] rec[-45] rec[-46] rec[-166] rec[-171] rec[-172] rec[-188] rec[-194] rec[-195]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        DETECTOR rec[-26] rec[-27] rec[-86] rec[-87]
        DETECTOR rec[-24] rec[-25] rec[-84] rec[-85]
        DETECTOR rec[-22] rec[-23] rec[-82] rec[-83]
        DETECTOR rec[-16] rec[-20] rec[-21] rec[-76] rec[-80] rec[-81]
        # OOPS DETECTOR rec[-14] rec[-15] rec[-17] rec[-74] rec[-75] rec[-77]
        DETECTOR rec[-13] rec[-18] rec[-19] rec[-73] rec[-78] rec[-79]
        DETECTOR rec[-6] rec[-30] rec[-31] rec[-66] rec[-90] rec[-91]
        DETECTOR rec[-5] rec[-32] rec[-33] rec[-65] rec[-92] rec[-93]
        DETECTOR rec[-4] rec[-28] rec[-29] rec[-64] rec[-88] rec[-89]
        DETECTOR rec[-3] rec[-7] rec[-12] rec[-63] rec[-67] rec[-72]
        DETECTOR rec[-2] rec[-10] rec[-11] rec[-62] rec[-70] rec[-71]
        DETECTOR rec[-1] rec[-8] rec[-9] rec[-61] rec[-68] rec[-69]
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 48 49 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44
        DETECTOR rec[-30] rec[-31] rec[-47] rec[-107] rec[-156] rec[-157]
        DETECTOR rec[-26] rec[-27] rec[-41] rec[-101] rec[-152] rec[-153]
        DETECTOR rec[-16] rec[-23] rec[-25] rec[-34] rec[-51] rec[-94] rec[-111] rec[-142] rec[-149] rec[-151]
        DETECTOR rec[-15] rec[-19] rec[-20] rec[-36] rec[-50] rec[-54] rec[-96] rec[-110] rec[-114] rec[-141] rec[-145] rec[-146]
        DETECTOR rec[-14] rec[-17] rec[-18] rec[-35] rec[-52] rec[-53] rec[-95] rec[-112] rec[-113] rec[-140] rec[-143] rec[-144]
        DETECTOR rec[-13] rec[-28] rec[-32] rec[-38] rec[-40] rec[-98] rec[-100] rec[-139] rec[-154] rec[-158]
        DETECTOR rec[-2] rec[-11] rec[-12] rec[-39] rec[-44] rec[-45] rec[-99] rec[-104] rec[-105] rec[-128] rec[-137] rec[-138]
        DETECTOR rec[-1] rec[-9] rec[-10] rec[-37] rec[-42] rec[-43] rec[-97] rec[-102] rec[-103] rec[-127] rec[-135] rec[-136]
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-20] rec[-21] rec[-31] rec[-39] rec[-40] rec[-157] rec[-165] rec[-166] rec[-201] rec[-206] rec[-207]
        DETECTOR rec[-14] rec[-18] rec[-19] rec[-30] rec[-37] rec[-38] rec[-156] rec[-163] rec[-164] rec[-200] rec[-204] rec[-205]
        DETECTOR rec[-6] rec[-11] rec[-12] rec[-34] rec[-35] rec[-42] rec[-160] rec[-161] rec[-168] rec[-192] rec[-197] rec[-198]
        DETECTOR rec[-5] rec[-9] rec[-10] rec[-32] rec[-33] rec[-41] rec[-158] rec[-159] rec[-167] rec[-191] rec[-195] rec[-196]
        DETECTOR rec[-3] rec[-25] rec[-26] rec[-29] rec[-45] rec[-46] rec[-155] rec[-171] rec[-172] rec[-189] rec[-211] rec[-212]
        DETECTOR rec[-2] rec[-23] rec[-24] rec[-28] rec[-43] rec[-44] rec[-154] rec[-169] rec[-170] rec[-188] rec[-209] rec[-210]
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 48 49 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44
        DETECTOR rec[-32] rec[-33] rec[-92] rec[-93]
        DETECTOR rec[-29] rec[-30] rec[-89] rec[-90]
        DETECTOR rec[-25] rec[-27] rec[-85] rec[-87]
        DETECTOR rec[-21] rec[-22] rec[-23] rec[-81] rec[-82] rec[-83]
        DETECTOR rec[-20] rec[-28] rec[-31] rec[-80] rec[-88] rec[-91]
        DETECTOR rec[-9] rec[-24] rec[-26] rec[-69] rec[-84] rec[-86]
        DETECTOR rec[-7] rec[-8] rec[-15] rec[-67] rec[-68] rec[-75]
        DETECTOR rec[-5] rec[-6] rec[-14] rec[-65] rec[-66] rec[-74]
        DETECTOR rec[-4] rec[-12] rec[-13] rec[-64] rec[-72] rec[-73]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-63] rec[-70] rec[-71]
        DETECTOR rec[-2] rec[-18] rec[-19] rec[-62] rec[-78] rec[-79]
        DETECTOR rec[-1] rec[-16] rec[-17] rec[-61] rec[-76] rec[-77]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        DETECTOR rec[-31] rec[-32] rec[-37] rec[-97] rec[-157] rec[-158]
        DETECTOR rec[-29] rec[-30] rec[-36] rec[-96] rec[-155] rec[-156]
        DETECTOR rec[-16] rec[-25] rec[-26] rec[-39] rec[-40] rec[-99] rec[-100] rec[-142] rec[-151] rec[-152]
        DETECTOR rec[-13] rec[-23] rec[-24] rec[-38] rec[-54] rec[-98] rec[-114] rec[-139] rec[-149] rec[-150]
        DETECTOR rec[-6] rec[-11] rec[-12] rec[-35] rec[-44] rec[-45] rec[-95] rec[-104] rec[-105] rec[-132] rec[-137] rec[-138]
        DETECTOR rec[-4] rec[-9] rec[-10] rec[-34] rec[-42] rec[-43] rec[-94] rec[-102] rec[-103] rec[-130] rec[-135] rec[-136]
        DETECTOR rec[-3] rec[-17] rec[-21] rec[-48] rec[-52] rec[-53] rec[-108] rec[-112] rec[-113] rec[-129] rec[-143] rec[-147]
        DETECTOR rec[-2] rec[-19] rec[-20] rec[-47] rec[-50] rec[-51] rec[-107] rec[-110] rec[-111] rec[-128] rec[-145] rec[-146]
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-26] rec[-27] rec[-30] rec[-34] rec[-39] rec[-156] rec[-160] rec[-165] rec[-201] rec[-212] rec[-213]
        DETECTOR rec[-14] rec[-24] rec[-25] rec[-29] rec[-37] rec[-38] rec[-155] rec[-163] rec[-164] rec[-200] rec[-210] rec[-211]
        DETECTOR rec[-13] rec[-22] rec[-23] rec[-28] rec[-35] rec[-36] rec[-154] rec[-161] rec[-162] rec[-199] rec[-208] rec[-209]
        DETECTOR rec[-4] rec[-7] rec[-12] rec[-41] rec[-42] rec[-44] rec[-167] rec[-168] rec[-170] rec[-190] rec[-193] rec[-198]
        DETECTOR rec[-3] rec[-10] rec[-11] rec[-43] rec[-47] rec[-48] rec[-169] rec[-173] rec[-174] rec[-189] rec[-196] rec[-197]
        DETECTOR rec[-2] rec[-8] rec[-9] rec[-40] rec[-45] rec[-46] rec[-166] rec[-171] rec[-172] rec[-188] rec[-194] rec[-195]
        TICK
        M 0 5 14 23 32 41 13 22 31 40 49 53
        MZZ 19 20 28 29 37 38 46 47 10 11 21 30 4 12 2 3 39 48 16 17 25 26 34 35 43 44 50 51 7 8 15 24 1 6 33 42 9 18 27 36 45 52
        DETECTOR rec[-26] rec[-27] rec[-86] rec[-87]
        DETECTOR rec[-24] rec[-25] rec[-84] rec[-85]
        DETECTOR rec[-22] rec[-23] rec[-82] rec[-83]
        DETECTOR rec[-16] rec[-20] rec[-21] rec[-76] rec[-80] rec[-81]
        DETECTOR rec[-14] rec[-15] rec[-17] rec[-74] rec[-75] rec[-77]
        DETECTOR rec[-13] rec[-18] rec[-19] rec[-73] rec[-78] rec[-79]
        DETECTOR rec[-6] rec[-30] rec[-31] rec[-66] rec[-90] rec[-91]
        DETECTOR rec[-5] rec[-32] rec[-33] rec[-65] rec[-92] rec[-93]
        DETECTOR rec[-4] rec[-28] rec[-29] rec[-64] rec[-88] rec[-89]
        DETECTOR rec[-3] rec[-7] rec[-12] rec[-63] rec[-67] rec[-72]
        DETECTOR rec[-2] rec[-10] rec[-11] rec[-62] rec[-70] rec[-71]
        DETECTOR rec[-1] rec[-8] rec[-9] rec[-61] rec[-68] rec[-69]
        TICK
        MX 0 1 2 3 4 8 51 50 52 41 47 53
        MXX 9 10 18 19 27 28 36 37 45 46 11 20 29 38 6 7 15 16 24 25 33 34 42 43 12 13 21 22 30 31 39 40 5 14 23 32 17 26 35 44 48 49
        DETECTOR rec[-30] rec[-31] rec[-47] rec[-107] rec[-156] rec[-157]
        DETECTOR rec[-26] rec[-27] rec[-41] rec[-101] rec[-152] rec[-153]
        DETECTOR rec[-17] rec[-23] rec[-25] rec[-34] rec[-51] rec[-94] rec[-111] rec[-142] rec[-149] rec[-151]
        DETECTOR rec[-16] rec[-20] rec[-21] rec[-36] rec[-50] rec[-54] rec[-96] rec[-110] rec[-114] rec[-141] rec[-145] rec[-146]
        DETECTOR rec[-15] rec[-18] rec[-19] rec[-35] rec[-52] rec[-53] rec[-95] rec[-112] rec[-113] rec[-140] rec[-143] rec[-144]
        DETECTOR rec[-14] rec[-28] rec[-32] rec[-38] rec[-40] rec[-98] rec[-100] rec[-139] rec[-154] rec[-158]
        DETECTOR rec[-3] rec[-12] rec[-13] rec[-39] rec[-44] rec[-45] rec[-99] rec[-104] rec[-105] rec[-128] rec[-137] rec[-138]
        DETECTOR rec[-2] rec[-10] rec[-11] rec[-37] rec[-42] rec[-43] rec[-97] rec[-102] rec[-103] rec[-127] rec[-135] rec[-136]
        TICK
        MYY 8 9 17 18 26 27 35 36 44 45 51 52 5 6 14 15 23 24 32 33 41 42 0 1 7 16 25 34 43 50 11 12 20 21 29 30 38 39 47 48 3 4 13 22 31 40 2 10 19 28 37 46 49 53
        DETECTOR rec[-15] rec[-20] rec[-21] rec[-32] rec[-40] rec[-41] rec[-157] rec[-165] rec[-166] rec[-201] rec[-206] rec[-207]
        DETECTOR rec[-14] rec[-18] rec[-19] rec[-31] rec[-38] rec[-39] rec[-156] rec[-163] rec[-164] rec[-200] rec[-204] rec[-205]
        DETECTOR rec[-6] rec[-11] rec[-12] rec[-35] rec[-36] rec[-43] rec[-160] rec[-161] rec[-168] rec[-192] rec[-197] rec[-198]
        DETECTOR rec[-5] rec[-9] rec[-10] rec[-33] rec[-34] rec[-42] rec[-158] rec[-159] rec[-167] rec[-191] rec[-195] rec[-196]
        DETECTOR rec[-3] rec[-25] rec[-26] rec[-30] rec[-46] rec[-47] rec[-155] rec[-171] rec[-172] rec[-189] rec[-211] rec[-212]
        DETECTOR rec[-2] rec[-23] rec[-24] rec[-29] rec[-44] rec[-45] rec[-154] rec[-169] rec[-170] rec[-188] rec[-209] rec[-210]
        TICK
        M 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53
        DETECTOR rec[-53] rec[-54] rec[-70] rec[-113] rec[-114]
        DETECTOR rec[-50] rec[-51] rec[-61] rec[-110] rec[-111]
        DETECTOR rec[-44] rec[-45] rec[-46] rec[-52] rec[-58] rec[-81] rec[-102] rec[-109] rec[-112]
        DETECTOR rec[-42] rec[-43] rec[-44] rec[-50] rec[-51] rec[-52] rec[-58] rec[-61] rec[-66] rec[-128] rec[-129] rec[-131] rec[-151] rec[-154] rec[-159]
        DETECTOR rec[-38] rec[-39] rec[-40] rec[-47] rec[-48] rec[-49] rec[-69] rec[-74] rec[-75] rec[-86] rec[-94] rec[-95]
        DETECTOR rec[-36] rec[-37] rec[-38] rec[-45] rec[-46] rec[-47] rec[-69] rec[-80] rec[-81] rec[-117] rec[-121] rec[-126] rec[-162] rec[-173] rec[-174]
        DETECTOR rec[-32] rec[-33] rec[-34] rec[-41] rec[-42] rec[-43] rec[-60] rec[-65] rec[-66] rec[-89] rec[-90] rec[-97]
        DETECTOR rec[-26] rec[-27] rec[-28] rec[-35] rec[-36] rec[-37] rec[-57] rec[-79] rec[-80] rec[-84] rec[-100] rec[-101]
        DETECTOR rec[-24] rec[-25] rec[-26] rec[-33] rec[-34] rec[-35] rec[-57] rec[-64] rec[-65] rec[-130] rec[-134] rec[-135] rec[-150] rec[-157] rec[-158]
        DETECTOR rec[-20] rec[-21] rec[-22] rec[-29] rec[-30] rec[-31] rec[-68] rec[-72] rec[-73] rec[-85] rec[-92] rec[-93]
        DETECTOR rec[-18] rec[-19] rec[-20] rec[-27] rec[-28] rec[-29] rec[-68] rec[-78] rec[-79] rec[-116] rec[-124] rec[-125] rec[-161] rec[-171] rec[-172]
        DETECTOR rec[-14] rec[-15] rec[-16] rec[-23] rec[-24] rec[-25] rec[-59] rec[-63] rec[-64] rec[-87] rec[-88] rec[-96]
        DETECTOR rec[-8] rec[-9] rec[-10] rec[-17] rec[-18] rec[-19] rec[-56] rec[-77] rec[-78] rec[-83] rec[-98] rec[-99]
        DETECTOR rec[-6] rec[-7] rec[-8] rec[-15] rec[-16] rec[-17] rec[-56] rec[-62] rec[-63] rec[-127] rec[-132] rec[-133] rec[-149] rec[-155] rec[-156]
        DETECTOR rec[-4] rec[-11] rec[-12] rec[-13] rec[-67] rec[-71] rec[-91] rec[-105] rec[-107]
        DETECTOR rec[-2] rec[-3] rec[-76] rec[-106] rec[-108]
        DETECTOR rec[-2] rec[-3] rec[-4] rec[-9] rec[-10] rec[-11] rec[-67] rec[-76] rec[-77] rec[-115] rec[-122] rec[-123] rec[-160] rec[-169] rec[-170]
        DETECTOR rec[-1] rec[-5] rec[-6] rec[-7] rec[-55] rec[-62] rec[-82] rec[-103] rec[-104]
        OBSERVABLE_INCLUDE(0) rec[-2] rec[-3] rec[-9] rec[-10] rec[-18] rec[-19] rec[-27] rec[-28] rec[-36] rec[-37] rec[-45] rec[-46] rec[-76] rec[-77] rec[-78] rec[-79] rec[-80] rec[-81] rec[-83] rec[-84] rec[-108] rec[-109] rec[-115] rec[-116] rec[-117] rec[-175] rec[-176] rec[-177] rec[-208] rec[-209] rec[-234] rec[-235] rec[-268] rec[-269] rec[-294] rec[-295] rec[-301] rec[-302] rec[-303] rec[-361] rec[-362] rec[-363] rec[-394] rec[-395] rec[-420] rec[-421] rec[-454] rec[-455] rec[-480] rec[-481] rec[-487] rec[-488] rec[-489] rec[-547] rec[-548] rec[-549] rec[-580] rec[-581] rec[-606] rec[-607] rec[-634] rec[-635] rec[-636] rec[-637] rec[-638] rec[-639]
    )CIRCUIT");
    Circuit suffix = missing_detectors(c, true);
    ASSERT_EQ(suffix, Circuit(R"CIRCUIT(
        DETECTOR rec[-377] rec[-375] rec[-374] rec[-317] rec[-315] rec[-314]
    )CIRCUIT"));
}

TEST(missing_detectors, toric_code_global_stabilizer_product) {
    Circuit c = Circuit(R"CIRCUIT(
        R 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
        TICK
        MPP X0*X4*X5*X1 X2*X6*X7*X3 X10*X14*X15*X11 X8*X12*X13*X9
        TICK
        MPP X5*X9*X10*X6 X1*X13*X14*X2 X0*X12*X15*X3 X4*X8*X11*X7
        TICK
        MPP Z4*Z8*Z9*Z5 Z6*Z10*Z11*Z7 Z2*Z14*Z15*Z3 Z0*Z12*Z13*Z1
        TICK
        MPP Z1*Z5*Z6*Z2 Z9*Z13*Z14*Z10 Z8*Z12*Z15*Z11 Z0*Z4*Z7*Z3
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        DETECTOR rec[-4]
        DETECTOR rec[-5]
        DETECTOR rec[-6]
        DETECTOR rec[-7]
        DETECTOR rec[-8]
    )CIRCUIT");
    Circuit suffix = missing_detectors(c, true);
    ASSERT_EQ(suffix, Circuit(R"CIRCUIT(
        DETECTOR rec[-16] rec[-15] rec[-14] rec[-13] rec[-12] rec[-11] rec[-10] rec[-9]
    )CIRCUIT"));
}

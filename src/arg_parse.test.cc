#include "arg_parse.h"

#include <gtest/gtest.h>
#include <stdio.h>
#include <string>

#include "assert.h"

TEST(arg_parse, check_for_unknown_arguments_recognize_arguments) {
    const char *known[] = {
        "--mode",
        "--test",
        "--other",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--test", "5"};
    check_for_unknown_arguments(sizeof(known) / sizeof(char *), known, sizeof(argv) / sizeof(char *), argv);
}

TEST(arg_parse, check_for_unknown_arguments_bad_arguments) {
    const char *known[] = {
        "--mode",
        "--test",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--unknown", "5"};

    ASSERT_DEATH(
        { check_for_unknown_arguments(sizeof(known) / sizeof(char *), known, sizeof(argv) / sizeof(char *), argv); },
        "");
}

TEST(arg_parse, check_for_unknown_arguments_terminator) {
    const char *known[] = {
        "--mode",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--", "--unknown"};
    check_for_unknown_arguments(sizeof(known) / sizeof(char *), known, sizeof(argv) / sizeof(char *), argv);
}

TEST(arg_parse, find_argument) {
    const char *argv[] = {
        "skipped", "--mode", "2", "--test", "aba", "--a", "-b",
    };
    int n = sizeof(argv) / sizeof(char *);
    assert(find_argument("--mode", n, argv) == argv[2]);
    assert(find_argument("-mode", n, argv) == 0);
    assert(find_argument("mode", n, argv) == 0);
    assert(find_argument("--test", n, argv) == argv[4]);
    assert(find_argument("--a", n, argv) == argv[5] + 3);
    assert(find_argument("-a", n, argv) == 0);
    assert(find_argument("-b", n, argv) == argv[6] + 2);
    assert(find_argument("--b", n, argv) == 0);
    assert(!strcmp(find_argument("--a", n, argv), ""));
    assert(!strcmp(find_argument("-b", n, argv), ""));
}

TEST(arg_parse, require_find_argument) {
    const char *argv[] = {
        "skipped", "--mode", "2", "--test", "aba", "--a", "-b",
    };
    int n = sizeof(argv) / sizeof(char *);
    assert(require_find_argument("--mode", n, argv) == argv[2]);
    assert(require_find_argument("--test", n, argv) == argv[4]);
    assert(require_find_argument("--a", n, argv) == argv[5] + 3);
    assert(require_find_argument("-b", n, argv) == argv[6] + 2);
    assert(!strcmp(require_find_argument("--a", n, argv), ""));
    assert(!strcmp(require_find_argument("-b", n, argv), ""));
    ASSERT_DEATH({ require_find_argument("-mode", n, argv); }, "");
    ASSERT_DEATH({ require_find_argument("-a", n, argv); }, "");
    ASSERT_DEATH({ require_find_argument("--b", n, argv); }, "");
}

TEST(arg_parse, find_bool_argument) {
    const char *argv[] = {
        "", "-other", "2", "do", "-not", "-be", "activate", "-par", "--", "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_bool_argument("-activate", n, argv), 0);
    ASSERT_EQ(find_bool_argument("-okay", n, argv), 0);
    ASSERT_EQ(find_bool_argument("-not", n, argv), 1);
    ASSERT_EQ(find_bool_argument("-par", n, argv), 1);
    ASSERT_DEATH({ find_bool_argument("-be", n, argv); }, "non-empty value");
    ASSERT_DEATH({ find_bool_argument("-other", n, argv); }, "non-empty value");
}

TEST(arg_parse, find_int_argument) {
    const char *argv[] = {
        "", "-small=-23", "-empty", "-text", "abc", "-zero", "0", "-large", "50", "--", "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_int_argument("-missing", 5, -100, +100, n, argv), 5);
    ASSERT_EQ(find_int_argument("-small", 5, -100, +100, n, argv), -23);
    ASSERT_EQ(find_int_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_int_argument("-zero", 5, -100, +100, n, argv), 0);
    ASSERT_DEATH({ find_int_argument("-large", 0, 0, 49, n, argv); }, "50 <= 49");
    ASSERT_DEATH({ find_int_argument("-large", 100, 51, 100, n, argv); }, "51 <= 50");
    ASSERT_DEATH({ find_int_argument("-text", 0, 0, 0, n, argv); }, "non-integer");

    ASSERT_DEATH({ find_int_argument("-missing", -1, 0, 10, n, argv); }, "Must specify");
    ASSERT_DEATH({ find_int_argument("-missing", 11, 0, 10, n, argv); }, "Must specify");
    ASSERT_DEATH({ find_int_argument("-missing", -101, -100, 100, n, argv); }, "Must specify");
}

TEST(arg_parse, find_float_argument) {
    const char *argv[] = {
        "",    "-small=-23.5", "-empty", "-text",  "abc", "-inf", "inf",   "-nan",
        "nan", "-zero",        "0",      "-large", "50",  "--",   "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_float_argument("-missing", 5.5, -100, +100, n, argv), 5.5);
    ASSERT_EQ(find_float_argument("-small", 5, -100, +100, n, argv), -23.5);
    ASSERT_EQ(find_float_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_float_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_float_argument("-zero", 5, -100, +100, n, argv), 0);
    ASSERT_DEATH({ find_float_argument("-large", 0, 0, 49, n, argv); }, "0 <= 49");
    ASSERT_DEATH({ find_float_argument("-nan", 0, -100, 100, n, argv); }, "nan <= 100");
    ASSERT_DEATH({ find_float_argument("-large", 100, 51, 100, n, argv); }, "<= 50");
    ASSERT_DEATH({ find_float_argument("-text", 0, 0, 0, n, argv); }, "non-float");

    ASSERT_DEATH({ find_float_argument("-missing", -1, 0, 10, n, argv); }, "Must specify");
    ASSERT_DEATH({ find_float_argument("-missing", -1, 11, 10, n, argv); }, "Must specify");
    ASSERT_DEATH({ find_float_argument("-missing", -101, -100, 100, n, argv); }, "Must specify");
}

TEST(arg_parse, find_enum_argument) {
    std::vector<const char *> args{
        "",
        "-a=test",
        "-b",
        "-c=rest",
    };
    std::vector<const char *> enums{
        "",
        "test",
        "rest",
    };
    ASSERT_EQ(find_enum_argument("-a", -1, enums.size(), enums.data(), args.size(), args.data()), 1);
    ASSERT_EQ(find_enum_argument("-b", -1, enums.size(), enums.data(), args.size(), args.data()), 0);
    ASSERT_EQ(find_enum_argument("-c", -1, enums.size(), enums.data(), args.size(), args.data()), 2);
    ASSERT_EQ(find_enum_argument("-d", 0, enums.size(), enums.data(), args.size(), args.data()), 0);
    ASSERT_EQ(find_enum_argument("-d", 4, enums.size(), enums.data(), args.size(), args.data()), 4);
    ASSERT_DEATH(
        { find_enum_argument("-d", -1, enums.size(), enums.data(), args.size(), args.data()); }, "specify a value");

    enums = {
        "not in list",
    };
    ASSERT_DEATH(
        { find_enum_argument("-a", -1, enums.size(), enums.data(), args.size(), args.data()); }, "Unrecognized value");
}

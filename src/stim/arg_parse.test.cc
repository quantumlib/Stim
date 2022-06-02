// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/arg_parse.h"

#include <stdio.h>
#include <string>

#include "gtest/gtest.h"

#include "stim/test_util.test.h"

using namespace stim;

TEST(arg_parse, check_for_unknown_arguments_recognize_arguments) {
    std::vector<const char *> known{
        "--mode",
        "--test",
        "--other",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--test", "5"};
    check_for_unknown_arguments(known, {}, nullptr, sizeof(argv) / sizeof(char *), argv);
    check_for_unknown_arguments({}, known, nullptr, sizeof(argv) / sizeof(char *), argv);
}

TEST(arg_parse, check_for_unknown_arguments_bad_arguments) {
    std::vector<const char *> known{
        "--mode",
        "--test",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--unknown", "5"};

    ASSERT_THROW(
        { check_for_unknown_arguments(known, {}, nullptr, sizeof(argv) / sizeof(char *), argv); },
        std::invalid_argument);
    ASSERT_THROW(
        { check_for_unknown_arguments({}, known, nullptr, sizeof(argv) / sizeof(char *), argv); },
        std::invalid_argument);
}

TEST(arg_parse, check_for_unknown_arguments_terminator) {
    std::vector<const char *> known{
        "--mode",
    };
    const char *argv[] = {"skipped", "--mode", "2", "--", "--unknown"};
    check_for_unknown_arguments(known, {}, nullptr, sizeof(argv) / sizeof(char *), argv);
}

TEST(arg_parse, find_argument) {
    const char *argv[] = {
        "skipped",
        "--mode",
        "2",
        "--test",
        "aba",
        "--a",
        "-b",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_TRUE(find_argument("--mode", n, argv) == argv[2]);
    ASSERT_TRUE(find_argument("-mode", n, argv) == 0);
    ASSERT_TRUE(find_argument("mode", n, argv) == 0);
    ASSERT_TRUE(find_argument("--test", n, argv) == argv[4]);
    ASSERT_TRUE(find_argument("--a", n, argv) == argv[5] + 3);
    ASSERT_TRUE(find_argument("-a", n, argv) == 0);
    ASSERT_TRUE(find_argument("-b", n, argv) == argv[6] + 2);
    ASSERT_TRUE(find_argument("--b", n, argv) == 0);
    ASSERT_TRUE(!strcmp(find_argument("--a", n, argv), ""));
    ASSERT_TRUE(!strcmp(find_argument("-b", n, argv), ""));
}

TEST(arg_parse, require_find_argument) {
    const char *argv[] = {
        "skipped",
        "--mode",
        "2",
        "--test",
        "aba",
        "--a",
        "-b",
    };
    int n = sizeof(argv) / sizeof(char *);
    assert(require_find_argument("--mode", n, argv) == argv[2]);
    assert(require_find_argument("--test", n, argv) == argv[4]);
    assert(require_find_argument("--a", n, argv) == argv[5] + 3);
    assert(require_find_argument("-b", n, argv) == argv[6] + 2);
    assert(!strcmp(require_find_argument("--a", n, argv), ""));
    assert(!strcmp(require_find_argument("-b", n, argv), ""));
    ASSERT_THROW({ require_find_argument("-mode", n, argv); }, std::invalid_argument);
    ASSERT_THROW({ require_find_argument("-a", n, argv); }, std::invalid_argument);
    ASSERT_THROW({ require_find_argument("--b", n, argv); }, std::invalid_argument);
}

TEST(arg_parse, find_bool_argument) {
    const char *argv[] = {
        "",
        "-other",
        "2",
        "do",
        "-not",
        "-be",
        "activate",
        "-par",
        "--",
        "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_bool_argument("-activate", n, argv), 0);
    ASSERT_EQ(find_bool_argument("-okay", n, argv), 0);
    ASSERT_EQ(find_bool_argument("-not", n, argv), 1);
    ASSERT_EQ(find_bool_argument("-par", n, argv), 1);
    ASSERT_THROW({ find_bool_argument("-be", n, argv); }, std::invalid_argument);
    ASSERT_THROW({ find_bool_argument("-other", n, argv); }, std::invalid_argument);
}

template <typename TEx>
std::string catch_msg_helper(std::function<void(void)> func, std::string expected_substring) {
    try {
        func();
        return "Expected an exception with message '" + expected_substring + "', but no exception was thrown.";
    } catch (const TEx &ex) {
        std::string s = ex.what();
        if (s.find(expected_substring) == std::string::npos) {
            return "Didn't find '" + expected_substring + "' in '" + std::string(ex.what()) + "'.";
        }
        return "";
    }
}

#define ASSERT_THROW_MSG(body, type, msg) ASSERT_EQ("", catch_msg_helper<type>([&]() body, msg))

TEST(arg_parse, find_int_argument) {
    const char *argv[] = {
        "",
        "-small=-23",
        "-empty",
        "-text",
        "abc",
        "-zero",
        "0",
        "-large",
        "50",
        "--",
        "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_int64_argument("-missing", 5, -100, +100, n, argv), 5);
    ASSERT_EQ(find_int64_argument("-small", 5, -100, +100, n, argv), -23);
    ASSERT_EQ(find_int64_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_int64_argument("-zero", 5, -100, +100, n, argv), 0);
    ASSERT_THROW_MSG({ find_int64_argument("-large", 0, 0, 49, n, argv); }, std::invalid_argument, "50 <= 49");
    ASSERT_THROW_MSG({ find_int64_argument("-large", 100, 51, 100, n, argv); }, std::invalid_argument, "51 <= 50");
    ASSERT_THROW_MSG({ find_int64_argument("-text", 0, 0, 0, n, argv); }, std::invalid_argument, "non-int");

    ASSERT_THROW_MSG({ find_int64_argument("-missing", -1, 0, 10, n, argv); }, std::invalid_argument, "Must specify");
    ASSERT_THROW_MSG({ find_int64_argument("-missing", 11, 0, 10, n, argv); }, std::invalid_argument, "Must specify");
    ASSERT_THROW_MSG(
        { find_int64_argument("-missing", -101, -100, 100, n, argv); }, std::invalid_argument, "Must specify");

    std::vector<const char *> args;
    args = {"", "-val", "99999999999999999999999999999999999999999999999999"};
    ASSERT_THROW_MSG(
        { find_int64_argument("-val", 0, INT64_MIN, INT64_MAX, args.size(), args.data()); },
        std::invalid_argument,
        "non-int64");
    args = {"", "-val", "9223372036854775807"};
    ASSERT_EQ(find_int64_argument("-val", 0, INT64_MIN, INT64_MAX, args.size(), args.data()), INT64_MAX);
    args = {"", "-val", "9223372036854775808"};
    ASSERT_THROW_MSG(
        { find_int64_argument("-val", 0, INT64_MIN, INT64_MAX, args.size(), args.data()); },
        std::invalid_argument,
        "non-int64");
    args = {"", "-val", "-9223372036854775808"};
    ASSERT_EQ(find_int64_argument("-val", 0, INT64_MIN, INT64_MAX, args.size(), args.data()), INT64_MIN);
    args = {"", "-val", "-9223372036854775809"};
    ASSERT_THROW_MSG(
        { find_int64_argument("-val", 0, INT64_MIN, INT64_MAX, args.size(), args.data()); },
        std::invalid_argument,
        "non-int64");
}

TEST(arg_parse, find_float_argument) {
    const char *argv[] = {
        "",
        "-small=-23.5",
        "-empty",
        "-text",
        "abc",
        "-inf",
        "inf",
        "-nan",
        "nan",
        "-zero",
        "0",
        "-large",
        "50",
        "--",
        "-okay",
    };
    int n = sizeof(argv) / sizeof(char *);
    ASSERT_EQ(find_float_argument("-missing", 5.5, -100, +100, n, argv), 5.5);
    ASSERT_EQ(find_float_argument("-small", 5, -100, +100, n, argv), -23.5);
    ASSERT_EQ(find_float_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_float_argument("-large", 5, -100, +100, n, argv), 50);
    ASSERT_EQ(find_float_argument("-zero", 5, -100, +100, n, argv), 0);
    ASSERT_THROW_MSG({ find_float_argument("-large", 0, 0, 49, n, argv); }, std::invalid_argument, "0 <= 49");
    ASSERT_THROW_MSG({ find_float_argument("-nan", 0, -100, 100, n, argv); }, std::invalid_argument, "nan <= 100");
    ASSERT_THROW_MSG({ find_float_argument("-large", 100, 51, 100, n, argv); }, std::invalid_argument, "<= 50");
    ASSERT_THROW_MSG({ find_float_argument("-text", 0, 0, 0, n, argv); }, std::invalid_argument, "non-float");

    ASSERT_THROW_MSG({ find_float_argument("-missing", -1, 0, 10, n, argv); }, std::invalid_argument, "Must specify");
    ASSERT_THROW_MSG({ find_float_argument("-missing", -1, 11, 10, n, argv); }, std::invalid_argument, "Must specify");
    ASSERT_THROW_MSG(
        { find_float_argument("-missing", -101, -100, 100, n, argv); }, std::invalid_argument, "Must specify");
}

TEST(arg_parse, find_enum_argument) {
    std::vector<const char *> args{
        "",
        "-a=test",
        "-b",
        "-c=rest",
    };
    std::map<std::string, int> enums{
        {"", 10},
        {"test", 20},
        {"rest", 30},
    };
    ASSERT_EQ(find_enum_argument("-a", nullptr, enums, args.size(), args.data()), 20);
    ASSERT_EQ(find_enum_argument("-b", nullptr, enums, args.size(), args.data()), 10);
    ASSERT_EQ(find_enum_argument("-c", nullptr, enums, args.size(), args.data()), 30);
    ASSERT_EQ(find_enum_argument("-d", "test", enums, args.size(), args.data()), 20);
    ASSERT_EQ(find_enum_argument("-d", "rest", enums, args.size(), args.data()), 30);
    ASSERT_THROW_MSG(
        { find_enum_argument("-d", nullptr, enums, args.size(), args.data()); },
        std::invalid_argument,
        "specify a value");

    enums.erase("test");
    ASSERT_THROW_MSG(
        { find_enum_argument("-a", nullptr, enums, args.size(), args.data()); },
        std::invalid_argument,
        "Unrecognized value");
}

TEST(arg_parse, find_open_file_argument) {
    std::vector<const char *> args;
    FILE *tmp = tmpfile();

    args = {""};
    ASSERT_THROW_MSG(
        { find_open_file_argument("-arg", nullptr, "r", args.size(), args.data()); }, std::invalid_argument, "Missing");
    args = {""};
    ASSERT_EQ(find_open_file_argument("-arg", tmp, "r", args.size(), args.data()), tmp);

    args = {"", "-arg"};
    ASSERT_THROW_MSG(
        { find_open_file_argument("-arg", nullptr, "r", args.size(), args.data()); }, std::invalid_argument, "empty");
    args = {"", "-arg"};
    ASSERT_THROW_MSG(
        { find_open_file_argument("-arg", tmp, "r", args.size(), args.data()); }, std::invalid_argument, "empty");

    RaiiTempNamedFile f;
    FILE *f2 = fdopen(f.descriptor, "w");
    putc('x', f2);
    fclose(f2);
    args = {"", "-arg", f.path.data()};
    f2 = find_open_file_argument("-arg", nullptr, "r", args.size(), args.data());
    ASSERT_NE(f2, nullptr);
    ASSERT_EQ(getc(f2), 'x');
    fclose(f2);

    remove(f.path.data());
    args = {"", "-arg", f.path.data()};
    ASSERT_THROW_MSG(
        { find_open_file_argument("-arg", nullptr, "r", args.size(), args.data()); },
        std::invalid_argument,
        "Failed to open");
    f2 = find_open_file_argument("-arg", nullptr, "w", args.size(), args.data());
    fclose(f2);

    fclose(tmp);
}

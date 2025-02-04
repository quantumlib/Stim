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

#include "stim/stabilizers/tableau_iter.h"

#include "gtest/gtest.h"

#include "stim/mem/simd_word.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(tableau_iter, CommutingPauliStringIterator_1, {
    CommutingPauliStringIterator<W> iter(1);
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+X"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter({}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+X"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    auto ps = PauliString<W>::from_str("X");
    PauliStringRef<W> r = ps;

    iter.restart_iter({&r}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+X"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter({}, {&r});
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);
})

TEST_EACH_WORD_SIZE_W(tableau_iter, CommutingPauliStringIterator_2, {
    std::vector<PauliString<W>> coms;
    std::vector<PauliStringRef<W>> refs;
    coms.push_back(PauliString<W>::from_str("+Z_"));
    refs.push_back(coms[0]);

    std::vector<PauliString<W>> anti_coms;
    std::vector<PauliStringRef<W>> anti_refs;
    anti_coms.push_back(PauliString<W>::from_str("+XX"));
    anti_refs.push_back(anti_coms[0]);

    CommutingPauliStringIterator<W> iter(2);
    iter.restart_iter(refs, anti_refs);
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Z_"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZX"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+_Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+_Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);
})

TEST_EACH_WORD_SIZE_W(tableau_iter, CommutingPauliStringIterator_4, {
    CommutingPauliStringIterator<W> iter(4);

    iter.restart_iter({}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+X___"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+_X__"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+XX__"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+__X_"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+X_X_"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+_XX_"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+XXX_"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Z___"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+Y___"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZX__"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+YX__"));

    std::vector<PauliString<W>> coms;
    std::vector<PauliStringRef<W>> refs;
    coms.push_back(PauliString<W>::from_str("+Z___"));
    coms.push_back(PauliString<W>::from_str("+_Z__"));
    coms.push_back(PauliString<W>::from_str("+___Z"));
    refs.push_back(coms[0]);
    refs.push_back(coms[1]);
    refs.push_back(coms[2]);

    std::vector<PauliString<W>> anti_coms;
    std::vector<PauliStringRef<W>> anti_refs;
    anti_coms.push_back(PauliString<W>::from_str("+X___"));
    anti_coms.push_back(PauliString<W>::from_str("+_X__"));
    anti_coms.push_back(PauliString<W>::from_str("+___X"));
    anti_refs.push_back(anti_coms[0]);
    anti_refs.push_back(anti_coms[1]);
    anti_refs.push_back(anti_coms[2]);

    iter.restart_iter(refs, anti_refs);
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZZ_Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZZXZ"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZZZZ"));
    ASSERT_EQ(*iter.iter_next(), PauliString<W>::from_str("+ZZYZ"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter(&refs[0], &refs[0]);
    ASSERT_EQ(iter.iter_next(), nullptr);
})

TEST_EACH_WORD_SIZE_W(tableau_iter, iter_tableau, {
    TableauIterator<W> iter1(1, false);
    TableauIterator<W> iter1_signs(1, true);
    TableauIterator<W> iter2(2, false);
    TableauIterator<W> iter3(3, false);
    int n1 = 0;
    while (iter1.iter_next()) {
        n1++;
    }
    ASSERT_EQ(n1, 6);

    int s1 = 0;
    while (iter1_signs.iter_next()) {
        s1++;
    }
    ASSERT_EQ(s1, 24);

    int n2 = 0;
    while (iter2.iter_next()) {
        n2++;
    }
    ASSERT_EQ(n2, 720);

    // Note: disabled because it takes 2-3 seconds.
    // int n3 = 0;
    // while (iter3.iter_next()) {
    //     n3++;
    // }
    // ASSERT_EQ(n3, 1451520);
})

TEST_EACH_WORD_SIZE_W(tableau_iter, iter_tableau_distinct, {
    std::set<std::string> seen;
    TableauIterator<W> iter2_signs(2, true);
    while (iter2_signs.iter_next()) {
        seen.insert(iter2_signs.result.str());
    }
    ASSERT_EQ(seen.size(), 11520);
})

TEST_EACH_WORD_SIZE_W(tableau_iter, iter_tableau_copy, {
    TableauIterator<W> iter2(2, true);
    {
        TableauIterator<W> iter1(3, false);
        iter2 = iter1;
    }
    ASSERT_TRUE(iter2.iter_next());

    {
        TableauIterator<W> iter1(3, false);
        for (size_t k = 0; k < 100; k++) {
            iter2 = iter1;
            ASSERT_EQ(iter1.iter_next(), iter2.iter_next());
            ASSERT_EQ(iter2.result, iter1.result);
        }
        for (size_t k = 0; k < 1000; k++) {
            ASSERT_EQ(iter1.iter_next(), iter2.iter_next());
            ASSERT_EQ(iter2.result, iter1.result);
        }
    }
})

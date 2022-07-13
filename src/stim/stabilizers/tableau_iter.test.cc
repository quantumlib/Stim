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

using namespace stim;

TEST(tableau_iter, CommutingPauliStringIterator_1) {
    CommutingPauliStringIterator iter(1);
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+X"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter({}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+X"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    PauliString ps = PauliString::from_str("X");
    PauliStringRef r = ps;

    iter.restart_iter({&r}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+X"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter({}, {&r});
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);
}

TEST(tableau_iter, CommutingPauliStringIterator_2) {
    std::vector<PauliString> coms;
    std::vector<PauliStringRef> refs;
    coms.push_back(PauliString::from_str("+Z_"));
    refs.push_back(coms[0]);

    std::vector<PauliString> anti_coms;
    std::vector<PauliStringRef> anti_refs;
    anti_coms.push_back(PauliString::from_str("+XX"));
    anti_refs.push_back(anti_coms[0]);

    CommutingPauliStringIterator iter(2);
    iter.restart_iter(refs, anti_refs);
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Z_"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZX"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+_Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+_Y"));
    ASSERT_EQ(iter.iter_next(), nullptr);
}

TEST(tableau_iter, CommutingPauliStringIterator_4) {
    CommutingPauliStringIterator iter(4);

    iter.restart_iter({}, {});
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+X___"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+_X__"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+XX__"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+__X_"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+X_X_"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+_XX_"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+XXX_"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Z___"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+Y___"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZX__"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+YX__"));

    std::vector<PauliString> coms;
    std::vector<PauliStringRef> refs;
    coms.push_back(PauliString::from_str("+Z___"));
    coms.push_back(PauliString::from_str("+_Z__"));
    coms.push_back(PauliString::from_str("+___Z"));
    refs.push_back(coms[0]);
    refs.push_back(coms[1]);
    refs.push_back(coms[2]);

    std::vector<PauliString> anti_coms;
    std::vector<PauliStringRef> anti_refs;
    anti_coms.push_back(PauliString::from_str("+X___"));
    anti_coms.push_back(PauliString::from_str("+_X__"));
    anti_coms.push_back(PauliString::from_str("+___X"));
    anti_refs.push_back(anti_coms[0]);
    anti_refs.push_back(anti_coms[1]);
    anti_refs.push_back(anti_coms[2]);

    iter.restart_iter(refs, anti_refs);
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZZ_Z"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZZXZ"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZZZZ"));
    ASSERT_EQ(*iter.iter_next(), PauliString::from_str("+ZZYZ"));
    ASSERT_EQ(iter.iter_next(), nullptr);

    iter.restart_iter(&refs[0], &refs[0]);
    ASSERT_EQ(iter.iter_next(), nullptr);
}

TEST(tableau_iter, iter_tableau) {
    TableauIterator iter1(1, false);
    TableauIterator iter1_signs(1, true);
    TableauIterator iter2(2, false);
    TableauIterator iter3(3, false);
    int s1 = 0;
    int n1 = 0;
    int n2 = 0;
    int n3 = 0;
    while (iter1.iter_next()) {
        n1++;
    }
    while (iter1_signs.iter_next()) {
        s1++;
    }
    while (iter2.iter_next()) {
        n2++;
    }
    while (iter3.iter_next()) {
        n3++;
    }
    ASSERT_EQ(n1, 6);
    ASSERT_EQ(s1, 24);
    ASSERT_EQ(n2, 720);
    ASSERT_EQ(n3, 1451520);
}

TEST(tableau_iter, iter_tableau_distinct) {
    std::set<std::string> seen;
    TableauIterator iter2_signs(2, true);
    while (iter2_signs.iter_next()) {
        seen.insert(iter2_signs.result.str());
    }
    ASSERT_EQ(seen.size(), 11520);
}

TEST(tableau_iter, iter_tableau_copy) {
    TableauIterator iter2(2, true);
    {
        TableauIterator iter1(3, false);
        iter2 = iter1;
    }
    ASSERT_TRUE(iter2.iter_next());

    {
        TableauIterator iter1(3, false);
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
}

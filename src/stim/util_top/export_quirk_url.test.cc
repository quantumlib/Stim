#include "stim/util_top/export_quirk_url.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"

using namespace stim;

TEST(export_quirk, simple) {
    auto actual = export_quirk_url(Circuit(R"CIRCUIT(
        R 0
        H 0 1
        S 2
        H 2
        CX 0 1
        M 1
        SQRT_ZZ 2 3
        MXX 0 1
    )CIRCUIT"));
    auto expected = R"URL(https://algassert.com/quirk#circuit={"cols":[)URL"
                    R"URL(["ZDetectControlReset"],)URL"
                    R"URL(["H","H","Z^½"],)URL"
                    R"URL([1,1,"H"],)URL"
                    R"URL(["•","X"],)URL"
                    R"URL([1,"ZDetector"],)URL"
                    R"URL([1,1,"zpar","zpar","i"],)URL"
                    R"URL(["xpar","xpar",1,1,"X"],)URL"
                    R"URL([1,1,1,1,"ZDetectControlReset"])URL"
                    R"URL(]})URL";
    ASSERT_EQ(actual, expected);

    actual = export_quirk_url(Circuit(R"CIRCUIT(
        MRY 0
    )CIRCUIT"));
    expected = R"URL(https://algassert.com/quirk#circuit={"cols":[)URL"
               R"URL(["YDetectControlReset"],)URL"
               R"URL(["~Hyz"])URL"
               R"URL(],"gates":[)URL"
               R"URL({"id":"~Hyz","name":"Hyz","matrix":"{{-√½i,-√½},{√½,√½i}}"}]})URL";
    ASSERT_EQ(actual, expected);

    actual = export_quirk_url(Circuit(R"CIRCUIT(
        R 0
        H_XY 0 1
        S 2
        H 2
        CX 0 1
        M 1
        SQRT_ZZ 2 3
        MXX 0 1
    )CIRCUIT"));
    expected = R"URL(https://algassert.com/quirk#circuit={"cols":[)URL"
               R"URL(["ZDetectControlReset"],)URL"
               R"URL(["~Hxy","~Hxy","Z^½"],)URL"
               R"URL([1,1,"H"],)URL"
               R"URL(["•","X"],)URL"
               R"URL([1,"ZDetector"],)URL"
               R"URL([1,1,"zpar","zpar","i"],)URL"
               R"URL(["xpar","xpar",1,1,"X"],)URL"
               R"URL([1,1,1,1,"ZDetectControlReset"])URL"
               R"URL(],"gates":[)URL"
               R"URL({"id":"~Hxy","name":"Hxy","matrix":"{{0,-√½-√½i},{√½-√½i,0}}"}]})URL";
    ASSERT_EQ(actual, expected);

    actual = export_quirk_url(Circuit(R"CIRCUIT(
        R 0
        H_XY 0
        H_YZ 1
        S 2
        H 2
        CX 0 1
        M 1
        SQRT_ZZ 2 3
        MXX 0 1
    )CIRCUIT"));
    expected = R"URL(https://algassert.com/quirk#circuit={"cols":[)URL"
               R"URL(["ZDetectControlReset"],)URL"
               R"URL(["~Hxy","~Hyz","Z^½"],)URL"
               R"URL([1,1,"H"],)URL"
               R"URL(["•","X"],)URL"
               R"URL([1,"ZDetector"],)URL"
               R"URL([1,1,"zpar","zpar","i"],)URL"
               R"URL(["xpar","xpar",1,1,"X"],)URL"
               R"URL([1,1,1,1,"ZDetectControlReset"])URL"
               R"URL(],"gates":[)URL"
               R"URL({"id":"~Hxy","name":"Hxy","matrix":"{{0,-√½-√½i},{√½-√½i,0}}"},)URL"
               R"URL({"id":"~Hyz","name":"Hyz","matrix":"{{-√½i,-√½},{√½,√½i}}"}]})URL";
    ASSERT_EQ(actual, expected);
}

TEST(export_quirk, all_operations) {
    auto actual = export_quirk_url(generate_test_circuit_with_all_operations());
    auto expected =
        R"URL(https://algassert.com/quirk#circuit={"cols":[)URL"
        R"URL(["…","X","Y","Z"],)URL"
        R"URL(["~Cxyz","~Cnxyz","~Cxnyz","~Cxynz","~Czyx","~Cnzyx","~Cznyx","~Czynx"],)URL"
        R"URL(["~Hxy","H","~Hyz","~Hnxy","~Hnxz","~Hnyz"],)URL"
        R"URL(["X^½","X^-½","Y^½","Y^-½","Z^½","Z^-½"],)URL"
        R"URL(["Swap","Swap"],)URL"
        R"URL(["⊖","Z"],)URL"
        R"URL([1,1,"Swap","Swap"],)URL"
        R"URL(["i",1,"zpar","zpar"],)URL"
        R"URL([1,1,1,1,"Swap","Swap"],)URL"
        R"URL(["-i",1,1,1,"zpar","zpar"],)URL"
        R"URL([1,1,1,1,1,1,"Swap","Swap"],)URL"
        R"URL([1,1,1,1,1,1,1,1],)URL"
        R"URL([1,1,1,1,1,1,1,1,"Swap","Swap"],)URL"
        R"URL([1,1,1,1,1,1,1,1,"•","X"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,"Swap","Swap"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,"•","Z"],)URL"
        R"URL(["xpar","xpar","i"],)URL"
        R"URL(["-i",1,"xpar","xpar"],)URL"
        R"URL(["i",1,1,1,"ypar","ypar"],)URL"
        R"URL(["-i",1,1,1,1,1,"ypar","ypar"],)URL"
        R"URL(["i",1,1,1,1,1,1,1,"zpar","zpar"],)URL"
        R"URL(["-i",1,1,1,1,1,1,1,1,1,"zpar","zpar"],)URL"
        R"URL(["⊖","X"],)URL"
        R"URL([1,1,"⊖","Y"],)URL"
        R"URL([1,1,1,1,"⊖","Z"],)URL"
        R"URL([1,1,1,1,1,1,"(/)","X"],)URL"
        R"URL([1,1,1,1,1,1,1,1,"(/)","Y"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,"(/)","Z"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,1,1,"•","X"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,1,1,1,1,"•","Y"],)URL"
        R"URL([1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,"•","Z"],)URL"
        R"URL(["Z","ypar","zpar"],)URL"
        R"URL(["XDetector"],)URL"
        R"URL(["Z","ypar","zpar"],)URL"
        R"URL(["X","zpar"],)URL"
        R"URL(["ZDetector"],)URL"
        R"URL(["X","zpar"],)URL"
        R"URL(["xpar","ypar","zpar","i"],)URL"
        R"URL(["i",1,1,"xpar"],)URL"
        R"URL(["xpar","ypar","zpar","-i"],)URL"
        R"URL(["-i",1,"xpar"],)URL"
        R"URL(["XDetectControlReset","YDetectControlReset","ZDetectControlReset",1,1,1,1,"XDetectControlReset","YDetectControlReset","ZDetectControlReset"],)URL"
        R"URL(["H","~Hyz",1,"XDetector","YDetector","ZDetector","ZDetector","H","~Hyz"],)URL"
        R"URL(["Z","xpar"],)URL"
        R"URL(["XDetector"],)URL"
        R"URL(["Z","xpar"],)URL"
        R"URL([1,1,"Z","xpar"],)URL"
        R"URL([1,1,"XDetector"],)URL"
        R"URL([1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,"XDetector"],)URL"
        R"URL([1,1,1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,1,1,"XDetector"],)URL"
        R"URL([1,1,1,1,1,1,"Z","xpar"],)URL"
        R"URL(["H"],)URL"
        R"URL(["•","X"],)URL"
        R"URL([1,"Z^½"],)URL"
        R"URL(["H"],)URL"
        R"URL(["•","X"],)URL"
        R"URL([1,"Z^½"],)URL"
        R"URL(["H"],)URL"
        R"URL(["•","X"],)URL"
        R"URL([1,"Z^½"],)URL"
        R"URL(["ZDetectControlReset"],)URL"
        R"URL(["ZDetectControlReset"],)URL"
        R"URL(["XDetectControlReset"],)URL"
        R"URL(["H","YDetector"],)URL"
        R"URL([1,1,"Z","xpar"],)URL"
        R"URL([1,1,"XDetector"],)URL"
        R"URL([1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,"XDetector"],)URL"
        R"URL([1,1,1,1,"Z","xpar"],)URL"
        R"URL([1,1,1,1,1,1,"Z","ypar","zpar"],)URL"
        R"URL([1,1,1,1,1,1,"XDetector"],)URL"
        R"URL([1,1,1,1,1,1,"Z","ypar","zpar"]],"gates":)URL"
        R"URL([{"id":"~Hxy","name":"Hxy","matrix":"{{0,-√½-√½i},{√½-√½i,0}}"},)URL"
        R"URL({"id":"~Hyz","name":"Hyz","matrix":"{{-√½i,-√½},{√½,√½i}}"},)URL"
        R"URL({"id":"~Hnxy","name":"Hnxy","matrix":"{{0,√½+√½i},{√½-√½i,0}}"},)URL"
        R"URL({"id":"~Hnxz","name":"Hnxz","matrix":"{{-√½,√½},{√½,√½}}"},)URL"
        R"URL({"id":"~Hnyz","name":"Hnyz","matrix":"{{-√½,-√½i},{√½i,√½}}"},)URL"
        R"URL({"id":"~Cxyz","name":"Cxyz","matrix":"{{½-½i,-½-½i},{½-½i,½+½i}}"},)URL"
        R"URL({"id":"~Czyx","name":"Czyx","matrix":"{{½+½i,½+½i},{-½+½i,½-½i}}"},)URL"
        R"URL({"id":"~Cnxyz","name":"Cnxyz","matrix":"{{½+½i,½-½i},{-½-½i,½-½i}}"},)URL"
        R"URL({"id":"~Cxnyz","name":"Cxnyz","matrix":"{{½+½i,-½+½i},{½+½i,½-½i}}"},)URL"
        R"URL({"id":"~Cxynz","name":"Cxynz","matrix":"{{½-½i,½+½i},{-½+½i,½+½i}}"},)URL"
        R"URL({"id":"~Cnzyx","name":"Cnzyx","matrix":"{{½+½i,-½-½i},{½-½i,½-½i}}"},)URL"
        R"URL({"id":"~Cznyx","name":"Cznyx","matrix":"{{½-½i,½-½i},{-½-½i,½+½i}}"},)URL"
        R"URL({"id":"~Czynx","name":"Czynx","matrix":"{{½-½i,-½+½i},{½+½i,½+½i}}"}]})URL";
    ASSERT_EQ(actual, expected);
}

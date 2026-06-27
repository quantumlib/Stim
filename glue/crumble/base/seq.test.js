import {assertThat, test} from "../test/test_util.js"
import {groupBy} from "./seq.js"

test("seq.groupBy", () => {
    assertThat(groupBy([], e => e)).isEqualTo(new Map([]))
    assertThat(groupBy([2, 3, 5, 11, 15, 17, 2], e => e % 5)).isEqualTo(new Map([
        [1, [11]],
        [2, [2, 17, 2]],
        [3, [3]],
        [0, [5, 15]],
    ]))
});

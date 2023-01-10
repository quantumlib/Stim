import {test, assertThat} from "./test_util.js";

function expectFailure(func, match) {
    try {
        func();
    } catch (ex) {
        if (match.test('' + ex)) {
            return;
        }
        throw new Error('message "' + ex + '" does not match ' + match, {cause: ex});
    }
    throw new Error('Expected an exception matching ' + match);
}

/**
 * @param {!function(): void} func
 * @param {!RegExp} match
 */
async function asyncExpectFailure(func, match) {
    try {
        await func();
    } catch (ex) {
        if (match.test('' + ex)) {
            return;
        }
        throw new Error('message "' + ex + '" does not match ' + match, {cause: ex});
    }
    throw new Error('Expected an exception matching ' + match);
}

test("test_util.throws", () => {
    assertThat(() => {
        throw new Error('test');
    }).throwsAnExceptionThat().matches(/test/);

    let a = 1;
    assertThat(() => {
        a += 1;
    }).runsWithoutThrowingAnException();
    assertThat(a).isEqualTo(2);

    expectFailure(() => {
        assertThat(() => {
            throw new Error('XYZ');
        }).runsWithoutThrowingAnException();
    }, /expected the following not to throw:/);

    expectFailure(() => {
        assertThat(() => {
        }).throwsAnExceptionThat();
    }, /Expected the following to throw/);

    expectFailure(() => {
        assertThat(() => {
            throw new Error('xyz')
        }).throwsAnExceptionThat().matches(/XYZ/);
    }, /Expected the following to match/);
});

test("test_util.isEqualTo", () => {
    assertThat(undefined).isEqualTo(undefined);
    assertThat(null).isEqualTo(null);
    assertThat("1").isEqualTo("1");
    assertThat(1).isEqualTo(1);
    assertThat("1").isNotEqualTo(1);
    assertThat([1, 2, 3]).isEqualTo([1, 2, 3]);
    assertThat([1, 2, 3]).isNotEqualTo([1, 2, 4]);
    assertThat(new Set([1, 2, 3])).isEqualTo(new Set([1, 2, 3]));
    assertThat(new Set([1, 2, 3])).isNotEqualTo(new Set([1, 2, 4]));
    assertThat(new Set([1, 2, 3])).isNotEqualTo(new Set([1, 2]));
    assertThat(new Set([1, 2, 3])).isNotEqualTo(new Set([1, 2, 3, 4]));
    assertThat(new Map([[1, 2], [3, 4]])).isEqualTo(new Map([[1, 2], [3, 4]]));
    assertThat(new Map([[1, 2], [3, 4]])).isNotEqualTo(new Map([[1, 2], [3, 5]]));
    assertThat(new Map([[1, 2], [3, 4]])).isNotEqualTo(new Map([[1, 2]]));
    assertThat(new Map([[1, 2], [3, 4]])).isNotEqualTo(new Map([[1, 2], [3, 4], [5, 6]]));

    expectFailure(() => {
        assertThat(undefined).isNotEqualTo(undefined);
    }, /Expected the following objects to NOT be equal/);
    expectFailure(() => {
        assertThat("1").isEqualTo(1);
    }, /Expected the following objects to be equal/);
});

test("test_util.async", async () => {
    let e = (async () => 5)();
    let ex = (async () => {
        throw new Error("test");
    })();
    await assertThat(e).asyncResolvesToAValueThat().isEqualTo(5);
    await assertThat(ex).asyncRejectsWithAnExceptionThat().matches(/test/);

    expectFailure(() => {
        assertThat(e).isEqualTo(5);
    }, /to be equal/);
    await asyncExpectFailure(async () => {
        await assertThat(e).asyncRejectsWithAnExceptionThat();
    }, /should have rejected/);
    await asyncExpectFailure(async () => {
        await assertThat(e).asyncResolvesToAValueThat().isEqualTo(6);
    }, /to be equal/);

    expectFailure(() => {
        assertThat(ex).isEqualTo(5);
    }, /to be equal/);
    await asyncExpectFailure(async () => {
        await assertThat(ex).asyncResolvesToAValueThat();
    }, /should have resolved/);
    await asyncExpectFailure(async () => {
        await assertThat(ex).asyncRejectsWithAnExceptionThat().matches(/bla/);
    }, /test/);
});

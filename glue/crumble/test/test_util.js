import {describe} from "../base/describe.js";
import {equate} from "../base/equate.js";

let _tests = /** @type {!Array<!{name: !string, body: !function}>} */ [];
let _usedAssertIndices = 0;
let _unawaitedAsserts = 0;

/**
 * @param {!string} name
 * @param {!function} body
 */
function test(name, body) {
    _tests.push({name, body});
}

/**
 * @param {!string} text
 * @param {!int} indentation
 * @param {!boolean} indent_first
 * @returns {!string}
 */
function indent(text, indentation, indent_first=true) {
    let lines = text.split('\n');
    let prefix = ' '.repeat(indentation);
    for (let k = 0; k < lines.length; k++) {
        let p = k > 0 || indent_first ? prefix : '';
        lines[k] = p + lines[k];
    }
    return lines.join('\n');
}

/**
 * @param {*} obj
 * @returns {!boolean}
 */
function isSequence(obj) {
    return obj instanceof Array || obj instanceof Uint32Array;
}

/**
 * @param {*} actual
 * @param {*} expected
 * @returns {!string}
 */
function diff(actual, expected) {
    let differences = [];
    if (isSequence(actual) && isSequence(expected)) {
        for (let k = 0; k < Math.max(actual.length, expected.length); k++) {
            if (!equate(actual[k], expected[k])) {
                differences.push(`${describe(k)}: ${diff(actual[k], expected[k])}`);
            }
        }
    }
    if ((actual instanceof Set || actual instanceof Map) && ((expected instanceof Set || expected instanceof Map))) {
        for (let k of actual.keys()) {
            if (!expected.has(k)) {
                differences.push(`${describe(k)} missing from expected`);
            }
        }
        for (let k of expected.keys()) {
            if (!actual.has(k)) {
                differences.push(`${describe(k)} missing from actual`);
            }
        }
    }
    if (actual instanceof Map && expected instanceof Map) {
        for (let k of actual.keys()) {
            if (expected.has(k)) {
                let v1 = actual.get(k);
                let v2 = expected.get(k);
                if (!equate(v1, v2)) {
                    differences.push(`${describe(k)}: ${diff(actual.get(k), expected.get(k))}`);
                }
            }
        }
    }
    if (typeof actual === 'number' && typeof expected === 'number') {
        differences.push(`${actual} != ${expected}`);
    }
    if (differences.length === 0) {
        let lines1 = describe(actual).split('\n');
        let lines2 = describe(expected).split('\n');
        for (let k = 0; k < Math.max(lines1.length, lines2.length); k++) {
            let line1 = lines1[k];
            let line2 = lines2[k];
            if ((line1 === undefined && line2 === '') || (line2 === undefined && line1 === '')) {
                differences.push('▯');
            } else if (line1 === undefined) {
                differences.push('█'.repeat(line2.length));
            } else if (line2 === undefined) {
                differences.push('█'.repeat(line1.length));
            } else {
                let t = '';
                for (let x = 0; x < Math.max(line1.length, line2.length); x++) {
                    let c1 = line1[x];
                    let c2 = line2[x];
                    t += c1 === c2 ? c1 : '█';
                }
                differences.push(t);
            }
        }
    }

    return differences.map(e => indent(e, 4, false)).join('\n');
}

class AsyncAssertSubject {
    /**
     * @param {!Promise<!AssertSubject>} asyncSubject
     */
    constructor(asyncSubject) {
        this.asyncSubject = asyncSubject;
    }

    /**
     * @param {!string} info
     * @returns {!AsyncAssertSubject}
     */
    withInfo(info) {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.withInfo(info)));
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    runsWithoutThrowingAnException() {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.runsWithoutThrowingAnException()));
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    resolvesToAValueThat() {
        return new AsyncAssertSubject(this.asyncSubject.then(async subject => await subject.asyncResolvesToAValueThat()));
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    rejectsWithAnExceptionThat() {
        return new AsyncAssertSubject(this.asyncSubject.then(async subject => await subject.asyncRejectsWithAnExceptionThat()));
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    throwsAnExceptionThat() {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.throwsAnExceptionThat()));
    }

    /**
     * @param {!RegExp} pattern
     * @returns {!AsyncAssertSubject}
     */
    matches(pattern) {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.matches(pattern)));
    }

    /**
     * @param {*} expected
     * @returns {!AsyncAssertSubject}
     */
    isEqualTo(expected) {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.isEqualTo(expected)));
    }

    /**
     * @param {*} unexpected
     * @returns {!AsyncAssertSubject}
     */
    isNotEqualTo(unexpected) {
        return new AsyncAssertSubject(this.asyncSubject.then(subject => subject.isNotEqualTo(unexpected)));
    }

    then(resolve, reject) {
        this.asyncSubject.then(e => {
            _unawaitedAsserts -= 1;
            resolve(e);
        }, ex => {
            _unawaitedAsserts -= 1;
            reject(ex);
        });
    }
}

class AssertSubject {
    /**
     * @param {*} value
     * @param {!int} index
     * @param {!*} info
     */
    constructor(value, index, info) {
        this.value = value;
        this.index = index;
        this.info = info;
    }

    /**
     * @param {!*} info
     * @returns {!AssertSubject}
     */
    withInfo(info) {
        return new AssertSubject(this.value, this.index, info);
    }

    /**
     * @returns {!string}
     * @private
     */
    _trace() {
        try {
            throw new Error('');
        } catch (ex) {
            let lines = `${ex.stack}`.split('\n');
            return 'Stack trace:\n' + lines.slice(3).join('\n')
        }
    }

    /**
     * @returns {!string}
     * @private
     */
    _whichAssertion() {
        if (this.info === undefined) {
            return `Assertion #${this.index}`;
        }
        return `Assertion #${this.index} with info ${describe(this.info)}`;
    }

    runsWithoutThrowingAnException() {
        try {
            this.value();
        } catch (ex) {
            throw new Error(`${this._whichAssertion()} failed. Got an exception:

${indent('' + ex, 4)}

but expected the following not to throw:

${indent('' + this.value, 4)}

`);
        }
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    asyncResolvesToAValueThat() {
        _unawaitedAsserts += 1;
        return new AsyncAssertSubject(
            new Promise((resolve, reject) => {
                this.value.then(v => resolve(new AssertSubject(v, this.index, this.info))).catch(
                    ex => reject(new Error(`Promise should have resolved, but instead rejected with:

${describe(ex)}
`, {cause: ex})));
            }));
    }

    /**
     * @returns {!AsyncAssertSubject}
     */
    asyncRejectsWithAnExceptionThat() {
        _unawaitedAsserts += 1;
        return new AsyncAssertSubject(
            new Promise((resolve, reject) => {
                this.value.then(v => reject(new Error(`Promise should have rejected, but instead resolved to:

${describe(v)}
`))).catch(ex => resolve(new AssertSubject(ex, this.index, this.info)));
            }));
    }

    /**
     * @returns {!AssertSubject}
     */
    throwsAnExceptionThat() {
        try {
            this.value();
        } catch (ex) {
            return new AssertSubject(ex, this.index, this.info);
        }
        throw new Error(`${this._whichAssertion()} failed. Expected the following to throw an exception:

${indent('' + this.value, 4)}
`);
    }

    /**
     * @param {!RegExp} pattern
     * @returns void
     */
    matches(pattern) {
        if (!pattern.test('' + this.value)) {
        throw new Error(`${this._whichAssertion()} failed. Expected the following to match:

Actual value {
${indent(describe(this.value), 4)}
}

Expected pattern {
    ${pattern}
}
`);
        }
    }

    /**
     * @param {*} expected
     * @returns void
     */
    isEqualTo(expected) {
        if (!equate(this.value, expected)) {
            let s1 = describe(this.value);
            let s2 = describe(expected);
            throw new Error(`${this._whichAssertion()} failed. Expected the following objects to be equal.
${s1 === s2 ? 'WARNING! Objects not equal but strings equal!' : ''}
Actual value {
${indent(s1, 4)}
}

Expected value {
${indent(s2, 4)}
}

Differences {
${indent(diff(this.value, expected), 4)}
}

${this._trace()}
`);
        }
    }

    /**
     * @param {*} unexpected
     * @returns void
     */
    isNotEqualTo(unexpected) {
        if (equate(this.value, unexpected)) {
            let s1 = describe(this.value);
            let s2 = describe(unexpected);
            throw new Error(`${this._whichAssertion()} failed. Expected the following objects to NOT be equal.

Actual value {
${indent(s1, 4)}
}

Expected different value {
${indent(s2, 4)}
}
`);
        }
    }
}

/**
 * @param {*} value
 * @returns {!AssertSubject}
 */
function assertThat(value) {
    _usedAssertIndices++;
    return new AssertSubject(value, _usedAssertIndices, undefined);
}

function skipRestOfTestIfHeadless() {
    try {
        // Actually, even attempting to read the variable will raise an error.
        if (document === undefined) {
            throw new Error();
        }
    } catch (_) {
        throw new Error("skipRestOfTestIfHeadless:document === undefined");
    }
}

class TestProgress {
    /**
     * @param {!string} name
     * @param {!boolean} passed
     * @param {*} error
     * @param {!int} num_tests
     * @param {!int} num_tests_failed
     * @param {!int} num_tests_left
     * @param {!int} num_skipped
     */
    constructor(name, passed, error, num_tests, num_tests_failed, num_tests_left, num_skipped) {
        this.name = name;
        this.passed = passed;
        this.error = error;
        this.num_tests = num_tests;
        this.num_tests_failed = num_tests_failed;
        this.num_tests_left = num_tests_left;
        this.num_skipped = num_skipped;
    }
}

/**
 * @param {!function(progress: !TestProgress)} callback
 * @param {!function(name: !string): !boolean} name_filter
 */
async function run_tests(callback, name_filter) {
    let num_tests = _tests.length;
    let num_tests_left = _tests.length;
    let num_tests_failed = 0;
    let all_passed = true;
    let num_skipped = 0;
    for (let test of _tests) {
        if (!name_filter(test.name)) {
            num_skipped += 1;
            continue;
        }
        console.log("run test", test.name);
        _usedAssertIndices = 0;
        let name = test.name;
        let passed = false;
        let error = undefined;
        let skipped = false;
        try {
            await test.body();
            if (_usedAssertIndices === 0) {
                throw new Error("No assertions in test.");
            }
            if (_unawaitedAsserts > 0) {
                throw new Error("An async assertion wasn't awaited.");
            }
            passed = true;
        } catch (ex) {
            if (ex instanceof Error && ex.message === "skipRestOfTestIfHeadless:document === undefined") {
                console.warn(`skipped part of test '${test.name}' because tests are running headless`);
                skipped = true;
                num_skipped += 1;
                passed = true;
            } else {
                error = ex;
                all_passed = false;
                num_tests_failed++;
                console.error("failed test", test.name, ex);
            }
        }
        num_tests_left--;
        callback(new TestProgress(name, passed, error, num_tests, num_tests_failed, num_tests_left, num_skipped));
    }

    let msg = `done running tests: ${num_tests_failed} failed, ${num_skipped} skipped`;
    if (num_tests_failed > 0) {
        console.error(msg);
    } else if (num_skipped > 0) {
        console.warn(msg);
    } else {
        console.log(msg);
    }
    return new TestProgress('', all_passed, undefined, num_tests, num_tests_failed, 0, num_skipped);
}

export {test, run_tests, assertThat, skipRestOfTestIfHeadless}

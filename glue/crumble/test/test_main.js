import {run_tests, test, assertThat} from "./test_util.js"

let imported = import("./test_import_all.js");
test("init", () => assertThat(imported).asyncResolvesToAValueThat().isNotEqualTo(undefined));
await imported.catch(() => {});

let status = /** @type {!HTMLDivElement} */ document.getElementById("status");
let acc = /** @type {!HTMLDivElement} */ document.getElementById("acc");

let testFilter = undefined;
let hash = document.location.hash;
if (hash.startsWith('#')) {
    hash = hash.substring(1);
}
if (hash.startsWith('test=')) {
    hash = hash.substring(5);
    testFilter = hash;
    console.log(`Only running '${testFilter}'`)
}

status.textContent = "Running tests...";

let total = await run_tests(progress => {
    status.textContent = `${progress.num_tests_left - progress.num_tests}/${progress.num_tests} ${progress.name} ${progress.passed ? 'passed' : 'failed'} (${progress.num_tests_failed} failed)`;
    if (!progress.passed) {
        let d = document.createElement("pre");
        d.textContent = `Test '${progress.name}' failed:`;
        acc.appendChild(d);

        let p = document.createElement("pre");
        p.style.marginLeft = '40px';
        p.style.backgroundColor = 'lightgray';
        p.textContent = `${progress.error}`;
        acc.appendChild(p);
    } else if (progress.skipped) {
        let d = document.createElement("pre");
        d.textContent = `Test '${progress.name}' skipped`;
        acc.appendChild(d);
    }
}, name => testFilter === undefined || name === testFilter);
if (!total.passed) {
    if (total.num_skipped > 0) {
        status.textContent = `${total.num_tests_failed} tests failed out of ${total.num_tests - total.num_skipped} (${total.num_skipped} skipped).`;
    } else {
        status.textContent = `${total.num_tests_failed} tests failed out of ${total.num_tests}.`;
    }
} else if (total.num_skipped > 0) {
    status.textContent = `All ${total.num_tests} tests passed (${total.num_skipped} skipped).`;
} else {
    status.textContent = `All ${total.num_tests} tests passed.`;
}

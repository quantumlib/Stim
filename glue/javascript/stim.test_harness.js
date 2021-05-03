let tests = [];
let __any_failures = false;

function test(name, handler) {
    tests.push({name, handler});
}

function tryPromiseRun(method) {
    try {
        return Promise.resolve(method());
    } catch (ex) {
        return Promise.reject(ex);
    }
}

async function run_all_tests() {
    let stim = await load_stim_module();
    let results = [];
    for (let {name, handler} of tests) {
        let result = tryPromiseRun(() => {
            let count = 0;
            let assert = condition => {
                count += 1;
                if (!condition) {
                    throw new Error(`Assert #${count} failed.`);
                }
            };
            return handler({stim, assert});
        });
        results.push({name, result});
    }
    for (let {result, name} of results) {
        try {
            await result;
            console.log(`pass ${name}`);
        } catch (ex) {
            __any_failures = true;
            console.error(`FAIL ${name}: ${ex}`);
        }
    }
    try {
        if (__any_failures) {
            console.error("THERE WERE TEST FAILURES");
            throw Error("THERE WERE TEST FAILURES");
        } else {
            console.info("All tests passed.");
        }
    } finally {
        var doneDiv = document.createElement('div');
        doneDiv.id = 'done';
        doneDiv.innerText = 'Done';
        document.body.appendChild(doneDiv);
    }
}

setTimeout(run_all_tests, 0);

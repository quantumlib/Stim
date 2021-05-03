let tests = [];

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
    let failed = false;
    for (let {result, name} of results) {
        try {
            await result;
            console.log(`pass ${name}`);
        } catch (ex) {
            failed = true;
            console.error(`FAIL ${name}: ${ex}`);
        }
    }
    if (failed) {
        console.error("THERE WERE TEST FAILURES");
    } else {
        console.info("All tests passed.");
    }
}

setTimeout(run_all_tests, 0);

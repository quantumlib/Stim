/**
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import {HistoryPusher} from "../base/history_pusher.js"
import {Circuit} from "../circuit/circuit.js";

/**
 * @param {!string} text
 * @return {!string}
 */
function urlWithCircuitHash(text) {
    text = text.
        replaceAll('QUBIT_COORDS', 'Q').
        replaceAll(', ', ',').
        replaceAll(') ', ')').
        replaceAll(' ', '_').
        replaceAll('\n', ';');
    if (text.indexOf('%') !== -1 || text.indexOf('&') !== -1) {
        text = encodeURIComponent(text);
    }
    return "#circuit=" + text;
}

/**
 * @param {!Revision} revision
 */
function initUrlCircuitSync(revision) {
    // Pull initial circuit out of URL '#x=y' arguments.
    const getHashParameters = () => {
        let hashText = document.location.hash.substring(1);
        let paramsMap = new Map();
        if (hashText !== "") {
            for (let keyVal of hashText.split("&")) {
                let eq = keyVal.indexOf("=");
                if (eq === -1) {
                    continue;
                }
                let key = keyVal.substring(0, eq);
                let val = decodeURIComponent(keyVal.substring(eq + 1));
                paramsMap.set(key, val);
            }
        }
        return paramsMap;
    };

    const historyPusher = new HistoryPusher();
    const loadCircuitFromUrl = () => {
        // Leave a sign that the URL shouldn't be overwritten if we don't understand it.
        historyPusher.currentStateIsMemorableButUnknown();

        try {
            // Extract the circuit parameter.
            let params = getHashParameters();
            if (!params.has("circuit")) {
                let txtDefault = /** @type {!HTMLTextAreaElement} */ document.getElementById('txtDefaultCircuit');
                if (txtDefault.value.replaceAll('_', '-') === "[[[DEFAULT-CIRCUIT-CONTENT-LITERAL]]]") {
                    params.set("circuit", "");
                } else {
                    params.set("circuit", txtDefault.value);
                }
            }

            // We only want to change the browser URL if we end up with a circuit state
            // that's different from the one from the initial URL.
            historyPusher.currentStateIsMemorableAndEqualTo(params.get("circuit"));

            // Repack the circuit data, so we can tell if there are round trip changes.
            let circuit = Circuit.fromStimCircuit(params.get("circuit"));
            let cleanedState = circuit.toStimCircuit();
            revision.clear(cleanedState);

            // If the initial state is an empty circuit without any round trip spandrels, no need to put it in history.
            if (circuit.layers.every(e => e.isEmpty()) && params.size === 1 && cleanedState === params.get('circuit')) {
                historyPusher.currentStateIsNotMemorable();
            } else {
                historyPusher.stateChange(cleanedState, urlWithCircuitHash(cleanedState));
            }
        } catch (ex) {
            // TODO: HANDLE BETTER.
            throw new Error(ex);
        }
    };

    window.addEventListener('popstate', loadCircuitFromUrl);
    loadCircuitFromUrl();

    revision.latestActiveCommit().whenDifferent().skip(1).subscribe(jsonText => {
        historyPusher.stateChange(jsonText, urlWithCircuitHash(jsonText));
    });
}

export {initUrlCircuitSync}

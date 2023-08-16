class PauliFrame {
    /**
     * @param {!int} num_frames
     * @param {!int} num_qubits
     */
    constructor(num_frames, num_qubits) {
        if (num_frames > 32) {
            throw new Error('num_frames > 32');
        }
        this.num_qubits = num_qubits;
        this.num_frames = num_frames;
        this.xs = new Uint32Array(num_qubits);
        this.zs = new Uint32Array(num_qubits);
        this.flags = new Uint32Array(num_qubits);
    }

    /**
     * @returns {!PauliFrame}
     */
    copy() {
        let result = new PauliFrame(this.num_frames, this.num_qubits);
        for (let q = 0; q < this.num_qubits; q++) {
            result.xs[q] = this.xs[q];
            result.zs[q] = this.zs[q];
            result.flags[q] = this.flags[q];
        }
        return result;
    }

    /**
     * @param {!Array<!string>} qubit_keys
     * @returns {!Array<!Map<!string, !string>>}
     */
    to_dicts(qubit_keys) {
        if (qubit_keys.length !== this.num_qubits) {
            throw new Error("qubit_keys.length !== this.num_qubits");
        }
        let result = [];
        for (let k = 0; k < this.num_frames; k++) {
            result.push(new Map());
        }
        for (let q = 0; q < this.num_qubits; q++) {
            let key = qubit_keys[q];
            let x = this.xs[q];
            let z = this.zs[q];
            let f = this.flags[q];
            let m = x | z | f;
            let k = 0;
            while (m) {
                if (m & 1) {
                    if (f & 1) {
                        result[k].set(key, 'ERR:flag');
                    } else if (x & z & 1) {
                        result[k].set(key, 'Y');
                    } else if (x & 1) {
                        result[k].set(key, 'X');
                    } else {
                        result[k].set(key, 'Z');
                    }
                }
                k++;
                x >>= 1;
                z >>= 1;
                f >>= 1;
                m >>= 1;
            }
        }
        return result;
    }

    /**
     * @param {!Array<!string>} strings
     * @returns {!PauliFrame}
     */
    static from_strings(strings) {
        let num_frames = strings.length;
        if (num_frames === 0) {
            throw new Error("strings.length === 0");
        }
        let num_qubits = strings[0].length;
        for (let s of strings) {
            if (s.length !== num_qubits) {
                throw new Error("Inconsistent string length.");
            }
        }

        let result = new PauliFrame(num_frames, num_qubits);
        for (let f = 0; f < num_frames; f++) {
            for (let q = 0; q < num_qubits; q++) {
                let c = strings[f][q];
                if (c === 'X') {
                    result.xs[q] |= 1 << f;
                } else if (c === 'Y') {
                    result.xs[q] |= 1 << f;
                    result.zs[q] |= 1 << f;
                } else if (c === 'Z') {
                    result.zs[q] |= 1 << f;
                } else if (c === 'I' || c === '_') {
                    // Identity.
                } else if (c === '!') {
                    result.flags[q] |= 1 << f;
                } else if (c === '%') {
                    result.flags[q] |= 1 << f;
                    result.xs[q] |= 1 << f;
                } else if (c === '&') {
                    result.flags[q] |= 1 << f;
                    result.xs[q] |= 1 << f;
                    result.zs[q] |= 1 << f;
                } else if (c === '$') {
                    result.flags[q] |= 1 << f;
                    result.zs[q] |= 1 << f;
                } else {
                    throw new Error("Unrecognized pauli string character: '" + c + "'");
                }
            }
        }
        return result;
    }

    /**
     * @returns {!Array<!string>}
     */
    to_strings() {
        let result = [];
        for (let f = 0; f < this.num_frames; f++) {
            let s = '';
            for (let q = 0; q < this.num_qubits; q++) {
                let flag = (this.flags[q] >> f) & 1;
                let x = (this.xs[q] >> f) & 1;
                let z = (this.zs[q] >> f) & 1;
                s += '_XZY!%$&'[x + 2*z + 4*flag];
            }
            result.push(s);
        }
        return result;
    }

    /**
     * @param {!Array<!Map<!string, !string>>} dicts
     * @param {!Array<!string>} qubit_keys
     * @returns {!PauliFrame}
     */
    static from_dicts(dicts, qubit_keys) {
        let result = new PauliFrame(dicts.length, qubit_keys.length);
        for (let f = 0; f < dicts.length; f++) {
            for (let q = 0; q < qubit_keys.length; q++) {
                let p = dicts[f].get(qubit_keys[q]);
                if (p === 'X') {
                    result.xs[q] |= 1 << f;
                } else if (p === 'Z') {
                    result.zs[q] |= 1 << f;
                } else if (p === 'Y') {
                    result.xs[q] |= 1 << f;
                    result.zs[q] |= 1 << f;
                } else if (p !== 'I' && p !== undefined) {
                    result.flags[q] |= 1 << f;
                }
            }
        }
        return result;
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_exchange_xz(targets) {
        for (let t of targets) {
            let x = this.xs[t];
            let z = this.zs[t];
            this.zs[t] = x;
            this.xs[t] = z;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_exchange_xy(targets) {
        for (let t of targets) {
            this.zs[t] ^= this.xs[t];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_exchange_yz(targets) {
        for (let t of targets) {
            this.xs[t] ^= this.zs[t];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_discard(targets) {
        for (let t of targets) {
            this.flags[t] |= this.xs[t];
            this.flags[t] |= this.zs[t];
            this.xs[t] = 0;
            this.zs[t] = 0;
        }
    }

    /**
     * @param {!string} observable
     * @param {!Array<!int>} targets
     */
    do_measure(observable, targets) {
        for (let k = 0; k < targets.length; k += observable.length) {
            let anticommutes = 0;
            for (let k2 = 0; k2 < observable.length; k2++) {
                let t = targets[k + k2];
                let obs = observable[k2];
                if (obs === 'X') {
                    anticommutes ^= this.zs[t];
                } else if (obs === 'Z') {
                    anticommutes ^= this.xs[t];
                } else if (obs === 'Y') {
                    anticommutes ^= this.xs[t] ^ this.zs[t];
                } else {
                    throw new Error(`Unrecognized measure obs: '${obs}'`);
                }
            }
            for (let k2 = 0; k2 < observable.length; k2++) {
                let t = targets[k + k2];
                this.flags[t] |= anticommutes;
            }
        }
    }

    /**
     * @param {!string} bases
     * @param {!Uint32Array} targets
     */
    do_mpp(bases, targets) {
        let anticommutes = 0;
        for (let k = 0; k < bases.length; k++) {
            let t = targets[k];
            let obs = bases[k];
            if (obs === 'X') {
                anticommutes ^= this.zs[t];
            } else if (obs === 'Z') {
                anticommutes ^= this.xs[t];
            } else if (obs === 'Y') {
                anticommutes ^= this.xs[t] ^ this.zs[t];
            } else {
                throw new Error(`Unrecognized measure obs: '${obs}'`);
            }
        }
        for (let k = 0; k < bases.length; k++) {
            let t = targets[k];
            this.flags[t] |= anticommutes;
        }
    }

    /**
     * @param {!string} observable
     * @param {!Array<!int>} targets
     */
    do_demolition_measure(observable, targets) {
        if (observable === 'X') {
            for (let q of targets) {
                this.flags[q] |= this.zs[q];
                this.xs[q] = 0;
                this.zs[q] = 0;
            }
        } else if (observable === 'Z') {
            for (let q of targets) {
                this.flags[q] |= this.xs[q];
                this.xs[q] = 0;
                this.zs[q] = 0;
            }
        } else if (observable === 'Y') {
            for (let q of targets) {
                this.flags[q] |= this.xs[q] ^ this.zs[q];
                this.xs[q] = 0;
                this.zs[q] = 0;
            }
        } else {
            throw new Error("Unrecognized demolition obs");
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cycle_xyz(targets) {
        for (let t of targets) {
            this.xs[t] ^= this.zs[t];
            this.zs[t] ^= this.xs[t];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cycle_zyx(targets) {
        for (let t of targets) {
            this.zs[t] ^= this.xs[t];
            this.xs[t] ^= this.zs[t];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_swap(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let a = k;
            let b = k + 1;
            let xa = this.xs[a];
            let za = this.zs[a];
            let xb = this.xs[b];
            let zb = this.zs[b];
            this.xs[a] = xb;
            this.zs[a] = zb;
            this.xs[b] = xa;
            this.zs[b] = za;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_iswap(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let a = k;
            let b = k + 1;

            let xa = this.xs[a];
            let za = this.zs[a];
            let xb = this.xs[b];
            let zb = this.zs[b];
            let xab = xa ^ xb;
            this.xs[a] = xb;
            this.zs[a] = zb ^ xab;
            this.xs[b] = xa;
            this.zs[b] = za ^ xab;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_sqrt_xx(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let a = k;
            let b = k + 1;
            let zab = this.zs[a] ^ this.zs[b];
            this.xs[a] ^= zab;
            this.xs[b] ^= zab;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_sqrt_yy(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let a = k;
            let b = k + 1;

            let xa = this.xs[a];
            let za = this.zs[a];
            let xb = this.xs[b];
            let zb = this.zs[b];

            za ^= xa;
            zb ^= xb;
            xa ^= zb;
            xb ^= za;
            zb ^= xb;
            za ^= xa;

            this.xs[a] = za;
            this.zs[a] = xa;
            this.xs[b] = zb;
            this.zs[b] = xb;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_sqrt_zz(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let a = k;
            let b = k + 1;
            let xab = this.xs[a] ^ this.xs[b];
            this.zs[a] ^= xab;
            this.zs[b] ^= xab;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_xcx(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            this.xs[target] ^= this.zs[control];
            this.xs[control] ^= this.zs[target];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_xcy(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            this.xs[target] ^= this.zs[control];
            this.zs[target] ^= this.zs[control];
            this.xs[control] ^= this.xs[target];
            this.xs[control] ^= this.zs[target];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_ycy(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            let y = this.xs[control] ^ this.zs[control];
            this.xs[target] ^= y;
            this.zs[target] ^= y;
            y = this.xs[target] ^ this.zs[target];
            this.xs[control] ^= y;
            this.zs[control] ^= y;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cx(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            this.xs[target] ^= this.xs[control];
            this.zs[control] ^= this.zs[target];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cx_swap(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let c = k;
            let t = k + 1;
            let xc = this.xs[c];
            let zc = this.zs[c];
            let xt = this.xs[t];
            let zt = this.zs[t];
            this.xs[c] = xt ^ xc;
            this.zs[c] = zt;
            this.xs[t] = xc;
            this.zs[t] = zc ^ zt;
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cy(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            this.xs[target] ^= this.xs[control];
            this.zs[target] ^= this.xs[control];
            this.zs[control] ^= this.zs[target];
            this.zs[control] ^= this.xs[target];
        }
    }

    /**
     * @param {!Array<!int>} targets
     */
    do_cz(targets) {
        for (let k = 0; k < targets.length; k += 2) {
            let control = k;
            let target = k + 1;
            this.zs[target] ^= this.xs[control];
            this.zs[control] ^= this.xs[target];
        }
    }

    /**
     * @param {!Gate} gate
     * @param {!Array<!int>} targets
     */
    do_gate(gate, targets) {
        gate.frameDo(this, targets);
    }

    /**
     * @param {*} other
     * @returns {!boolean}
     */
    isEqualTo(other) {
        if (!(other instanceof PauliFrame) || other.num_frames !== this.num_frames || other.num_qubits !== this.num_qubits) {
            return false;
        }
        for (let q = 0; q < this.num_qubits; q++) {
            if (this.xs[q] !== other.xs[q] || this.zs[q] !== other.zs[q] || this.flags[q] !== other.flags[q]) {
                return false;
            }
        }
        return true;
    }

    /**
     * @returns {!string}
     */
    toString() {
        return this.to_strings().join('\n');
    }
}

export {PauliFrame}

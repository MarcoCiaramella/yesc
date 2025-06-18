import crypto from 'crypto-js';


function arr2hex(uint8Array) {
    return uint8Array.reduce((a, b) => a + b.toString(16).padStart(2, '0'), '');
}

function hex2Uint8Array(hex) {
    const bytes = []
    for (let c = 0; c < hex.length; c += 2) {
        bytes.push(parseInt(hex.substr(c, 2), 16));
    }
    return new Uint8Array(bytes);
}

function sha256d(hex) {
    return crypto.SHA256(crypto.SHA256(crypto.enc.Hex.parse(hex))).toString();
}

function swap32(val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}

function swap32Arr(arr) {
    for (let i = 0; i < 8; i++) {
        arr[i] = swap32(arr[i]);
    }
    return arr;
}

function swap32Hex(hex) {
    const arr = hex.match(/(..?)/g);
    return arr[3] + arr[2] + arr[1] + arr[0];
}


/*
   {
       "extraNonce1": "00",
       "extraNonce2Size": 8,
       "miningDiff": 8192,
       "jobId": "0$5IVz5DIvg}^i{EMKB8PbIpp^&4j{@RWhFw1$bIzctx?",
       "prevhash": "2462c7e3a39643b2976d687a294ac88d526d0b1803c3e0ee0000000000000000",
       "coinb1": "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff5f03e8690b04196dc9622f706f6f6c696e2e636f6d2ffabe6d6d368692d1a1f105567427b67a4bada69b4e21cd22d8e9b262d821e6c83f35217401000000000000005e547e3ba6a79f1a65a4d319e47dfdfd0dca63a31f",
       "coinb2": "ffffffff03a0795225000000001976a914097da706a8ff95f10141951cd481b5c307ba312c88ac0000000000000000266a24b9e11b6ddeb1f7479d9aad65a24cb4a7ce1e690cd64daa73cbacdea27e30749ff8b52ec500000000000000002b6a2952534b424c4f434b3adc5d1181726fefc6d375df815be9b14a61d910f3ee5d7410f44b94340043ff495b6d59d5",
       "merkle_branch": [
           "6939507a3a5c4b2cd6734cffae5c028d6d642c4a7468d46b4e666e903c1a7f00",
           "a3d094ef28b5db0e3b9adb23aff9c84df1c35fff7140bde9c5391359311eec5e",
           "2f5f311f035c29701ed16ebf077ff0b7c342e336eeec1ed0cdfe28d6d041c2dd",
           "f3028f657913ef387d00af6832c30c8910a0b9ff5c4af3498c037c407d05dfdc",
           "65229c42de408d8cc3405e2bc1327a31bc4704d3f901d00ee796a3413fac8542",
           "b15ab897c6dec1d6c21c6ad54129033cccc4e404bc69e1e11cf2fa7892334bf9",
           "4fb09b87b29f21a96914ca5f2c9a42ecfe4bdca8c2e646fa2fa2c7d7545827a2",
           "225fbf2cec99e6b3eb887793175627a8576b480e99f36d9aca2dd32139a71329",
           "115eedce858f9cfafdd6ce25eb47b3637f650e61771f9f9f4416fc740a111b54"
       ],
       "version": "20000000",
       "nbits": "18075395",
       "ntime": "62c96d17",
       "clean_jobs": false // If true, miners should abort their current work and immediately use the new job. If false, they can still use the current job, but should move to the new one after exhausting the current nonce range.
   }
   */


class Header {

    #header;

    /**
     * Builds the header from the work received from the stratum server (mining pool), extraNonce2 and nonce.
     * @param {object} work 
     * @param {number} extraNonce2 
     * @param {number} nonce 
     */
    constructor(work, extraNonce2, nonce) {

        const coinbase = work.coinb1 + work.extraNonce1 + extraNonce2.toString(16).padStart(work.extraNonce2Size * 2, "0") + work.coinb2;
        const coinbaseHash = sha256d(coinbase);

        let merkleRoot = coinbaseHash;
        for (const d of work.merkle_branch) {
            merkleRoot = sha256d(merkleRoot + d);
        }

        const version = swap32Hex(work.version);
        const ntime = swap32Hex(work.ntime);
        const nbits = swap32Hex(work.nbits);
        this.#header = version + this.#prevHash(work.prevhash) + merkleRoot + ntime + nbits + swap32Hex(nonce.toString(16).padStart(8, "0"));
    }

    #prevHash(prevHash) {
        const prevHashArr = new Uint8Array(new ArrayBuffer(32));
        prevHashArr.set(hex2Uint8Array(prevHash));
        swap32Arr(new Uint32Array(prevHashArr.buffer));
        return arr2hex(prevHashArr);
    }

    /**
     * Gets header in hexadecimal format.
     */
    get header() {
        return this.#header;
    }

    /**
     * Returns this header as Uint8Array.
     * @returns this header as Uint8Array
     */
    toUint8Array() {
        return hex2Uint8Array(this.#header);
    }

    /**
     * Returns this header as Uint32Array.
     * @returns this header as Uint32Array
     */
    toUint32Array() {
        return new Uint32Array(hex2Uint8Array(this.#header).buffer);
    }

    /**
     * Prints this header on console.
     * @param {string} tag 
     * @returns this
     */
    print(tag) {
        console.log(tag, this.#header);
        return this;
    }

    static test() {
        let work = {
            extraNonce1: "60021014",
            extraNonce2Size: 4,
            jobId: "5c04",
            prevhash: "da0dadb0eda4381df442bde08d23d54d7d371d5ce7af3ee716bd2a7e017eacb8",
            coinb1: "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff2a03700a08062f503253482f04953f1a5308",
            coinb2: "102f776166666c65706f6f6c2e636f6d2f0000000001d07e582a010000001976a9145d8f33b0a7c94c878d572c40cbff22a49268467d88ac00000000",
            merkle_branch: [
                "50a4a386ab344d40d29a833b6e40ea27dab6e5a79a2f8648d3bc0d1aa65ecd3f",
                "7952ecc836fb104f41b2cb06608eeeaa6d1ca2fe4391708fb13bb10ccf8da179",
                "9400ec6453aac577fb6807f11219b4243a3e50ca6d1c727e6d05663211960c94",
                "c11a630fa9332ab51d886a47509b5cbace844316f4fc52b493359b305fd489ae",
                "85891e7c5773f234d647f1d5fca7fbcabb59b261322d16c0ae486ccf5143383d",
                "faa26bbc17f99659f64136bea29b3fc8d772b339c52707d5f2ccfe1195317f43"
            ],
            version: "00000002",
            nbits: "1b10b60e",
            ntime: "531a3f95",
            clean_jobs: false
        };

        console.assert(new Header(work, 0, 1791689120).header === "02000000b0ad0dda1d38a4ede0bd42f44dd5238d5c1d377de73eafe77e2abd16b8ac7e0143c4345eb9ad9135836f5c31b697f62429c1be08d55906ff407852adfba680a5953f1a530eb6101ba001cb6a", "Test 1: header build failed");

        work = {
            extraNonce1: "67ffed53",
            extraNonce2Size: 4,
            jobId: "e0ff",
            prevhash: "dd970b967fcd7ba611c0ca4149313e2255704a820a70ead0b2ef3c2900000a0a",
            coinb1: "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4e03d93d1f0424f5016308fabe6d6d00000000000000000000000000000000000000000000000000000000000000000100000000000000",
            coinb2: "0f706f6f6c2e72706c616e742e78797a00000000020000000000000000266a24aa21a9ed889e8ce0216f207f75106adee83fac289bce8fe957fb2cca2be15c990099875a601fe70e000000001976a914bb89477996b8a915f686a1ba7109987a7b7730c188ac00000000",
            merkle_branch: [
                "cf00b937757b98d92f0eaea7c7c3139014b37266bee658cbcd60466822796328"
            ],
            version: "20000000",
            nbits: "1e0bfaf3",
            ntime: "6301f524",
            clean_jobs: true
        };

        console.assert(new Header(work, 0, 2863312587).header === "00000020960b97dda67bcd7f41cac011223e3149824a7055d0ea700a293cefb20a0a0000c1558f21a9c06e4864db4c09dfb99f396e6ca66732f59e8731bfd73d55fe84c124f50163f3fa0b1ecbaeaaaa", "Test 2: header build failed");
    }
}

Header.test();
console.log("Header class test passed.");

/*
==== Job Details ====
Job ID: 2f7c3
Previous Hash: 78d313ef2ef92873afa64dd1096c5979c916d995915909de85c93fff0000005a
Version: 20000000
nBits: 1e016449
nTime: 6851f02f
Clean Jobs: true
Merkle Branches: 0 branches
Coinb1 (first 40 chars): 0100000001000000000000000000000000000000...
Coinb2 (first 40 chars): 2f7a706f6f6c2e63612fc89a3b8f2f0000000000...
Extranonce1: id
Extranonce2 Size: 4
====================
*/

const work = {
    extraNonce1: "60021014",
    extraNonce2Size: 4,
    jobId: "5c04",
    prevhash: "da0dadb0eda4381df442bde08d23d54d7d371d5ce7af3ee716bd2a7e017eacb8",
    coinb1: "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff2a03700a08062f503253482f04953f1a5308",
    coinb2: "102f776166666c65706f6f6c2e636f6d2f0000000001d07e582a010000001976a9145d8f33b0a7c94c878d572c40cbff22a49268467d88ac00000000",
    merkle_branch: [
        "50a4a386ab344d40d29a833b6e40ea27dab6e5a79a2f8648d3bc0d1aa65ecd3f",
        "7952ecc836fb104f41b2cb06608eeeaa6d1ca2fe4391708fb13bb10ccf8da179",
        "9400ec6453aac577fb6807f11219b4243a3e50ca6d1c727e6d05663211960c94",
        "c11a630fa9332ab51d886a47509b5cbace844316f4fc52b493359b305fd489ae",
        "85891e7c5773f234d647f1d5fca7fbcabb59b261322d16c0ae486ccf5143383d",
        "faa26bbc17f99659f64136bea29b3fc8d772b339c52707d5f2ccfe1195317f43"
    ],
    version: "00000002",
    nbits: "1b10b60e",
    ntime: "531a3f95",
    clean_jobs: false
};

new Header(work, extraNonce2, nonce0).toUint8Array();
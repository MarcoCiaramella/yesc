

function swap32(val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}

function swap32Arr(arr) {
    for (let i = 0; i < 8; i++) {
        arr[i] = swap32(arr[i]);
    }
    return arr;
}

function hex2Uint8Array(hex) {
    const bytes = []
    for (let c = 0; c < hex.length; c += 2) {
        bytes.push(parseInt(hex.substr(c, 2), 16));
    }
    return new Uint8Array(bytes);
}

function calcTarget(miningDiff) {
    return (parseInt("0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 16) / miningDiff).toString(16).padStart(64, "0");
}

const target = hex2Uint8Array(calcTarget(parseFloat(process.argv[2])));
console.log(target);
console.log("Target: " + Array.from(target).map(b => b.toString(16).padStart(2, '0')).join(''));


/**
 * 
 * {"job_id":"21f41","extranonce2":"00000001","ntime":"685131a2","nonce":"00002fcc"}
 */
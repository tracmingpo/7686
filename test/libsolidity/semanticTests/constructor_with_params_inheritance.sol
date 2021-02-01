contract C {
    uint public i;
    uint public k;

    constructor(uint newI, uint newK) {
        i = newI;
        k = newK;
    }
}
contract D is C {
    constructor(uint newI, uint newK) C(newI, newK + 1) {}
}
// ====
// compileViaYul: also
// ----
// constructor(): 2, 0 ->
// gas ir: 220283
// gas irOptimized: 157642
// gas legacy: 154368
// gas legacyOptimized: 138726
// i() -> 2
// k() -> 1

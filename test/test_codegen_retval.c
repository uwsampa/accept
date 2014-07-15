// RUN: clang %s -emit-llvm -O0 -o - -S | FileCheck %s

#include <enerc.h>

int main () {
    return 0;
}

int fp(int p) {
    return p * 2;
    // CHECK: ret i32 %mul, !quals !0
}

APPROX int fa(int p) {
    return p * 2;
    // CHECK: ret i32 %mul, !quals !1
}

int fp2(int p) {
    if (p > 2) {
        return p * 3;
        // CHECK: store i32 %mul, i32* %retval, !quals !0
    } else {
        return p + 3;
        // CHECK: store i32 %add, i32* %retval, !quals !0
    }
    // CHECK: ret i32 %3, !quals !0
}

APPROX int fa2(int p) {
    if (p > 2) {
        return p * 3;
        // CHECK: store i32 %mul, i32* %retval, !quals !1
    } else {
        return p + 3;
        // CHECK: store i32 %add, i32* %retval, !quals !1
    }
    // CHECK: ret i32 %3, !quals !1
}

// CHECK: !0 = metadata !{i32 0}
// CHECK: !1 = metadata !{i32 1}

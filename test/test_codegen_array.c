// RUN: clang %s -emit-llvm -O0 -o - -S | FileCheck %s

#include <enerc.h>

int main () {
    int p[3];
    p[0] = 5.0f;
    // CHECK: store i32 5, i32* %arrayidx, align 4, !quals !0

    APPROX int a;
    a = 5.0;
    // CHECK: store i32 5, i32* %a, align 4, !quals !1

    APPROX int d1[3];
    d1[0] = 5.0f;
    // CHECK: store i32 5, i32* %arrayidx1, align 4, !quals !1

    APPROX int d2[3][3];
    d2[0][0] = 5.0f;
    // CHECK: store i32 5, i32* %arrayidx3, align 4, !quals !1

    return 0;
}

// CHECK: !0 = metadata !{i32 0}
// CHECK: !1 = metadata !{i32 1}
// CHECK: !2 = metadata !{i32 2}

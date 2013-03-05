// RUN: clang %s -emit-llvm -O0 -o - -S | FileCheck %s

#include <enerc.h>

int main() {
    // CHECK: %x = alloca i32, align 4, !quals !0
    int x;
    // CHECK: %y = alloca i32, align 4, !quals !1
    APPROX int y;

    // CHECK: store i32 2, i32* %x, align 4, !quals !0
    x = 2;
    // CHECK: store i32 3, i32* %y, align 4, !quals !1
    y = 3;

    // CHECK: %0 = load i32* %y, align 4, !quals !1
    // CHECK: store i32 %0, i32* %i, align 4, !quals !1
    APPROX int i = y;

    // CHECK: %1 = load i32* %x, align 4, !quals !0
    // CHECK: store i32 %1, i32* %j, align 4, !quals !0
    int j = x;

    // CHECK: %2 = load i32* %x, align 4, !quals !0
    // CHECK: store i32 %2, i32* %i, align 4, !quals !1
    i = x;

    // CHECK: %3 = load i32* %x, align 4, !quals !0
    // CHECK: %4 = load i32* %y, align 4, !quals !1
    // CHECK: %add = add nsw i32 %4, %3, !quals !1
    // CHECK: store i32 %add, i32* %y, align 4, !quals !1
    y += x;

    return x;
}

// CHECK: !0 = metadata !{i32 0}
// CHECK: !1 = metadata !{i32 1}

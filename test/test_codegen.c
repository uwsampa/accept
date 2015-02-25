// RUN: clang %s -g -emit-llvm -O0 -o - -S | FileCheck %s

#include <enerc.h>

int *gpp;
APPROX int *gpa;

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

    // CHECK: %arrayidx = getelementptr inbounds [5 x i32]* %a, i32 0, i64 2, !quals !2
    APPROX int a[5];
    y = a[2];

    // CHECK: %f = getelementptr inbounds %struct.anon* %s, i32 0, i32 0, !quals !2
    struct {
        APPROX int f;
    } s;
    y = s.f;

    // CHECK: %7 = load i32** @gpp, align 8, !quals !0
    // CHECK: %arrayidx1 = getelementptr inbounds i32* %7, i64 0, !quals !0
    // CHECK: store i32 5, i32* %arrayidx1, align 4, !quals !0
    gpp[0] = 5;
    // CHECK: %8 = load i32** @gpa, align 8, !quals !2
    // CHECK: %arrayidx2 = getelementptr inbounds i32* %8, i64 0, !quals !2
    // CHECK: store i32 7, i32* %arrayidx2, align 4, !quals !1
    gpa[0] = 7;

    return x;
}

// CHECK: !0 = metadata !{i32 0}
// CHECK: !1 = metadata !{i32 1}
// CHECK: !2 = metadata !{i32 2}

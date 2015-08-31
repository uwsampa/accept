// RUN: clang %s -emit-llvm -O0 -o - -S | FileCheck %s

#include <enerc.h>

int *gpp;
APPROX int *gpa;

int main() {
    // CHECK: %x = alloca i32, align {{[0-9]+}}, !quals [[PRECISE:![0-9]+]]
    int x;
    // CHECK: %y = alloca i32, align {{[0-9]+}}, !quals [[APPROX:![0-9]+]]
    APPROX int y;

    // CHECK: store i32 2, i32* %x, align 4, !quals [[PRECISE]]
    x = 2;
    // CHECK: store i32 3, i32* %y, align 4, !quals [[APPROX]]
    y = 3;

    // CHECK: %0 = load i32* %y, align 4, !quals [[APPROX]]
    // CHECK: store i32 %0, i32* %i, align 4, !quals [[APPROX]]
    APPROX int i = y;

    // CHECK: %1 = load i32* %x, align 4, !quals [[PRECISE]]
    // CHECK: store i32 %1, i32* %j, align 4, !quals [[PRECISE]]
    int j = x;

    // CHECK: %2 = load i32* %x, align 4, !quals [[PRECISE]]
    // CHECK: store i32 %2, i32* %i, align 4, !quals [[APPROX]]
    i = x;

    // CHECK: %3 = load i32* %x, align 4, !quals [[PRECISE]]
    // CHECK: %4 = load i32* %y, align 4, !quals [[APPROX]]
    // CHECK: %add = add nsw i32 %4, %3, !quals [[APPROX]]
    // CHECK: store i32 %add, i32* %y, align 4, !quals [[APPROX]]
    y += x;

    // CHECK: %arrayidx = getelementptr inbounds [5 x i32]* %a, i32 0, i64 2, !quals [[APPROX_P:![0-9]+]]
    APPROX int a[5];
    y = a[2];

    // CHECK: %f = getelementptr inbounds %struct.anon* %s, i32 0, i32 0, !quals [[APPROX_P]]
    struct {
        APPROX int f;
    } s;
    y = s.f;

    // CHECK: %7 = load i32** @gpp, align 8, !quals [[PRECISE]]
    // CHECK: %arrayidx1 = getelementptr inbounds i32* %7, i64 0, !quals [[PRECISE]]
    // CHECK: store i32 5, i32* %arrayidx1, align 4, !quals [[PRECISE]]
    gpp[0] = 5;
    // CHECK: %8 = load i32** @gpa, align 8, !quals [[APPROX_P]]
    // CHECK: %arrayidx2 = getelementptr inbounds i32* %8, i64 0, !quals [[APPROX_P]]
    // CHECK: store i32 7, i32* %arrayidx2, align 4, !quals [[APPROX]]
    gpa[0] = 7;

    return x;
}

// CHECK-DAG: [[PRECISE]] = metadata !{i32 0}
// CHECK-DAG: [[APPROX]] = metadata !{i32 1}
// CHECK-DAG: [[APPROX_P]] = metadata !{i32 2}

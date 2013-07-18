; ModuleID = 'parsec_barrier.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.8.0"

%struct.__sFILE = type { i8*, i32, i32, i16, i16, %struct.__sbuf, i32, i8*, i32 (i8*)*, i32 (i8*, i8*, i32)*, i64 (i8*, i64, i32)*, i32 (i8*, i8*, i32)*, %struct.__sbuf, %struct.__sFILEX*, i32, [3 x i8], [1 x i8], %struct.__sbuf, i32, i64 }
%struct.__sbuf = type { i8*, i32 }
%struct.__sFILEX = type opaque
%struct.parsec_barrier_t = type { %struct._opaque_pthread_mutex_t, %struct._opaque_pthread_cond_t, i32, i32, i32 }
%struct._opaque_pthread_mutex_t = type { i64, [56 x i8] }
%struct._opaque_pthread_cond_t = type { i64, [40 x i8] }
%struct._opaque_pthread_mutexattr_t = type { i64, [8 x i8] }
%struct._opaque_pthread_condattr_t = type { i64, [8 x i8] }

@__FUNCTION__._Z19parsec_barrier_initP16parsec_barrier_tPKij = private unnamed_addr constant [20 x i8] c"parsec_barrier_init\00", align 1
@.str = private unnamed_addr constant [19 x i8] c"parsec_barrier.cpp\00", align 1
@__func__._Z22parsec_barrier_destroyP16parsec_barrier_t = private unnamed_addr constant [23 x i8] c"parsec_barrier_destroy\00", align 1
@.str1 = private unnamed_addr constant [14 x i8] c"barrier!=NULL\00", align 1
@__FUNCTION__._Z29parsec_barrierattr_setpsharedPii = private unnamed_addr constant [30 x i8] c"parsec_barrierattr_setpshared\00", align 1
@__stderrp = external global %struct.__sFILE*
@.str2 = private unnamed_addr constant [73 x i8] c"ERROR: Something in function %s in file %s, line %u is not implemented.\0A\00", align 1

define i32 @_Z19parsec_barrier_initP16parsec_barrier_tPKij(%struct.parsec_barrier_t* %barrier, i32* %attr, i32 %count) uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %barrier.addr = alloca %struct.parsec_barrier_t*, align 8
  %attr.addr = alloca i32*, align 8
  %count.addr = alloca i32, align 4
  %rv = alloca i32, align 4, !quals !2
  store %struct.parsec_barrier_t* %barrier, %struct.parsec_barrier_t** %barrier.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{%struct.parsec_barrier_t** %barrier.addr}, metadata !76), !dbg !77
  store i32* %attr, i32** %attr.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %attr.addr}, metadata !78), !dbg !77
  store i32 %count, i32* %count.addr, align 4, !quals !2
  call void @llvm.dbg.declare(metadata !{i32* %count.addr}, metadata !79), !dbg !77
  call void @llvm.dbg.declare(metadata !{i32* %rv}, metadata !80), !dbg !82
  %0 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !83, !quals !2
  %cmp = icmp eq %struct.parsec_barrier_t* %0, null, !dbg !83, !quals !2
  br i1 %cmp, label %if.then, label %if.end, !dbg !83

if.then:                                          ; preds = %entry
  store i32 22, i32* %retval, !dbg !83
  br label %return, !dbg !83

if.end:                                           ; preds = %entry
  %1 = load i32* %count.addr, align 4, !dbg !84, !quals !2
  %cmp1 = icmp ule i32 %1, 0, !dbg !84, !quals !2
  br i1 %cmp1, label %if.then2, label %if.end3, !dbg !84

if.then2:                                         ; preds = %if.end
  store i32 22, i32* %retval, !dbg !84
  br label %return, !dbg !84

if.end3:                                          ; preds = %if.end
  %2 = load i32** %attr.addr, align 8, !dbg !85, !quals !2
  %cmp4 = icmp ne i32* %2, null, !dbg !85, !quals !2
  br i1 %cmp4, label %land.lhs.true, label %if.end7, !dbg !85

land.lhs.true:                                    ; preds = %if.end3
  %3 = load i32** %attr.addr, align 8, !dbg !85, !quals !2
  %4 = load i32* %3, align 4, !dbg !85, !quals !2
  %cmp5 = icmp eq i32 %4, 1, !dbg !85, !quals !2
  br i1 %cmp5, label %if.then6, label %if.end7, !dbg !85

if.then6:                                         ; preds = %land.lhs.true
  call void @_ZL15Not_ImplementedPKcS0_j(i8* getelementptr inbounds ([20 x i8]* @__FUNCTION__._Z19parsec_barrier_initP16parsec_barrier_tPKij, i32 0, i32 0), i8* getelementptr inbounds ([19 x i8]* @.str, i32 0, i32 0), i32 69), !dbg !85
  br label %if.end7, !dbg !85

if.end7:                                          ; preds = %if.then6, %land.lhs.true, %if.end3
  %5 = load i32* %count.addr, align 4, !dbg !86, !quals !2
  %6 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !86, !quals !2
  %max = getelementptr inbounds %struct.parsec_barrier_t* %6, i32 0, i32 2, !dbg !86, !quals !2
  store i32 %5, i32* %max, align 4, !dbg !86, !quals !2
  %7 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !87, !quals !2
  %n = getelementptr inbounds %struct.parsec_barrier_t* %7, i32 0, i32 3, !dbg !87, !quals !2
  store volatile i32 0, i32* %n, align 4, !dbg !87, !quals !2
  %8 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !88, !quals !2
  %is_arrival_phase = getelementptr inbounds %struct.parsec_barrier_t* %8, i32 0, i32 4, !dbg !88, !quals !2
  store volatile i32 1, i32* %is_arrival_phase, align 4, !dbg !88, !quals !2
  %9 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !89, !quals !2
  %mutex = getelementptr inbounds %struct.parsec_barrier_t* %9, i32 0, i32 0, !dbg !89, !quals !2
  %call = call i32 @pthread_mutex_init(%struct._opaque_pthread_mutex_t* %mutex, %struct._opaque_pthread_mutexattr_t* null), !dbg !89, !quals !2
  store i32 %call, i32* %rv, align 4, !dbg !89, !quals !2
  %10 = load i32* %rv, align 4, !dbg !90, !quals !2
  %cmp8 = icmp ne i32 %10, 0, !dbg !90, !quals !2
  br i1 %cmp8, label %if.then9, label %if.end10, !dbg !90

if.then9:                                         ; preds = %if.end7
  %11 = load i32* %rv, align 4, !dbg !90, !quals !2
  store i32 %11, i32* %retval, !dbg !90
  br label %return, !dbg !90

if.end10:                                         ; preds = %if.end7
  %12 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !91, !quals !2
  %cond = getelementptr inbounds %struct.parsec_barrier_t* %12, i32 0, i32 1, !dbg !91, !quals !2
  %call11 = call i32 @"\01_pthread_cond_init"(%struct._opaque_pthread_cond_t* %cond, %struct._opaque_pthread_condattr_t* null), !dbg !91, !quals !2
  store i32 %call11, i32* %rv, align 4, !dbg !91, !quals !2
  %13 = load i32* %rv, align 4, !dbg !92, !quals !2
  store i32 %13, i32* %retval, !dbg !92
  br label %return, !dbg !92

return:                                           ; preds = %if.end10, %if.then9, %if.then2, %if.then
  %14 = load i32* %retval, !dbg !93
  ret i32 %14, !dbg !93, !quals !2
}

declare void @llvm.dbg.declare(metadata, metadata) nounwind readnone

define internal void @_ZL15Not_ImplementedPKcS0_j(i8* %function, i8* %file, i32 %line) uwtable inlinehint ssp {
entry:
  %function.addr = alloca i8*, align 8
  %file.addr = alloca i8*, align 8
  %line.addr = alloca i32, align 4
  store i8* %function, i8** %function.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i8** %function.addr}, metadata !94), !dbg !95
  store i8* %file, i8** %file.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i8** %file.addr}, metadata !96), !dbg !95
  store i32 %line, i32* %line.addr, align 4, !quals !2
  call void @llvm.dbg.declare(metadata !{i32* %line.addr}, metadata !97), !dbg !95
  %0 = load %struct.__sFILE** @__stderrp, align 8, !dbg !98, !quals !2
  %1 = load i8** %function.addr, align 8, !dbg !98, !quals !2
  %2 = load i8** %file.addr, align 8, !dbg !98, !quals !2
  %3 = load i32* %line.addr, align 4, !dbg !98, !quals !2
  %call = call i32 (%struct.__sFILE*, i8*, ...)* @fprintf(%struct.__sFILE* %0, i8* getelementptr inbounds ([73 x i8]* @.str2, i32 0, i32 0), i8* %1, i8* %2, i32 %3), !dbg !98, !quals !2
  call void @exit(i32 1) noreturn, !dbg !100
  unreachable, !dbg !100

return:                                           ; No predecessors!
  ret void, !dbg !101
}

declare i32 @pthread_mutex_init(%struct._opaque_pthread_mutex_t*, %struct._opaque_pthread_mutexattr_t*)

declare i32 @"\01_pthread_cond_init"(%struct._opaque_pthread_cond_t*, %struct._opaque_pthread_condattr_t*)

define i32 @_Z22parsec_barrier_destroyP16parsec_barrier_t(%struct.parsec_barrier_t* %barrier) uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %barrier.addr = alloca %struct.parsec_barrier_t*, align 8
  %rv = alloca i32, align 4, !quals !2
  store %struct.parsec_barrier_t* %barrier, %struct.parsec_barrier_t** %barrier.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{%struct.parsec_barrier_t** %barrier.addr}, metadata !102), !dbg !103
  call void @llvm.dbg.declare(metadata !{i32* %rv}, metadata !104), !dbg !106
  %0 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !107, !quals !2
  %cmp = icmp ne %struct.parsec_barrier_t* %0, null, !dbg !107, !quals !2
  %lnot = xor i1 %cmp, true, !dbg !107, !quals !2
  %conv = zext i1 %lnot to i64, !dbg !107, !quals !2
  %expval = call i64 @llvm.expect.i64(i64 %conv, i64 0), !dbg !107, !quals !2
  %tobool = icmp ne i64 %expval, 0, !dbg !107, !quals !2
  br i1 %tobool, label %cond.true, label %cond.false, !dbg !107

cond.true:                                        ; preds = %entry
  call void @__assert_rtn(i8* getelementptr inbounds ([23 x i8]* @__func__._Z22parsec_barrier_destroyP16parsec_barrier_t, i32 0, i32 0), i8* getelementptr inbounds ([19 x i8]* @.str, i32 0, i32 0), i32 84, i8* getelementptr inbounds ([14 x i8]* @.str1, i32 0, i32 0)) noreturn, !dbg !107
  unreachable, !dbg !107
                                                  ; No predecessors!
  br label %cond.end, !dbg !107

cond.false:                                       ; preds = %entry
  br label %cond.end, !dbg !107

cond.end:                                         ; preds = %cond.false, %1
  %2 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !108, !quals !2
  %mutex = getelementptr inbounds %struct.parsec_barrier_t* %2, i32 0, i32 0, !dbg !108, !quals !2
  %call = call i32 @pthread_mutex_destroy(%struct._opaque_pthread_mutex_t* %mutex), !dbg !108, !quals !2
  store i32 %call, i32* %rv, align 4, !dbg !108, !quals !2
  %3 = load i32* %rv, align 4, !dbg !109, !quals !2
  %cmp1 = icmp ne i32 %3, 0, !dbg !109, !quals !2
  br i1 %cmp1, label %if.then, label %if.end, !dbg !109

if.then:                                          ; preds = %cond.end
  %4 = load i32* %rv, align 4, !dbg !109, !quals !2
  store i32 %4, i32* %retval, !dbg !109
  br label %return, !dbg !109

if.end:                                           ; preds = %cond.end
  %5 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !110, !quals !2
  %cond = getelementptr inbounds %struct.parsec_barrier_t* %5, i32 0, i32 1, !dbg !110, !quals !2
  %call2 = call i32 @pthread_cond_destroy(%struct._opaque_pthread_cond_t* %cond), !dbg !110, !quals !2
  store i32 %call2, i32* %rv, align 4, !dbg !110, !quals !2
  %6 = load i32* %rv, align 4, !dbg !111, !quals !2
  %cmp3 = icmp ne i32 %6, 0, !dbg !111, !quals !2
  br i1 %cmp3, label %if.then4, label %if.end5, !dbg !111

if.then4:                                         ; preds = %if.end
  %7 = load i32* %rv, align 4, !dbg !111, !quals !2
  store i32 %7, i32* %retval, !dbg !111
  br label %return, !dbg !111

if.end5:                                          ; preds = %if.end
  %8 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !112, !quals !2
  %n = getelementptr inbounds %struct.parsec_barrier_t* %8, i32 0, i32 3, !dbg !112, !quals !2
  %9 = load volatile i32* %n, align 4, !dbg !112, !quals !2
  %cmp6 = icmp ne i32 %9, 0, !dbg !112, !quals !2
  br i1 %cmp6, label %if.then7, label %if.end8, !dbg !112

if.then7:                                         ; preds = %if.end5
  store i32 16, i32* %retval, !dbg !112
  br label %return, !dbg !112

if.end8:                                          ; preds = %if.end5
  store i32 0, i32* %retval, !dbg !113
  br label %return, !dbg !113

return:                                           ; preds = %if.end8, %if.then7, %if.then4, %if.then
  %10 = load i32* %retval, !dbg !114
  ret i32 %10, !dbg !114, !quals !2
}

declare i64 @llvm.expect.i64(i64, i64) nounwind readnone

declare void @__assert_rtn(i8*, i8*, i32, i8*) noreturn

declare i32 @pthread_mutex_destroy(%struct._opaque_pthread_mutex_t*)

declare i32 @pthread_cond_destroy(%struct._opaque_pthread_cond_t*)

define i32 @_Z26parsec_barrierattr_destroyPi(i32* %attr) nounwind uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %attr.addr = alloca i32*, align 8
  store i32* %attr, i32** %attr.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %attr.addr}, metadata !115), !dbg !116
  %0 = load i32** %attr.addr, align 8, !dbg !117, !quals !2
  %cmp = icmp eq i32* %0, null, !dbg !117, !quals !2
  br i1 %cmp, label %if.then, label %if.end, !dbg !117

if.then:                                          ; preds = %entry
  store i32 22, i32* %retval, !dbg !117
  br label %return, !dbg !117

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval, !dbg !119
  br label %return, !dbg !119

return:                                           ; preds = %if.end, %if.then
  %1 = load i32* %retval, !dbg !120
  ret i32 %1, !dbg !120, !quals !2
}

define i32 @_Z23parsec_barrierattr_initPi(i32* %attr) nounwind uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %attr.addr = alloca i32*, align 8
  store i32* %attr, i32** %attr.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %attr.addr}, metadata !121), !dbg !122
  %0 = load i32** %attr.addr, align 8, !dbg !123, !quals !2
  %cmp = icmp eq i32* %0, null, !dbg !123, !quals !2
  br i1 %cmp, label %if.then, label %if.end, !dbg !123

if.then:                                          ; preds = %entry
  store i32 22, i32* %retval, !dbg !123
  br label %return, !dbg !123

if.end:                                           ; preds = %entry
  store i32 0, i32* %retval, !dbg !125
  br label %return, !dbg !125

return:                                           ; preds = %if.end, %if.then
  %1 = load i32* %retval, !dbg !126
  ret i32 %1, !dbg !126, !quals !2
}

define i32 @_Z29parsec_barrierattr_getpsharedPKiPi(i32* %attr, i32* %pshared) nounwind uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %attr.addr = alloca i32*, align 8
  %pshared.addr = alloca i32*, align 8
  store i32* %attr, i32** %attr.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %attr.addr}, metadata !127), !dbg !128
  store i32* %pshared, i32** %pshared.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %pshared.addr}, metadata !129), !dbg !128
  %0 = load i32** %attr.addr, align 8, !dbg !130, !quals !2
  %cmp = icmp eq i32* %0, null, !dbg !130, !quals !2
  br i1 %cmp, label %if.then, label %lor.lhs.false, !dbg !130

lor.lhs.false:                                    ; preds = %entry
  %1 = load i32** %pshared.addr, align 8, !dbg !130, !quals !2
  %cmp1 = icmp eq i32* %1, null, !dbg !130, !quals !2
  br i1 %cmp1, label %if.then, label %if.end, !dbg !130

if.then:                                          ; preds = %lor.lhs.false, %entry
  store i32 22, i32* %retval, !dbg !130
  br label %return, !dbg !130

if.end:                                           ; preds = %lor.lhs.false
  %2 = load i32** %attr.addr, align 8, !dbg !132, !quals !2
  %3 = load i32* %2, align 4, !dbg !132, !quals !2
  %4 = load i32** %pshared.addr, align 8, !dbg !132, !quals !2
  store i32 %3, i32* %4, align 4, !dbg !132, !quals !2
  store i32 0, i32* %retval, !dbg !133
  br label %return, !dbg !133

return:                                           ; preds = %if.end, %if.then
  %5 = load i32* %retval, !dbg !134
  ret i32 %5, !dbg !134, !quals !2
}

define i32 @_Z29parsec_barrierattr_setpsharedPii(i32* %attr, i32 %pshared) uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %attr.addr = alloca i32*, align 8
  %pshared.addr = alloca i32, align 4
  store i32* %attr, i32** %attr.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{i32** %attr.addr}, metadata !135), !dbg !136
  store i32 %pshared, i32* %pshared.addr, align 4, !quals !2
  call void @llvm.dbg.declare(metadata !{i32* %pshared.addr}, metadata !137), !dbg !136
  %0 = load i32** %attr.addr, align 8, !dbg !138, !quals !2
  %cmp = icmp eq i32* %0, null, !dbg !138, !quals !2
  br i1 %cmp, label %if.then, label %if.end, !dbg !138

if.then:                                          ; preds = %entry
  store i32 22, i32* %retval, !dbg !138
  br label %return, !dbg !138

if.end:                                           ; preds = %entry
  %1 = load i32* %pshared.addr, align 4, !dbg !140, !quals !2
  %cmp1 = icmp ne i32 %1, 0, !dbg !140, !quals !2
  br i1 %cmp1, label %land.lhs.true, label %if.end4, !dbg !140

land.lhs.true:                                    ; preds = %if.end
  %2 = load i32* %pshared.addr, align 4, !dbg !140, !quals !2
  %cmp2 = icmp ne i32 %2, 1, !dbg !140, !quals !2
  br i1 %cmp2, label %if.then3, label %if.end4, !dbg !140

if.then3:                                         ; preds = %land.lhs.true
  store i32 22, i32* %retval, !dbg !140
  br label %return, !dbg !140

if.end4:                                          ; preds = %land.lhs.true, %if.end
  %3 = load i32* %pshared.addr, align 4, !dbg !141, !quals !2
  %cmp5 = icmp ne i32 %3, 1, !dbg !141, !quals !2
  br i1 %cmp5, label %if.then6, label %if.end7, !dbg !141

if.then6:                                         ; preds = %if.end4
  call void @_ZL15Not_ImplementedPKcS0_j(i8* getelementptr inbounds ([30 x i8]* @__FUNCTION__._Z29parsec_barrierattr_setpsharedPii, i32 0, i32 0), i8* getelementptr inbounds ([19 x i8]* @.str, i32 0, i32 0), i32 121), !dbg !141
  br label %if.end7, !dbg !141

if.end7:                                          ; preds = %if.then6, %if.end4
  %4 = load i32* %pshared.addr, align 4, !dbg !142, !quals !2
  %5 = load i32** %attr.addr, align 8, !dbg !142, !quals !2
  store i32 %4, i32* %5, align 4, !dbg !142, !quals !2
  store i32 0, i32* %retval, !dbg !143
  br label %return, !dbg !143

return:                                           ; preds = %if.end7, %if.then3, %if.then
  %6 = load i32* %retval, !dbg !144
  ret i32 %6, !dbg !144, !quals !2
}

define i32 @_Z19parsec_barrier_waitP16parsec_barrier_t(%struct.parsec_barrier_t* %barrier) uwtable ssp {
entry:
  %retval = alloca i32, align 4
  %barrier.addr = alloca %struct.parsec_barrier_t*, align 8
  %master = alloca i32, align 4, !quals !2
  %rv = alloca i32, align 4, !quals !2
  %i = alloca i64, align 8, !quals !2
  %i46 = alloca i64, align 8, !quals !2
  store %struct.parsec_barrier_t* %barrier, %struct.parsec_barrier_t** %barrier.addr, align 8, !quals !2
  call void @llvm.dbg.declare(metadata !{%struct.parsec_barrier_t** %barrier.addr}, metadata !145), !dbg !146
  call void @llvm.dbg.declare(metadata !{i32* %master}, metadata !147), !dbg !149
  call void @llvm.dbg.declare(metadata !{i32* %rv}, metadata !150), !dbg !151
  %0 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !152, !quals !2
  %cmp = icmp eq %struct.parsec_barrier_t* %0, null, !dbg !152, !quals !2
  br i1 %cmp, label %if.then, label %if.end, !dbg !152

if.then:                                          ; preds = %entry
  store i32 22, i32* %retval, !dbg !152
  br label %return, !dbg !152

if.end:                                           ; preds = %entry
  %1 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !153, !quals !2
  %mutex = getelementptr inbounds %struct.parsec_barrier_t* %1, i32 0, i32 0, !dbg !153, !quals !2
  %call = call i32 @pthread_mutex_lock(%struct._opaque_pthread_mutex_t* %mutex), !dbg !153, !quals !2
  store i32 %call, i32* %rv, align 4, !dbg !153, !quals !2
  %2 = load i32* %rv, align 4, !dbg !154, !quals !2
  %cmp1 = icmp ne i32 %2, 0, !dbg !154, !quals !2
  br i1 %cmp1, label %if.then2, label %if.end3, !dbg !154

if.then2:                                         ; preds = %if.end
  %3 = load i32* %rv, align 4, !dbg !154, !quals !2
  store i32 %3, i32* %retval, !dbg !154
  br label %return, !dbg !154

if.end3:                                          ; preds = %if.end
  %4 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !155, !quals !2
  %is_arrival_phase = getelementptr inbounds %struct.parsec_barrier_t* %4, i32 0, i32 4, !dbg !155, !quals !2
  %5 = load volatile i32* %is_arrival_phase, align 4, !dbg !155, !quals !2
  %tobool = icmp ne i32 %5, 0, !dbg !155, !quals !2
  br i1 %tobool, label %if.end31, label %if.then4, !dbg !155

if.then4:                                         ; preds = %if.end3
  %6 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !156, !quals !2
  %mutex5 = getelementptr inbounds %struct.parsec_barrier_t* %6, i32 0, i32 0, !dbg !156, !quals !2
  %call6 = call i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t* %mutex5), !dbg !156, !quals !2
  call void @llvm.dbg.declare(metadata !{i64* %i}, metadata !158), !dbg !160
  store volatile i64 0, i64* %i, align 8, !dbg !160, !quals !2
  br label %while.cond, !dbg !161

while.cond:                                       ; preds = %while.body, %if.then4
  %7 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !161, !quals !2
  %is_arrival_phase7 = getelementptr inbounds %struct.parsec_barrier_t* %7, i32 0, i32 4, !dbg !161, !quals !2
  %8 = load volatile i32* %is_arrival_phase7, align 4, !dbg !161, !quals !2
  %tobool8 = icmp ne i32 %8, 0, !dbg !161, !quals !2
  br i1 %tobool8, label %land.end, label %land.rhs, !dbg !161

land.rhs:                                         ; preds = %while.cond
  %9 = load volatile i64* %i, align 8, !dbg !161, !quals !2
  %cmp9 = icmp ult i64 %9, 35000, !dbg !161, !quals !2
  br label %land.end

land.end:                                         ; preds = %land.rhs, %while.cond
  %10 = phi i1 [ false, %while.cond ], [ %cmp9, %land.rhs ], !quals !2
  br i1 %10, label %while.body, label %while.end

while.body:                                       ; preds = %land.end
  %11 = load volatile i64* %i, align 8, !dbg !161, !quals !2
  %inc = add i64 %11, 1, !dbg !161
  store volatile i64 %inc, i64* %i, align 8, !dbg !161, !quals !2
  br label %while.cond, !dbg !161

while.end:                                        ; preds = %land.end
  br label %while.cond10, !dbg !162

while.cond10:                                     ; preds = %while.body14, %while.end
  %12 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !162, !quals !2
  %mutex11 = getelementptr inbounds %struct.parsec_barrier_t* %12, i32 0, i32 0, !dbg !162, !quals !2
  %call12 = call i32 @pthread_mutex_trylock(%struct._opaque_pthread_mutex_t* %mutex11), !dbg !162, !quals !2
  store i32 %call12, i32* %rv, align 4, !dbg !162, !quals !2
  %cmp13 = icmp eq i32 %call12, 16, !dbg !162, !quals !2
  br i1 %cmp13, label %while.body14, label %while.end15, !dbg !162

while.body14:                                     ; preds = %while.cond10
  br label %while.cond10, !dbg !162

while.end15:                                      ; preds = %while.cond10
  %13 = load i32* %rv, align 4, !dbg !163, !quals !2
  %cmp16 = icmp ne i32 %13, 0, !dbg !163, !quals !2
  br i1 %cmp16, label %if.then17, label %if.end18, !dbg !163

if.then17:                                        ; preds = %while.end15
  %14 = load i32* %rv, align 4, !dbg !163, !quals !2
  store i32 %14, i32* %retval, !dbg !163
  br label %return, !dbg !163

if.end18:                                         ; preds = %while.end15
  br label %while.cond19, !dbg !164

while.cond19:                                     ; preds = %if.end29, %if.end18
  %15 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !164, !quals !2
  %is_arrival_phase20 = getelementptr inbounds %struct.parsec_barrier_t* %15, i32 0, i32 4, !dbg !164, !quals !2
  %16 = load volatile i32* %is_arrival_phase20, align 4, !dbg !164, !quals !2
  %tobool21 = icmp ne i32 %16, 0, !dbg !164, !quals !2
  %lnot = xor i1 %tobool21, true, !dbg !164, !quals !2
  br i1 %lnot, label %while.body22, label %while.end30, !dbg !164

while.body22:                                     ; preds = %while.cond19
  %17 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !165, !quals !2
  %cond = getelementptr inbounds %struct.parsec_barrier_t* %17, i32 0, i32 1, !dbg !165, !quals !2
  %18 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !165, !quals !2
  %mutex23 = getelementptr inbounds %struct.parsec_barrier_t* %18, i32 0, i32 0, !dbg !165, !quals !2
  %call24 = call i32 @"\01_pthread_cond_wait"(%struct._opaque_pthread_cond_t* %cond, %struct._opaque_pthread_mutex_t* %mutex23), !dbg !165, !quals !2
  store i32 %call24, i32* %rv, align 4, !dbg !165, !quals !2
  %19 = load i32* %rv, align 4, !dbg !167, !quals !2
  %cmp25 = icmp ne i32 %19, 0, !dbg !167, !quals !2
  br i1 %cmp25, label %if.then26, label %if.end29, !dbg !167

if.then26:                                        ; preds = %while.body22
  %20 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !168, !quals !2
  %mutex27 = getelementptr inbounds %struct.parsec_barrier_t* %20, i32 0, i32 0, !dbg !168, !quals !2
  %call28 = call i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t* %mutex27), !dbg !168, !quals !2
  %21 = load i32* %rv, align 4, !dbg !170, !quals !2
  store i32 %21, i32* %retval, !dbg !170
  br label %return, !dbg !170

if.end29:                                         ; preds = %while.body22
  br label %while.cond19, !dbg !171

while.end30:                                      ; preds = %while.cond19
  br label %if.end31, !dbg !172

if.end31:                                         ; preds = %while.end30, %if.end3
  %22 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !173, !quals !2
  %n = getelementptr inbounds %struct.parsec_barrier_t* %22, i32 0, i32 3, !dbg !173, !quals !2
  %23 = load volatile i32* %n, align 4, !dbg !173, !quals !2
  %cmp32 = icmp eq i32 %23, 0, !dbg !173, !quals !2
  %conv = zext i1 %cmp32 to i32, !dbg !173, !quals !2
  store i32 %conv, i32* %master, align 4, !dbg !173, !quals !2
  %24 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !174, !quals !2
  %n33 = getelementptr inbounds %struct.parsec_barrier_t* %24, i32 0, i32 3, !dbg !174, !quals !2
  %25 = load volatile i32* %n33, align 4, !dbg !174, !quals !2
  %inc34 = add i32 %25, 1, !dbg !174
  store volatile i32 %inc34, i32* %n33, align 4, !dbg !174, !quals !2
  %26 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !175, !quals !2
  %n35 = getelementptr inbounds %struct.parsec_barrier_t* %26, i32 0, i32 3, !dbg !175, !quals !2
  %27 = load volatile i32* %n35, align 4, !dbg !175, !quals !2
  %28 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !175, !quals !2
  %max = getelementptr inbounds %struct.parsec_barrier_t* %28, i32 0, i32 2, !dbg !175, !quals !2
  %29 = load i32* %max, align 4, !dbg !175, !quals !2
  %cmp36 = icmp uge i32 %27, %29, !dbg !175, !quals !2
  br i1 %cmp36, label %if.then37, label %if.else, !dbg !175

if.then37:                                        ; preds = %if.end31
  %30 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !176, !quals !2
  %is_arrival_phase38 = getelementptr inbounds %struct.parsec_barrier_t* %30, i32 0, i32 4, !dbg !176, !quals !2
  store volatile i32 0, i32* %is_arrival_phase38, align 4, !dbg !176, !quals !2
  %31 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !178, !quals !2
  %cond39 = getelementptr inbounds %struct.parsec_barrier_t* %31, i32 0, i32 1, !dbg !178, !quals !2
  %call40 = call i32 @pthread_cond_broadcast(%struct._opaque_pthread_cond_t* %cond39), !dbg !178, !quals !2
  br label %if.end79, !dbg !179

if.else:                                          ; preds = %if.end31
  %32 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !180, !quals !2
  %is_arrival_phase41 = getelementptr inbounds %struct.parsec_barrier_t* %32, i32 0, i32 4, !dbg !180, !quals !2
  %33 = load volatile i32* %is_arrival_phase41, align 4, !dbg !180, !quals !2
  %tobool42 = icmp ne i32 %33, 0, !dbg !180, !quals !2
  br i1 %tobool42, label %if.then43, label %if.end78, !dbg !180

if.then43:                                        ; preds = %if.else
  %34 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !182, !quals !2
  %mutex44 = getelementptr inbounds %struct.parsec_barrier_t* %34, i32 0, i32 0, !dbg !182, !quals !2
  %call45 = call i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t* %mutex44), !dbg !182, !quals !2
  call void @llvm.dbg.declare(metadata !{i64* %i46}, metadata !184), !dbg !185
  store volatile i64 0, i64* %i46, align 8, !dbg !185, !quals !2
  br label %while.cond47, !dbg !186

while.cond47:                                     ; preds = %while.body53, %if.then43
  %35 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !186, !quals !2
  %is_arrival_phase48 = getelementptr inbounds %struct.parsec_barrier_t* %35, i32 0, i32 4, !dbg !186, !quals !2
  %36 = load volatile i32* %is_arrival_phase48, align 4, !dbg !186, !quals !2
  %tobool49 = icmp ne i32 %36, 0, !dbg !186, !quals !2
  br i1 %tobool49, label %land.rhs50, label %land.end52, !dbg !186

land.rhs50:                                       ; preds = %while.cond47
  %37 = load volatile i64* %i46, align 8, !dbg !186, !quals !2
  %cmp51 = icmp ult i64 %37, 35000, !dbg !186, !quals !2
  br label %land.end52

land.end52:                                       ; preds = %land.rhs50, %while.cond47
  %38 = phi i1 [ false, %while.cond47 ], [ %cmp51, %land.rhs50 ], !quals !2
  br i1 %38, label %while.body53, label %while.end55

while.body53:                                     ; preds = %land.end52
  %39 = load volatile i64* %i46, align 8, !dbg !186, !quals !2
  %inc54 = add i64 %39, 1, !dbg !186
  store volatile i64 %inc54, i64* %i46, align 8, !dbg !186, !quals !2
  br label %while.cond47, !dbg !186

while.end55:                                      ; preds = %land.end52
  br label %while.cond56, !dbg !187

while.cond56:                                     ; preds = %while.body60, %while.end55
  %40 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !187, !quals !2
  %mutex57 = getelementptr inbounds %struct.parsec_barrier_t* %40, i32 0, i32 0, !dbg !187, !quals !2
  %call58 = call i32 @pthread_mutex_trylock(%struct._opaque_pthread_mutex_t* %mutex57), !dbg !187, !quals !2
  store i32 %call58, i32* %rv, align 4, !dbg !187, !quals !2
  %cmp59 = icmp eq i32 %call58, 16, !dbg !187, !quals !2
  br i1 %cmp59, label %while.body60, label %while.end61, !dbg !187

while.body60:                                     ; preds = %while.cond56
  br label %while.cond56, !dbg !187

while.end61:                                      ; preds = %while.cond56
  %41 = load i32* %rv, align 4, !dbg !188, !quals !2
  %cmp62 = icmp ne i32 %41, 0, !dbg !188, !quals !2
  br i1 %cmp62, label %if.then63, label %if.end64, !dbg !188

if.then63:                                        ; preds = %while.end61
  %42 = load i32* %rv, align 4, !dbg !188, !quals !2
  store i32 %42, i32* %retval, !dbg !188
  br label %return, !dbg !188

if.end64:                                         ; preds = %while.end61
  br label %while.cond65, !dbg !189

while.cond65:                                     ; preds = %if.end76, %if.end64
  %43 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !189, !quals !2
  %is_arrival_phase66 = getelementptr inbounds %struct.parsec_barrier_t* %43, i32 0, i32 4, !dbg !189, !quals !2
  %44 = load volatile i32* %is_arrival_phase66, align 4, !dbg !189, !quals !2
  %tobool67 = icmp ne i32 %44, 0, !dbg !189, !quals !2
  br i1 %tobool67, label %while.body68, label %while.end77, !dbg !189

while.body68:                                     ; preds = %while.cond65
  %45 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !190, !quals !2
  %cond69 = getelementptr inbounds %struct.parsec_barrier_t* %45, i32 0, i32 1, !dbg !190, !quals !2
  %46 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !190, !quals !2
  %mutex70 = getelementptr inbounds %struct.parsec_barrier_t* %46, i32 0, i32 0, !dbg !190, !quals !2
  %call71 = call i32 @"\01_pthread_cond_wait"(%struct._opaque_pthread_cond_t* %cond69, %struct._opaque_pthread_mutex_t* %mutex70), !dbg !190, !quals !2
  store i32 %call71, i32* %rv, align 4, !dbg !190, !quals !2
  %47 = load i32* %rv, align 4, !dbg !192, !quals !2
  %cmp72 = icmp ne i32 %47, 0, !dbg !192, !quals !2
  br i1 %cmp72, label %if.then73, label %if.end76, !dbg !192

if.then73:                                        ; preds = %while.body68
  %48 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !193, !quals !2
  %mutex74 = getelementptr inbounds %struct.parsec_barrier_t* %48, i32 0, i32 0, !dbg !193, !quals !2
  %call75 = call i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t* %mutex74), !dbg !193, !quals !2
  %49 = load i32* %rv, align 4, !dbg !195, !quals !2
  store i32 %49, i32* %retval, !dbg !195
  br label %return, !dbg !195

if.end76:                                         ; preds = %while.body68
  br label %while.cond65, !dbg !196

while.end77:                                      ; preds = %while.cond65
  br label %if.end78, !dbg !197

if.end78:                                         ; preds = %while.end77, %if.else
  br label %if.end79

if.end79:                                         ; preds = %if.end78, %if.then37
  %50 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !198, !quals !2
  %n80 = getelementptr inbounds %struct.parsec_barrier_t* %50, i32 0, i32 3, !dbg !198, !quals !2
  %51 = load volatile i32* %n80, align 4, !dbg !198, !quals !2
  %dec = add i32 %51, -1, !dbg !198
  store volatile i32 %dec, i32* %n80, align 4, !dbg !198, !quals !2
  %52 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !199, !quals !2
  %n81 = getelementptr inbounds %struct.parsec_barrier_t* %52, i32 0, i32 3, !dbg !199, !quals !2
  %53 = load volatile i32* %n81, align 4, !dbg !199, !quals !2
  %cmp82 = icmp eq i32 %53, 0, !dbg !199, !quals !2
  br i1 %cmp82, label %if.then83, label %if.end87, !dbg !199

if.then83:                                        ; preds = %if.end79
  %54 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !200, !quals !2
  %is_arrival_phase84 = getelementptr inbounds %struct.parsec_barrier_t* %54, i32 0, i32 4, !dbg !200, !quals !2
  store volatile i32 1, i32* %is_arrival_phase84, align 4, !dbg !200, !quals !2
  %55 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !202, !quals !2
  %cond85 = getelementptr inbounds %struct.parsec_barrier_t* %55, i32 0, i32 1, !dbg !202, !quals !2
  %call86 = call i32 @pthread_cond_broadcast(%struct._opaque_pthread_cond_t* %cond85), !dbg !202, !quals !2
  br label %if.end87, !dbg !203

if.end87:                                         ; preds = %if.then83, %if.end79
  %56 = load %struct.parsec_barrier_t** %barrier.addr, align 8, !dbg !204, !quals !2
  %mutex88 = getelementptr inbounds %struct.parsec_barrier_t* %56, i32 0, i32 0, !dbg !204, !quals !2
  %call89 = call i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t* %mutex88), !dbg !204, !quals !2
  %57 = load i32* %master, align 4, !dbg !205, !quals !2
  %tobool90 = icmp ne i32 %57, 0, !dbg !205, !quals !2
  %cond91 = select i1 %tobool90, i32 4, i32 0, !dbg !205, !quals !2
  store i32 %cond91, i32* %retval, !dbg !205
  br label %return, !dbg !205

return:                                           ; preds = %if.end87, %if.then73, %if.then63, %if.then26, %if.then17, %if.then2, %if.then
  %58 = load i32* %retval, !dbg !206
  ret i32 %58, !dbg !206, !quals !2
}

declare i32 @pthread_mutex_lock(%struct._opaque_pthread_mutex_t*)

declare i32 @pthread_mutex_unlock(%struct._opaque_pthread_mutex_t*)

declare i32 @pthread_mutex_trylock(%struct._opaque_pthread_mutex_t*)

declare i32 @"\01_pthread_cond_wait"(%struct._opaque_pthread_cond_t*, %struct._opaque_pthread_mutex_t*)

declare i32 @pthread_cond_broadcast(%struct._opaque_pthread_cond_t*)

declare i32 @fprintf(%struct.__sFILE*, i8*, ...)

declare void @exit(i32) noreturn

!llvm.dbg.cu = !{!0}

!0 = metadata !{i32 786449, i32 0, i32 4, metadata !"parsec_barrier.cpp", metadata !"/Users/asampson/uw/research/enerc/apps/canneal", metadata !"clang version 3.2 ", i1 true, i1 false, metadata !"", i32 0, metadata !1, metadata !1, metadata !3, metadata !65} ; [ DW_TAG_compile_unit ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !2}
!2 = metadata !{i32 0}
!3 = metadata !{metadata !4}
!4 = metadata !{metadata !5, metadata !45, metadata !48, metadata !52, metadata !53, metadata !56, metadata !59, metadata !60}
!5 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrier_init", metadata !"parsec_barrier_init", metadata !"_Z19parsec_barrier_initP16parsec_barrier_tPKij", metadata !6, i32 50, metadata !7, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (%struct.parsec_barrier_t*, i32*, i32)* @_Z19parsec_barrier_initP16parsec_barrier_tPKij, null, null, metadata !1, i32 50} ; [ DW_TAG_subprogram ] [line 50] [def] [parsec_barrier_init]
!6 = metadata !{i32 786473, metadata !"parsec_barrier.cpp", metadata !"/Users/asampson/uw/research/enerc/apps/canneal", null} ; [ DW_TAG_file_type ]
!7 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !8, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!8 = metadata !{metadata !9, metadata !10, metadata !43, metadata !38}
!9 = metadata !{i32 786468, null, metadata !"int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!10 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !11} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from ]
!11 = metadata !{i32 786451, null, metadata !"", metadata !12, i32 70, i64 1024, i64 64, i32 0, i32 0, null, metadata !13, i32 0, null, null} ; [ DW_TAG_structure_type ] [line 70, size 1024, align 64, offset 0] [from ]
!12 = metadata !{i32 786473, metadata !"./parsec_barrier.hpp", metadata !"/Users/asampson/uw/research/enerc/apps/canneal", null} ; [ DW_TAG_file_type ]
!13 = metadata !{metadata !14, metadata !27, metadata !37, metadata !39, metadata !41}
!14 = metadata !{i32 786445, metadata !11, metadata !"mutex", metadata !12, i32 71, i64 512, i64 64, i64 0, i32 0, metadata !15} ; [ DW_TAG_member ] [mutex] [line 71, size 512, align 64, offset 0] [from pthread_mutex_t]
!15 = metadata !{i32 786454, null, metadata !"pthread_mutex_t", metadata !12, i32 84, i64 0, i64 0, i64 0, i32 0, metadata !16} ; [ DW_TAG_typedef ] [pthread_mutex_t] [line 84, size 0, align 0, offset 0] [from __darwin_pthread_mutex_t]
!16 = metadata !{i32 786454, null, metadata !"__darwin_pthread_mutex_t", metadata !12, i32 120, i64 0, i64 0, i64 0, i32 0, metadata !17} ; [ DW_TAG_typedef ] [__darwin_pthread_mutex_t] [line 120, size 0, align 0, offset 0] [from _opaque_pthread_mutex_t]
!17 = metadata !{i32 786451, null, metadata !"_opaque_pthread_mutex_t", metadata !18, i32 67, i64 512, i64 64, i32 0, i32 0, null, metadata !19, i32 0, null, null} ; [ DW_TAG_structure_type ] [_opaque_pthread_mutex_t] [line 67, size 512, align 64, offset 0] [from ]
!18 = metadata !{i32 786473, metadata !"/usr/include/sys/_types.h", metadata !"/Users/asampson/uw/research/enerc/apps/canneal", null} ; [ DW_TAG_file_type ]
!19 = metadata !{metadata !20, metadata !22}
!20 = metadata !{i32 786445, metadata !17, metadata !"__sig", metadata !18, i32 67, i64 64, i64 64, i64 0, i32 0, metadata !21} ; [ DW_TAG_member ] [__sig] [line 67, size 64, align 64, offset 0] [from long int]
!21 = metadata !{i32 786468, null, metadata !"long int", null, i32 0, i64 64, i64 64, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [long int] [line 0, size 64, align 64, offset 0, enc DW_ATE_signed]
!22 = metadata !{i32 786445, metadata !17, metadata !"__opaque", metadata !18, i32 67, i64 448, i64 8, i64 64, i32 0, metadata !23} ; [ DW_TAG_member ] [__opaque] [line 67, size 448, align 8, offset 64] [from ]
!23 = metadata !{i32 786433, null, metadata !"", null, i32 0, i64 448, i64 8, i32 0, i32 0, metadata !24, metadata !25, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 448, align 8, offset 0] [from char]
!24 = metadata !{i32 786468, null, metadata !"char", null, i32 0, i64 8, i64 8, i64 0, i32 0, i32 6} ; [ DW_TAG_base_type ] [char] [line 0, size 8, align 8, offset 0, enc DW_ATE_signed_char]
!25 = metadata !{metadata !26}
!26 = metadata !{i32 786465, i64 0, i64 55}       ; [ DW_TAG_subrange_type ] [0, 55]
!27 = metadata !{i32 786445, metadata !11, metadata !"cond", metadata !12, i32 72, i64 384, i64 64, i64 512, i32 0, metadata !28} ; [ DW_TAG_member ] [cond] [line 72, size 384, align 64, offset 512] [from pthread_cond_t]
!28 = metadata !{i32 786454, null, metadata !"pthread_cond_t", metadata !12, i32 69, i64 0, i64 0, i64 0, i32 0, metadata !29} ; [ DW_TAG_typedef ] [pthread_cond_t] [line 69, size 0, align 0, offset 0] [from __darwin_pthread_cond_t]
!29 = metadata !{i32 786454, null, metadata !"__darwin_pthread_cond_t", metadata !12, i32 115, i64 0, i64 0, i64 0, i32 0, metadata !30} ; [ DW_TAG_typedef ] [__darwin_pthread_cond_t] [line 115, size 0, align 0, offset 0] [from _opaque_pthread_cond_t]
!30 = metadata !{i32 786451, null, metadata !"_opaque_pthread_cond_t", metadata !18, i32 65, i64 384, i64 64, i32 0, i32 0, null, metadata !31, i32 0, null, null} ; [ DW_TAG_structure_type ] [_opaque_pthread_cond_t] [line 65, size 384, align 64, offset 0] [from ]
!31 = metadata !{metadata !32, metadata !33}
!32 = metadata !{i32 786445, metadata !30, metadata !"__sig", metadata !18, i32 65, i64 64, i64 64, i64 0, i32 0, metadata !21} ; [ DW_TAG_member ] [__sig] [line 65, size 64, align 64, offset 0] [from long int]
!33 = metadata !{i32 786445, metadata !30, metadata !"__opaque", metadata !18, i32 65, i64 320, i64 8, i64 64, i32 0, metadata !34} ; [ DW_TAG_member ] [__opaque] [line 65, size 320, align 8, offset 64] [from ]
!34 = metadata !{i32 786433, null, metadata !"", null, i32 0, i64 320, i64 8, i32 0, i32 0, metadata !24, metadata !35, i32 0, i32 0} ; [ DW_TAG_array_type ] [line 0, size 320, align 8, offset 0] [from char]
!35 = metadata !{metadata !36}
!36 = metadata !{i32 786465, i64 0, i64 39}       ; [ DW_TAG_subrange_type ] [0, 39]
!37 = metadata !{i32 786445, metadata !11, metadata !"max", metadata !12, i32 73, i64 32, i64 32, i64 896, i32 0, metadata !38} ; [ DW_TAG_member ] [max] [line 73, size 32, align 32, offset 896] [from unsigned int]
!38 = metadata !{i32 786468, null, metadata !"unsigned int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ] [unsigned int] [line 0, size 32, align 32, offset 0, enc DW_ATE_unsigned]
!39 = metadata !{i32 786445, metadata !11, metadata !"n", metadata !12, i32 74, i64 32, i64 32, i64 928, i32 0, metadata !40} ; [ DW_TAG_member ] [n] [line 74, size 32, align 32, offset 928] [from ]
!40 = metadata !{i32 786485, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !38} ; [ DW_TAG_volatile_type ] [line 0, size 0, align 0, offset 0] [from unsigned int]
!41 = metadata !{i32 786445, metadata !11, metadata !"is_arrival_phase", metadata !12, i32 75, i64 32, i64 32, i64 960, i32 0, metadata !42} ; [ DW_TAG_member ] [is_arrival_phase] [line 75, size 32, align 32, offset 960] [from ]
!42 = metadata !{i32 786485, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_volatile_type ] [line 0, size 0, align 0, offset 0] [from int]
!43 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !44} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from ]
!44 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from int]
!45 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrier_destroy", metadata !"parsec_barrier_destroy", metadata !"_Z22parsec_barrier_destroyP16parsec_barrier_t", metadata !6, i32 81, metadata !46, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (%struct.parsec_barrier_t*)* @_Z22parsec_barrier_destroyP16parsec_barrier_t, null, null, metadata !1, i32 81} ; [ DW_TAG_subprogram ] [line 81] [def] [parsec_barrier_destroy]
!46 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !47, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!47 = metadata !{metadata !9, metadata !10}
!48 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrierattr_destroy", metadata !"parsec_barrierattr_destroy", metadata !"_Z26parsec_barrierattr_destroyPi", metadata !6, i32 98, metadata !49, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (i32*)* @_Z26parsec_barrierattr_destroyPi, null, null, metadata !1, i32 98} ; [ DW_TAG_subprogram ] [line 98] [def] [parsec_barrierattr_destroy]
!49 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !50, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!50 = metadata !{metadata !9, metadata !51}
!51 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !9} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from int]
!52 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrierattr_init", metadata !"parsec_barrierattr_init", metadata !"_Z23parsec_barrierattr_initPi", metadata !6, i32 104, metadata !49, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (i32*)* @_Z23parsec_barrierattr_initPi, null, null, metadata !1, i32 104} ; [ DW_TAG_subprogram ] [line 104] [def] [parsec_barrierattr_init]
!53 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrierattr_getpshared", metadata !"parsec_barrierattr_getpshared", metadata !"_Z29parsec_barrierattr_getpsharedPKiPi", metadata !6, i32 111, metadata !54, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (i32*, i32*)* @_Z29parsec_barrierattr_getpsharedPKiPi, null, null, metadata !1, i32 111} ; [ DW_TAG_subprogram ] [line 111] [def] [parsec_barrierattr_getpshared]
!54 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !55, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!55 = metadata !{metadata !9, metadata !43, metadata !51}
!56 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrierattr_setpshared", metadata !"parsec_barrierattr_setpshared", metadata !"_Z29parsec_barrierattr_setpsharedPii", metadata !6, i32 117, metadata !57, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (i32*, i32)* @_Z29parsec_barrierattr_setpsharedPii, null, null, metadata !1, i32 117} ; [ DW_TAG_subprogram ] [line 117] [def] [parsec_barrierattr_setpshared]
!57 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !58, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!58 = metadata !{metadata !9, metadata !51, metadata !9}
!59 = metadata !{i32 786478, i32 0, metadata !6, metadata !"parsec_barrier_wait", metadata !"parsec_barrier_wait", metadata !"_Z19parsec_barrier_waitP16parsec_barrier_t", metadata !6, i32 127, metadata !46, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, i32 (%struct.parsec_barrier_t*)* @_Z19parsec_barrier_waitP16parsec_barrier_t, null, null, metadata !1, i32 127} ; [ DW_TAG_subprogram ] [line 127] [def] [parsec_barrier_wait]
!60 = metadata !{i32 786478, i32 0, metadata !6, metadata !"Not_Implemented", metadata !"Not_Implemented", metadata !"_ZL15Not_ImplementedPKcS0_j", metadata !6, i32 33, metadata !61, i1 true, i1 true, i32 0, i32 0, null, i32 256, i1 false, void (i8*, i8*, i32)* @_ZL15Not_ImplementedPKcS0_j, null, null, metadata !1, i32 33} ; [ DW_TAG_subprogram ] [line 33] [local] [def] [Not_Implemented]
!61 = metadata !{i32 786453, i32 0, metadata !"", i32 0, i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !62, i32 0, i32 0} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!62 = metadata !{null, metadata !63, metadata !63, metadata !38}
!63 = metadata !{i32 786447, null, metadata !"", null, i32 0, i64 64, i64 64, i64 0, i32 0, metadata !64} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from ]
!64 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !24} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from char]
!65 = metadata !{metadata !66}
!66 = metadata !{metadata !67, metadata !70, metadata !67, metadata !67, metadata !71, metadata !71, metadata !75}
!67 = metadata !{i32 786484, i32 0, metadata !12, metadata !"PARSEC_PROCESS_PRIVATE", metadata !"PARSEC_PROCESS_PRIVATE", metadata !"PARSEC_PROCESS_PRIVATE", metadata !12, i32 81, metadata !68, i32 1, i32 1, i32 1} ; [ DW_TAG_variable ] [PARSEC_PROCESS_PRIVATE] [line 81] [local] [def]
!68 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !69} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from parsec_barrierattr_t]
!69 = metadata !{i32 786454, null, metadata !"parsec_barrierattr_t", metadata !12, i32 67, i64 0, i64 0, i64 0, i32 0, metadata !9} ; [ DW_TAG_typedef ] [parsec_barrierattr_t] [line 67, size 0, align 0, offset 0] [from int]
!70 = metadata !{i32 786484, i32 0, metadata !12, metadata !"PARSEC_PROCESS_SHARED", metadata !"PARSEC_PROCESS_SHARED", metadata !"PARSEC_PROCESS_SHARED", metadata !12, i32 80, metadata !68, i32 1, i32 1, i32 0} ; [ DW_TAG_variable ] [PARSEC_PROCESS_SHARED] [line 80] [local] [def]
!71 = metadata !{i32 786484, i32 0, metadata !6, metadata !"SPIN_COUNTER_MAX", metadata !"SPIN_COUNTER_MAX", metadata !"SPIN_COUNTER_MAX", metadata !6, i32 45, metadata !72, i32 1, i32 1, i64 35000} ; [ DW_TAG_variable ] [SPIN_COUNTER_MAX] [line 45] [local] [def]
!72 = metadata !{i32 786470, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !73} ; [ DW_TAG_const_type ] [line 0, size 0, align 0, offset 0] [from spin_counter_t]
!73 = metadata !{i32 786454, null, metadata !"spin_counter_t", metadata !6, i32 40, i64 0, i64 0, i64 0, i32 0, metadata !74} ; [ DW_TAG_typedef ] [spin_counter_t] [line 40, size 0, align 0, offset 0] [from long long unsigned int]
!74 = metadata !{i32 786468, null, metadata !"long long unsigned int", null, i32 0, i64 64, i64 64, i64 0, i32 0, i32 7} ; [ DW_TAG_base_type ] [long long unsigned int] [line 0, size 64, align 64, offset 0, enc DW_ATE_unsigned]
!75 = metadata !{i32 786484, i32 0, metadata !12, metadata !"PARSEC_BARRIER_SERIAL_THREAD", metadata !"PARSEC_BARRIER_SERIAL_THREAD", metadata !"PARSEC_BARRIER_SERIAL_THREAD", metadata !12, i32 79, metadata !44, i32 1, i32 1, i32 4} ; [ DW_TAG_variable ] [PARSEC_BARRIER_SERIAL_THREAD] [line 79] [local] [def]
!76 = metadata !{i32 786689, metadata !5, metadata !"barrier", metadata !6, i32 16777266, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [barrier] [line 50]
!77 = metadata !{i32 50, i32 0, metadata !5, null}
!78 = metadata !{i32 786689, metadata !5, metadata !"attr", metadata !6, i32 33554482, metadata !43, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [attr] [line 50]
!79 = metadata !{i32 786689, metadata !5, metadata !"count", metadata !6, i32 50331698, metadata !38, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [count] [line 50]
!80 = metadata !{i32 786688, metadata !81, metadata !"rv", metadata !6, i32 51, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [rv] [line 51]
!81 = metadata !{i32 786443, metadata !5, i32 50, i32 0, metadata !6, i32 0} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!82 = metadata !{i32 51, i32 0, metadata !81, null}
!83 = metadata !{i32 66, i32 0, metadata !81, null}
!84 = metadata !{i32 67, i32 0, metadata !81, null}
!85 = metadata !{i32 69, i32 0, metadata !81, null}
!86 = metadata !{i32 71, i32 0, metadata !81, null}
!87 = metadata !{i32 72, i32 0, metadata !81, null}
!88 = metadata !{i32 73, i32 0, metadata !81, null}
!89 = metadata !{i32 75, i32 0, metadata !81, null}
!90 = metadata !{i32 76, i32 0, metadata !81, null}
!91 = metadata !{i32 77, i32 0, metadata !81, null}
!92 = metadata !{i32 78, i32 0, metadata !81, null}
!93 = metadata !{i32 79, i32 0, metadata !81, null}
!94 = metadata !{i32 786689, metadata !60, metadata !"function", metadata !6, i32 16777249, metadata !63, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [function] [line 33]
!95 = metadata !{i32 33, i32 0, metadata !60, null}
!96 = metadata !{i32 786689, metadata !60, metadata !"file", metadata !6, i32 33554465, metadata !63, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [file] [line 33]
!97 = metadata !{i32 786689, metadata !60, metadata !"line", metadata !6, i32 50331681, metadata !38, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [line] [line 33]
!98 = metadata !{i32 34, i32 0, metadata !99, null}
!99 = metadata !{i32 786443, metadata !60, i32 33, i32 0, metadata !6, i32 16} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!100 = metadata !{i32 35, i32 0, metadata !99, null}
!101 = metadata !{i32 36, i32 0, metadata !99, null}
!102 = metadata !{i32 786689, metadata !45, metadata !"barrier", metadata !6, i32 16777297, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [barrier] [line 81]
!103 = metadata !{i32 81, i32 0, metadata !45, null}
!104 = metadata !{i32 786688, metadata !105, metadata !"rv", metadata !6, i32 82, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [rv] [line 82]
!105 = metadata !{i32 786443, metadata !45, i32 81, i32 0, metadata !6, i32 1} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!106 = metadata !{i32 82, i32 0, metadata !105, null}
!107 = metadata !{i32 84, i32 0, metadata !105, null}
!108 = metadata !{i32 86, i32 0, metadata !105, null}
!109 = metadata !{i32 87, i32 0, metadata !105, null}
!110 = metadata !{i32 88, i32 0, metadata !105, null}
!111 = metadata !{i32 89, i32 0, metadata !105, null}
!112 = metadata !{i32 93, i32 0, metadata !105, null}
!113 = metadata !{i32 94, i32 0, metadata !105, null}
!114 = metadata !{i32 95, i32 0, metadata !105, null}
!115 = metadata !{i32 786689, metadata !48, metadata !"attr", metadata !6, i32 16777314, metadata !51, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [attr] [line 98]
!116 = metadata !{i32 98, i32 0, metadata !48, null}
!117 = metadata !{i32 99, i32 0, metadata !118, null}
!118 = metadata !{i32 786443, metadata !48, i32 98, i32 0, metadata !6, i32 2} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!119 = metadata !{i32 101, i32 0, metadata !118, null}
!120 = metadata !{i32 102, i32 0, metadata !118, null}
!121 = metadata !{i32 786689, metadata !52, metadata !"attr", metadata !6, i32 16777320, metadata !51, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [attr] [line 104]
!122 = metadata !{i32 104, i32 0, metadata !52, null}
!123 = metadata !{i32 105, i32 0, metadata !124, null}
!124 = metadata !{i32 786443, metadata !52, i32 104, i32 0, metadata !6, i32 3} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!125 = metadata !{i32 107, i32 0, metadata !124, null}
!126 = metadata !{i32 108, i32 0, metadata !124, null}
!127 = metadata !{i32 786689, metadata !53, metadata !"attr", metadata !6, i32 16777327, metadata !43, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [attr] [line 111]
!128 = metadata !{i32 111, i32 0, metadata !53, null}
!129 = metadata !{i32 786689, metadata !53, metadata !"pshared", metadata !6, i32 33554543, metadata !51, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [pshared] [line 111]
!130 = metadata !{i32 112, i32 0, metadata !131, null}
!131 = metadata !{i32 786443, metadata !53, i32 111, i32 0, metadata !6, i32 4} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!132 = metadata !{i32 113, i32 0, metadata !131, null}
!133 = metadata !{i32 114, i32 0, metadata !131, null}
!134 = metadata !{i32 115, i32 0, metadata !131, null}
!135 = metadata !{i32 786689, metadata !56, metadata !"attr", metadata !6, i32 16777333, metadata !51, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [attr] [line 117]
!136 = metadata !{i32 117, i32 0, metadata !56, null}
!137 = metadata !{i32 786689, metadata !56, metadata !"pshared", metadata !6, i32 33554549, metadata !9, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [pshared] [line 117]
!138 = metadata !{i32 118, i32 0, metadata !139, null}
!139 = metadata !{i32 786443, metadata !56, i32 117, i32 0, metadata !6, i32 5} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!140 = metadata !{i32 119, i32 0, metadata !139, null}
!141 = metadata !{i32 121, i32 0, metadata !139, null}
!142 = metadata !{i32 122, i32 0, metadata !139, null}
!143 = metadata !{i32 123, i32 0, metadata !139, null}
!144 = metadata !{i32 124, i32 0, metadata !139, null}
!145 = metadata !{i32 786689, metadata !59, metadata !"barrier", metadata !6, i32 16777343, metadata !10, i32 0, i32 0} ; [ DW_TAG_arg_variable ] [barrier] [line 127]
!146 = metadata !{i32 127, i32 0, metadata !59, null}
!147 = metadata !{i32 786688, metadata !148, metadata !"master", metadata !6, i32 128, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [master] [line 128]
!148 = metadata !{i32 786443, metadata !59, i32 127, i32 0, metadata !6, i32 6} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!149 = metadata !{i32 128, i32 0, metadata !148, null}
!150 = metadata !{i32 786688, metadata !148, metadata !"rv", metadata !6, i32 129, metadata !9, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [rv] [line 129]
!151 = metadata !{i32 129, i32 0, metadata !148, null}
!152 = metadata !{i32 131, i32 0, metadata !148, null}
!153 = metadata !{i32 133, i32 0, metadata !148, null}
!154 = metadata !{i32 134, i32 0, metadata !148, null}
!155 = metadata !{i32 148, i32 0, metadata !148, null}
!156 = metadata !{i32 149, i32 0, metadata !157, null}
!157 = metadata !{i32 786443, metadata !148, i32 148, i32 0, metadata !6, i32 7} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!158 = metadata !{i32 786688, metadata !157, metadata !"i", metadata !6, i32 150, metadata !159, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 150]
!159 = metadata !{i32 786485, null, metadata !"", null, i32 0, i64 0, i64 0, i64 0, i32 0, metadata !73} ; [ DW_TAG_volatile_type ] [line 0, size 0, align 0, offset 0] [from spin_counter_t]
!160 = metadata !{i32 150, i32 0, metadata !157, null}
!161 = metadata !{i32 151, i32 0, metadata !157, null}
!162 = metadata !{i32 152, i32 0, metadata !157, null}
!163 = metadata !{i32 153, i32 0, metadata !157, null}
!164 = metadata !{i32 156, i32 0, metadata !157, null}
!165 = metadata !{i32 157, i32 0, metadata !166, null}
!166 = metadata !{i32 786443, metadata !157, i32 156, i32 0, metadata !6, i32 8} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!167 = metadata !{i32 158, i32 0, metadata !166, null}
!168 = metadata !{i32 159, i32 0, metadata !169, null}
!169 = metadata !{i32 786443, metadata !166, i32 158, i32 0, metadata !6, i32 9} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!170 = metadata !{i32 160, i32 0, metadata !169, null}
!171 = metadata !{i32 162, i32 0, metadata !166, null}
!172 = metadata !{i32 163, i32 0, metadata !157, null}
!173 = metadata !{i32 167, i32 0, metadata !148, null}
!174 = metadata !{i32 168, i32 0, metadata !148, null}
!175 = metadata !{i32 169, i32 0, metadata !148, null}
!176 = metadata !{i32 172, i32 0, metadata !177, null}
!177 = metadata !{i32 786443, metadata !148, i32 169, i32 0, metadata !6, i32 10} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!178 = metadata !{i32 173, i32 0, metadata !177, null}
!179 = metadata !{i32 174, i32 0, metadata !177, null}
!180 = metadata !{i32 181, i32 0, metadata !181, null}
!181 = metadata !{i32 786443, metadata !148, i32 174, i32 0, metadata !6, i32 11} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!182 = metadata !{i32 182, i32 0, metadata !183, null}
!183 = metadata !{i32 786443, metadata !181, i32 181, i32 0, metadata !6, i32 12} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!184 = metadata !{i32 786688, metadata !183, metadata !"i", metadata !6, i32 183, metadata !159, i32 0, i32 0} ; [ DW_TAG_auto_variable ] [i] [line 183]
!185 = metadata !{i32 183, i32 0, metadata !183, null}
!186 = metadata !{i32 184, i32 0, metadata !183, null}
!187 = metadata !{i32 185, i32 0, metadata !183, null}
!188 = metadata !{i32 186, i32 0, metadata !183, null}
!189 = metadata !{i32 189, i32 0, metadata !183, null}
!190 = metadata !{i32 190, i32 0, metadata !191, null}
!191 = metadata !{i32 786443, metadata !183, i32 189, i32 0, metadata !6, i32 13} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!192 = metadata !{i32 191, i32 0, metadata !191, null}
!193 = metadata !{i32 192, i32 0, metadata !194, null}
!194 = metadata !{i32 786443, metadata !191, i32 191, i32 0, metadata !6, i32 14} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!195 = metadata !{i32 193, i32 0, metadata !194, null}
!196 = metadata !{i32 195, i32 0, metadata !191, null}
!197 = metadata !{i32 196, i32 0, metadata !183, null}
!198 = metadata !{i32 199, i32 0, metadata !148, null}
!199 = metadata !{i32 201, i32 0, metadata !148, null}
!200 = metadata !{i32 202, i32 0, metadata !201, null}
!201 = metadata !{i32 786443, metadata !148, i32 201, i32 0, metadata !6, i32 15} ; [ DW_TAG_lexical_block ] [/Users/asampson/uw/research/enerc/apps/canneal/parsec_barrier.cpp]
!202 = metadata !{i32 203, i32 0, metadata !201, null}
!203 = metadata !{i32 204, i32 0, metadata !201, null}
!204 = metadata !{i32 205, i32 0, metadata !148, null}
!205 = metadata !{i32 207, i32 0, metadata !148, null}
!206 = metadata !{i32 208, i32 0, metadata !148, null}

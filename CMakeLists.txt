cmake_minimum_required(VERSION 3.6)
project(code10)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        pc_sem.c
#        pc_mutex_cond.c
#        pc_spinlock.c
#        smoke.c
        spinlock.h
        uthread.c
        uthread.h
        uthread_mutex_cond.c
        uthread_mutex_cond.h
        uthread_sem.c
        uthread_sem.h
        uthread_util.h
#        washroom.c
#        washroom_sem.c
        )
add_executable(code10 ${SOURCE_FILES})
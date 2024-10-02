# Sample IR

# Simple Hello World Program

global_string %hello = "Hello, World!\n"

extern fn void puts(%ptr msg);

define fn i32 main():
    call puts(%hello)
    ret 0

# Simple Fibonacci Program

define fn i32 fib(i32 %n):
    %ptr 2 = allocate 4 # i32
    store %n, %2
    %3 = load %ptr 2
    %4 = icmp leq %3, 1
    if %4 goto base_case else recursive_case

base_case:
    %5 = load %ptr 2
    return %5

recursive_case:
    %6 = load %ptr 2
    %7 = sub %6, 1
    %8 = call fib(%7)
    %9 = load %ptr 2
    %10 = sub %9, 2
    %11 = call fib(%10)
    %12 = add %8, %11
    ret %12
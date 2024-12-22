# Example Compilations

### *Compilations Up To Date As Of: November 27th, 2024

This section contains example compilations of various IR snippets. Provided as well will be an approximate
C equivalent of the IR, however it is important to note that this code will be manually back-converted, and
there is no current method of converting C code to IR.

### Example 1 -- Simple Hello World

C Code:
```c
#import <stdio.h>

int main() {
    puts("Hello, World!");
}
```

IR:
```ir
global_string %msg = "Hello, World!"

extern fn i32 puts(ptr %msg)

define fn void main()
    call i32 puts ptr %msg
    ret
end
```

Produced Assembly:
```asm
[bits 64]
section .global_strings
msg db "Hello, World!", 0
section .external_functions
extern puts
section .text

global main

main:
.__stacksave:

.entry:
	mov     rdi, msg
	mov     rax, 0
	call    puts
	ret
```

### Example 2 -- Naive Recursive Fibonacci

C Code:
```c
int fib(int n) {
    if (n <= 1)
    return n;

    return fib(n - 1) + fib(n - 2);
}

int main() {
    return fib(10);
}
```

IR:
```ir
define fn i32 main()
    %1 = call i32 fib i32 10
    ret i32 %1
end

define fn i32 fib(i32 %n)
    %1 = icmp ule i32 %n, i32 1
    branch base_case recursive_case i1 %1
.base_case:
    ret i32 %n
.recursive_case:
    %2 = sub i32 %n, i32 1
    %3 = call i32 fib i32 %2
    %4 = sub i32 %2, i32 1
    %5 = call i32 fib i32 %4
    %6 = add i32 %5, i32 %3
    ret i32 %6
end
```

Produced Assembly:
```asm
[bits 64]
section .global_strings
section .external_functions
section .text

global main

main:
.__stacksave:

.entry:
	mov     edi, 10
	mov     rax, 0
	call    fib
	ret     

global fib

fib:
.__stacksave:
	push    rbx
	push    rcx
	push    rdx
	push    rsi

.entry:
	cmp     edi, 1
	jbe     .base_case
	jmp     .recursive_case
.base_case:
	mov     eax, edi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	ret     
.recursive_case:
	mov     ebx, edi
	sub     ebx, 1
	mov     edi, ebx
	mov     rax, 0
	call    fib
	mov     ecx, ebx
	sub     ecx, 1
	mov     edi, ecx
	mov     edx, eax
	mov     rax, 0
	call    fib
	mov     esi, eax
	add     esi, edx
	mov     eax, esi
	pop     rsi
	pop     rdx
	pop     rcx
	pop     rbx
	ret
```

Exits With: 55

### Example 3 -- 'Arith Select Test' (Ternary Operator with Constant Output)

C Code:
```c
int arith_select(int n) {
    return n == 0 ? 5 : 10;
}

int main() {
    return arith_select(0);
}
```

IR:
```ir
define fn i32 main()
    %1 = call i32 arith_select i32 0
    ret i32 %1
end

define fn i32 arith_select(i32 %n)
    %1 = icmp eq i32 %n, i32 0
    %2 = select i32 %1, i32 5, i32 10
    ret i32 %2
end
```

Produced Assembly:
```asm
[bits 64]
section .global_strings
section .external_functions
section .text

global main

main:
.__stacksave:

.entry:
	mov     edi, 0
	mov     rax, 0
	call    arith_select
	ret     

global arith_select

arith_select:
.__stacksave:
	push    rbx

.entry:
	cmp     edi, 0
	setne   bl
	lea     ebx, [5 * rbx + 5]
	mov     eax, ebx
	pop     rbx
	ret
```

Exit Code: 5

### Example 4 -- 'Select Test' (Ternary Operator with Non-Constant Output)

C Code:
```c
int arith_select(int n, int c1, int c2) {
    return n == 0 ? c1 : c2;
}

int main() {
    return arith_select(0, 1, 2);
}
```

IR:
```ir
define fn i32 main()
    %1 = call i32 arith_select i32 0, i32 1, i32 2
    ret i32 %1
end

define fn i32 arith_select(i32 %n, i32 %c1, i32 %c2)
    %1 = icmp eq i32 %n, i32 0
    %2 = select i1 %1, i32 %c1, i32 %c2
    ret i32 %2
end
```

Produced Assembly:
```asm
[bits 64]
section .global_strings
section .external_functions
section .text

global main

main:
.__stacksave:

.entry:
	mov     edi, 0
	mov     esi, 1
	mov     edx, 2
	mov     rax, 0
	call    arith_select
	ret     

global arith_select

arith_select:
.__stacksave:
	push    rbx

.entry:
	cmp     edi, 0
	mov     ebx, edx
	cmove   ebx, esi
	mov     eax, ebx
	pop     rbx
	ret
```

Returns: 1

### Example 5 -- 'Pointer Test'

C Code:
```c
void fill_array(int* arr) {
    arr[1] = 2;
    arr[2] = 4;
    arr[3] = 6;
}

int main() {
    int arr[4];
    fill_array(arr);
    return arr[1];
}

```

IR:
```ir
define fn i32 main()
    %arr = allocate 32

    call void fill_array ptr %arr

    %ptr = getarrayptr i32 ptr %arr, i32 1
    %ret = load i32 ptr %ptr
    ret i32 %ret
end

define fn void fill_array(ptr %arr)
    %1 = getarrayptr i32 ptr %arr, i32 1
    store i32 ptr %1, i32 2

    %2 = getarrayptr i32 ptr %arr, i32 2
    store i32 ptr %2, i32 4

    %3 = getarrayptr i32 ptr %arr, i32 3
    store i32 ptr %3, i32 6

    ret
end
```

Produced Assembly:
```asm
[bits 64]
section .global_strings
section .external_functions
section .text

global main

main:
.__stacksave:
	push    rbp
	mov     rbp, rsp
	sub     rsp, 32
	push    rbx

.entry:
	lea     rdi, [rbp - 32]
	mov     rax, 0
	call    fill_array
	mov     ebx, DWORD [rbp - 28]
	mov     eax, ebx
	pop     rbx
	leave   
	ret     

global fill_array

fill_array:
.__stacksave:

.entry:
	mov     DWORD [rdi + 4], 2
	mov     DWORD [rdi + 8], 4
	mov     DWORD [rdi + 12], 6
	ret
```

Exit Code: 2
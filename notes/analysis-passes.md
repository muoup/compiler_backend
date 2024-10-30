# Analysis Passes

A brief description of each stage in the process from IR to the outputted assembly.

### 1. IR AST

Contains the supported intermediate representation of the code. This is where most
optimizations will be performed in the future. The AST works as a basic tree structure
with little surprising about it. The only quirk is that all block instructions are stored
in a single block_instruction struct, with a unique pointer to a polymorphic instruction. 
This allows for a simpler system to find what operators the instruction uses, as the operators
are stored uniformly in a vector of a non-polymorphic type.

### 2. IR Analysis

This does not generate a new tree, rather it generates attached metadata about the data structures in the
IR AST (i.e. function metadata, instruction metadata, etc.). This may not be necessary in the future
[note 1](#codegen-order), but for now it allows for things such as finding the first and last instruction 
a certain labeled operand is used in for more efficient register/memory management.

### 3. Asm Node Array / Codegen

During codegen, the IR AST is converted instruction-by-instruction for each function into an array of
approximate 1-1 assembly nodes. Of course for some things such as saving and restoring stack frames,
this will be done with a single multi-instruction node to prevent redundancy, but for the most part
a single instruction will be converted to a single assembly node. What this allows for is that during
instruction emitting, an assembly node itself can have a list of rules to prevent redundancy. For example,
if a 'mov' instruction is emitted and the source and destination are the same, the instruction is pointless
and can be removed. While this could be done in the IR stage, it gets a bit convoluted and a lot of code
gets generated to handle these passes for the handling of different IR nodes.

### 4. Assembly Output

During the parsing of a function, after the assembly vector is generated, the vector is then ran through
instruction by instruction so that each assembly node can emit its own assembly code. As mentioned before,
this also allows for the nodes to handle preventing redundancy. In the future it may be possible to use
this to handle better control flow -- i.e. if a value is moved to a register and then immediated moved from
that register to another register, this can be squashed into a single move instruction. It is also possible
however that this will be handled by using a more efficient Codegen pass [note 1](#codegen-order).

## Notes

### Codegen Order
The Codegen pass currently reads the IR AST forward-to-back and emits ASM nodes accordingly. I am considering
it may be more efficient to read the IR AST back-to-front. It seems a bit complicated to set up correctly, but I am
regularly running into an issue where for instance a value is moved into a register, and as it is a parameter for a
call instruction, it is then just moved into another register. Therefore, if the IR AST reads back-to-front, it can
bear in mind the intended destination of a value when finding a storage location for it.
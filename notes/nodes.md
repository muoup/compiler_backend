# Basic Overview of Node Types

## Global-Scope Nodes

global_string {name} = "...": 
    Contains a read-only string that can be used in the program.

declare fn (return_type|void) {name}(type {val1}, type {val2}...): 
    Both defines a function prototype and declares its implementation.

extern fn (return_type|void) {name}(type1, type2...):
    Defines a function implementation. For use with external functions.
    Works as well with libc functions as the backend links using gcc.

## Function-Scope Instructions

.{branch_label}:
    A label for branching instructions to jump to.

%{ptr} = allocate {size}:
    Ensures there is space in stack memory for data of the given size.

store {size} {value}, %{ptr}:
    Stores a value in stack memory at the given ptr_var.

%{value} = load {size} %{ptr}:
    Loads a value from stack memory at the given ptr_var.

%{condition} = icmp {icmp_type} %{value1}, %{value2}:
    Compares two values for use with an if branching instruction.

branch {true_label} {false_label} %{condition} 
    Branches to one of two labels based on the result of a comparison.

%{value} = add %{value1}, %{value2}:
    Adds two values together.

%{value} = sub %{value1}, %{value2}:
    Subtracts one value from another.

ret %{value}:
    Returns a value from a function.

%{value_var} = zext {value_size} %{value_var}:
    Zero extends a value_var to a larger or smaller size.

%{value_var} = sext {value_size} %{value_var}:
    Sign extends a value_var to a larger or smaller size.

return %[value_var]:
    Returns a value_var from a function.

call {function_name}({val1}, {val2}...):
    Calls a function with the given parameters.

%{value} = call {function_name}({val1}, {val2}...):
    Calls a function and stores the result in the given value.

%{value} = phi label1 label2 {value1}, {value2}:
    A phi node that for now only supports two labels. This is used for branching,
    where it can ensure that the correct value will be stored depending on
    which label the program is branching from.

%{value} = select %{condition}, {true_value}, {false_value}:
    Selects a value based on a condition. Essentially acts as a ternary operator.
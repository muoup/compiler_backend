# Basic Overview of Node Types

## Global-Scope Nodes

global_string [name] = "...": 
    Contains a read-only string that can be used in the program.

declare fn [name]({(param_type %|%ptr )param_name}...){ -> return_type}: 
    Both defines a function prototype and declares its implementation.

extern fn [name]({(param_type %|%ptr )param_name}...){ -> return_type}:
    Defines a function implementation. For use with external functions.

## Function-Scope Instructions

[branch_label]:
    A label for branching instructions to jump to.

%ptr [ptr] = allocate [size]:
    Ensures there is space in stack memory for data of the given return_type.

store [size] %[value_var], %ptr [ptr]:
    Stores a value_var in stack memory at the given ptr_var.

%[value_var] = load [size] %ptr [ptr]:
    Loads a value_var from stack memory at the given ptr_var.

%[value_var] = icmp [size] [condition] %[value1], %[value2]:
    Compares two values for use with an if branching instruction.

if %[condition] goto [true_label] else [false_label]:
    Branches to one of two labels based on the result of a comparison.

%[value_var] = add [value_size] %[value1], %[value2]:
    Adds two values together.

%[value_var] = sub [value_size] %[value1], %[value2]:
    Subtracts one value_var from another.

%[value_var] = zext [value_size] %[value_var]:
    Zero extends a value_var to a larger or smaller size.

%[value_var] = sext [value_size] %[value_var]:
    Sign extends a value_var to a larger or smaller size.

return %[value_var]:
    Returns a value_var from a function.

call [function_name]({param-type %param_name}...):
    Calls a function with the given parameters.

%[value_var] = call [function_name]({param-type %param_name}...):
    Calls a function and stores the result

%ptr [ptr] = call [function_name]({param-type %param_name}...):
    Gets a ptr_var to an element at a given index in an array.
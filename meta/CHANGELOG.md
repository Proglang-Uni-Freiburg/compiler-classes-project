## Added dictionaries
### AST
* add type TDict(key_ty, val_ty)
* add expressions EDict (literal), EDictAccess (`d[k]`), EDictHas (`k in d`) and EDictDel (`del d[k]`) to all ASTs
* add statement SFor to ast_1 for iteration over dicts
* add expressions EDictNext, EDictKey and EDictValue (ast_2 to ast_8) for the lowered iteration
* add lhs LDictSet (ast_4 to ast_8) for `d[k] = v`
* make ETupleAccess and ETupleLen in ast_1 mutable, as the type checker stores the type of their operand

### parsing
* parse dict literals, `dict[K, V]` annotations, subscripts with non-constant keys, subscript assignments
* parse `k in d`, `k not in d`, `del d[k]`
* parse `for k in d`, `for k in d.keys()`, `for v in d.values()`, `for k, v in d.items()`

### type checker
* type check dict literals (types inferred from the first item), accesses, assignments, `in`, `del`, `len` and `for`
* record the operand type on ETupleAccess/ETupleLen so that constant-key accesses and `len` on dicts can be told apart from tuples
* print only accepts ints

### interpreter
* add VDict with structural key equality
* interpret all dict operations and `for` loops; a missing key raises exception value 0

### shrinking
* turn constant-key accesses on dicts into EDictAccess and `len(d)` into a read of the count field
* lower `for` loops to while loops over the slots of the hash table

### allocation
* check `k in d` before `d[k]` and `del d[k]` and raise exception value 0 if the key is missing

### monadic / explicate
* atomize dict operands and the value of dict assignments

### select instructions
* emit calls to the runtime for all dict operations; `dict_create` and `dict_reserve` (before every `dict_set`) receive the stack pointer so they can start a garbage collection

### runtime
* hash table with linear probing, structural hashing and equality, doubling at load factor 0.7
* dicts are allocated on the GC heap: a wrapper tuple (count, capacity, backing) and a backing tuple of (flag, key, value) slots; flags have LSB 0 so the GC skips them
* deleted slots are marked so probe chains stay intact
* add `dict_create`, `dict_reserve`, `dict_get`, `dict_set`, `dict_has`, `dict_delete`, `dict_next`, `dict_key_at`, `dict_value_at`

### misc
* add flake.nix dev shell with python3.12, RISC-V gcc and qemu to run the tests without docker
* add tests in tests/9_dict and unit tests for parser, shrink, uniquify and reveal

## Extended the type checker to check Classes
### AST
* add expressions EField for field access and EMethod for method calls to the AST
* add Statement SClass for class definitions to the AST
* change SAssign to also take EField (ast_1) and ETupleAccess (ast_2_shrunk, ast_3_revealed) as value of lhs (this is needed to be able to reassign to membervariables of dataclass objects)
* make pretty printing compatible with extended AST

### parsing
* add parsing context to keep track of defined classes
* change how assignments are parsed to allow assignments to member variables
* add parsing of classes
* change type node mapping to accomodate classes
* change structure of Program to allow easy handling of classes in later steps

### shrinking
* add functionality to shrink class definitions
* change shrinking of assignments to allow for membervariable assigns
* add functionality to shrink Efield and EMethod expressions

### uniquify
* change assignment to convert member variable assignment to tuple access

### reveal functions
* make assignments compatible with the rest of the changes

### convert assignments
* add functionality to be able to treat tuples and dataclass objects the same in later passes

### interpreter
* add VClass and VObject to model classes
* add functionality for constructors and methods in apply_fun
* add fields and objects logic for interpretation
* interpret class definitions 

### type checker
* now also annotates the type of member variables inside of methods
* expanded type equality check to handle inheritance correctly
* type check member variable assignment
* type check class definition and inheritance
* type check constructor calls
* type check method calls for arguments and return value
* type check method overrides
* make instances of TClass be treated like any other type

## misc
* switch Docker image from debian to ubuntu
* add tests for membervariables, methods and inheritance

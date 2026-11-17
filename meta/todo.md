## Open
- Iteration order: compiled code iterates in hash-table order, the interpreter (like Python) in insertion order. Loops that print while iterating give different output. Fixing this needs an insertion-ordered layout (dense entry array + index table, like CPython).
- Adding keys while iterating is unsupported: a resize can make the loop skip or repeat keys. Python raises a RuntimeError; we could detect it by remembering the count/capacity when the loop starts.
- `d[k]` and `del d[k]` look the key up twice (`dict_has` check, then the operation).
- Deleted slots don't count towards the load factor, so many deletes make probe chains long until the next resize.
- Structural hashing: no cycle detection (a self-referencing object as key recurses forever), class instances are compared by contents instead of identity, and dicts are allowed as keys.
- The dict/tuple distinction still lives in two places: the parser emits `ETupleAccess` for constant keys, and shrink converts it based on the type recorded by the type checker. A single subscript node split in the type checker would be cleaner.


## Quirks
- We are using the type to distinguish dict and tuple accesses (the type checker stores the operand type on `ETupleAccess`/`ETupleLen`, shrink reads it)
- `len(d)` is compiled as a tuple access to the count field of the dict wrapper
- A missing key raises the exception value 0 in both interpreter and compiled code


## Fragen
- Kann ich structural hashing verwenden?
- Wie komplex soll Typeinference sein?
  - Für die Distinction von Accesses geht momentan nur der Type

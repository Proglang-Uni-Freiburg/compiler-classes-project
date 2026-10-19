
- The clean fix would be to merge the two parser cases into a single EDictAccess/subscript node and let the type checker split them based on type. But that's a bigger refactor touching all the downstream passes that currently match ETupleAccess.
- Allocation pass should actually do the work


## Quirks
- We are using the type to distinguish dict and tuple accesses


## Fragen
- Kann ich structural hashing verwenden?
- Wie komplex soll Typeinference sein?
  - Für die Distinction von Accesses geht momentan nur der Type


Now I have the full picture. Here's what's missing:

1. len(d) — no support
The type checker's ETupleLen case (line 268–274 of type_checker.py) only handles TTuple, not TDict. The compiler would raise a TypeError before even reaching the select pass.

2. key in d — no support at any level
No membership test exists in the parser, AST, type checker, or compiler. The runtime also has no dict_has function, so it would need to be added to runtime.c too.

3. Iteration — no support
for k in d / for k, v in d.items() isn't supported. This would require significant new machinery: an iterator protocol or unrolled loop, plus either dict_keys/dict_items runtime functions.

4. print(d) — silently prints garbage
SPrint in pass_8_9_select.py (line 32–40) always does div by 2 + print_int64, treating every value as a tagged integer. Printing a dict prints the raw pointer value. The type checker doesn't catch this either — there's no SPrint type restriction preventing print(d).

5. The dict-variable tracking hack is fragile
pass_1_2_shrink.py tracks dict variables in a set[Id] and rewrites ETupleAccess(EVar(x), i) → EDictAccess only when x is in that set. This breaks when a dict comes from a function return value, a class field, or any path that didn't go through a direct annotated assignment. The todo already notes the clean fix would be to do this distinction in the type checker instead.

6. del d[key] — no deletion
No dict_delete in runtime.c, no del support in the compiler.

In short: set/get/create work. Everything else — membership, length, iteration, printing, deletion, and robust dict-tracking through non-trivial expressions — is missing.

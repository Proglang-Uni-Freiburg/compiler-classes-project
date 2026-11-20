def find(d: dict[int, int], target: int) -> int:
    for k, v in d.items():
        if v == target:
            return k
    return 0 - 1

def count(d: dict[int, int]) -> int:
    n = 0
    for k in d:
        n = n + 1
    return n

d: dict[int, int] = {1: 10, 2: 20, 3: 30}
print(find(d, 20))
print(find(d, 99))
del d[1]
print(count(d))
e: dict[int, int] = {}
print(count(e))
pairs = 0
for a in d:
    for b in d:
        pairs = pairs + 1
print(pairs)

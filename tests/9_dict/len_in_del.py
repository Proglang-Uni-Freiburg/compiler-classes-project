d: dict[int, int] = {1: 10, 2: 20, 3: 30}
print(len(d))
if 2 in d:
    print(1)
if 4 not in d:
    print(2)
del d[2]
print(len(d))
if 2 not in d:
    print(3)
d[2] = 99
print(d[2])
print(len(d))

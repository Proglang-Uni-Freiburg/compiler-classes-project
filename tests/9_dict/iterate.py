d: dict[int, int] = {}
i = 0
while i < 30:
    d[i] = i + 100
    i = i + 1
keys = 0
for k in d:
    keys = keys + k
print(keys)
vals = 0
for v in d.values():
    vals = vals + v
print(vals)
n = 0
both = 0
for k, v in d.items():
    n = n + 1
    both = both + v - k
    t = (k, v, n)
print(n)
print(both)
for k in d.keys():
    d[k] = d[k] + 1
total = 0
for v in d.values():
    total = total + v
print(total)

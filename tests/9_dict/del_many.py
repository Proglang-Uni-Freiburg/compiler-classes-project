d: dict[int, int] = {}
i = 0
while i < 100:
    d[i] = i + i
    i = i + 1
i = 0
while i < 100:
    if i < 90:
        del d[i]
    i = i + 1
print(len(d))
print(d[95])
i = 0
while i < 50:
    d[i] = i
    t = (i, i)
    i = i + 1
print(len(d))
print(d[49])
print(d[99])

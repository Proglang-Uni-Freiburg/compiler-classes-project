d: dict[int, int] = {}
i = 0
while i < 200:
    d[i] = i + i
    t = (i, i, i)
    i = i + 1
print(d[0])
print(d[13])
print(d[199])

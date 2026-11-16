d: dict[tuple[int, int], int] = {}
i = 0
while i < 30:
    d[(i, i + 1)] = i
    i = i + 1
print(d[(5, 6)])
print(d[(29, 30)])

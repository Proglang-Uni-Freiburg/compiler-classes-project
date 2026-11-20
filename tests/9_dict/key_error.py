d: dict[int, int] = {1: 10}
try:
    print(d[2])
except Exception as e:
    print(7)
try:
    del d[3]
except Exception as e:
    print(8)
print(d[5])

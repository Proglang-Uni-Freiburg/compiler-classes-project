class Point:
    x: int
    y: int

    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y

d: dict[int, Point] = {}
i = 0
while i < 40:
    d[i] = Point(i, i + 1)
    t = (i, i, i, i)
    i = i + 1
print(d[3].x)
print(d[39].y)

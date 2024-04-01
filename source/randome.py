import random

# 15 random numbers between -100 and 100 using a while

i = 0
array = []
while i < 15:
    array.append(random.randint(-100, 100))
    i += 1

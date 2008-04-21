from random import randrange

def getRandomInterval(midpoint, range=5):
    return randrange(max(midpoint-range,0), midpoint+range)

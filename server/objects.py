import struct
from rpc import *

class GameState(NetObject):
    netPack = struct.Struct('5B')

    MODE_LOBBY = 1
    MODE_RUN = 2
    MODE_OVER = 3

    def __init__(self):
        self.numPlayers = 0
        self.playersAlive = self.numPlayers
        self.mode = GameState.MODE_LOBBY
        self.fire_id = 0
        self.timer = 0
        self.gametimer = None
        self.cooldown = None

    def packValues(self):
        return (self.numPlayers, self.playersAlive, self.mode, self.fire_id, int(self.timer))

class Player(NetObject):
    netPack = struct.Struct('B2?3f')

    def __init__(self, id, x, y, z):
        self.id = id
        self.alive = True
        self.connected = True
        self.x = x
        self.y = y
        self.z = z

    def packValues(self):
        return (self.id, self.alive, self.connected, self.x, self.y, self.z)

    def unpack(obj):
        data = Player.netPack.unpack(obj)
        return Player(data[0], data[3], data[4], data[5])

    def dist(self, x, y, z):
        return sqrt((self.x - x)**2 + (self.y - y)**2 + (self.z - z)**2)

    def pos(self):
        return (self.x, self.y, self.z)

    def setPos(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

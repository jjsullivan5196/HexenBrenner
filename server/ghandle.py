import sys
import time
import struct
from rpc import *

class GameHandler(RpcHandler):
    HEAD_CONNECT = 1
    HEAD_GETSTATE = 2
    HEAD_GETPLAYERS = 3
    HEAD_PUSHPLAYER = 4

    def __init__(self):
        super().__init__()
        self.state = GameState()
        self.players = {}

    def packPlayers(self):
        data = b''
        for key in self.players:
            pl = self.players[key]
            data = data + pl.pack()
        
        return data

    def terminateConnection(self, socket, arg):
        self.players[arg].alive = False
        self.players[arg].connected = False
        self.state.numPlayers -= 1
        print('Player ' + str(arg) + ' disconnected')

    def gameLogic():
        for key in self.players

    def handleRemoteBytes(self, data, header):
        byte = None
        size = None 

        if(header == GameHandler.HEAD_CONNECT):
            id = self.state.numPlayers
            newPlayer = Player(id, 0, 0, 0)
            self.players[id] = newPlayer
            self.state.numPlayers += 1

            byte = newPlayer.pack()
            size = type(newPlayer).netPack.size

        elif(header == GameHandler.HEAD_GETSTATE):
            byte = self.state.pack()
            size = type(self.state).netPack.size

        elif(header == GameHandler.HEAD_GETPLAYERS):
            byte = self.packPlayers()
            size = len(self.players) * Player.netPack.size

        elif(header == GameHandler.HEAD_PUSHPLAYER):
            netPlayer = Player.unpack(data)
            self.players[netPlayer.id] = netPlayer

            print('Player ' + str(netPlayer.id) + ' ' + str(netPlayer.x) + ' ' + str(netPlayer.y) + ' ' + str(netPlayer.z))

            byte = super().ackPack.pack(True)
            size = super().ackPack.size

        return (byte, size)

class GameState(NetObject):
    netPack = struct.Struct('3B')

    def __init__(self):
        self.numPlayers = 0
        self.fire_id = 0
        self.timer = 20

    def packValues(self):
        return (self.numPlayers, self.fire_id, self.timer)

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

import sys
import time
import struct
import random
from math import *
from rpc import *

class GameHandler(RpcHandler):
    HEAD_CONNECT = 1
    HEAD_GETSTATE = 2
    HEAD_GETPLAYERS = 3
    HEAD_PUSHPLAYER = 4
    TIME_RESET = 10

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
        self.state.playersAlive -= 1
        print('Player ' + str(arg) + ' disconnected')

    def gameLogic(self):
        if(self.state.mode == GameState.MODE_LOBBY):
            if(self.state.numPlayers < 2):
                return
            else:
                self.state.gametimer = time.time() + 20
                self.state.mode = GameState.MODE_RUN

        if(self.state.mode == GameState.MODE_OVER):
            return

        if(self.state.numPlayers == 0):
            return

        if(self.state.playersAlive <= 1):
            for key in self.players:
                aPlayer = self.players[key]
                if(aPlayer.alive == True):
                    print('Player ' + str(key) + ' wins!')
                    self.state.mode = GameState.MODE_OVER
            return

        mPlayer = self.players[self.state.fire_id]
        for nKey in self.players:
            if(nKey == self.state.fire_id):
                continue
            nPlayer = self.players[nKey]

            #print(str(mPlayer.dist(*nPlayer.pos())))

            if(mPlayer.dist(*nPlayer.pos()) < 2 and time.time() > self.state.cooldown):
                self.state.fire_id = nKey
                self.state.cooldown = time.time() + 5
                self.state.gametimer = time.time() + GameHandler.TIME_RESET
                print('Player ' + str(nKey) + ' is on fire!')
                break

        if(self.state.gametimer < time.time()):
            mPlayer.alive = False
            self.state.playersAlive -= 1
            print('Player ' + str(self.state.fire_id) + ' has died!')

            nextVictim = random.choice(list(self.players.keys()))
            while(nextVictim == self.state.fire_id or self.players[nextVictim].alive == False):
                nextVictim = random.choice(list(self.players.keys()))
            self.state.fire_id = nextVictim

            print('Player ' + str(self.state.fire_id) + ' is on fire!')
            self.state.gametimer = time.time() + GameHandler.TIME_RESET

        self.state.timer = self.state.gametimer - time.time()


    def handleRemoteBytes(self, data, header):
        byte = None
        size = None 

        if(header == GameHandler.HEAD_CONNECT):
            id = None
            for key in self.players:
                mPlayer = self.players[key]
                if(mPlayer.connected == False):
                    id = mPlayer.id
                    break

            if(id == None):
                id = self.state.numPlayers

            newPlayer = Player(id, 0, 0, 0)
            self.players[id] = newPlayer
            self.state.numPlayers += 1
            self.state.playersAlive += 1

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
            self.players[netPlayer.id].setPos(*netPlayer.pos())

            #print('Player ' + str(netPlayer.id) + ' ' + str(netPlayer.x) + ' ' + str(netPlayer.y) + ' ' + str(netPlayer.z))

            byte = super().ackPack.pack(True)
            size = super().ackPack.size

        return (byte, size)

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
        self.gametimer = time.time() + 20
        self.cooldown = time.time()

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

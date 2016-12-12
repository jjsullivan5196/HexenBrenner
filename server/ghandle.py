import sys
import time
import struct
import random
from math import *
from rpc import *
from objects import *

class GameHandler(RpcHandler):
    HEAD_CONNECT = 1
    HEAD_GETSTATE = 2
    HEAD_GETPLAYERS = 3
    HEAD_PUSHPLAYER = 4
    TIME_RESET = 15
    TIME_LOBBY = 20

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
                if(self.state.cooldown == None):
                    self.state.cooldown = time.time() + GameHandler.TIME_LOBBY
                elif(time.time() > self.state.cooldown):
                    self.state.cooldown = time.time() + 5
                    self.state.gametimer = time.time() + GameHandler.TIME_RESET
                    self.state.mode = GameState.MODE_RUN
                return


        if(self.state.mode == GameState.MODE_OVER):
            if(self.state.cooldown == None):
                for key in self.players:
                    mPlayer = self.players[key]
                    if(mPlayer.connected == True):
                        mPlayer.alive = True
                self.state.playersAlive = self.state.numPlayers
                self.state.cooldown = time.time() + GameHandler.TIME_LOBBY
            elif(time.time() > self.state.cooldown):
                self.state.cooldown = time.time() + 5
                self.state.gametimer = time.time() + GameHandler.TIME_RESET
                self.state.mode = GameState.MODE_RUN
            return

        if(self.state.numPlayers == 0):
            self.state.cooldown = time.time() + 5
            self.state.gametimer = time.time() + GameHandler.TIME_RESET
            return

        if(self.state.playersAlive <= 1):
            for key in self.players:
                aPlayer = self.players[key]
                if(aPlayer.alive == True):
                    print('Player ' + str(key) + ' wins!')
                    self.state.cooldown = None
                    self.state.mode = GameState.MODE_OVER
            return

        mPlayer = self.players[self.state.fire_id]
        for nKey in self.players:
            if(nKey == self.state.fire_id):
                continue
            nPlayer = self.players[nKey]

            if(nPlayer.alive == False or nPlayer.connected == False):
                continue

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
            while(nextVictim == self.state.fire_id or self.players[nextVictim].alive == False or self.players[nextVictim].connected == False):
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

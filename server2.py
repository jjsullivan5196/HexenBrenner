import sys
import socket
import struct
import threading
import time

class RpcHandler: #Abstract RPC handler class
    messagePack = struct.Struct('BI')
    ackPack = struct.Struct('B')
    HEAD_TERMINATE = 0
    
    def __init__(self):
        self.clients = []

    def addClient(self, socket):
        self.clients.append(socket)

    #handleRemoteBytes: bytes to send back over the connection, return a tuple of (bytesReturned, sizeOfBytes)
    def handleRemoteBytes(self, data, header):
        return (data, len(data))

    def terminateConneciton(self):
        return

    #This shouldn't change, define handleRemoteBytes instead
    def handleMessages(self):
        for socket in self.clients:
            (header, size) = messagePack.unpack(socket.recv(messagePack.size))

            if(header == HEAD_TERMINATE):
                self.terminateConnection()
                continue

            socket.sendall(ackPack.pack(1))
            (retBytes, sizeRet) = self.handleRemoteBytes(socket.recv(size), header)
            
            socket.sendall(messagePack.pack(header, sizeRet))

            socket.recv(ackPack.size)
            socket.sendall(retBytes)

class GameHandler(RpcHandler):
    HEAD_CONNECT = 1
    HEAD_GETSTATE = 2
    HEAD_GETPLAYERS = 3
    HEAD_PUSHPLAYER = 4

    def __init__(self):
        super(GameHandler, self)
        self.state = GameState()
        self.players = {}

    def handleRemoteBytes(self, data, header):
        data = None
        size = 

        if(header == HEAD_CONNECT):
            id = self.state.numPlayers
            newPlayer = Player(id, 0, 0, 0)
            self.players[id] = newPlayer
            self.state.numPlayers += 1

            data = newPlayer.pack()
            size = newPlayer.size()
        elif(header == HEAD_GETSTATE):

        elif(header == HEAD_GETPLAYERS):

        elif(header == HEAD_PUSHPLAYER):

        else:
            
        return (data, size)

class NetObject: #Interface for network objects
    netPack = struct.Struct('B')

    def __init__(self, byte)
        self.byte = byte

    def size(self):
        return type(self).netPack.size

    def unpack(self, obj):
        childClass = type(self)
        self = childClass(*childClass.netPack.unpack(obj))

    def packValues(self):
        return (self.byte,)

    def pack(self):
        childClass = type(self)
        return childClass.netPack.pack(*self.packValues)

class GameState(NetObject):
    netPack = struct.Struct('3B')

    def __init__(self):
        self.numPlayers = 0
        self.fire_id = 0
        self.timer = 20

    def packValues(self):
        return (self.numPlayers, self.fire_id, self.timer)

class Player(NetObject):
    netPack = struct.Struct('c2?3f')

    def __init__(self, id, x, y, z):
        self.id = id
        self.alive = True
        self.connected = True
        self.x = x
        self.y = y
        self.z = z

    def packValues(self):
        return (self.id, self.alive, self.connected, self.x, self.y, self.z)

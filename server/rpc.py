import sys
import socket
import struct

class RpcHandler: #Abstract RPC handler class
    messagePack = struct.Struct('BI')
    ackPack = struct.Struct('?')
    HEAD_TERMINATE = 0
    
    def __init__(self):
        self.clients = []

    def addClient(self, socket):
        self.clients.append(socket)

    #handleRemoteBytes: bytes to send back over the connection, return a tuple of (bytesReturned, sizeOfBytes)
    def handleRemoteBytes(self, data, header):
        return (data, len(data))

    def terminateConneciton(self, socket, arg):
        return

    #This shouldn't change, define handleRemoteBytes instead
    def handleMessages(self):
        for socket in self.clients:
            (header, size) = RpcHandler.messagePack.unpack(socket.recv(RpcHandler.messagePack.size))

            if(header == RpcHandler.HEAD_TERMINATE):
                self.terminateConnection(socket, size)
                self.clients.remove(socket)
                continue

            socket.sendall(RpcHandler.ackPack.pack(True))
            (retBytes, sizeRet) = self.handleRemoteBytes(socket.recv(size), header)
            
            socket.sendall(RpcHandler.messagePack.pack(header, sizeRet))

            socket.recv(RpcHandler.ackPack.size)
            socket.sendall(retBytes)

class NetObject: #Interface for network objects
    netPack = struct.Struct('B')

    def __init__(self, byte):
        self.byte = byte

    def unpack(obj):
        return NetObject(*netPack.unpack(obj))

    def packValues(self):
        return (self.byte,)

    def pack(self):
        childClass = type(self)
        return childClass.netPack.pack(*self.packValues())


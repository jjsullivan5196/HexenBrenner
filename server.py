import time
import socket
import struct
import threading
import sys


pStruct = struct.Struct('BBBfff')
stateStruct = struct.Struct('BBBI')

#Class for player information
class playerInfo:
    def __init__(self, id, x, y, z):
        self.id = id
        self.alive = True
        self.touch = id
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return 'Player: {}\nPos: ({},{},{})\n'.format(self.id, self.x, self.y, self.z)

    def pack(self):
        return pStruct.pack((self.id, self.alive, self.touch, self.x, self.y, self.z))

    def unpack(data):
        self.id = data[0]
        self.alive = data[1]
        self.touch = data[2]
        self.x = data[3]
        self.y = data[4]
        self.z = data[5]

class currentState:
    def __init__(self):
        self.time = 0
        self.lit = 0
        self.dTime = 0
        self.numPlayers = 0

players = {} #Player dictionary: associative array of connected players
clientSockets = []

def clientHandler():
    while True:
        #Lock player dictionary so main thread can't add new players
        #with plDictLock:
        for cinfo in clientSockets:
            try:
                csock = cinfo[0]
                #Get client's new position
                rawData = csock.recv(playerPack.size)
                data = playerPack.unpack(rawData)
                players[data[0]] = playerInfo(data[0], data[2], data[3], data[4])
                gameState.id = data[1]
                #print('Position Received:\n' + str(players[int(data[0])]))
                print('Position received')

                #Send number of players
                nPlayers = struct.pack('B', numPlayers)
                csock.sendall(nPlayers)

                print('Sent no. players')

                #Check that client got message
                clientOK = bool(struct.unpack('B',csock.recv(struct.calcsize('B'))))
                if clientOK:
                    bigMsg = b''
                    for plKey in players:
                        pl = players[plKey]
                        tagVals = (pl.id, gameState.id, pl.x, pl.y, pl.z)
                        tagData = playerPack.pack(*tagVals)
                        bigMsg = bigMsg + tagData
                    csock.sendall(bigMsg)
                else:
                    print('Something bad happened')

                print('Sent players')

            except:
                del players[cinfo[1]]
                clientSockets.remove(cinfo)

def serverMain():
    global numPlayers
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #Create new TCP listen socket
    serversocket.bind(('', 27015)) #Bind to any host on port 27015 (game connections)
    serversocket.listen(5) #Listen for clients
    cHandle = threading.Thread(target=clientHandler)
    cHandle.setDaemon(True)
    cHandle.start()
    gameState = currentState(0)

    print('Server running')
    
    #Do stuff with incoming connections here
    try:
        while True:
            #New client has arrived
            (clientsocket, address) = serversocket.accept()
            print('Client has connected')

            #Make a new player object for them
            newPlayer = playerInfo(numPlayers, 0.0, 0.0, 0.0)

            #Add new player to players dictionary (Lock first)
            print('Adding player')
            clientSockets.append((clientsocket, newPlayer.id))
            players[newPlayer.id] = newPlayer

            tagVals = (newPlayer.id, 0, newPlayer.x, newPlayer.y, newPlayer.z)
            tagData = playerPack.pack(*tagVals)
            clientsocket.sendall(tagData)
            print('Player added')

            #Increment number of players
            numPlayers += 1
    except KeyboardInterrupt:
        sys.exit('Server terminating')

serverMain()

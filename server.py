import time
import socket
import struct
import threading
import sys

#Class for player information
class playerInfo:
    def __init__(self, id, x, y, z):
        self.id = id
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return 'Player: {}\nPos: ({},{},{})\n'.format(self.id, self.x, self.y, self.z)

class currentState:
    def __init__(self, id):
        self.id = id
        self.time = time.time()

players = {} #Player dictionary: associative array of connected players
clientSockets = []
plDictLock = threading.Lock() #Lock for modifying/reading the player dictionary (Thread safety)
numPlayers = 0
witchTimer = 20

playerPack = struct.Struct('ccfff') #C struct object for handling network messages

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
                players[int(data[0])] = playerInfo(int(data[0]), data[2], data[3], data[4])
                #print('Position Received:\n' + str(players[int(data[0])]))
                print('Position received')

                #Send number of players
                nPlayers = struct.pack('c', str(numPlayers).encode())
                csock.sendall(nPlayers)

                print('Sent no. players')

                #Check that client got message
                clientOK = bool(struct.unpack('c',csock.recv(struct.calcsize('c'))))
                if clientOK:
                    bigMsg = b''
                    for plKey in players:
                        pl = players[plKey]
                        tagVals = (str(pl.id).encode(), str(1).encode(), pl.x, pl.y, pl.z)
                        tagData = playerPack.pack(*tagVals)
                        bigMsg = bigMsg + tagData
                    csock.sendall(bigMsg)
                else:
                    print('Something bad happened')

                print('Sent players')

                state = int(struct.unpack('c', csock.recv(struct.calcsize('c'))))
                if(gameState.time < time.time()):
                    gameState.id = state
                    gameState.time = time.time() + 5

                print('State received')

                sendState = struct.pack('c', str(gameState.id).encode())
                csock.sendall(sendState)
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
            #with plDictLock:
            print('Adding player')
            clientSockets.append((clientsocket, newPlayer.id))
            players[newPlayer.id] = newPlayer

            tagVals = (str(newPlayer.id).encode(), str(1).encode(), newPlayer.x, newPlayer.y, newPlayer.z)
            tagData = playerPack.pack(*tagVals)
            clientsocket.sendall(tagData)
            print('Player added')

            #Increment number of players
            numPlayers += 1
    except KeyboardInterrupt:
        sys.exit('Server terminating')

serverMain()

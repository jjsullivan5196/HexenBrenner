import time
import socket
import struct
import threading
import sys
from ghandle import *

gameManager = GameHandler()

def clientHandler():
    while True:
        try:
            gameManager.handleMessages()
        except OSError as e:
            print(e)

def serverMain():
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #Create new TCP listen socket
    serversocket.bind(('', 27015)) #Bind to any host on port 27015 (game connections)
    serversocket.listen(5) #Listen for clients
    cHandle = threading.Thread(target=clientHandler)
    cHandle.setDaemon(True)
    cHandle.start()

    print('Server running')
    
    #Do stuff with incoming connections here
    try:
        while True:
            #New client has arrived
            (clientsocket, address) = serversocket.accept()
            print('Client has connected')

            gameManager.addClient(clientsocket)

    except KeyboardInterrupt:
        sys.exit('Server terminating')

serverMain()

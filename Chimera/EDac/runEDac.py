import socket
import Edac as edac
import sys
import os
import queue
import time
import numpy as np
import directory_watcher as dw

'''
sys.argv[0] - eDAC channel to change the voltage value
sys.argv[1] - currently an array of eDAC voltage values
'''



timeout = 1.02
#The port and the IP address are harcoded while configuring the FPGA. Contact Felix if this needs to change.
port = 804
host = '192.168.7.179'
dest = (host, int(port))
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
sock.settimeout(timeout)

####Implementing Sr Labs File Wather in Python

edacFileName = "EDACFile.txt.tmp"
if __name__ == "__main__":
    watcher = dw.DirectoryWatcher(
        r'B:\Yb heap\Experiment_code_Yb\Chimera-Control-Master\Chimera',
        filter=['$RECYCLE.BIN\\**\\*', '$RECYCLE.BIN\\*', 'tmp*']
    )
    watcher.start()
    print('Begin watching for files from Chimera')
    while True:
        try:
            file_path, action_name = watcher.events.get(timeout=.005)
            print(os.path.basename(file_path))
            isPyStartFile = os.path.basename(file_path) == "EDACFile.txt"
            print(os.path.basename(file_path))
            print(file_path)
            print(action_name)
            if action_name == 'rename' and isPyStartFile:
                time.sleep(10e-3)
                print("Detected communication from Chimera!")
                
                with open(file_path, 'r') as f:
                    values = f.read()
                    print (values)

                    valueArray = []

                for idx, val in enumerate(values.split(',')):
                    if idx == 0:
                        pass
                    else:
                        valueArray.append(float(val))
                print(valueArray)

                
                #Send Message - Voltage Values For All dacs, right now we are limited to setting all channels together
                data = edac.get_message(valueArray)
                tosend = bytearray.fromhex(str(data))
                print(tosend)
                sock.sendto(tosend, dest)


        except queue.Empty:
            pass






print('complete')

import sys
import os
import queue
import time
import numpy as np

from lib import (
    directory_watcher
)

import analysis_and_feedback
import srs_update

PY_START_FILE_NAME = "pyStart.txt"
PY_FINISH_FILE_NAME = "pyFinish.txt"
PY_CURRENT_VARIABLE_FILE_NAME = 'currentVariables.txt'

if __name__ == "__main__":
    watcher = directory_watcher.DirectoryWatcher(
        r'C:\Users\KLab\Desktop\Chimera-Control-Master\Python Chimera Communication',
        filter=['$RECYCLE.BIN\\**\\*', '$RECYCLE.BIN\\*', 'tmp*']
    )
    watcher.start()
    print('Begin watching for files from Chimera')
    while True:
        try:
            file_path, action_name = watcher.events.get(timeout=.005)
            isPyStartFile = os.path.basename(file_path) == PY_START_FILE_NAME
            isCurrentVariablesFile = os.path.basename(file_path) == PY_CURRENT_VARIABLE_FILE_NAME
            print(file_path)
            print(action_name)
            if action_name == 'rename' and isCurrentVariablesFile:
                time.sleep(10e-3)
                print("Detected communication from Chimera!")

                with open(file_path, 'r') as f:
                    configFile, dataFile = [line for line in f.read().splitlines() if line.strip()]
                    print(configFile)
                    print(dataFile)

                analysis_and_feedback.main(configFile, dataFile)

            # elif action_name == 'rename' and isCurrentVariablesFile:
            #     time.sleep(1e-3)
            #     print("Detected communication from Chimera!")

            #     srs_update.main(file_path)

        except queue.Empty:
            pass

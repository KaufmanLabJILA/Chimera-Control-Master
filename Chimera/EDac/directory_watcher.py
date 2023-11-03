# see http://timgolden.me.uk/python/win32_how_do_i/watch_directory_for_changes.html # noqa
# for details of the file watcher implementation on Windows

import os
import threading
import queue
import pathlib

import win32file
import win32con


class DirectoryWatcher():
    _FILE_LIST_DIRECTORY = 0x0001
    _ACTIONS = {
        1: 'create',
        2: 'delete',
        3: 'update',
        4: 'rename-from',
        5: 'rename-to'
    }
    _NOTIFY_FILTER = (
        win32con.FILE_NOTIFY_CHANGE_FILE_NAME |
        win32con.FILE_NOTIFY_CHANGE_DIR_NAME |
        win32con.FILE_NOTIFY_CHANGE_ATTRIBUTES |
        win32con.FILE_NOTIFY_CHANGE_SIZE |
        win32con.FILE_NOTIFY_CHANGE_LAST_WRITE |
        win32con.FILE_NOTIFY_CHANGE_SECURITY)

    def __init__(self, watch_path: str,
                 filter=['$RECYCLE.BIN\\**\\*', '$RECYCLE.BIN\\*']):
        if not os.path.exists(watch_path):
            raise ValueError(f'Watch path {watch_path} does not exist.')

        if not os.path.isdir(watch_path):
            raise ValueError(f'Watch path {watch_path} is not a directory.')

        self._watch_path = watch_path
        self._watch_handle = win32file.CreateFile(
            watch_path,
            self._FILE_LIST_DIRECTORY,
            win32con.FILE_SHARE_READ | win32con.FILE_SHARE_WRITE,
            None,
            win32con.OPEN_EXISTING,
            win32con.FILE_FLAG_BACKUP_SEMANTICS,
            None)

        self._thread = None
        self._exiting = False
        self.events = queue.SimpleQueue()

        self._filter = filter

    def start(self) -> bool:
        """Start watching for file and directory changes."""
        if self._thread is not None:
            return False

        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

        return True

    def stop(self) -> bool:
        """Stop watching for file and directory changes."""
        if self._thread is None:
            return False

        self._exiting = True
        self._thread.join()
        self._thread = None
        self._exiting = False

        return True

    def _loop(self):
        while self._exiting is False:
            events = win32file.ReadDirectoryChangesW(
                self._watch_handle, 16384, True, self._NOTIFY_FILTER,
                None, None)

            for action, file_name in events:
                if self._match_filter(file_name):
                    continue

                action_name = self._ACTIONS.get(action, 'unknown')

                # sanitize rename actions
                if action_name == 'rename-from':
                    continue

                if action_name == 'rename-to':
                    action_name = 'rename'

                file_path = os.path.join(self._watch_path, file_name)
                self.events.put_nowait([file_path, action_name])

    def _match_filter(self, file_name):
        pp = pathlib.PurePath(file_name)
        for pattern in self._filter:
            if pp.match(pattern):
                return True

        return False

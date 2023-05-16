# Logging daemon task

## About
[Task assignment](https://github.com/Cropi/task)

## Building and running
```
mkdir build
cd build
cmake ../CMakeLists.txt
make
sudo ./log_daemon
```

## Note
I am aware the error handling is not ideal, but I haven't got time to improve it.
Malloc/realloc are not checked, and errors should be propagated from logger.c to main.
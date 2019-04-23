# Webserver in C

Project assignment for Computer Networks course. Webserver written in C that satisfies HTTP GET requests. Webserver is implemented in three different ways: single-threaded, multi-threaded, and thread-pooled

## Single-threaded server

1) Waits for client request
2) Processes the request
3) Repeats this cycle.
All of these steps are executed in a single thread

## Multi-threaded server

1) Waits for client request
2) When it accepts a new client connection, creates a new thread, and waits for other
clients
3) The new thread processes the new request

## Thread-pooled server

Web server creates threads very frequently, so it can consume lots of computing
resources to create new threads. To solve this problem, thread-pooled server creates
the threads in advance, wakes a thread and gives task to the thread whenever it is needed.
https://en.wikipedia.org/wiki/Thread_pool

## Usage

Download files, use **make** to compile servers. To run execute ./server. To connect to the server, send the requests with browser to your local IP and port 5000, like http://IP:5000 
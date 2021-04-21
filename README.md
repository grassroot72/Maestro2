# Maestro2 

This project aims to create a tiny but functional application server by 
leveraging multithread parallelism and epoll scheduling of linux OS. 

All the source code is written in C. 
Some of the datastructures, such as, red black tree is used as the container 
of the connection nodes, http keep alive feature is carefully tangled in the 
rolling of connection nodes. Please dive into the source code for more details 
:)

Writing a webserver is quite a good practice to know how the datastructures 
can work with each other. It also involves quite a lot of crypto techniques if 
authrization should be applied to the web application. I use JWT auth theme 
with cookies in Maetro. Other auth themes are not considered for now.

Just for interests, there is also a skiplist version of Maestro2 in another 
branch if you want to see how other datastruct container works.


## Features

  - Multithread pool
  - Epoll (so, linux specific)
  - Non-blocking
  - HTTP/1.1 GET method (static file)
  - HTTP/1.1 HEAD method (static file)
  - HTTP/1.1 POST method
  - HTTP/1.1 chunked transfer
  - HTTP/1.1 keep-alive (long connection, disconnected after timeouts)
  - built-in cache to provide better GET performance
  - deflate compression
  - download resumption
  - jwt auth theme


## Applicable Scenarios
The source code could be adapted to work as a framework for developing 
high perfomance appication servers.

## Contact
If you are interested in the project, drop me an email at Edward_lei72@hotmail.com
# HttpDebugger

HttpDebugger is a lightweight HTTP/HTTPS proxy server written in C for Windows. It captures and logs HTTP requests, and supports HTTPS traffic via tunneling using the CONNECT method. 
This tool is ideal for developers and network enthusiasts who need to monitor HTTP requests from their local machine.

## Features
Handles both HTTP and HTTPS traffic
Logs HTTP request headers and bodies
Supports HTTPS tunneling via the CONNECT method
Built using native Windows sockets (WinSock2)
No third-party dependencies
    
### Getting Started
Prerequisites
1.Windows operating system
2.C compiler (e.g., MSVC, MinGW)

### Building the Project
1) Clone the repository:
```powershell
git clone https://github.com/Raulisr00t/HttpDebugger.git
cd HttpDebugger
```
2) Compile the source code:
```powershell
  cl /W4 /FeHttpDebugger.exe HttpDebugger.c ws2_32.lib
```

### Running the Proxy Server

Start the proxy server by executing the compiled binary:
```powershell
HttpDebugger.exe
```
By default, the proxy listens on 127.0.0.1:8080.

### Configuring Your Browser

To route your browser's traffic through the proxy:

1.Open your browser's network/proxy settings.
2.Set the HTTP and HTTPS proxy to 127.0.0.1 and port 8080.
3.Save the settings and browse as usual.

The proxy will log HTTP requests to the console. HTTPS traffic is tunneled without inspection.

## Limitations
Does not decrypt or inspect HTTPS content.
Designed for single-threaded operation; concurrent connections are handled sequentially.
No support for advanced proxy features like caching or authentication.

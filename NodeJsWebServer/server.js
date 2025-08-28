const net = require('net');

function createWebServer(reqHandler){
    const server = net.createServer();
    server.on('connection', handleConnection)
    function handleConnection(socket){
    //Subscribe to readable event once to start calling .read()
        socket.once('readable', function(){
            //set up buffer to hold incoming data
            let reqBuffer = Buffer.from('');
            //temp buffer to read in chunk
            let buf;
            let reqHead;
            while(true){
                //read data from buf
                buf = socket.read();
                //stop if no data
                if(buf === null) break;

                //concat existing request buffer with new data
                reqBuffer = Buffer.concat([reqBuffer, buf])

                //Check if we've reached \r\n\r\n, indicating end of header
                let marker = reqBuffer.indexOf('\r\n\r\n')
                if(marker !== -1){
                    // If we reached \r\n\r\n, there could be data after it. Take note.
                    let remaining = reqBuffer.slice(marker + 4);
                    // The header is everything we read, up to and not including \r\n\r\n
                    reqHead = reqBuffer.slice(0, marker).toString()

                    socket.unshift(remaining)
                    break
                }
            }
          
            const reqHeaders = reqHead.split('\r\n')
            const reqLine = reqHeaders.shift().split(' ')
            const headers = reqHeaders.reduce((acc, currentHeader) => {
                const [key, value] = currentHeader.split(':')
                return {
                    ...acc,
                    [key.trim().toLowerCase()] : value.trim()
                }
            }, {})

            const request = {
                method: reqLine[0],
                url: reqLine[1],
                httpVersion: reqLine[2].split('/')[1],
                headers,
                socket
            }

            let status = 200, statusText = 'OK', headersSent = false, isChunked = false;

            const responseHeaders = {
                server: 'my-special-server-5000'
            }

            function setHeader(key, value){
                responseHeaders[key.toLowerCase()] = value
            }

            function sendHeaders(){
                if(!headersSent){
                    headersSent = true
                    setHeader('date', new Date().toGMTString())

                    socket.write(`HTTP/1.1, ${status} ${statusText}\n\r`)

                    Object.keys(responseHeaders).forEach(headerKey => {
                        socket.write(`${headerKey}: ${responseHeaders[headerKey]}\r\n`)
                    })
                    socket.write('\r\n')
                }
            }

            const response = {
                write(chunk){
                    if(!headersSent){
                        if(!responseHeaders['content-length']){
                            isChunked = true
                            setHeader('transfer-encoding', 'chunked')
                        }
                        sendHeaders()
                    }
                    if(isChunked){
                        const size = chunk.length.toString(16)
                        socket.write(`${size}\r\n`)
                        socket.write(chunk)
                        socket.write('\r\n')
                    }else{
                        socket.end(chunk)
                    }
                }, end(chunk){
                    if(!headersSent){
                        if(!responseHeaders['content-length']){
                            setHeader('transfer-encoding', chunk? chunk.length : 0)
                        }
                        sendHeaders()
                    }
                    if(isChunked){
                        if(chunk){
                            const size = chunk.length.toString(16)
                            socket.write(`${size}\r\n`)
                            socket.write(chunk)
                            socket.write('\r\n')
                        }
                        socket.end('0\r\n\r\n')
                    }else{
                        socket.end(chunk)
                    }
                },setHeader, setStatus(newStatus, newStatusText){status = newStatus, statusText = newStatusText},
                json(data){
                    if(headersSent){
                        throw new Error('Headers sent, can not proceed to send json')
                    }
                    const json = Buffer.from(JSON.stringify(data))
                    setHeader('content-type', 'application/json; charset=utf-8');
                    setHeader('content-length', json.length);
                    sendHeaders();
                    socket.end(json);
                }
            }
            reqHandler(request, response)
        })
    }

    return{
        listen: (port) => server.listen(port)
    }
}

const webServer = createWebServer((req, res) => {
  console.log(`${new Date().toISOString()} - ${req.method} ${req.url}`);
  res.setHeader('Content-Type','text/plain');
  res.end('Hello World!');
});

webServer.listen(5000);

//server.listen(5000);
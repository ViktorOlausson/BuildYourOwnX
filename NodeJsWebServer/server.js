const net = require('net');
const fs = require('fs');
const path = require('path');

const configPath = path.join(__dirname, 'config.json');
const config = JSON.parse(fs.readFileSync(configPath, 'utf-8'));

const HOST = config.host || 'localhost';
const PORT = config.port || 5000;

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
                //read data from socket
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
                    // This pushes the extra data we read back to the socket's readable stream
                    socket.unshift(remaining)
                    break
                }
            }
          
            /* Request-related business */
            // Start parsing the header
            const reqHeaders = reqHead.split('\r\n')
            const reqLine = reqHeaders.shift().split(' ')
            const headers = reqHeaders.reduce((acc, currentHeader) => {
                const [key, value] = currentHeader.split(':')
                return {
                    ...acc,
                    [key.trim().toLowerCase()] : value.trim()
                }
            }, {})

            // This object will be sent to the handleRequest callback.
            const request = {
                method: reqLine[0],
                url: decodeURIComponent(reqLine[1].split('?')[0] || '/'),
                httpVersion: reqLine[2].split('/')[1],
                headers,
                socket
            }


            /* Response-related business */
            // Initial values
            let status = 200, statusText = 'OK', headersSent = false, isChunked = false;

            const responseHeaders = {
                server: 'my-special-server-5000'
            }
            function setHeader(key, value){
                responseHeaders[key.toLowerCase()] = value
            }

            function sendHeaders(){
                // Only do this once :)
                if(!headersSent){
                    headersSent = true
                    // Add the date header
                    setHeader('date', new Date().toUTCString())
                    // Send the status line
                    socket.write(`HTTP/1.1 ${status} ${statusText}\n\r`)
                    // Send each following header
                    Object.keys(responseHeaders).forEach(headerKey => {
                        socket.write(`${headerKey}: ${responseHeaders[headerKey]}\r\n`)
                    })
                    // Add the final \r\n that delimits the response headers from body
                    socket.write('\r\n')
                }
            }

            const response = {
                write(chunk){
                    if(!headersSent){
                        // If there's no content-length header, then specify Transfer-Encoding chunked
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
                        // We know the full length of the response, let's set it
                        if(!responseHeaders['content-length']){
                            // Assume that chunk is a buffer, not a string!
                            setHeader('content-length', chunk? chunk.length : 0)
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
                // Convenience method to send JSON through server
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
            // Send the request to the handler!
            reqHandler(request, response)
        })
    }

    
    console.log("listening to: ")
    return{
        listen: (port) => server.listen(port)
    }
}
// MIME helper
const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',  
    '.js': 'application/javascript; charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.json': 'application/json; charset=utf-8',
    '.txt': 'text/plain; charset=utf-8'
}

function serveFile(res, filePath){
    const root = path.join(__dirname, 'Site')
    const safePath = path.normalize(path.join(root, filePath))
    
    if(!safePath.startsWith(root)){
        res.setStatus(403, 'Forbidden')
        return res.end('Forbidden')
    }

    fs.readFile(safePath, (err, data) => {
        if (err) {
            res.setStatus(err.code === 'ENOENT' ? 404 : 500);
            res.setHeader('Content-Type', 'text/plain; charset=utf-8');
            res.end(err.code === 'ENOENT' ? 'Not Found' : 'Error reading file');
        } else {
            const ext = path.extname(safePath).toLowerCase();
            res.setHeader('Content-Type', MIME[ext] || 'application/octet-stream');
            res.setHeader('Content-Length', data.length);
            res.end(data);
        }
    });
}

const webServer = createWebServer((req, res) => {
    console.log(`${new Date().toISOString()} - ${req.method} ${req.url}`);

    if(req.method !== 'GET'){
        res.setStatus(405, 'Method Not Allowed')
        res.setHeader('Allow', 'GET')
        return res.end('Method Not Allowed')
    }

    switch(req.url){
        case '/':
        case '/index.html':
            return serveFile(res, 'index.html');

        default:
            if (req.url.startsWith('/static/')) {
            const rel = req.url.replace(/^\/static\//, '');
            return serveFile(res, path.join('static', rel));
            }
            // Fallback for React Router routes:
            return serveFile(res, 'index.html'); // <--- important for React Router
    }
});

webServer.listen(PORT);

//server.listen(5000);
const net = require('net');
const server = net.createServer();
server.on('connection', handleConnection)
server.listen(5000);

function handleConnection(socket){
    // socket.on('data', (chunk) =>{
    //     console.log('received chunk', chunk.toString())
    // });
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
            if(data == null) break;

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
        console.log(`Request header:\n${reqHead}`)

        reqBuffer = Buffer.from('')
        while((buf = socket.read()) != null){
            reqBuffer = Buffer.concat([])
        }
        let reqBody = reqBuffer.toString()
        console.log(`Request body:\n${reqBody}`)

        socket.write('HTTP/1.1 200 OK\r\nServer: my-web-server\r\nContent-Length: 0\r\n\r\n');
    })
}
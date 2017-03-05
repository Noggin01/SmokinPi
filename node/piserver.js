var express = require('express');
var server = require('http').createServer(app);  
var io = require('socket.io')(server);
var net = require('net');

var app = express();
var statusData = '';

var client = new net.Socket();
    client.connect(46879, '127.0.0.1', function() {
	console.log('Connected');
	var cmd = new Buffer("SETTEMP=300.0");
    client.write(cmd);
   
   setInterval(function(){ client.write("STATUS?")}, 1000);
});

client.setEncoding('utf8');
client.on('data', function(rsp)
{
   console.log('Response: %s', rsp);
   statusData = rsp;
});

app.use(express.static('public'));

app.get('/', function (req, res)  {
	res.sendFile( __dirname + "/public/" + "index.htm" );
});

var server = app.listen(8081, function () {
   var host = server.address().address
   var port = server.address().port

   console.log("Example app listening at http://%s:%s", host, port)
})

var listener = io.listen(server);

listener.sockets.on('connection', function(socket)
{
   	// Send data to client
	setInterval(function(){socket.emit('date', {'date': new Date()});},	1000);
	setInterval(function(){
		if (statusData != '')
		{
			socket.emit('statusData', {'statusData': statusData }); 
			statusData = '';
		}}, 1000);
	// Recieve client data
   
	socket.on('client_data', function(data){process.stdout.write(data.letter);});
});

io.listen(server);

var udp = require('dgram');

var pcttn = udp.createSocket('udp4');
var gwypc = udp.createSocket('udp4');

var gwaddress = "192.168.1.135";

gwypc.on('message', function(msg, info) {
	gwaddress = info.address;
	console.log('GW --> PX : ', msg.length + " bytes ", info.address + ":" + info.port);
	pcttn.send(msg, 1700, 'router.us.thethings.network', function(error) {
		if (error) {
			client.close();
		} else {
			console.log('PX --> TTN');
		}
	});
});

gwypc.on('listening', function() {
	var address = gwypc.address();
	console.log('GATEWAY <-> PROXY is listening ' + address.address + ":" + address.port);
});

gwypc.on('error',function(error) { console.log('gwypc Error: ' + error); gwypc.close();});
gwypc.on('close',function() { console.log('gwypc closed !'); });
gwypc.bind(1700);


pcttn.on('message', function(msg, info) {
	console.log('PX <-- TTN : ', msg.length + " bytes " + info.address + ":" + info.port,  msg);
	gwypc.send(msg, 1700, gwaddress, function(error) {
		if(error) {
			client.close();
		} else {
			console.log('GW <-- PX ', msg);
		}
	});
});
pcttn.on('listening', function() {
	var address = pcttn.address();
	console.log('PROXY <-> TTN is listening ' + address.address + ":" + address.port);
});

pcttn.on('error',function(error) { console.log('pcttn Error: ' + error); gwypc.close();});
pcttn.on('close',function() { console.log('pcttn closed !'); });
pcttn.bind(1800);


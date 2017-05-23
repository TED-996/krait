function get_websocket_url() {
    var loc = window.location;
    var new_uri;
    if (loc.protocol === "https:") {
        new_uri = "wss:";
    } else {
        new_uri = "ws:";
    }
    new_uri += "//" + loc.host;
    new_uri += "/ws_socket";
    return new_uri;
}

function WebsocketPingPong() {
    this.socket = new WebSocket(get_websocket_url(), "pingpong");
    this.ticker = new Ticker();

    this.on_send = function(msg){
        this.ticker.ticker_add(msg, "msg_client")
    };
    this.on_recv = function(msg) {
        this.ticker.ticker_add(msg, "msg_server")
    };

    this.random_string = function() {
        var text = "";
        var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        var length = Math.floor(Math.random() * 5 + 5);

        for( var i=0; i < length; i++ )
            text += possible.charAt(Math.floor(Math.random() * possible.length));

        return text;
    };

    this.closed = false;
    this.msg_queue = [];
    this.loop = function() {
        var self = this;
        setTimeout(function () {
            if (!self.closed) {
                var msg = self.msg_queue.shift();
                if (msg !== undefined){
                    self.on_recv(msg);

                    if (msg.startsWith("PING")){
                        var pong_msg = "PONG: " + msg;
                        self.socket.send(pong_msg);
                        self.on_send(pong_msg);
                    }
                }
                if (Math.random() < 0.05){
                    var ping_msg = "PING: " + self.random_string();
                    self.socket.send(ping_msg);
                    self.on_send(ping_msg)
                }

                self.loop();
            }
        }, 100);
    };

    this.run = function () {
        var self = this;
        this.socket.onmessage = function(event) {
            self.msg_queue.push(event.data)
        };
        this.socket.onclose = function () {
            self.closed = true;
        };

        this.loop();
    };
}

$(function() {
    new WebsocketPingPong().run();
});

var _             = require('underscore');
var net           = require('net');
var sg            = require('./sg');
var ARGV          = sg.ARGV();

var lib           = {};

var main = function(callback) {
  lib.actLikePrinter(ARGV.port, callback);
};

var Printer = lib.Printer = function(port, options_) {
  var self = this;

  var reportError, errResult = null;

  self.port     = port;

  var options   = options_ || {};
  var callback  = options.callback || function() {};

  self.open = function(port) {

    self.port = self.port || port;

    self.server = net.createServer(function(client) {

      var chunks = [];
      client.on('data', function(chunk) {
        chunks.push(chunk);
      });

      client.on('end', function() {
        var foundMagic = chunks.join('').indexOf("-12345X@PJL") !== -1;
        self.server.close();
        callback(errResult, foundMagic);
      });

      client.on('error', function(err) {
        reportError(err, 'client.error');
      });

      client.on('close', function(hadError) {
        if (hadError) {
          reportError(err, 'client.close:hadError');
        }
      });
    });

    self.server.listen(self.port, function() {
      //console.log('net.Server.bound');
    });

    self.server.on('close', function() {
      //console.log('net.Server.closing');
    });

    self.server.on('error', function(err) {
      reportError(err, 'net.Server.error');
    });

  };

  reportError = function(err, msg) {
    console.error(err, msg);

    if (errResult === null) {
      errResult = err;
    } else if (_.isArray(errResult)) {
      errResult.push(err);
    } else {
      errResult = [errResult];
      errResult.push(err);
    }
  };

  if (self.port) {
    self.open(self.port);
  }
};

lib.actLikePrinter = function(port, callback) {
  new Printer(port, {callback : callback});
};

if (process.argv[1] === __filename) {
  return main(function(err, didPass) {
    if (err) { console.log(err); }

    if (didPass) {
      console.log("Pass.");
    }
  });
}

sg.exportify(lib, exports);


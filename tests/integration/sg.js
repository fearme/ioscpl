
/**
 *  sg!
 */

var _             = require('underscore');
var fs            = require('fs');
var util          = require('util'), sys = util;
var glob          = require('glob');
var path          = require('path');
var urlLib        = require('url');
var properties    = require ("properties");

var EventEmitter  = require('events').EventEmitter;
var exec          = require('child_process').exec;
var spawn         = require('child_process').spawn;


var slice = Array.prototype.slice;
var lib = {};
var shutdownHandlers = [];

var noop = lib.noop = function(){};

var verbosity = lib.verbosity = function() {

  var vLevel = 0;
  if      (theARGV.vvvvvvvvverbose)  { vLevel = 9; }
  else if (theARGV.vvvvvvvverbose)   { vLevel = 8; }
  else if (theARGV.vvvvvvverbose)    { vLevel = 7; }
  else if (theARGV.vvvvvverbose)     { vLevel = 6; }
  else if (theARGV.vvvvverbose)      { vLevel = 5; }
  else if (theARGV.vvvverbose)       { vLevel = 4; }
  else if (theARGV.vvverbose)        { vLevel = 3; }
  else if (theARGV.vverbose)         { vLevel = 2; }
  else if (theARGV.verbose)          { vLevel = 1; }

  return vLevel;
};

var isDebug = lib.isDebug = function() {
  if ('MARIO_DEBUG' in process.env && process.env.MARIO_DEBUG !== '0') { return true; }

  if (theARGV.debug) { return true; }
  //if (verbosity() > 0) { return true; }

  return false;
};

var pad = lib.pad = function(str_, width, ch_) {
  var ch;
  if (ch_ === 0) { ch='0'; }
  else           { ch= ''+ (ch_ || ' '); }

  var str = ''+str_;
  while(str.length < width) {
    str = ch + str;
  }
  return str;
};

/**
 *
 *  printf("here it %03s1 is", 4) ==> "here it 004 is"
 *
 *  args are 1-based
 *
 *
 */
var printf = lib.printf = function(fmt) {
  var params = _.rest(arguments);
  return fmt.replace(/(%([0-9]*)s([0-9]))/g, function(m, p, width, argNum) {
    var repl = params[argNum-1];
    if (width.length > 0) {
      if (width[0] === '0') {
        repl = pad(repl, width, '0');
      } else {
        repl = pad(repl, width);
      }
    }

    return repl;
  });
};

/*var firstKey =*/ lib.firstKey = function(obj) {
  for (var k in obj) {
    return k;
  }
  return ;
};

/*var numKeys =*/ lib.numKeys = function(obj) {
  var num = 0;
  for (var k in obj) {
    num++;
  }

  return num;
};

var kv = lib.kv = function(o, k, v) {
  if (arguments.length === 2) {
    return kv(null, o, k);
  }

  o = o || {};
  o[k] = v;
  return o;
};

var mvKv = lib.mvKv = function(b, a, key, bKey_) {
  var bKey = bKey_ || key;

  var result;
  if (a[key]) {
    result = b[bKey] = a[key];
    delete a[key];
  } else {
    result = b[bKey] = null;
  }

  return result;
};

lib.Lineifier = function() {
  var self = this;

  self.remaining = '';
  self.lines = function(chunk, eachFn) {

    if (!chunk) {
      if (self.remaining.length > 0) {
        return eachFn(self.remaining);
      }
    }

    /* otherwise */
    var lines = self.remaining + chunk;
    lines = lines.split('\n');
    self.remaining = lines.pop();

    _.each(lines, eachFn);
  };
};

var tryToSendJson = function(line, fn) {

  var json = lib.safeJSONParse(line, null);

  if (json) { fn(line, json); }
  else      { fn(line); }
};

lib.watchProcess = function(proc, stdoutLineCb_, options_) {
  var options         = options_        || {};
  var stdoutLineCb    = stdoutLineCb_   || function() {};
  var stderrLineCb    = options.stderr  || function() {};
  var finalCb         = options.finalCb || function() {};
  var verbose         = options.verbose || false;
  var quiet           = options.quiet   || false;

  var lineifier       = new lib.Lineifier();
  var stderrLineifier = new lib.Lineifier();

  var error           = null;

  proc.stdout.setEncoding('utf8');
  proc.stdout.on('data', function(chunk) {
    lineifier.lines(chunk, function(line) {
      tryToSendJson(line, stdoutLineCb);
    });
  });

  proc.stderr.setEncoding('utf8');
  proc.stderr.on('data', function(chunk) {
    stderrLineifier.lines(chunk, function(line) {
      tryToSendJson(line, stderrLineCb);
    });
  });

  proc.on('close', function(code) {
    lineifier.lines(null, function(line) {
      tryToSendJson(line, stdoutLineCb);
    });

    stderrLineifier.lines(null, function(line) {
      tryToSendJson(line, stderrLineCb);
    });

    finalCb(error, code);
  });

  proc.on('error', function(err) {
    error = err;
  });
};

/*var ezJsonParse =*/ lib.ezJsonParse = function(str) {
  str = str.replace(/([0-9a-z_]+)(\s*):/ig, function(m, p1, p2) {
    return '"' + p1 + '"' + p2 + ':';
  });
  try {
    return JSON.parse(str);
  } catch(err) {
    return null;
  }
};

var safeJSONParse = lib.safeJSONParse = function(str, def) {
  try {
    return JSON.parse(str);
  } catch(err) {
    if (arguments.length <= 1) {
      verbose(2, "Error parsing JSON", str, err);
    }
  }

  return def || {};
};

var smartParse = lib.smartParse = function(str /*, options, def*/) {
  var args    = _.rest(arguments);
  var options = args.shift() || {};
  var def     = args.pop();

  // First, try to parse the easy way
  try {
    return JSON.parse(str);
  } catch(err) {}

  // TODO: Then, assume form-encoding

  // Then, just safely eval it
  if (options.evalOk) {
    try {
      var result;
      eval('result=' + str.replace(/\n/g, ''));
      return result;
    } catch (e) { console.log(e); }
  }

  return def || options.def || {just:str};
};

/**
 *  Like smartParse, but also turns numbers into numbers
 */
var smarterParse = lib.smarterParse = function(str /*, options, def*/) {
  return _.reduce(smartParse.apply(this, arguments), function(m, value, key) {
    if (/^[0-9]+$/.exec(value)) {
      return kv(m, key, parseInt(value, 10));
    }
    return kv(m, key, value);
  }, {});
};

// Makes the attributes on a data object be the 'right' type (like '0' -> the number zero)
lib.smartAttrs = function(obj) {
  return _.reduce(obj, function(m, value, key) {
    if (/^[0-9]+$/.exec(value)) {
      return kv(m, key, parseInt(value, 10));
    }
    return kv(m, key, value);
  }, {});
};

var TheARGV = function(params_) {
  var self = this;

  var params = params_ || {};

  self.executable = process.argv[0];
  self.script = process.argv[1];
  self.flags = {};
  self.flagNames = [];
  self.args = [];

  self.setFlag = function(key_, value) {
    var key = key_.replace(/-/g, '_');

    self.flags[key] = value;
    self.flagNames.push(key);
    if (self.flags.hasOwnProperty(key) || !self.hasOwnProperty(key)) {
      self[key] = value;
    }
    if (params.short && params.short[key]) {
      self.setFlag(params.short[key], value);
    }
  };

  // Initialize -- scan the arguments
  var curr;
  for (var i = 2; i < process.argv.length; i++) {
    var next = i+1 < process.argv.length ? process.argv[i+1] : null;
    var m, m2;

    curr = process.argv[i];

    // --foo=bar, --foo=
    if ((m = /^--([a-zA-Z_0-9\-]+)=([^ ]+)$/.exec(curr)) && m.length === 3) {
      self.setFlag(m[1], m[2]);
    }
    // --foo-
    else if ((m = /^--([^ ]+)-$/.exec(curr))) {
      self.setFlag(m[1], false);
    }
    // --foo= bar
    else if ((m = /^--([^ ]+)=$/.exec(curr)) && next && (m2 = /^([^\-][^ ]*)/.exec(next))) {
      self.setFlag(m[1], m2[1]);
      i++;
    }
    // --foo
    else if ((m = /^--([^ ]+)$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    // -f-
    else if ((m = /^-(.)-$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    // -f bar
    else if ((m = /^-(.)$/.exec(curr)) && next && (m2 = /^([^\-][^ ]*)/.exec(next))) {
      self.setFlag(m[1], m2[1]);
      i++;
    }
    // -f
    else if ((m = /^-(.)$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    else if (curr === '--') {
      break;
    }
    else {
      self.args.push(curr);
    }
  }

  for (; i < process.argv.length; i++) {
    curr = process.argv[i];
    self.args.push(curr);
  }
};

var TheARGV2 = function(params_) {
  var self = this;

  var params = params_ || {};

  self.executable = process.argv[0];
  self.script = process.argv[1];
  self.flags = {};
  self.flagNames = [];
  self.args = [];

  self.setFlag = function(key_, value_) {
    var key = key_.replace(/-/g, '_');
    var value = value_;

    if (value[0] === '[') {   // ]
      value = smarterParse(value, {evalOk:true}, value);

    } else if (value[0] === '{') {   // }
      value = smarterParse(value, {evalOk:true}, value);
      if (self.flags[key] && _.isObject(self.flags[key]) && _.isObject(value)) {
        // The user is using multiple objects, to be merged
        value = _.extend({}, self.flags[key], value);
      }
    }

    self.flags[key] = value;
    self.flagNames.push(key);
    if (self.flags.hasOwnProperty(key) || !self.hasOwnProperty(key)) {
      self[key] = value;
    }
    if (params.short && params.short[key]) {
      self.setFlag(params.short[key], value);
    }
  };

  // Initialize -- scan the arguments
  var curr;
  for (var i = 2; i < process.argv.length; i++) {
    var next = i+1 < process.argv.length ? process.argv[i+1] : null;
    var m, m2;

    curr = process.argv[i];

    // --foo=bar, --foo=
    if ((m = /^--([a-zA-Z_0-9\-]+)=(.+)$/.exec(curr)) && m.length === 3) {
      self.setFlag(m[1], m[2]);
    }
    // --foo-
    else if ((m = /^--([^ ]+)-$/.exec(curr))) {
      self.setFlag(m[1], false);
    }
    // --foo= bar
    else if ((m = /^--([^ ]+)=$/.exec(curr)) && next && (m2 = /^([^\-].*)/.exec(next))) {
      self.setFlag(m[1], m2[1]);
      i++;
    }
    // --foo
    else if ((m = /^--([^ ]+)$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    // -f-
    else if ((m = /^-(.)-$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    // -f bar
    else if ((m = /^-(.)$/.exec(curr)) && next && (m2 = /^([^\-][^ ]*)/.exec(next))) {
      self.setFlag(m[1], m2[1]);
      i++;
    }
    // -f
    else if ((m = /^-(.)$/.exec(curr))) {
      self.setFlag(m[1], true);
    }
    else if (curr === '--') {
      break;
    }
    else {
      if (curr[0] === '{' || curr[0] === '[') {   // ]}
        curr = smartParse(curr, {evalOk:true}, curr);
      }

      self.args.push(curr);
    }
  }

  for (; i < process.argv.length; i++) {
    curr = process.argv[i];
    self.args.push(curr);
  }
};

var theARGV = null;
var ARGV = lib.ARGV = function(params) {
  if (params) {
    return (theARGV = new TheARGV(params));
  }

  return theARGV || (theARGV = new TheARGV(params));
};
theARGV = ARGV();

var theARGV2 = null;
var ARGV2 = lib.ARGV2 = function(params) {
  if (params) {
    return (theARGV2 = new TheARGV2(params));
  }

  return theARGV2 || (theARGV2 = new TheARGV2(params));
};
theARGV2 = ARGV2();

var inspect = lib.inspect = function() {
  var result = [];

  _.each(arguments, function(arg) {
    result.push(util.inspect(arg, {depth:null, colors:true}));
  });

  return result.join('\n');
};

var logFormat = function(args) {
  return _.chain(args).map(function(arg) {
    // Strip out _id
    if (!arg || !_.isObject(arg) || !arg._id) { return arg; }

    /* otherwise */
    var result = _.extend({}, arg);
    delete result['_id'];
    return result;

  }).map(function(arg) {

    // Inspect
    return inspect(arg);
  }).value();
};

var inspectFlat = lib.inspectFlat = function() {
  var result = [];

  _.each(arguments, function(arg) {
    result.push(util.inspect(arg, {depth:null, colors:true}));
  });

  return result.join('\n').replace(/\n/g, '');
};

var logFormatFlat = function(args) {
  return _.chain(args).map(function(arg) {
    // Strip out _id
    if (!arg || !_.isObject(arg) || !arg._id) { return arg; }

    /* otherwise */
    var result = _.extend({}, arg);
    delete result['_id'];
    return result;

  }).map(function(arg) {

    // Inspect
    return inspectFlat(arg);
  }).value();
};

var mkLogi = lib.logi = function(libName_) {
  var self = this;
  var libName = printf("%20s1", libName_);

  var ret = function() {
    console.log.apply(console, [libName +': '].concat(logFormat(arguments)));
  };

  ret.warn = function() {
    console.warn.apply(console, [libName +': '].concat(logFormat(arguments)));
  };

  ret.error = function() {
    console.error.apply(console, [libName +': '].concat(logFormat(arguments)));
  };

  /**
   *  A handler for Node err first cb params.
   *
   *  Will output an error message iff there was an error, so it is OK to
   *  call without checking the err parameter, first.
   *
   *  Can be used thus:
   *
   *  x.foo(blah, function(err, otherStuff) {
   *    log.err(err);
   *  });
   *
   *  - or -
   *
   *  x.foo(blah, function(err, otherStuff) {
   *    if (log.err(err)) { return callback(err); }
   *  });
   *
   */
  ret.err = function(err) {
    if (!err) { return false; }
    /* otherwise */
    ret.error.apply(this, arguments);
    return err;
  };

  return ret;
};

var mkLog = lib.log = function(libName_) {
  var self = this;
  var libName = printf("%20s1", libName_);

  var ret = function() {
    console.log.apply(console, [libName +': '].concat(Array.prototype.slice.apply(arguments)));
  };

  ret.error = function() {
    console.error.apply(console, [libName +': '].concat(Array.prototype.slice.apply(arguments)));
  };

  return ret;
};

if (theARGV.cluster) {
  var logMessages = [], logFileFd = null;

  var dispatchLogMessages_ = function(force) {
    var needToDispatch = false, message;

    // If the argument is 'true', that means the log file has just opened, and we are flushing
    // the array of messages
    if (force === true) {
      needToDispatch = true;

    } else {

      // The arguments are the objects that make up this message
      message = _.toArray(arguments).join(';');

      if (logMessages === null) {
        needToDispatch = true;
        logMessages = [message];
      } else {
        logMessages.push(message);
      }
    }

    // If there were already messages, someone else is already dispatching the messages,
    // otherwise, we will be the thread doing it
    if (!needToDispatch) {
      return;
    }

    /* otherwise */
    var dispatch = function() {
      var localMessages = logMessages;
      logMessages = [];

      localMessages.push('');
      var buf = new Buffer(localMessages.join('\n'), 'utf8');
      return fs.write(logFileFd, buf, 0, buf.length, null, function(err, written, buffer) {
        // Maybe more messages got added while we were waiting for the write
        if (logMessages.length > 0) {
          return dispatch();
        }

        /* otherwise */
        logMessages = null;
      });
    };
    dispatch();
  };

  (function() {
    return fs.open(path.join(process.env.HOME, 'tmp/mario/debug.log'), 'a', function(err, fd) {
      if (err) {
        console.error("Error opening log file: " + lib.inspect(err));
        return;
      }
      logFileFd = fd;

      if (logMessages.length > 0) {
        dispatchLogMessages = dispatchLogMessages_;
        return dispatchLogMessages(true);
      }
    });
  }());

  // Start with a fn that waits for the log file to open
  var dispatchLogMessages = function() {
    var message = _.toArray(arguments).join(';');
    logMessages.push(message);
  };

  mkLogi = lib.logi = function(libName_) {
    var self = this;
    var libName = printf("%20s1", libName_);

    var ret = function() {
      dispatchLogMessages.apply(this, [libName +': '].concat(logFormatFlat(arguments)));
    };

    ret.warn = function() {
      dispatchLogMessages.apply(this, [libName +': '].concat(logFormatFlat(arguments)));
    };

    ret.error = function() {
      dispatchLogMessages.apply(this, [libName +': '].concat(logFormatFlat(arguments)));
    };

    /**
     *  A handler for Node err first cb params.
     *
     *  Will output an error message iff there was an error, so it is OK to
     *  call without checking the err parameter, first.
     *
     *  Can be used thus:
     *
     *  x.foo(blah, function(err, otherStuff) {
     *    log.err(err);
     *  });
     *
     *  - or -
     *
     *  x.foo(blah, function(err, otherStuff) {
     *    if (log.err(err)) { return callback(err); }
     *  });
     *
     */
    ret.err = function(err) {
      if (!err) { return false; }
      /* otherwise */
      ret.error.apply(this, arguments);
      return err;
    };

    return ret;
  };

  mkLog = lib.log = function(libName_) {
    var self = this;
    var libName = printf("%20s1", libName_);

    var ret = function() {
      dispatchLogMessages.apply(this, [libName +': '].concat(Array.prototype.slice.apply(arguments)));
    };

    ret.error = function() {
      dispatchLogMessages.apply(this, [libName +': '].concat(Array.prototype.slice.apply(arguments)));
    };

    return ret;
  };
}

var logx = mkLog('sg');
var log = mkLogi('sg');

/*var mkDebug =*/ lib.mkDebug = function(logFn_) {
  var logFn = logFn_ || log;

  return function(msg) {
    if (isDebug()) {
      logFn.apply(this, arguments);
    }
  };
};

lib.mkJsonLogger = function(mod) {
  var lg = function(a, b) {
    if (arguments.length === 2 && _.isString(a)) {
      if (a[0] === '!' && /^![a-zA-Z_][a-zA-Z0-9_]*$/.exec(a)) {
        return lg({error:a.substr(1), value:b});
      } else if (a[0] === '%' && /^%[a-zA-Z_][a-zA-Z0-9_]*$/.exec(a)) {
        return lg({warning:a.substr(1), value:b});
      } else if (a[0] === '#' && /^#[a-zA-Z_][a-zA-Z0-9_]*$/.exec(a)) {
        return lg({event:a.substr(1), value:b});
      }
      return lg(kv(a,b));
    }

    var line = printf("%20s1:  ", mod) + _.chain(_.toArray(arguments)).map(function(arg) {
      return util.inspect(arg, {depth:null, colors:true});
    }).value().join(', ').replace(/\n/g, '').replace(/: +/g, ': ').replace(/, +/g, ', ');

    console.log(line);
  };

  return lg;
};

var mkVerbose = lib.mkVerbose = function(logFn_) {
  var logFn = logFn_ || log;

  return function(level /*, msg*/) {
    if (verbosity() >= level) {
      logFn.apply(this, _.rest(arguments));
    }
  };
};

var verbosex = mkVerbose(logx);
var verbose = mkVerbose(log);

var die = lib.die = function(code_, msg_) {
  var args = Array.prototype.slice.apply(arguments);
  var code = _.isNumber(args[0]) ? args.shift() : 1;

  args.unshift(args.shift() || "Fatal Error");
  console.error.apply(this, args);
  process.exit(code);
};

var protector = lib.protector = function(errback_, options_) {
  var errback   = errback_ || function() {};
  var options   = options_ || {};

  return function(callback) {
    return function(err) {
      if (err) { return errback.call(options.errbackBinding || this, err, {args:arguments, context:options.context}); }

      return callback.apply(this, arguments);
    };
  };
};

var haltOnError = lib.haltOnError = protector(function(err, info) {
  console.error("!!!Halting on error!!!" + info.context? inspect(info.context) : '');

  die.apply(this, [9].concat(_.rest(info.args)));
});

/*var or_die =*/ lib.or_die = lib.die_trying = haltOnError;

/*var on_err =*/ lib.on_err = lib.on_error = function(errback, callback) {
  return protector(errback)(callback);
};

var startsWith = lib.startsWith = function(longStr, start) {
  if (!longStr)                      { return false; }
  if (longStr.length < start.length) { return false; }

  if (longStr.substr(0, start.length).toLowerCase() === start.toLowerCase()) {
    return true;
  }

  return false;
};

lib.compactDate = function(time, suffix, prefix) {
  time = time || new Date();
  prefix = prefix || '';
  suffix = suffix || '';

  return prefix +
    time.getFullYear() +
    pad((time.getMonth()+1)    , 2, 0) +
    pad(time.getDate()         , 2, 0)  +
    pad(time.getHours()        , 2, 0)  +
    pad(time.getMinutes()      , 2, 0)  +
    pad(time.getSeconds()      , 2, 0)  +
    pad(time.getMilliseconds() , 4, 0) + suffix;
};

/**
 *  Generate a random string of the given length.
 */
var alnumCharSet = lib.alnumCharSet = 'ABCDEFGHIJKLNMOPQRSTUVWXYZ0123456789abcdefghijklnmopqrstuvwxyz';
/*var randomString =*/ lib.randomString = function(length, charSet) {
  length  = length  || 64;
  charSet = charSet || alnumCharSet;

  var result = '';
  for (var i = 0; i < length; i++) {
    result = result + charSet[Math.floor(Math.random() * charSet.length)];
  }

  return result;
};

var toNormal = lib.toNormal = function(x) {
  if (arguments.length === 1) {
    if (!_.isString(x))     { return x; }
    if (/^[0-9]$/.exec(x))  { return parseInt(x, 10); }
    if (/^[0-9.]$/.exec(x)) { return parseFloat(x); }

    /* otherwise */
    return x;
  }

  var args = _.toArray(arguments);

  return _.map(args, function(arg) {
    return toNormal(arg);
    //result.push(toNormal(arg));
  });
};

/**
 *
 * NamedCapture(/^(([0-9]+)x([0-9]+))$/i, 'mediaSize', 'width', 'height')
 */
/*var NamedCapture =*/ lib.NamedCapture = function(re /*, names...*/) {
  var self = this;
  self.names = _.rest(arguments);

  self.exec = function(str) {
    var m = re.exec(str);
    if (!m) { return; /*undefined*/ }

    _.each(self.names, function(name, index) {
      m[name] = toNormal(m[index+1]);
      if (!m[index+1]) {
//        log.warn('----- WARN: no part to capture ' + index, str, m);
      }
    });

    return m;
  };
};

lib.namedCapture = function(str, re /*, names...*/) {
  var nc = {};
  lib.NamedCapture.apply(nc, _.rest(arguments));
  return nc.exec(str);
};

var RexParser = lib.RexParser = function(/*namedCaptureRes*/) {
  var self = this;
  self.namedCaptureRes = _.rest(arguments, 0);

  self.exec2 = function(x_) {
    var x = x_, result;
    if (_.isArray(x))  { return self.exec_(x); }

    /* otherwise */
    if (_.isObject(x) && self.propName && x.hasOwnProperty(self.propName)) {
      return self.exec2(x[self.propName]);
    }

    /* otherwise */
    if (_.isString(x)) {
      return self.exec_(_.rest(x.split('/')));
    }

    /* otherwise -- we do not really know what this is */
    return self.exec_(x);
  };

  self.exec_ = function(x_) {
    var x = x_;
    if (!_.isArray(x)) {
      x = [x];
    }
    return self._exec(x);
  };

  self.exec = self.exec_;

  self._exec = function(strArray) {
    var result = new RexParserResult(strArray);

    // Loop over the strings, and apply the REs
    var stop = false;
    _.each(strArray, function(str, index) {
      if (stop) { return; }

      var found = false;
      _.each(self.namedCaptureRes, function(cap) {
        if (found) { return; }

        var m = cap.exec(str);
        if (m) {
          result._set(m, cap.names);
          found = true;
        }
      });

      // Maybe there was no match at all on the first item?
      if (index === 0 && result._numMatched === 0) {
        stop = true;
      }

      // The found items must be consecutive
      if (!found) {
        stop = true;
      }
    });

    return result._finish();
  };
};

var RexParserResult = lib.RexParserResult = function(parts) {
  var self = this;

  self._keys = [];
  self._numMatched = 0;
  self._origStr = parts.join('/');
  self._origParts = parts.slice();

  self._setKv = function(key, value) {
    self[key] = value;
    if (self._keys.indexOf(key) === -1) {
      self._keys.unshift(key);
    }
  };

  self._set = function(match, names) {
    _.each(names, function(name) {
      self[name] = match[name];
      self._keys.unshift(name);
    });
    self._numMatched += 1;
  };

  self._finish = function() {
    self._restParts = _.rest(parts, self._numMatched);
    self._rest      = self._restParts.join('/');
    return self;
  };

  self._peek = function(index_) {
    var index = index_ || 0;
    return self._restParts[index] || '';
  };

  self._shift = function() {
    var result = self._restParts.shift();
    self._rest = self._restParts.join('/');
    return result;
  };

  self._unshift = function(/* vals... */) {
    _.chain(_.toArray(arguments)).reverse().each(function(x) {
      self._restParts.unshift(x);
    });

    self._rest = self._restParts.join('/');
    return self;
  };

  self._splice = function(/* index, howMany, a, b */) {
    Array.prototype.splice.apply(self._restParts, arguments);

    self._rest = self._restParts.join('/');
    return self;
  };

  self._put = function(value, index_) {
    var index = index_ || 0;
    self._restParts[index] = value;

    self._rest = self._restParts.join('/');
    return self;
  };

  self._json = function() {
    return lib.reduce(self, {}, function(m, val, name) {
      if (_.isFunction(val)) { return m; }
      return kv(m, name, val);
    });
  };
};

lib.PathnameRexParser = function(/*namedCaptureRes*/) {
  var self = this;

  RexParser.apply(self, arguments);
  self.propName = 'pathname';

  self.exec = function() {
    var opinion = self.exec2.apply(self, arguments);
    if (opinion._numMatched === 0) {
      return false;
    }
    return opinion;
  };
};

/**
 *  Strip all keys that start with underscore
 */
lib.parameters = function(params) {
  return lib.reduce(params, {}, function(m, value, key) {
    if (!startsWith(key, '_')) { m[key] = value; }

    return m;
  });
};

/*var context =*/ lib.context = function(key, def) {
  var keyIndex = -1;
  for (var i = 0; i < process.argv.length; i++) {
    if (process.argv[i] === "--" + key) {
      keyIndex = i;
      break;
    }
  }

  if (keyIndex >= 0 && keyIndex+1 < process.argv.length) {
    return process.argv[keyIndex + 1];
  }

  if (def) {
    return def;
  }
};

/*var captures =*/ lib.captures = function(re, str, callback) {
  var m = re.exec(str);
  if (!m) { return callback(); }

  return callback.apply(this, [m].concat(_.slice(m)));
};

/*var capturesSync  =*/ lib.capturesSync = function(re, str) {
  var m = re.exec(str);
  if (!m) { return ''; }

  return Array.prototype.slice.apply(m);
};

var Promise = lib.Promise = function(handler1) {
  var self = this;

  self.handler = handler1;
  self.then = function(handler2) {
    self.handler = handler2;
  };

  self.go = function(err) {
    return self.handler.apply(this, arguments);
  };
};

var delayStart = lib.delayStart = function(delayedFunction, retVal) {
  process.nextTick(delayedFunction);
  return retVal;
};

lib.reduce = function(collection, initial, fn) {
  return _.reduce(collection, fn, initial);
};

lib._extract = function(collection, name) {
  var value = collection[name];
  delete collection[name];
  return value;
};

var __each = lib.__each = function(a, b, c) {
  // If the caller has a named function, like 'next', it is easier to call this
  // function with that name first
  if (_.isFunction(a)) { return __each_(b, c, a); }

  // Normal
  return __each_(a, b, c);
};

var __each_ = function(coll, fn, callback) {


  if (_.isArray(coll) && coll.length <= 0) {
    return callback();
  }

  var i = 0, end;
  var indexes, values, errors, hasError = false;

  var continuation = new Promise(callback);

  if (_.isArray(coll)) {
    indexes = _.range(coll.length);
    values = [];
    errors = [];
  }
  else {
    indexes = _.keys(coll);
    values = {};
    errors = {};
  }

  if ((end = indexes.length) === 0) {
    return continuation.go(hasError ? errors : null, values);
  }

  var doOne = function() {
    var item = coll[indexes[i]];
    var nextI = i + 1;
    var next = function(err, val) {
      if (err) { hasError = true; }

      errors[i] = err;
      values[i] = val;

      i = nextI;
      if (i < end) {
        return process.nextTick(function() {
          doOne();
        });
      }

      return continuation.go(hasError ? errors : null, values);
    };

    return fn(item, next, indexes[i], coll);
  };

  return delayStart(doOne, /* return: */continuation);
};

var __eachll = /*lib.__eachll =*/ function(coll, fn, callback) {
  var finalFn = _.after(coll.length, function() {
    callback();
  });

  for (var i = 0, l = coll.length; i < l; i++) {
    fn(coll[i], finalFn, i);
  }
};

/*var __eachll2 =*/ lib.__eachll = function(list_ /*, max_, fn_, callback_*/ ) {

  var args = _.rest(arguments);
  var callback = args.pop();
  var fn = args.pop();
  var max = args.length > 0 ? args.shift() : 10000000;

  if (_.isArray(list_)) {
    var list = list_.slice();

    if (list.length === 0) { return callback(); }

    var outstanding = 0;
    var launch = function(incr) {
      outstanding += (incr || 0);

      if (list.length > 0 && outstanding < max) {
        outstanding++;
        fn(list.shift(), function() {
          process.nextTick(function() {
            launch(-1);
          });
        }, list.length, list_);
        //process.nextTick(launch);
        launch();
      }
      else if (list.length === 0 && outstanding === 0) {
        callback();
      }
    };
    launch(1);
    outstanding -= 1;
    return;
  }

  /* otherwise */
  return lib.__eachll(_.keys(list_), max, function(key, nextKey) {
    fn(list_[key], nextKey, key, list_);
  }, callback);
};

var __run = lib.__run = function(a, b) {
  var fns, callback;

  if (_.isArray(a)) {
    fns = a; callback = b;
  } else {
    fns = b; callback = a;
  }

  return __each(
    fns,
    function(fn, next, index, coll) {
      return fn(next, index, coll);
    },
    callback || function() {}
  );
};

// TODO: replace __run with this, when it is OK
var __run2 = lib.__run2 = function(a, b) {
  var fns, callback;

  if (_.isArray(a)) {
    fns = a; callback = b;
  } else {
    fns = b; callback = a;
  }

  var results = [null];
  return __each(
    fns,
    function(fn, next) {

      var previousResult = results[0];
      var args = [function() {
        results.unshift(Array.prototype.slice.apply(arguments));
        return next();
        //return next.apply(this, arguments);
      }];

      args = args.concat(previousResult);

      return fn.apply(this, args);

    }, function() {
      // results is an array of arrays.  If the first item (the final passed-along arg)
      // is an array of one item, unwrap it.
      var finalResults = results.slice();
      if (finalResults[0].length === 1) {
        finalResults = [results[0][0]].concat(_.rest(results));
      }
      (callback || noop).apply(this, finalResults);
    }
  );
};

var findFiles = lib.findFiles = function(pattern, options, callback) {
  return glob(pattern, options, function(err, filenames_) {
    if (err) { return callback(err); }

    var filenames = [];
    return __eachll(filenames_,
      function(filename, next) {
        return fs.stat(filename, function(err, stats) {
          if (!err && stats.isFile()) {
            filenames.push(filename);
          }
          return next();
        });
      },
      function(errs) {
        return callback(null, filenames);
      }
    );
  });
};

lib.eachLine = function(a /*pattern*/, b /*options_*/, c /*eachCallback*/, d /*finalCallback*/) {

  // if b is a string, it is dir
  if (_.isString(b)) {
    b = {cwd : b};

  // Three args means a is a path with glob at the end
  } else if (arguments.length === 3 && _.isString(a)) {
    d = c; c = b;
    b = {cwd : path.dirname(a)};
    a = path.basename(a);
  }

  var pattern       = a;
  var options_      = b;
  var eachCallback  = c;
  var finalCallback = d;

  var options = _.defaults({}, options_ || {}, {cwd: process.cwd()}),
      total = 0;

  var eachLineOfOneFile = function(filename, next) {
    return fs.readFile(path.join(options.cwd, filename), 'utf8', function(err, contents) {
      if (err) { return next(err); }

      var lines = contents.split('\n');
      if (options.lineFilter) {
        lines = lines.filter(options.lineFilter);
      }

      var i = 0, l = lines.length;
      var oneLine = function() {
        total++;
        //var result = eachCallback(lines[i], i, path.join(options_.cwd, filename), total);
        var result = eachCallback(lines[i], i, filename, total);
        if (result === 'SG.nextFile') {
          return next();
        }

        i += 1;

        if (i < l) {
          if (result === 'SG.breathe') {
            //console.log('Breathing');
            return setTimeout(oneLine, 500);
          }

          if (i % 200 === 0) {
            return process.nextTick(oneLine);
          }

          return oneLine();
        }

        return next();
      };

      return oneLine();
    });
  };

  // Is this a glob?
  if (!/\*/.exec(pattern)) {
    // No, not a glob
    return eachLineOfOneFile(arguments[0], function(err) {
      return finalCallback(err, total);
    });
  }

  /* otherwise */
  options.filenameFilter = options.filenameFilter || function(){return true;};

  return glob(pattern, options, function(err, files) {
    if (err) { return finalCallback(err); }

    verbose(4, files);

    return __each(files,
      function(filename_, next) {
        var filename = path.join(options.cwd || '', filename_);

        return fs.stat(filename, function(err, stats) {
          if (err) { return next(); }
          if (!stats.isFile()) { return next(); }

          if (!options.filenameFilter(filename)) { return next(); }

          return eachLineOfOneFile(filename_, next);
        });
      },

      function() {
        return finalCallback(null, total);
      }
    );
  });
};

/*var parseOn2Chars =*/ lib.parseOn2Chars = function(str, sep1, sep2) {
  var ret = {};
  // use the following code if using 1284DeviceID
  // _.each(str.split(sep1).filter(_.identity), function(kv) {
  //   var arr = kv.split(sep2), k = arr[0], v = arr[1] || '';
  //   ret[k.toLowerCase()] = v.toLowerCase();
  // });
  // use the following code if using ModelName instead of 1284DeviceId
  var strng = str.toLowerCase();
  var firstWord = strng.substr(0,strng.indexOf(' '));
  var restWord = strng.substr(strng.indexOf(' ')+1);
  if(firstWord === "hp" || firstWord === "brother" || firstWord === "epson" || firstWord === "canon") {
    ret["mfg"] = firstWord;
    ret["mdl"] = restWord;
    ret["des"] = restWord;
  } else {
    ret["mfg"] = "";
    ret["mdl"] = strng;
    ret["des"] = strng;
  }

  return ret;
};

/*var writeChunks =*/ lib.writeChunks = function(path, chunks, callback_) {
  var callback = callback_ || function() {};

  if (!chunks) {
    return callback();
  }

  return fs.open(path, "w", function(err, fd) {
    if (err) { return callback.apply(this, arguments); }

    var doneFn;

    __run([
      function(next) {

        var i = -1;
        var writeNextChunk = function() {
          i += 1;
          if (i >= chunks.length) { return next(); }

          /* otherwise */
          return fs.write(fd, chunks[i], 0, chunks[i].length, null, function(err /*, written, buffer*/ ) {
            if (err) { return doneFn.apply(this, arguments); }

            return setImmediate(writeNextChunk);
          });
        };

        // Kick it off
        writeNextChunk();
      }],

      // finally
      doneFn = function() {
        var args = arguments;
        return fs.close(fd, function(err) {
          return callback.apply(this, args.length > 0 ? args : arguments);
        });
      }
    );
  });
}

var exportify = lib.exportify = function(obj, exports_) {
  var theExports = exports_ || exports;

  for (var key in obj) {
    theExports[key] = obj[key];
  }
};

var lineNum = 0;
var remainder = '';
var ARGF = lib.ARGF = function(callback, fnDone_) {
  var fnDone = fnDone_ || function() {};

  var doOneLine = function(line) {
    lineNum++;
    callback(line, lineNum);
  };

  process.stdin.setEncoding('utf8');

  process.stdin.on('data', function(chunk) {
    remainder += chunk;
    var lines = remainder.split('\n');
    remainder = lines.pop();

    _.each(lines, doOneLine);
  });

  process.stdin.on('end', function() {
    var lines = remainder.split('\n');
    _.each(lines, doOneLine);
    fnDone();
  });
};

/*var awk =*/ lib.awk = function(callback, fnDone_) {
  return ARGF(function(line, lineNum) {
    return callback(line.split(' '), lineNum);
  }, fnDone_);
};


var redisEncode = lib.redisEncode = function(list) {

  if (_.isString(arguments[0])) { return redisEncode(arguments[0].split(/\s+/)); }

  var ret = '*' + list.length + '\r\n';
  _.each(list, function(item_) {
    var item = item_.toString();
    ret += '$' + item.length + '\r\n';
    ret += item + '\r\n';
  });

  return ret;
};

var RedisDecoder = lib.RedisDecoder = function(callback) {
  var self = this;

  var remainder = '';
  self.write = function(chunk) {
    //var items = (remainder + chunk).split(/(\r\n|^)\*/);
    var items = (remainder + chunk).split(/\r\n/);
    remainder = items.pop();
    return dispatchItems(items);
  };

  self.end = function() {
    return dispatchItems(remainder.split(/(\r\n|^)\*/));
  };

  // Parsing Redis bulk-reply string.  The only tricky thing is that bulk strings
  // are two lines: $6\r\nfoobar\r\n where everything else is one.  Except for the NULL
  // string, which is one line: $-1\r\n  Actually, arrays are not handled
  var dispatchItems = function(items_) {

    var items = [];
    for (var i = 0; i < items_.length; ++i) {
      if (items_[i][0] !== '$') {
        items.push(items_[i]);
        continue;
      }
      if (items_[i].length === 3 && items_[i][1] === '-' && items_[i][2] === '1') {
        items.push(items_[i]);
        continue;
      }
      if (i + 1 < items_.length) {
        items.push([items_[i], items_[i+1]].join('\r\n'));
        i++;
        continue;
      }

      remainder += items_[i];
    }

    _.each(items, function(item) {
      var lines = item.split('\r\n'), m, len = null, ret = [];
      for (var i = 0; i+1 < lines.length; i += 2) {
        // The first item should be $len
        m = /^\$([0-9])+$/.exec(lines[i]);
        if (m) {
          len = parseInt(m[1], 10);
        }
        else {
          console.error('Parse error from redisDecoder');
        }
        ret.push(lines[i+1]);
      }

      if (theProfiler) {
        theProfiler.count('redis-parse');
      }
      callback(ret.join(' '));
    });
  };
};

var Redis = lib.Redis = function(options_) {
  var self = this;
  //var options = _.extend((options_ || {}), {port: 6379});
  var options = _.extend({port: 6379}, (options_ || {}));

  self.events = new EventEmitter();

  console.log('connecting to ' + options.port, util.inspect(options_));
  var server = require('net').connect({port: options.port}, function(){ });
  var emitter = new EventEmitter();

  var ARGV = lib.ARGV();

  // Load scripts
  var loadedScripts = {}, loadedLibScript = '';
  if (process.env.jmt_dir) {
    __run([
      function(next) {
        findFiles(path.dirname(__filename) + '/../../sh/_redis/_lua/_lib/*.lua', {}, function(err, filenames) {
          if (err) { return; }

          __each(next, filenames, function(filename, nextFilename) {
            var reportErr = function(err) {
              if (err) { console.error('Error while loading ' + filename, err); }
              return nextFilename(err);
            };

            return fs.readFile(filename, function(err, contents) {
              if (err) { return reportErr(err); }

              loadedLibScript += '\n' + contents;
              return nextFilename();
            });
          });
        });
      },
      function(next) {
        findFiles(path.dirname(__filename) + '/../../sh/_redis/_lua/*.lua', {}, function(err, filenames) {
          if (err) { return; }

          __each(next, filenames, function(filename, nextFilename) {
            var reportErr = function(err) {
              if (err) { console.error('Error while loading ' + filename, err); }
              process.exit(2);
              return nextFilename(err);
            };

            return fs.readFile(filename, function(err, contents) {
              if (err) { return reportErr(err); }

              var cmd = ["redis-cli"];
              if (options.port) { cmd.push('-p', options.port); }
              cmd.push('SCRIPT', 'LOAD');
              cmd.push('"' + loadedLibScript + '\n' + contents + '"');
              //if (ARGV.verbose) { console.error('Loading script: ', cmd, filename); }
              return exec(cmd.join(' '), function(err, stdout, stderr) {
                //if (ARGV.verbose) { console.error('Loaded: ' + filename, stdout); }
                //if (ARGV.verbose) { console.error('Stderr: ', stderr); }
                if (err) { return reportErr(err); }
                if (stdout.match(/ /)) { return reportErr(stdout); }

                loadedScripts[path.basename(filename, '.lua')] = stdout.split("\n")[0];
                return nextFilename();
              });
            });
          });
        });
      }],

      function done() {
        // All scripts loaded
        //if (ARGV.verbose) { console.error('loadedScripts: ', loadedScripts); }
        self.events.emit('loaded');
      }
    );
  }

  self.SCRIPT_LOAD = function(filename /*, prefix_, suffix_, callback*/) {
    var args = _.rest(arguments);
    var callback = args.pop();
    var prefix = args.shift();
    var suffix = args.shift();

    var reportErr = function(err) {
      if (err) { console.error('Error while loading ' + filename, err); }
      return callback(err);
    };

    var cmd = ["redis-cli"];
    if (options.port) { cmd.push('-p', options.port); }
    cmd.push('SCRIPT', 'LOAD');
    //if (ARGV.verbose) { console.error('Loading script: ', cmd, filename); }

    return fs.readFile(filename, function(err, contents) {
      if (err) { return callback(err); }

      cmd.push('"' + (prefix || '') + '\n' + contents + (suffix || '') + '"');

      return exec(cmd.join(' '), function(err, stdout, stderr) {
        //if (ARGV.verbose) { console.error('Loaded: ' + filename, stdout); }
        //if (ARGV.verbose) { console.error('Stderr: ', stderr); }

        if (err) { return callback(err); }
        if (stdout.match(/ /)) { return reportErr(stdout); }

        loadedScripts[path.basename(filename, '.lua')] = stdout.split("\n")[0];
        return callback(null, path.basename(filename, '.lua'));
      });
    });
  };

  shutdownHandlers.push(function() {
    server.end();
  });

  self.on = function() {
    return emitter.on.apply(emitter, arguments);
  };

  var decoder = new RedisDecoder(function(item) {
    if (item[0] === '-') {
      console.error(item);
    }
    //console.log('decoder', item);
    emitter.emit('data', item);
  });

  server.setEncoding('utf8');
  server.on('data', function(chunk) {
    //console.log(chunk);
    //decoder.write(chunk);
  });

  self.dbNum = 0;
  self.SELECT = function(dbNum) {
    self.dbNum = dbNum;
    return self.write('SELECT', dbNum);
  };

  self.RUN_SCRIPT = function(script) {
    var tries = 0;
    if (!(script in loadedScripts) && tries === 0) {
      //if (ARGV.verbose) { console.error('DelayRunning: ' + script + ' ' + loadedScripts[script]); }
      return setTimeout(function(){ self.RUN_SCRIPT(script); }, 1000);
    }

    if (!(script in loadedScripts)) {
      console.error('-----------------ERROR ---------- Calling unknown script: ' + script);
    }
    //if (ARGV.verbose) { console.error('Running: ' + script + ' ' + loadedScripts[script]); }
    //if (ARGV.verbose) { console.error(arguments); }
    return self._varargs('EVALSHA', loadedScripts[script], arguments);
  };

  //var profileName, profileStart;
  //self.startProfile = function(name) {
  //  profileName = name;
  //  profileStart = new Date().getTime();
  //  self.DEL('sg_redis_profile:end', profileName);
  //  self.SET('sg_redis_profile:name', profileName);
  //  self.SET('sg_redis_profile:start', profileStart);
  //};

  //self.endProfile = function() {
  //  var cmd = ["redis-cli"];
  //  if (options.port) { cmd.push('-p', options.port); }

  //  // Must wait until the end key is removed
  //  var waitForEndRemoved = function() {
  //    return exec(cmd.concat(['EXISTS', 'sg_redis_profile:end']).join(' '), function(err, stdout, stderr) {
  //      console.log(err, stdout);
  //      console.error(err, stderr);

  //      var end = new Date().getTime();
  //    });
  //  };
  //  return watiForEndRemoved();
  //  //self.SET('sg_redis_profile:end', end);

  //};

  self.SADD_VALUE = function(key, key_of_value) {
    self.RUN_SCRIPT('sadd_value', 2, key_of_value, key);
  };

  self.SADD_VALUE4 = function(prefix, key_of_obj, type, value) {
    self.RUN_SCRIPT('sadd_value4', 0, prefix, key_of_obj, type, value);
  };

  self.SET_VALUE = function(prefix, key_of_obj, type, value) {
    self.RUN_SCRIPT('set_value', 0, prefix, key_of_obj, type, value);
  };

  self.SET_VALUE_ID = function(prefix, key_of_obj, type, value_key) {
    self.RUN_SCRIPT('set_value_id', 2, key_of_obj, value_key, prefix, type);
  };

  self.SET = function(key, value) {
    return self.write('SET', self._fixkey(key), value);
  };

  self.DEL = function(key) {
    return self.write('DEL', self._fixkey(key));
  };

  self.SADD = function(key) {
    return self._varargs('SADD', self._fixkey(key), arguments);
  };

  self.SREM = function() {
    return self._varargs('SREM', arguments);
  };

  self.SMEMBERS = function() {
    return self._varargs('SMEMBERS', arguments);
  };

  self.SDIFF = function() {
    return self._varargs('SDIFF', arguments);
  };

  self.SDIFFSTORE = function(destination) {
    return self._varargs('SDIFFSTORE', self._fixkey(destination), arguments);
  };

  self.SINTER = function() {
    return self._varargs('SINTER', arguments);
  };

  self.SINTERSTORE = function(destination) {
    return self._varargs('SINTERSTORE', self._fixkey(destination), arguments);
  };

  self.SUNION = function() {
    return self._varargs('SUNION', arguments);
  };

  self.SUNIONSTORE = function(destination) {
    return self._varargs('SUNIONSTORE', self._fixkey(destination), arguments);
  };

  self.ZINCRBY = function() {
    return self._varargs('ZINCRBY', arguments);
  };

  self.KEYS = function(keys, callback) {
    return exec(['redis-cli', '-p', options.port, '-n', self.dbNum, 'KEYS', keys].join(' '), function(err, stdout, stderr) {
      if (err) { return callback(err); }

      return callback(null, _.compact(stdout.split('\n')));
    });
  };

  self.GET = function(key, callback) {
    return exec(['redis-cli', '-p', options.port, '-n', self.dbNum, 'GET', key].join(' '), function(err, stdout, stderr) {
      if (err) { return callback(err); }

      return callback(null, _.compact(stdout.split('\n')));
    });
  };

  self._fixkey = function(keyArray) {
    if (!_.isArray(keyArray)) { return keyArray; }

    var params = _.map(_.rest(keyArray), function(str) {
      return str.toString().replace(/:/g, '~');
    });

    return printf.apply(this, [keyArray[0]].concat(params));
  };

  self._varargs = function() {
    var args = slice.apply(arguments);
    var wargs = [];

    while(args.length > 0) {
      if (!_.isArguments(args[0])) {
        wargs.push(args.shift());
      }
      else {
        var numConsumed = wargs.length;
        wargs = wargs.concat(_.rest(args[0], numConsumed - 1));
        break;
      }
    }

    return self.write.apply(self, wargs);
  };

  self.write = function(a) {
    //if (theProfiler && arguments[0] !== 'ZINCRBY') {
    //  theProfiler.count('redis' + arguments[0]);
    //}
    return server.write(redisEncode(arguments));
  };

};

/*var RedisNoop =*/ lib.RedisNoop = function(example) {
  var self = this;

  _.each(example, function(item, name) {
    if (_.isFunction(item) && /^[A-Z]+$/.exec(name)) {
      self[name] = function() {};
    }
  });
};

/*var simpleRun =*/ lib.simpleRun = function(command, options_ /*, callback*/) {
  var options = options_ || {};
  var callback = _.last(arguments);

  return exec(command, function(err, stdout, stderr) {
    var isErr = err;

    if (stderr && stderr.length > 0 && !options.stderrIsNotAnError) {
      isErr = err || true;
    }

    if (isErr) {
      var errString = _.isString(isErr) ? isErr : JSON.stringify(isErr);
      if (stderr && stderr.length > 0 && !options.stderrIsNotAnError) {
        errString += '\n stderr: ' + stderr;
      }
      console.error('Error: ' + errString);
      console.error('    Command: ' + command);
      console.error("\n    One of the commands may not be available. Try 'which xyz' for each command to see if it is available.");
      console.error("    You can probably install with brew install xyz (or sudo apt-get install xyz)");

      if (!options.ignoreErr) {
        return process.exit(options.errExitCode || 2);
      }
    }

    /* otherwise */
    return callback.call(this, isErr, stdout, stderr);
  });
};

var isExecErrObject = lib.isExecErrObject = function(respCode) {
  return (respCode.hasOwnProperty("killed") && respCode.hasOwnProperty("signal") && respCode.hasOwnProperty("code"));
};

var theProfiler;
/*var Profiler =*/ lib.Profiler = function(options_) {
  var self = this;
  var options = _.extend({pname:'sg_profile'}, options_);
  var redis = new Redis(options);

  self.count = function(name, filename_) {
    return redis.ZINCRBY(options.pname, 1, name+ (filename_ || ''));
  };

  theProfiler = theProfiler || self;
};


/*var shutdown =*/ lib.shutdown = function() {
  _.each(shutdownHandlers, function(h) {
    h();
  });
};

process.on('exit', function() {
});


var log2_ = Math.log(2);
/*var log2Floor =*/ lib.log2Floor = function(x) {
  return Math.floor(Math.log(x)/log2_);
};

//---------------------------------------- File and path ----------------------------------
lib.path = {};
lib.path.join = function() {
  return path.normalize(path.join.apply(this, arguments));
};

//---------------------------------------- HTTP ----------------------------------

lib.url = {};

var parseUrl = lib.url.parse = function(req, parseQuery) {
  return req && req.url && urlLib.parse(req.url, parseQuery);
};

/*var getBody =*/ lib.getBody = function(req /*, options, callback */) {

  var args     = _.rest(arguments);
  var callback = args.pop() || noop;
  var options  = args.pop() || {};

  options.verbose = options.verbose || 5;
  verbose(options.verbose, "sg.getBody", options);

  var onEnd = function() {
    var cbArgs = [null];

    if (options.json) {
      req.bodyJson = req.bodyJson || safeJSONParse(req.chunks.join(''), {});
      cbArgs.push(req.bodyJson);
    }
    verbose(options.verbose, 'sg.getBody-cbArgs', cbArgs);
    return callback.apply(this, cbArgs);
  };

  // Only collect the data once
  if (req.chunks) {
//    log.warn("Duplicate getBody calls! for ", req.url, req.chunks.lenght, req.bodyJson);
    req.on('end', onEnd);
    return;
  }

  /* otherwise */
  req.chunks = [];
  req.bodyLength = 0;
  req.on('data', function(chunk) {
    verbose(options.verbose, 'sg.getBody-onData length', chunk.length);
    verbose(options.verbose+1, 'sg.getBody-onData', chunk.toString());

    req.chunks.push(chunk);
    req.bodyLength += chunk.length;
  });

  req.on('end', onEnd);
};

lib.getParams = function(req, res, params_, options, callback) {
  return lib.getBody(req, {json:true}, function(err, body) {
    if (err) { return callback(err); }

    /* otherwise */
    var params = lib.smartAttrs(_.extend({}, lib.parameters(params_), body, lib.url.parse(req, true).query));
    return callback(null, params);
  });
};

// ----- Parsing -----
/*var parseUrl =*/ lib.parseUrl = function(req, wantQuery) {
  var url = lib.url.parse(req, wantQuery);
  if (!url)       { return url; }
  if (!wantQuery) { return url; }

  // Maybe they want the query, but haven't gone through the effort to capture the body?
  if (!req.chunks) { return url; }

  /* otherwise */
  if (!req.bodyJson) {
    req.bodyStr = req.bodyStr || req.chunks.join('');
    try {
      req.bodyJson = JSON.parse(req.bodyStr || "{}");
    } catch(e) {
      // TODO: parse the form-encoding
    }
  }

  url.query = _.extend(url.query, req.bodyJson);
  return url;
};

/*var httpHostname =*/ lib.httpHostname = function(url, server, port) {
  var parts = [];
  if (url.auth) {
    parts.push(url.auth);
  }
  parts.push([server, port].join(':'));
  return parts.join('@');
};

var parseCookies = lib.parseCookies = function(request) {
    var list = {},
        rc = request.headers.cookie;

    rc && rc.split(';').forEach(function( cookie ) {
        var parts = cookie.split('=');
        list[parts.shift().trim()] = decodeURI(parts.join('='));
    });

    return list;
};

/**
 *  Get sub-parts of the pathname.  Generally to get the next (few)
 *  route parts.
 *
 *  For: url.pathname = '/a/b/c/d/e/f/g'
 *
 *  httpRoute(url)           -> 'a'
 *  httpRoute(url, 1)        -> 'b'
 *  httpRoute(url, 1, 2)     -> 'b/c'
 *  httpRoute(url, 1, 3)     -> 'b/d'
 *  httpRoute(url, 1, null)  -> 'b/c/d/e/f/g'
 *
 */
var httpRoute = lib.httpRoute = function(url) {
  if (_.isObject(url) && !url.hasOwnProperty('pathname')) {
    var req = url;
    return httpRoute.apply(this, [parseUrl(req)].concat(_.rest(arguments)));
  }

  var parts   = _.rest(url.pathname.split('/'));
  var fields  = _.flatten(_.rest(arguments));

  if (fields.length < 1) {
    fields.push(0);
  } else if (fields.length === 2 && fields[1] === null) {
    fields = _.range(fields[0], parts.length+1);
  }

  return _.chain(fields).
    map(function(fNum) { return parts[fNum]; }).
    compact().
    value().join('/');
};

/**
 *  Does the route match?
 */
lib.httpRouteMatches = function(a /*, [fields], route*/) {
  var args   = _.rest(arguments, 0);
  var route  = args.pop();

  if (route.toLowerCase() !== httpRoute.apply(this, args).toLowerCase()) {
    return false;
  }

  if (_.isObject(a) && a.hasOwnProperty('url')) {
    return parseUrl(a, true);
  }

  return true;
};


var stringForHttpCode = function(code) {
  // First, specific codes
  if (code === 301)                       { return 'Moved Permanently'; }
  else if (code === 302)                  { return 'Found'; }
  else if (code === 304)                  { return 'Not Modified'; }
  else if (code === 400)                  { return 'Bad Request'; }
  else if (code === 401)                  { return 'Unauthorized'; }
  else if (code === 403)                  { return 'Permission Denied'; }
  else if (code === 404)                  { return 'Not Found'; }
  else if (code === 418)                  { return "I'm a teapot"; }
  else if (code === 429)                  { return 'Too Many Requests'; }
  else if (code === 500)                  { return 'Internal Error'; }
  else if (code === 501)                  { return 'Not Implemented'; }
  else if (code === 521)                  { return 'Web server is down'; }

  // Ranges
  else if (200 <= code && code < 300)     { return 'OK'; }
  else if (300 <= code && code < 400)     { return 'Follow Location'; }
  else if (400 <= code && code < 500)     { return 'Client Error'; }
  else if (code < 600)                    { return 'Server Error'; }
  return '';
};

var isErrorCode = lib.isErrorCode = function(code) {
  if (code >= 400) { return true; }
  return false;
};

var isClientError = lib.isClientError = function(code) {
  if (code >= 400 && code < 500) { return true; }
  return false;
};

var isServerError = lib.isServerError = function(code) {
  if (code >= 500) { return true; }
  return false;
};

var isLogicError = lib.isLogicError = function(code) {
  return isServerError.apply(this, arguments);
};

/**
 *  Responds and helps out somewhat, but does not have the request
 *  object, so it can only do so much.
 */
var respond = lib.respond = function(res, code, content_, headers_) {
  var headers = headers_ || {};
  var content = content_ || {code:code, text: ('' + code + ' - ' + stringForHttpCode(code))};

  if (!headers['Content-Type']) {
    if (_.isString(content)) {
      headers['Content-Type'] = 'text/plain';
    } else {
      headers['Content-Type'] = 'application/json';
    }

    if (_.isObject(content)) {
      content = JSON.stringify(content);
    }
  }

  if (_.isArray(content)) {
    // If it is an array of buffers, do not set content-length, so the server will stream
    if (_.isString(content[0])) {
      headers['Content-Length'] = _.reduce(content, function(m, x) {
        return (m + x.length);
      }, 0);
    }
  } else if (_.isString(content)) {
    headers['Content-Length'] = content.length;
  }

  res.writeHead(code, headers);

  if (_.isArray(content)) {
    _.each(content, function(chunk) {
      res.write(chunk);
    });
  } else {
    verbosex(4, 'sg-respond', lib.inspect(code), lib.inspect(headers).replace(/\n/g, ''), lib.inspect(content).replace(/\n/g, ''));
    res.write(content);
  }

  return res.end();
};

var _200 = lib._200 = function(req, res, content_, headers_) {
  return respond(res, 200, content_, headers_);
};

// Moved 'permanently'
lib._301 = function(req, res, location, headers_) {
  var headers = _.extend({}, headers_ || {}, {Location: location});
  return respond(res, 301, "Moved to " + location, headers);
};

// Found (moved temporarily)
lib._302 = function(req, res, location, headers_) {
  var headers = _.extend({}, headers_ || {}, {Location: location});
  return respond(res, 302, "Found at " + location, headers);
};

/**
 *  304 - Not Modified
 */
lib._304 = function(req, res) {
  return respond(res, 304);
};

/**
 *  400 - Bad Request
 *
 *  The request is only partially correct - like the start of a route
 *  being OK, but params are not right for it.
 */
var _400 = lib._400 = function(req, res, a, b) {
  var content, goodPart = a, badPart = b;
  if (arguments.length === 3) {
    content = a;
  } else if (arguments.length === 4 && a.good && _.isString(b)) {
    content = _.extend({}, a, {bad:b});
  } else if (arguments.length === 4 && a.good) {
    content = _.extend({bad:{}}, a);
    content.bad = _.extend(content.bad, b);
  } else {
    content = {
      good  : goodPart  || 'unspecified',
      bad   : badPart   || 'unspecified'
    };
  }
  if (content) { return debugRespond(req, res, 400, content); }
  return respond(res, 400);
};
lib._400.hasDifferentSignature = true;

lib.mk400 = function(handlerName) {

  var self = function(req, res, b) {
    self._fixupForFinal.apply(self, arguments);
    return _400(req, res, self.payload);
  };

  self.is400 = true;
  self.payload = {handler:handlerName || '', good:{}, bad:{}};

  var genericRespPayload = {error: 'EUPSTREAM', msg:'Upstream server or service is unavailable'};

  self.do521 = function(req, res, foreignRespCode, other) {
    if (isExecErrObject(foreignRespCode)) {
      genericRespPayload.msg = "Service ran and indicated an error.";
    }

    self._fixupForFinal.apply(self, arguments);
    var foreignUrl = mvKv(self.payload.bad, self.payload.good, 'foreignUrl');
    if (isDebug()) {
      return _521(req, res, foreignUrl, foreignRespCode, _.extend({}, genericRespPayload, {_400_inProgress: self.payload, err_or_other: other}));
    }

    /* otherwise */
    return respond(res, 521, genericRespPayload);
  };

  self._fixupForFinal = function(req, res, b) {
    if (_.isString(b)) { return self._fixupForFinal(req, res, kv('msg', b)); }

    /* otherwise */
    if (b) {
      self.bad(b);
    }

    self.payload.reqUrl = req.url;

    return self;
  };

  self.good = function(a, b) {
    if (arguments.length === 2 && _.isString(a) && _.isString(b)) { return self.good(kv(a,b)); }

    /* otherwise */
    _.each(a, function(value, key) {
      self.payload.good[key] = value;
    });

    return self;
  };

  self.bad = function(a, b) {
    if (arguments.length === 2 && _.isString(a) && _.isString(b)) { return self.bad(kv(a,b)); }

    /* otherwise */
    _.each(a, function(value, key) {
      self.payload.bad[key] = value;
    });

    return self;
  };

  self.foreignUrl = function(url) {
    if (self.foreignUrl) {
      self.foreignUrls = self.foreignUrls || [];
      self.foreignUrls.push(self.foreignUrl);
    }

    return self.good({foreignUrl: url});
  };

  return self;
};

/**
 *  401 - Unauthorized
 *
 *  Authenticating will probably fix this.
 */
lib._401 = function(req, res, content, headers_) {
  if (content) { return debugRespond(req, res, 403, content); }
  return respond(res, 401, content, headers_);
};

/**
 *  403 - Forbidden
 *
 *  Authenticating will not allow access.
 */
lib._403 = function(req, res, content, headers_) {
  if (content) { return debugRespond(req, res, 403, content); }
  return respond(res, 403, content, headers_);
};

lib._404 = function(req, res, content_, headers_) {
  var content = content_;
  if (content_) {
//    if (!isDebug()) {
//      content = _.pick(content, 'error', 'err', 'error_message', 'errorMessage', 'errMessage', 'errMsg');
//    }
//    console.log('sending: ', content_, content);
    return sendSmartResponse(req, res, 404, content);
  }
  return respond(res, 404, content_, headers_);
};

lib._498 = function(req, res, content_, headers_) {
  var content = content_;
  if (content_) {
//    if (!isDebug()) {
//      content = _.pick(content, 'error', 'err', 'error_message', 'errorMessage', 'errMessage', 'errMsg');
//    }
//    console.log('sending: ', content_, content);
    return sendSmartResponse(req, res, 498, content);
  }
  return respond(res, 498, content_, headers_);
};

/**
 *  I'm a teapot :)
 *
 *  Yes, this is a valid response.  Here we use it where we would normally use 404 -
 *  when a handler does not handle the route asked for, but it's not like the whole
 *  system can't find the resource.  Mostly, this is to recognize that a sub-handler
 *  is 404'ing, but not in an error-ish way.
 *
 *  The server is a teapot, it does not understand the request, but not because there
 *  was an error.  If you ask for a route it does understand, it will work.
 */
lib._418 = function(req, res, content, headers_) {
  return respond(res, 418, content, headers_);
};

lib._500 = function(req, res, content, headers_) {
  if (content) { return debugRespond(req, res, 500, content); }
  return respond(res, 500, content, headers_);
};

/**
 *  501 - Not Implemented
 */
lib._501 = function(req, res, content, headers_) {
  if (content) { return debugRespond(req, res, 501, content); }
  return respond(res, 501, content, headers_);
};

/**
 *  521 - Web server is down
 *
 *  If a request needs assistance from another server, and that other server
 *  has a problem, use this code.
 */
var _521 = lib._521 = function(req, res, url, respCode_, content_, headers_) {
  var respCode = respCode_;

  // if respCode is the "err" of an "exec" call, fix it
  if (isExecErrObject(respCode)) {
    respCode = respCode.code;
  }

  var content = _.extend({}, content_, {foreignUrl: url || null, foreignRespCode: respCode || 404});
  if (content) { return debugRespond(req, res, 521, content); }
  return respond(res, 521, content, headers_);
};
lib._521.hasDifferentSignature = true;

_.each(_.range(200, 1000), function(i) {
  var fn = lib['_'+i];

  if (_.isFunction(fn) && !fn.hasDifferentSignature) {

    lib['__'+i] = function(req, res, contentType, contentChunks) {
      if (contentType) {
        return fn(req, res, contentChunks.join(''), {'content-type': contentType});
      }
      return fn(req, res, contentChunks.join(''));
    };
  }
});

/**
 *  Smart send response.
 *
 *  Send your content and HTTP status code, and this function will figure
 *  out what to send.
 *
 *  * An Object gets sent as JSON
 *  * An array of strings gets join'ed
 */
var sendSmartResponse = lib.sendSmartResponse = function(req, res, code, content_, type_, headers_) {
  var type    = type_;
  var content = content_;
  var headers;

  if (type) {
    // The caller wants a specific type

    // Do they want 'text'
    if (/^text\//i.exec(type)) {
      if (!_.isString(content)) {

        if (_.isArray(content)) {
          if (content.length > 0) {
            content = _.map(content, function(chunk) {
              return chunk.toString();
            }).join('');
          } else {
            // A zero length array, but forced as text
            content = '';
          }
        } else if (_.isObject(content)) {
          content = JSON.stringify(content);
        } else {
          content = '' + content;
        }
      }

      // Its already a string -- nothing to do

    // Do they want JSON?
    } else if (/json$/i.exec(type)) {
      if (_.isArray(content) && content.length > 0) {
        if (_.isString(content[0])) {
          // An array of strings.  Join them and parse as JSON
          content = _.map(content, function(chunk) {
            return chunk.toString();
          }).join('');

          content = JSON.stringify(safeJSONParse(content));
        } else if (Buffer.isBuffer(content[0])) {
          // The response is already buffer-ified
        }

      } else if (_.isArray(content)) {
        // An array, but not an array of strings.
        content = JSON.stringify(content);

      } else if (_.isString(content)) {
        // A string
        content = JSON.stringify(safeJSONParse(content));

      } else {
        // Some sort of object
        content = JSON.stringify(content);
      }

    } else {
      // The caller specified a type, but is was not text or JSON.  Generally, don't do anything
    }
  } else {
    // The caller did not specify a type -- we're a JSON server, so only a few things will be otherwise
    if (_.isArray(content) && content.length > 0 && _.isString(content[0])) {
      // An array of strings -- join them
      content = _.map(content, function(chunk) {
        return chunk.toString();
      }).join('');
    }

    if (_.isString(content)) {
      // Maybe text, but if it parses as JSON, use it
      content = safeJSONParse(content, content);
    }

    // If its still a string, I guess its text
    if (_.isString(content)) {
      type = 'text/plain';
    } else {

      if (content) {
        type = 'application/json';
        content = JSON.stringify(content);
      }

      /* otherwise */
      // They may have just sent 200 or 404 or something without content or type -- respond() will take care of that
    }
  }

  headers = _.extend({'Content-Type':type}, headers_ || {});;

  // If the caller uses 299, they want 200, but to log the response
  if (code === 299) {
    code = 200;
    verbose(1, "sendSmartResponse: ", code, content, headers);
  }

  return respond(res, code, content, headers);
};

/**
 *  Smart send response.
 *
 *  Sends 200 and makes a good guess of what the content type is.
 *  (use null for type to use default processing.)
 *
 *  * An Object gets sent as JSON
 *  * An array of strings gets join'ed
 */
var sendResponse = lib.sendResponse = function(req, res, type_, content_) {
  return sendSmartResponse(req, res, 200, content_, type_);
};

/*var sendJsonp =*/ lib.sendJsonp = function(req, res, object, fnName_) {
  var query = urlLib.parse(req.url, true).query;
  var fnName = fnName_ || query.callback || 'callback';
  var response = fnName + '(' + JSON.stringify(object) + ');';

  return _200(req, res, response, {'Content-Type':'application/javascript'});
};

/**
 *  If you have 404 or 500 or whatever, send it here.
 *
 *  This function will send debug info, if we are not in prod mode.
 */
var debugRespond = lib.debugRespond = function(req, res, code, obj) {

  if (isLogicError(code)) {
    log.error("Error Response: ", code, obj, req.url);
  }

  if (!isDebug()) { return respond(res, code); }

  /* otherwise */
  return sendSmartResponse(req, res, code, obj);
};

var serveData = lib.serveData = function(req, res, userPayload, options, callback) {
  var extension = path.extname(options.ext);
  var response = userPayload;

  if (options.dict) {
    response = _.template(userPayload, options.dict);
  }

  if (extension) {
    sendResponse(req, res, getMimeType(extension), response);
    return callback();
  }

  /* otherwise -- gotta send something */
  sendResponse(req, res, 'application/octet-stream', response);
  return callback();
};

// ------------------properties: read from config file----------------------
var readPropertiesConf = lib.readPropertiesConf = function(file_name, options, env_var, callback) {
  var valueFromEnv = theARGV[env_var] || process.env[env_var];
  if (valueFromEnv) { return callback(null, valueFromEnv); }

  return fs.stat(file_name, function(err, stats) {
    if (err || !stats.isFile()) { return callback({error:'no properties file: ' + file_name}, ''); }

    /* otherwise */
    return properties.parse(file_name, options, function (err, obj){
      if (err)  { return callback.apply(this, arguments); }
      if (!obj) { return callback({error:'bad properties file: ' + file_name}, ''); }

      return callback(null, obj[env_var] || '');
    });
  });
};

/**
 *  If some special properties are not present otherwise, they can be calculated.
 */
var specialEnvVarProcessing = function(envVar, origErr, origValue, callback) {
  var justKeepOrig = function() {
    return callback(origErr, origValue);
  };

  if (envVar === 'MARIO_SERVICE_BASE') {
    if (!origErr && origValue) { return justKeepOrig(); }

    return lib.getMarioProperty('MARIO_MY_SERVER_IP', function(err, myIp) {
      if (err) { return justKeepOrig(); }

      var parts = myIp.split('.');
      if (_.isArray(parts) && parts.length === 4) {
        var last = parseInt(parts.pop(), 10);

        if (last <=   9) { return callback(null, myIp); }                             /* No such thing, just use my IP */

        if (last <=  15) { return callback(null, parts.concat( '10').join('.')); }    /* 10  -  15; web tier, usually */
        if (last <=  99) { return callback(null, parts.concat( '16').join('.')); }    /* 16  -  99; app tier, usually - print servers (aka RIPs) */
        if (last <= 119) { return callback(null, parts.concat('100').join('.')); }    /* 100 - 119; other */
        if (last <= 129) { return callback(null, parts.concat('120').join('.')); }    /* 120 - 129; other */
        if (last <= 139) { return callback(null, parts.concat('130').join('.')); }    /* 130 - 139; other */

        /* otherwise */
        return callback(null, parts.concat('140').join('.'));                         /* 140 - 254; other */
      }

      /* otherwise -- best guess is a print server */
      return callback(null, parts.concat('16').join('.'));

    });
  }

  if (envVar === 'MARIO_MY_EXTERNAL_NAME') {
    if (!origErr && origValue) { return justKeepOrig(); }


    var myIp, serviceBaseIp, stackName, serviceName, domainName;
    return __run([function(next) {
      return lib.getMarioProperty('MARIO_SERVICE_BASE', function(err, serviceBaseIp_) {
        if (err) { return justKeepOrig(); }
        serviceBaseIp = serviceBaseIp_;
        return next();
      });

    }, function(next) {
      return lib.getMarioProperty('MARIO_MY_SERVER_IP', function(err, myIp_) {
        if (err) { return justKeepOrig(); }
        myIp = myIp_;
        return next();
      });

    }, function(next) {
      return lib.getMarioProperty('MARIO_STACK_NAME', function(err, stackName_) {
        if (err) { return justKeepOrig(); }
        stackName = stackName_;
        return next();
      });

    }, function(next) {
      return lib.getMarioProperty('MARIO_SERVICE', function(err, serviceName_) {
        if (err) { return justKeepOrig(); }
        serviceName = serviceName_;
        return next();
      });

    }, function(next) {
      return lib.getMarioProperty('MARIO_DOMAIN_NAME', function(err, domainName_) {
        if (err) { return justKeepOrig(); }
        domainName = domainName_;
        return next();
      });

    }], function() {
      var parts = myIp.split('.'), serviceBaseParts = serviceBaseIp.split('.');
      if (!_.isArray(parts) || parts.length !== 4 || !_.isArray(serviceBaseParts) || serviceBaseParts.length !== 4) {
        return justKeepOrig();
      }

      /* otherwise */
      var last = parts.pop(), serviceBaseLast = serviceBaseParts.pop();
      if (!/[0-9]+/.exec(last) || !/[0-9]+/.exec(serviceBaseLast)) {
        return justKeepOrig();
      }

      /* otherwise */
      var myId = parseInt(last, 10) - parseInt(serviceBaseLast, 10);
      var myExternalName = '';

      myExternalName += stackName;
      myExternalName += '-';
      myExternalName += serviceName;
      myExternalName += myId;
      myExternalName += '.';
      myExternalName += domainName;

      return callback(null, myExternalName);
    });
  }

  /* otherwise, no speical processing for this var */
  return justKeepOrig();
};

// ------------------properties: read from config file for Mario----------------------
var getMarioProperty = lib.getMarioProperty = function(env_var, callback) {
  var home = process.env.HOME || theARGV.HOME;
  return readPropertiesConf("/etc/rover/print_server.conf", { path: true, namespaces: true }, env_var, function(err) {

    if (!err) { return callback.apply(this, arguments); }

    /* otherwise -- see if there is a mario env file*/
    return fs.readFile(path.join(home, 'mario_env'), 'utf8', function(err, contents) {
      //if (err) { return callback (err); }

      var value = '';
      if (!err) {
        var m;
        _.each(contents.split('\n'), function(line) {
          if ((m = new RegExp(env_var + '="([^"]*)"').exec(line))) {
            value = m[1];
          } else if ((m = new RegExp(env_var + '=([^"]*)').exec(line))) {
            value = m[1];
          }
        });
      }

      // Some vars need extra-special attention
      if (err || !value) {
        return specialEnvVarProcessing(env_var, err, value, function(err, specialValue) {
          verbose(2, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! returning |" + specialValue + "| for " + env_var);
          return callback(err, specialValue);
        });
      }

      /* otherwise */
      return callback(null, value);
    });
  });
};

var getIsBrother = lib.getIsBrother = function(ppds, model){
  var ppdName = ppds.MDL[model].toLowerCase();
  var isBrother = (ppdName.indexOf("brother") < 0) ? false : true;
  if(!isBrother){
    var ppdInitial = _.find(["br", "mfc", "dcp", "hl", "fax"], function(initial){
      return ppdName.indexOf(initial) === 0;
    });
    isBrother = ppdInitial ? true : false;
  }
  return isBrother;
};

var getModelWithoutManufacturer = lib.getModelWithoutManufacturer = function(model){
  var firstWord = model.substr(0, model.indexOf(' '));
  if (firstWord === "hp" || firstWord === "epson" || firstWord === "canon" || firstWord === "brother"){
    model = model.substr(model.indexOf(' ') + 1);
  }
  return model;
};

/*var serveFile =*/ lib.serveFile = function(req, res, filename, options_, callback) {
  var contents;
  var extension = filename;

  return __run([

    // If we dont have an extension, use identify
    function(next) {
      var ext = path.extname(filename);
      if (ext && ext.length > 0) { return next(); }

      /* otherwise */
      return exec("identify " + filename, function(err, stdout, stderr) {
        if (err) { return next(); } // Just keep carying on

        /* otherwise */
        // I.e.: FILENAME JPEG 960x1280 960x1280+0+0 8-bit sRGB 90.8KB 0.000u 0:00.000
        var statsList = stdout.split("\n")[0].split(/[ \t]+/);
        if (!statsList || statsList.length < 2) { return next(); }

        /* otherwise */
        extension = filename + "." + statsList[1].toLowerCase();
        return next();
      });
    },

    // Get the file data
    function(next) {
      if (options_.isBinary) {
        return fs.readFile(filename, function(err, contents_) {
          if (err || !contents_) { return callback.apply(this, arguments); }
          contents = contents_;
          return next();
        });
      } else {
        return fs.readFile(filename, 'utf8', function(err, contents_) {
          if (err || !contents_) { return callback.apply(this, arguments); }
          contents = contents_;
          return next();
        });
      }
    }],

    function() {
      var options = _.extend({ext: extension}, options_);
      return serveData(req, res, contents, options, callback);
    }
  );
};

//var exts;
var getMimeType = function(ext_, def_) {
  var def = def_ || 'text/plain';

  var exts = exts || {
    "html"  : "text/html",
    "js"    : "application/javascript",
    "jpg"   : "image/jpeg",
    "jpeg"  : "image/jpeg",
    "png"   : "image/png"
  };

  var ext = ext_.replace(/^\./, '');

  if (exts[ext]) {
    return exts[ext];
  }

  return def;
};

var capitols = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
var lowerCases = 'abcdefghijklmnopqrstuvwxyz';

var capitolize = function(str) {
  var index = lowerCases.indexOf(str[0]);
  if (index === -1) { return str; }

  return capitols[index] + str.substr(1);
};

lib.knownScrewyHttpHeadersList = [
  'Content-MD5', 'TE', 'DNT', 'ETag', 'P3P', 'WWW-Authenticate', 'X-XSS-Protection', 'X-UA-Compatible'
];

lib.knownScrewyHttpHeaders = (function() {
  return lib.reduce(lib.knownScrewyHttpHeadersList, {}, function(m, h) {
    return kv(m, h.toLowerCase(), h);
  });
}());

var skrewy = lib.knownScrewyHttpHeaders;

lib.stdHeader = function(key) {
  if (key in skrewy) { return skrewy[key]; }

  return _.map(key.split('-'), function(x) {return capitolize(x);}).join('-')
};


var uuencode = function(str) {
  return str;
};

lib.QStringify = function(query) {
  return _.map(query, function(value, key) {
    return key + '=' + uuencode(value);
  }).join('&');
};

// -------------------- executable helpers --------------------

/**
 *  A lot like exec (just have one callback), but with all the goodness
 *  of spawn
 */
lib.exec = function(cmd, args /*, options, callback*/) {
  var args_    = _.rest(arguments, 2);
  var callback = args_.pop() || lib.noop;
  var options  = args_.pop() || null;

  var proc, stdoutChunks = [], stderrChunks = [];

console.log(cmd);
console.log(lib.inspect(args));
console.log(lib.inspect(arguments));
  if (options !== null) {
    proc = spawn(cmd, args, options);
  } else {
    proc = spawn(cmd, args);
  }

  proc.stdout.setEncoding('utf8');
  proc.stdout.on('data', function(chunk) {
    stdoutChunks.push(chunk);
  });

  proc.stderr.setEncoding('utf8');
  proc.stderr.on('data', function(chunk) {
    stderrChunks.push(chunk);
  });

  proc.on('close', function(exitCode) {
    return callback(exitCode, stdoutChunks, stderrChunks);
  });
};

// -------------------- MongoDB helpers --------------------
lib.MongoUpsert = function(collection) {
  var self = this;

  lib.MongoUpdater.apply(self, arguments);

  self.commit = function(key, value, callback) {
    self.cleanUpdates();

    var selector;

    if (arguments.length === 3) {
      selector    = kv(key, value);
    } else {
      selector    = arguments[0];
      callback    = arguments[1];
    }

    return collection.update(selector, self.updates, {upsert:true}, function(/*err, receipt*/) {
      return callback.apply(this, arguments);
    });
  };
};

lib.MongoUpdate = function(collection) {
  var self = this;

  lib.MongoUpdater.apply(self, arguments);

  self.commit = function(key, value, callback) {
    self.cleanUpdates();

    var selector;

    if (arguments.length === 3) {
      selector    = kv(key, value);
    } else {
      selector    = arguments[0];
      callback    = arguments[1];
    }

    return collection.update(selector, self.updates, function(/*err, receipt*/) {
      return callback.apply(this, arguments);
    });
  };
};

lib.MongoFindAndModify = function(collection) {
  var self = this;

  lib.MongoUpdater.apply(self, arguments);

  self.commit = function(key, value, callback) {
    self.cleanUpdates();

    return collection.findAndModify(kv(key, value), [[key, 1]], self.updates, {"new":true}, function(/*err, item*/) {
      return callback.apply(this, arguments);
    });
  };
};

var warnForBadMongoUpdates = function(updates, key) {
  key = key || '';

  if (_.isArray(updates)) {
    if (updates.length === 0) {
      log.warn("Warning mongo update object is empty array (" + key + ")");
    }

    _.each(updates, function(item, index) {
      warnForBadMongoUpdates(item, key + '[' + index + ']');
    });
    return;
  }

  /* otherwise */
  if (_.isDate(updates)) { return; }

  /* otherwise */
  if (_.isObject(updates)) {
    if (_.keys(updates).length === 0) {
      log.warn("Warning mongo update object is empty object (" + key + ")");
    }

    _.each(updates, function(item, itemKey) {
      warnForBadMongoUpdates(item, key + '.' + itemKey);
    });
    return;
  }

  /* otherwise */
  if (_.isString(updates)) {
    if (updates === '' || updates === '{}' || updates === 'null' || updates === 'undefined') {
      log.warn("Warning mongo update object is probably bad (" + key + "): ", updates);

    } else if (/^[0-9]+$/.exec(updates)) {
      log.warn("Warning mongo update object should probably be a number (" + key + "): ", updates);
    }
    // TODO: Also check for dates
    return;
  }
};

lib.MongoUpdater = function(collection) {
  var self = this;
  var now  = new Date();

  self.updates = {
    $setOnInsert : { ctime: now },
    $set         : { mtime: now },
    $inc         : {}
  };

  self.set = function(key, value) {

    var obj = arguments[0];
    if (arguments.length === 2) {
      obj = kv(key, value);
    }

    _.extend(self.updates.$set, obj);
  };

  self.setOnInsert = function(key, value) {

    var obj = arguments[0];
    if (arguments.length === 2) {
      obj = kv(key, value);
    }

    _.extend(self.updates.$setOnInsert, obj);
  };

  self.inc = function(key, value_) {
    var value = value_;
    if (!value_ && value !== 0) {
      value = 1;
    }

    self.updates.$inc[key] = (self.updates.$inc[key] || 0) + value;
  };

  self.addToSet = function(key, value) {

    if (_.isArray(value)) {
      _.each(value, function(item) {
        self.addToSet(key, item);
      });
      return;
    }

    // If we do not have $addToSet already, add it
    if (!self.updates.hasOwnProperty('$addToSet')) {
      self.updates.$addToSet = {};
    }

    // If this is the first time this key is used, the value is not an array
    if (!self.updates.$addToSet.hasOwnProperty(key)) {
      self.updates.$addToSet[key] = value;
      return;
    }

    /* otherwise -- if we already have an array, just add to it */
    if (self.updates.$addToSet[key].hasOwnProperty('$each')) {
      self.updates.$addToSet[key].$each.push(value);
      return;
    }

    /* otherwise -- update from a normal value to an array of two values */
    var oldValue = self.updates.$addToSet[key];
    self.updates.$addToSet[key] = {$each:[oldValue, value]};

  };

  self.cleanUpdates = function() {
    if (self.updates.$set && _.keys(self.updates.$set).length === 0) { delete self.updates['$set']; }
    if (self.updates.$inc && _.keys(self.updates.$inc).length === 0) { delete self.updates['$inc']; }

    if (isDebug()) {
      warnForBadMongoUpdates(self.updates);
    }
  };
};

/*var mongoArray =*/ lib.mongoArray = function(dbName, clusterName, query, callback) {
  return getMarioProperty("MARIO_DB_HOSTNAME", function(err, marioDbHostname) {
    var dbUrl = 'mongodb://' + marioDbHostname + ':27017/' + dbName;
    return require('mongodb').MongoClient.connect(dbUrl, function(err, db_) {
      var clusterCursor = db_.collection(clusterName).find(query);
      return clusterCursor.toArray(function(/*err, results*/) {
        return callback.apply(this, arguments);
      });
    });
  });
};

// ------------------------------------------ Platform ------------------------------------
//var sgAws = require('./sg_aws');
//
//_.each(['getServiceBase', 'getServerConfig', 'getClusterConfig'], function(fname) {
//  lib[fname] = sgAws[fname];
//});

lib._ = _;
exportify(lib);
//exportify(require('./sg_eval'));


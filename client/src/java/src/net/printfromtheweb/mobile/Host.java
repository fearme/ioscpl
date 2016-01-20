/**
 *  Abstract base for a Host.
 *
 *  A Host is like a platform, but this is all Java.  There are various
 *  host types, like Cordova, Android (raw), Desktop (running a workstation's Java).
 *
 *  This class is for how to do things on a specific platform - like acquiring a 
 *  "multicast lock" on Android, which isn't needed for the other platforms.
 *
 *  Contrast the SecureAssetPrintingApi class which is for Secure-Print-centric things.
 *
 *  Many functions are implemented here, even though this is intended to be an 'abstract'
 *  base class / interface.  Java is quite standardized, so the implementations below should
 *  work on all Javas that aren't out-of-the-ordinary; which means workstation Javas.  Android
 *  has its own derived host.
 *
 *  That being said, this class cannot refer to things that are not available on Android, and
 *  there is a DesktopHost.
 *
 *  [Another way to say it: this file contains an implementation that should work on POSIX
 *  systems, except those that require us to import stuff outside org.json.* and java.*]
 *
 */

package net.printfromtheweb.mobile;

import net.mobilewebprint.Client;
import net.mobilewebprint.PrintProgressChangesListener;
import net.mobilewebprint.PrinterListChangesListener;

import java.util.Properties;

//import net.printfromtheweb.mobile.Util.Js;
//import net.printfromtheweb.mobile.Util.Js.*;
//import net.printfromtheweb.mobile.Util.JSONAble;
//
//import org.json.*;
//
//import java.net.InetAddress;
//import java.io.*;
//import java.util.concurrent.*;
import java.util.concurrent.ExecutorService;
//import java.util.Map;
//import java.util.regex.*;
//import java.util.HashMap;
//import java.util.List;
//import java.util.ArrayList;
//import java.util.Iterator;

public class Host implements PrinterListChangesListener, PrintProgressChangesListener {

  private static final String     TAG               = "jMobileWebPrint";

//  public static final String      WIFI_NAG_NONE     = "";
//  public static final String      WIFI_NAG_NAG      = "net.printfromtheweb.mobile.WIFI_NAG";
//  public static final String      WIFI_NAG_PROMPT   = "net.printfromtheweb.mobile.WIFI_PROMPT";
//
//  // To parse a http URL: (http)://(the-server-name-but-not-colon-or-slash)(:(portNum))/the-rest/including/query-and-such
//  //    4 captures: the protocol, servername, port number with colon, port number
//  //    the port is optional, so we have to enclose in parens, and follow with that '?'; making it a capture, even though
//  //    we don't intend on using the captured colon
//  private static final Pattern             httpUrlRePattern   = Pattern.compile("(http.*)://([^:/]+)(:([0-9]+))?.*");
//
//  public    String                           protocol;
//  public    String                           pclServerName;
//  public    int                              port;
//
//  public static    String                    defProtocol      = "http";
//  public static    String                    defPclServerName = "hq.mobilewebprint.net";
//  public static    int                       defPort          = 80;
//
//  // Config
//  public           JSONObject                config;

//  public    EventEmitter                     emitter;
//  public    CoreApi                          api;
//  public    Client                           mwp;
//  public    String                           primaryAccount;
//
  public    PrintManager                     printManager;
  public    PrintManager                     internalPrintManager;
  public    net.mobilewebprint.Application   mwp_app;

  /**
   *  Constructor.
   */
  public Host() {

//    this.protocol       = null;
//    this.pclServerName  = null;
//    this.port           = 80;
//
//    this.config                     = Js.o();

    this.printManager               = null;
    this.internalPrintManager       = null;

  }

  public net.mobilewebprint.Client mwp()
  {
    return CoreApi.mwp;
  }

//  public String pclServerUrl(String path) {
//
//    while (protocol == null || pclServerName == null) {
//      log_d(TAG, "Waiting to determine server name");
//      sleep(250);
//    }
//
//    return protocol + "://" + pclServerName + ":" + port + path;
//  }
//
//  public boolean isSU() {
//    return config.optBoolean("isSU", false);
//  }
//
////  public Host(String protocol, String pclServerName, int port) {
////    this.protocol       = protocol;
////    this.pclServerName  = pclServerName;
////    this.port           = port;
////  }

  /**
   *  Start the host up.
   */
  public void start() {
    mwp().RegisterPrinterListChangesListener(this);
    mwp().RegisterPrintProgressChangesListener(this);
  }

  /**
   *  A function to notify that some function is unimplemented.
   */
  private void _unimplemented(String methodName) {
//    log_w(TAG, "UNIMPLEMENTED: " + methodName);
  }

//  /**
//   *  What is this device's primary account.
//   *
//   *  Used for ID.
//   */
//  private void getPrimaryAccount() { _unimplemented("getPrimaryAccount"); }
//
//  /**
//   *  A unique string for this user.
//   */
//  private String userUnique(String str) { return ""; }
//
//  /**
//   *  The app is going to scan.
//   *
//   *  Stub works for most platforms
//   */
//  public void prepForScan(String name) { }
//
//  /**
//   *  The app is done scanning.
//   *
//   *  Stub works for most platforms
//   */
//  public void scanFinished() { }
//
//  /**
//   *  Get an environment variable.
//   *
//   *  Beware: Android does not have these.
//   */
//  public String getenv(String key) { _unimplemented("getenv"); return ""; }
//
//  public String getTempDir() { _unimplemented("getTempDir"); return ""; }
//
//  /**
//   *  Get a temp file (stream) to write to.
//   */
//  public FileOutputStream getTempFileForWriting(String tmpDir, String name) throws FileNotFoundException {
//    File pclFile = new File(tmpDir, name);
//    log_v(TAG, "Writing to: " + pclFile);
//    return new FileOutputStream(pclFile);
//  }
//
//  /**
//   *  Get a read-stream from a file.
//   *
//   *  Should be used in conjunction with getTempFileForWriting()
//   */
//  public FileInputStream getTempFileForReading(String tmpDir, String name) throws FileNotFoundException {
//    File tmpFile = new File(tmpDir, name);
//    log_v(TAG, "Reading from: " + tmpFile);
//    return new FileInputStream(tmpFile);
//  }
//
//  /**
//   *  Post the JSON to the path.
//   */
//  public void httpPost(String path, JSONObject reqBody) { _unimplemented("httpPost(" + path + ")"); }
//  public void httpPost(String path, JSONArray  reqBody) { _unimplemented("httpPost(" + path + ")"); }
//
//  public JSONObject httpJsonRpc(String protocol, String pclServerName, int port, String path, JSONObject callerData) { _unimplemented("httpJsonRpc 5"); return null; }
//  public JSONObject httpJsonRpc(String path, JSONObject callerData) { _unimplemented("httpJsonRpc 2"); return null; }
//  public JSONObject httpJsonRpc(String path) { _unimplemented("httpJsonRpc 1"); return null; }
//  public JSONObject httpJsonRpcSync(String path, JSONObject callerData) { _unimplemented("httpJsonRpcSync 2x"); return null; }
//  public JSONObject httpUploadMultipartForm(String path, String fileName, InputStream is, JSONObject otherFormData) throws Exception {_unimplemented("httpGet"); return null;}
//
//  /**
//   *  Do an HTTP GET of the path, and write the result to the stream.
//   */
//  public boolean httpGet_and_writeTo(String path, FileOutputStream stream) throws IOException { _unimplemented("httpGet");  return false; }
//
//  /**
//   *  What is this host's IP address?
//   */
//  public InetAddress getDeviceIpAddress() { _unimplemented("getDeviceIpAddress");  return null; }
//
//  /**
//   *  Get the thread pool, so caller can start a thread.
//   */
  public ExecutorService getThreadPool() { _unimplemented("getThreadPool");  return null; }
//
  /**
   *  The print manager
   */
  public static interface PrintManager {
    public void onPrinterListChanged();
    public void onPrinterListReset();
    public void onPrinterListClear();
    public void onPrinterScanDone();
    public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId);
  }

  /**
   *  Register as the print manager.
   */
  public void registerAsPrintManager(PrintManager manager) {
    //mwp().logD(TAG, "API-entrypoint: registerAsPrintManager");
    printManager = manager;
  }

  /**
   *  Register as the (internal) print manager.
   */
  public void registerAsInternalPrintManager(PrintManager manager) {
    internalPrintManager = manager;
  }

  // PrinterListChangesListener
  @Override
  public void onNewPrinterList()
  {
    //mwp().logD(TAG, "Event - onNewPrinterList");
    printerListReset();
  }

  @Override
  public void onBeginPrinterListChanges()
  {
    //mwp().logD(TAG, "Event - onBeginPrinterListChanges");
  }

  @Override
  public void onPrinterChanged(String ip, Properties printer)
  {
    //mwp().logD(TAG, "Event - onPrinterChanged");
  }

  @Override
  public void onPrinterRemoved(String ip)
  {
    //mwp().logD(TAG, "Event - onPrinterRemoved");
  }

  @Override
  public void onEndPrinterListEnumeration()
  {
    //mwp().logD(TAG, "Event - onEndPrinterListEnumeration");
    printerListChanged();
  }

  // PrintProgressChangesListener
  @Override
  public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId, String jobStatus)
  {
    //mwp().logD(TAG, "Event - onPrintJobProgress");
    doPrintingProgress(jobStatus, numerator, denominator, message, rawState, jobId);
  }

  /**
   *  Notify the system that the printer list has changed.
   *
   *  Call the observer, if it exists.
   */
  public void printerListChanged() {
    if (printManager != null) {
      printManager.onPrinterListChanged();
    }
    if (internalPrintManager != null) {
      internalPrintManager.onPrinterListChanged();
    }
  }

  /**
   *  Notify the system that the printer list has been cleared (when we have no network connectivity.)
   *
   *  Call the observer, if it exists.
   */
  public void printerListClear() {
    if (printManager != null) {
      printManager.onPrinterListClear();
    }
    if (internalPrintManager != null) {
      internalPrintManager.onPrinterListClear();
    }
  }

  /**
   *  Notify the system that the printer scan is done.
   *
   *  Call the observer, if it exists.
   */
  public void printerScanDone() {
    if (printManager != null) {
      printManager.onPrinterScanDone();
    }
    if (internalPrintManager != null) {
      internalPrintManager.onPrinterScanDone();
    }
  }

  /**
   *  Notify the system that the printer list has been reset (start over with empty list).
   *
   *  Call the observer, if it exists.
   */
  public void printerListReset() {
    if (printManager != null) {
      printManager.onPrinterListReset();
    }
    if (internalPrintManager != null) {
      internalPrintManager.onPrinterListReset();
    }
  }

//  public CoreApi.PrintJobInterface mkPrintJob() {
//    return new CoreApi.PrintJob();
//  }
//
//  public CoreApi.PrintJobInterface mkPrintJob(String str) {
//    return new CoreApi.PrintJob(str);
//  }

  /**
   *  Notify the system that printing is progressing.
   */
  public void doPrintingProgress(String state, int numerator, int denominator, String message, String rawState, String jobId) {
//    String rawState      = progressMsg.get("rawState");
//    String message       = progressMsg.get("message");
//    String jobId         = progressMsg.get("jobId");
//
//    int    numerator     = progressMsg.getInt("numerator");
//    int    denominator   = progressMsg.getInt("denominator");
//
//    log_v(TAG, String.format("Printing status(%s,%s)(%d/%d): %s",
//          state,
//          progressMsg.get("rawState"),
//          progressMsg.getInt("numerator"),
//          progressMsg.getInt("denominator"),
//          progressMsg.get("message")));

    if (printManager != null) {
      printManager.onPrintJobProgress(state, numerator, denominator, message, rawState, jobId);
    }
    if (internalPrintManager != null) {
      internalPrintManager.onPrintJobProgress(state, numerator, denominator, message, rawState, jobId);
    }
  }

//  /**
//   *  Are we on a background thread?
//   */
//  public boolean isBgThread(boolean quiet) {
//    return true;
//  }
//
//  /**
//   *  Are we on a background thread?
//   */
//  public boolean isBgThread() {
//    return true;
//  }
//
//  /**
//   *  Sleep.
//   *
//   *  Allow other threads execution time, while the caller is waiting for something to
//   *  happen.
//   */
//  public void sleep(int ms) { _unimplemented("sleep"); }
//
//  // Logging
//  public void log_d(String tag, String msg, String shortMsg) {}
//  public void log_d(String tag, String msg) {}
//  public void log_e(String tag, String msg) {}
//  public void log_e(String tag, String msg, Exception ex) {}
//  public void log_w(String tag, String msg) {}
//  public void log_v(String tag, String msg) {}
//
//  /**
//   *  Display the printer on stdout, with the prefix (usually \t).
//   */
//  public void displayPrinter(String pre, Printer printer) { _unimplemented("displayPrinter"); }
//
//  // Print functions
//  public void println(String msg) { _unimplemented("println"); }
//  public void print(String msg) { _unimplemented("print"); }
//  public void println() { println(""); }
//
//  /**
//   *  Get a mapping from IP to MAC address.
//   */
//  public Map<String, String> getMacsFromArpCache() {
//    _unimplemented("getMacsFromArpCache"); 
//    return new HashMap<String, String>();
//  }
//
//  /**
//   *  Get a ProxyStats object, parsed from the appropriate env vars.
//   */
//  public ProxyStats newProxyStats() {
//    String   proxyName     = null;     // http://proxy.atlanta.hp.com:8080
//    String   proxyProtocol = "http";
//    int      proxyPort     = 8080;
//
//    if (getenv("HTTP_PROXY") != null) {
//      proxyName = getenv("HTTP_PROXY");
//    } else if (getenv("http_proxy") != null) {
//      proxyName = getenv("http_proxy");
//    }
//
//    if (proxyName != null) {
//      Matcher m = httpUrlRePattern.matcher(proxyName);
//      if (m.find()) {
//        proxyProtocol = m.group(1);
//        proxyName     = m.group(2);
//
//        if (m.groupCount() >= 4) {
//          proxyPort     = Integer.parseInt(m.group(4), 10);
//        }
//      }
//    }
//
//    return new ProxyStats(proxyName, proxyProtocol, proxyPort);
//  }
//
//  /**
//   *  Proxy server stats.
//   */
//  public static class ProxyStats {
//    public String name;
//    public String protocol;
//    public int    port;
//
//    public ProxyStats(String name, String protocol, int port) {
//      this.name     = name;
//      this.protocol = protocol;
//      this.port     = port;
//    }
//
//    public String toString() {
//      return Js.xo("name", name, "protocol", protocol, "port", port).toString();
//    }
//  }
//
//  // ---------- Telemetry ----------
//
//  public TelemetryBucket telemetryBucket(String name) {
//    return new TelemetryBucket(name);
//  }
//
//  public class TelemetryBucket {
//    public String name;
//    public String id;
//    public long   creationTime;
//
//    TelemetryBucket(String name) {
//      this.creationTime  = System.currentTimeMillis();
//      this.name = name;
//      this.id = Util.randomString(32);
//      this.telemetryData = new ConcurrentLinkedQueue<String>();
//    }
//
//    TelemetryBucket(String name, String id) {
//      this.creationTime  = System.currentTimeMillis();
//      this.name = name;
//      this.id = id;
//      this.telemetryData = new ConcurrentLinkedQueue<String>();
//    }
//
//    public void put(TelemetryEvent x) {
//      telemetryData.add(x.stringify());
//    }
//
//    public void send() {
//      flush_();
//    }
//
//    public void flush() {
//      //flush_();
//    }
//
//    public void flush_() {
//      int count = 0;
//      JSONArray arrData = new JSONArray();
//
//      String itemStr = "";
//      while (true) {
//        if ((itemStr = telemetryData.poll()) == null) { break; }
//
//        /* otherwise */
//        count += 1;
//        try {
//          arrData.put(new JSONObject(itemStr));
//        } catch(JSONException dontcare) {}
//      }
//
//      //log_d(TAG, arrData.toString());
//
//      log_v(TAG, "Flushing telemetry: " + count);
//      if (count > 0) {
//        JSONObject reqBody = Js.extend(Js.o("items", arrData, "bucket", name, "bucketId", id), Js.o("startTime", creationTime));
//        httpPost("/telemetry", reqBody);
//      }
//    }
//
//    private   ConcurrentLinkedQueue<String>  telemetryData;
//  }
//
//  public static class TelemetryEvent extends Util.JSONAble {
//    private String bucket;
//    private String bucketId;
//    private String event;
//    private long   eventTime;
//
//    public TelemetryEvent() {
//      this.eventTime  = System.currentTimeMillis();
//      this.bucket     = "";
//      this.bucketId   = "";
//      this.event      = "";
//      this.JSONString = "{}";
//    }
//
//    public TelemetryEvent(TelemetryBucket bucket, String event) {
//      this.eventTime  = System.currentTimeMillis();
//      this.bucket     = bucket.name;
//      this.bucketId   = bucket.id;
//      this.event      = event;
//      this.JSONString = "{}";
//    }
//
//    public void setBE(TelemetryBucket bucket, String event) {
//      this.bucket     = bucket.name;
//      this.bucketId   = bucket.id;
//      this.event      = event;
//      this.JSONString = "{}";
//    }
//
//    protected JSONObject _json() {
//      return Js.o(/*"bucket", bucket, "bucketId", bucketId,*/ "event", event, "eventTime", eventTime);
//    }
//  }
//
//  public static class TelemetryEventJust extends TelemetryEvent {
//
//    public <T> TelemetryEventJust(TelemetryBucket bucket, String eventName, T x) {
//      super(bucket, eventName);
//      populate("just", x);
//    }
//  }
//
////  public void telemetry(TelemetryEvent x) {
////    telemetryData.add(x.stringify());
////  }
//
//  public void showStack(String msg) {}
//  public void hugeWarning(String msg) {}
}





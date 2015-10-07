/**
 *  The core API.
 *
 */

package net.printfromtheweb.mobile;

import net.printfromtheweb.mobile.Printer;

//import  net.printfromtheweb.mobile.*;
//import  net.printfromtheweb.mobile.Util.Js;
//import  net.printfromtheweb.mobile.Util.Js.*;

import net.mobilewebprint.Client;

//import org.json.*;

////import java.lang.Math;
import java.util.*;
//import java.io.*;
//import java.text.Collator;
import java.util.regex.*;
//import java.util.Map.Entry;
import java.lang.Exception;

public class CoreApi {

//  public static final String              showRemoved                  = "showRemoved";
//
//  public static final String              STATUS_WAITING0              = "WAITING0";
//  public static final String              STATUS_WAITING1              = "WAITING1";
//  public static final String              STATUS_PRINTING              = "PRINTING";
//  public static final String              STATUS_WAITING2              = "WAITING2";
//  public static final String              STATUS_CANCELLED             = "CANCELLED";
//  public static final String              STATUS_CANCELLING            = "CANCELLING";
//  public static final String              STATUS_SUCCESS               = "SUCCESS";
//
//  public static final String              RAW_STATUS_CANCELLING        = "CANCELING";
//  public static final String              RAW_STATUS_IDLE              = "IDLE";
//  public static final String              RAW_STATUS_PRINTING          = "PRINTING";
//  public static final String              RAW_STATUS_VERY_LOW_ON_INK   = "VERY LOW ON INK";
//
//
  public static CoreApi                   api;
  public static Client                    mwp;

//  protected Controller                    controller;
//
  public  SecureAssetPrintingApp          app;
  public  HostAndroid                     host;

  public  long                            scanStartTime;

//  protected Map<String, Boolean>          flags;
//  protected Map<String, String>           options;
//  protected Map<String, Integer>          intOptions;
//
  protected static final String             TAG             = "jMobileWebPrint";
//  protected static final String             tmpDir          = "/tmp";

  public static int                       num_objects = 0;
  public static int                       num_starts = 0;

  protected CoreApi(SecureAssetPrintingApp app, HostAndroid host) {

    CoreApi.api = this;
    CoreApi.mwp = new Client(new net.mobilewebprint.MwpApplication());

    num_objects += 1;

    mwp.logD(TAG, "-----------------------------CoreApi-ctor (count:" + num_objects + ")");
    mwp.context = host.context;
//    if (num_objects > 1) {
//      throw new Exception("MWP (Mario) is a singleton!");
//    }

    this.app       = app;
    this.host      = host;
    //this.host.api  = this;
    //this.host.mwp  = mwp;

//    this.controller = new Controller(this);
//
//    Printer.host = host;
//
//    this.flags                      = new HashMap<String, Boolean>();
//    this.options                    = new HashMap<String, String>();
//    this.intOptions                 = new HashMap<String, Integer>();

  }

  public void start() {
    mwp.logD(TAG, "-----------------------------CoreApi-start");
    start(true);
  }

  public void start(boolean autoStartScan) {
    num_starts += 1;
    mwp.logD(TAG, "-----------------------------CoreApi-start(autoStartScan) " + autoStartScan + " num_starts: " + num_starts);

//    host.println("Secure-Asset-Printing: starting(autoStartScan=" + autoStartScan + ").");
//
//    host.log_d(TAG, "username: " + host.primaryAccount);
//    host.log_d(TAG, "SAP: start.  String options:");
//    for (Entry<String, String> entry : options.entrySet()) {
//      host.log_d(TAG, entry.getKey() + ": " + entry.getValue());
//    }
//
//    host.log_d(TAG, "SAP: start.  Boolean options:");
//    for (Entry<String, Boolean> entry : flags.entrySet()) {
//      host.log_d(TAG, entry.getKey() + ": " + entry.getValue());
//    }
//
//    host.log_d(TAG, "SAP: start.  Integer options:");
//    for (Entry<String, Integer> entry : intOptions.entrySet()) {
//      host.log_d(TAG, entry.getKey() + ": " + entry.getValue());
//    }

    mwp.start();
    host.start();
    scanStartTime = System.currentTimeMillis();
  }

  public void reScan(boolean autoScan) {
    mwp.logD(TAG, "-----------------------------CoreApi-reScan");
    mwp.reScan();
    scanStartTime = System.currentTimeMillis();
  }

  public void stop() {
    mwp.logD(TAG, "-----------------------------CoreApi-stop");
//    controller.stop();
  }

  public void exit() {
    mwp.logD(TAG, "-----------------------------CoreApi-exit");
//    controller.exit();
  }

//  public void hoggingNetwork(boolean isHogging) {
//    controller.hoggingNetwork(isHogging);
//  }
//
//  public void hoggingNetwork() {
//    hoggingNetwork(true);
//  }
//
//  public boolean print(String assetUri) {
//    host.println("Secure-Asset-Printing: printing...");
//
//    // Wait for scan to complete
//    if (controller.currentlyScanning) {
//      host.print("Waiting for scanner to stop...");
//      while (controller.currentlyScanning) {
//        host.print(".");
//        host.sleep(500);
//      }
//      host.println("");
//      host.println("Resuming printing...");
//    }
//
//    // Allow app to manipulate the printer list
//    ArrayList<String> ips = app.preFlight(controller.printers);
//
//    if (flag(showRemoved)) {
//      host.println("Removed printers:");
//      for (Printer printer: controller.printers.values()) {
//        if (!ips.contains(printer.ip)) {
//          host.displayPrinter("\t", printer);
//        }
//      }
//    }
//
//    if (ips.size() > 0) {
//
//      host.println();
//
//      // Scrub the list
//      for (String ip: ips) {
//        host.displayPrinter("", controller.printers.get(ip));
//      }
//
//      // Get printer from user
//      Scanner in = new Scanner(System.in);
//
//      host.println("Choose printer (IP): ");
//      String ip = in.nextLine();
//
//      if (assetUri == null) {
//        host.println("Asset URI: ");
//        assetUri = in.nextLine();
//      }
//
//      host.println("Printing to " + ip + ", URI: " + assetUri);
//
//      // Print!!!!
//      return print(ip, assetUri);
//    }
//
//    return false;
//  }

  public void setOption(String name, String value) {
    //mwp.logD(TAG, "setOption(" + name + ", " + value + ")");
    mwp.setOption(name, value);
  }

  public void setOption(String name, int value) {
    //mwp.logD(TAG, "setOption(" + name + ", " + value + ")");
    mwp.setIntOption(name, value);
  }

  public void setOption(String name, boolean value) {
    //mwp.logD(TAG, "setOption(" + name + ", " + value + ")");
    mwp.setFlag(name, value);
  }

  public void setOption(String name) {
    //mwp.logD(TAG, "setOption(" + name + ")");
    mwp.setFlag(name, true);
  }

  private boolean flag(String name) {
//    if (flags.containsKey(name)) {
//      return flags.get(name);
//    }

    return false;
  }

  public String getOption(String name, String def) {
//    String result = def;
//    try {
//      result = options.get(name);
//    } catch(Exception dontcare) {}
//
//    if (result != null) { return result; }

    return def;
  }

  public int getIntOption(String name, int def) {
//    int result;
//    try {
//      result = intOptions.get(name);
//      return result;
//    } catch(Exception dontcare) {}

    return def;
  }

//  public JSONObject printAttributesFor(String url) {
//    return controller.printAttributesFor(url);
//  }
//
//  public JSONObject photoPrintAttributes() {
//    return controller.photoPrintAttributes;
//  }
//
//  public JSONObject pdfPrintAttributes() {
//    return controller.pdfPrintAttributes;
//  }
//
//  public JSONObject unknownPrintAttributes() {
//    return controller.unknownPrintAttributes;
//  }
//
//
//  // ---------- Operations ----------
//
//  /**
//   *  Scan for printers
//   */
//  public void scan() {
//    controller.scan();
//  }
//
//  public boolean printJobIsDone(String state, String rawState) {
//    return controller.printJobIsDone(state, rawState);
//  }
//
//  public boolean printJobSuccess(String state, String rawState) {
//    return controller.printJobSuccess(state, rawState);
//  }
//
//  public boolean printJobCancelled(String state, String rawState) {
//    return controller.printJobCancelled(state, rawState);
//  }

  public Job prePrint(String assetUri) {
    mwp.logD(TAG, "-----------------------------CoreApi-prePrint");
//    return controller.prePrint(assetUri);
    return new Job();
  }

//  public boolean print(String ip, Job job) {
//    return controller.print(ip, job);
//  }
//
//  private void prePrintPin(String assetUri, AsyncAllocateJob asyncAllocateJob) {
//    controller.prePrintPin(assetUri, asyncAllocateJob);
//  }

  public boolean print(String ip, Job job, String token) throws Exception {
    mwp.logD(TAG, "-----------------------------CoreApi-print " + ip + " job: " + job + " " + token);
    return mwp.sendJob(token, ip);
//    showStack("coreapi-print");
//    return controller.print(ip, job, token);
  }

  public void showStack(String msg) throws Exception {
//    try {
      throw new Exception("Showing stack");
//    }catch(Exception e) {
//      Log.e(TAG, msg, e);
//    }
  }

//  public boolean print(String ip, Job job, JSONObject attributes) {
//    return controller.print(ip, job, attributes);
//  }
//
//
//
//
//  public Job prePrintPhoto(InputStream inStream, String filename) {
//    return controller.prePrintPhoto(inStream, filename);
//  }
//
//  public boolean printPhoto(String ip, Job job) {
//    return controller.printPhoto(ip, job);
//  }
//
//// TODO: Put this back.  It is a dup of the one that the cli uses
////  /**
////   *  Print an asset that is housed in the cloud.
////   */
////  public boolean print(String assetUri) {
////    return controller.print(assetUri);
////  }

  // Most callers will let the library handle the UI for the printer list chooser.
  public List<Printer> getSortedPrinters(boolean filterUnsupportedPrinters) {
    mwp.logD(TAG, "-----------------------------CoreApi-getSortedPrinters " + filterUnsupportedPrinters);

    long now = System.currentTimeMillis();

    ArrayList<Properties> printers = mwp.getSortedPrinterList(filterUnsupportedPrinters);
    //mwp.logD(TAG, "Printers:" + printers);

    ArrayList<Printer> result = new ArrayList<Printer>();
    for (Properties printerProps:printers) {

      mwp.logD(TAG, "Printer:" + printerProps);

      Printer printer = new Printer();

      if (printerProps.containsKey("ip"))              { printer.ip              = printerProps.getProperty("ip"); }
      if (printerProps.containsKey("name"))            { printer.name            = printerProps.getProperty("name"); }
      if (printerProps.containsKey("1284_device_id"))  { printer.ppdModel        = printerProps.getProperty("1284_device_id"); }
      if (printerProps.containsKey("status"))          { printer.knownStatus     = printerProps.getProperty("status"); }
      if (printerProps.containsKey("MFG"))             { printer.manufacturer    = printerProps.getProperty("MFG"); }
      if (printerProps.containsKey("mac"))             { printer.mac             = printerProps.getProperty("mac"); }

      if (printerProps.containsKey("is_supported")) {
//            mwp.logD(TAG, "is_supported Sandeep :" + printerProps.getProperty("is_supported"));

            if("1".equals(printerProps.getProperty("is_supported"))) {
              printer.is_supported = true;
            } else {
              printer.is_supported = false;
            }

//          printer.is_supported    = printerProps.getProperty("is_supported");
      }

      // TODO: knownEzStatus

      result.add(printer);
    }

    //mwp.logD(TAG, "-----------------------------CoreApi-getSortedPrinters,result " + result);
    return result;
  }

//  /**
//   *  Register as the print manager.
//   */
//  public void registerAsPrintManager(Host.PrintManager manager) {
//    host.registerAsPrintManager(manager);
//  }
//
//  public boolean print(String ip, String assetUri) {
//    return controller.print(ip, assetUri);
//  }

  //public static class PrinterStats extends Util.JsObject {
  public static class PrinterStats {

    public String ip;
    public String name;

    public PrinterStats(String ip, String name) {
      mwp.logD(TAG, "-----------------------------CoreApi::PrinterStats1");

      this.ip   = ip;
      this.name = name;
    }

//    public PrinterStats(Printer printer) {
//      mwp.logD(TAG, "-----------------------------CoreApi::PrinterStats2");
//      super(printer.name);
//
//      attributes.put("ip", printer.ip);
//      put("score", 0);
//    }
//
//    public PrinterStats(String ip, String name, int score) {
//      mwp.logD(TAG, "-----------------------------CoreApi::PrinterStats3");
//      super(name);
//
//      attributes.put("ip", ip);
//      put("score", score);
//    }

//    public String getId()      { return "ID"; /*getAttr("ip");*/ }
    public String getIp()      { return ip; }
//    public int    getScore()   { return 0; /*getInt("score");*/ }

//    public String toString() {
////      return getName() + " " + getIp();
//      return "";
//    }
  }

//  //public static ArrayList<CoreApi.PrinterStats> sort(ArrayList<CoreApi.PrinterStats> printerList) {
//  public void sort(ArrayList<CoreApi.PrinterStats> printerList) {
//    Collections.sort(printerList, CoreApi.printerStatsComparator);
//  }
//
//  public static final Comparator<PrinterStats> printerStatsComparator = new Comparator<PrinterStats>() {
//    Collator collator = Collator.getInstance();
//
//    @Override
//    public int compare(PrinterStats a, PrinterStats b) {
//      int aScore = a.getInt("score");
//      int bScore = b.getInt("score");
//
//      if (aScore == bScore) {
//        return collator.compare(a.get("name"), b.get("name"));
//      }
//
//      /* otherwise */
//      if (aScore < bScore) {
//        return 1;
//      }
//
//      /* otherwise */
//      return -1;
//    }
//  };

  public static interface PrintJobInterface {
    public boolean prePrint();
    public boolean print(PrinterStats item) throws Exception;
  }

//  public static interface AsyncAllocateJob {
//    public void resolved(boolean err, Job job);
//  }

  public static class PrintJob implements PrintJobInterface {

//    public                 JSONObject                   attributes;

    protected              String                       urlStr;
    protected              String                       token;
    protected              Job                          job;

    protected              boolean                      haveDonePrePrint;

    public PrintJob() {
      _init();
    }

    public PrintJob(String str) {
      _init();

      if (!str.startsWith("http")) {
        this.token  = str;
      } else {
        this.urlStr = str;
      }
    }

    public boolean prePrint() {

      if (haveDonePrePrint) {
        return true;
      }

//      if (token != null) {
//        api.prePrintPin(token, new AsyncAllocateJob() {
//          public void resolved(boolean err, Job job_) {
//            haveDonePrePrint = true;
//            job = job_;
//          }
//        });
//
//      } else
      if (urlStr != null) {
        job = api.prePrint(urlStr);
        haveDonePrePrint = true;
      }

      return true;
    }

    public boolean print(PrinterStats item) throws Exception {

      if (!prePrint()) { return false; }

      if (token != null) {
        return api.print(item.getIp(), job, token);
      }

      /* otherwise */
      if (urlStr != null) {
//        this.attributes = api.printAttributesFor(urlStr);
//        return api.print(item.getIp(), job, attributes);
        return api.print(item.getIp(), job, urlStr);
      }

      return false;
    }

    protected void _init() {
      this.urlStr               = null;
      this.token                = null;
      this.job                  = null;
      this.haveDonePrePrint     = false;
//      this.attributes           = null;
    }
  }

  private static  Pattern urlprintPattern = Pattern.compile("http.*urlprint\\.printfromtheweb\\.net/urlprint/([^/]+)/(.*)");

  //public static class Job extends Util.JsObject {
  public static class Job {

    public String getJobId()      { return "ID"; /*getAttr("jobId");*/ }
    public String getAssetUri()   { return "URI"; /*getAttr("assetUri");*/ }

    public boolean canRequest() {
//      return getBoolean("canRequest") == true;
      return false;
    }

    public Job(String jobId) {
//      super(jobId);
//      put("jobId", jobId);
//      put("canRequest", true);
    }

    public Job(String jobId, boolean waitForUpload) {
//      super(jobId);
//      put("jobId", jobId);

//      if (!waitForUpload) {
//        put("canRequest", true);
//      } else {
//        put("canRequest", "unknown");
//      }
    }

    public Job() {
//      super("");
//      put("jobId", "");
//      put("canRequest", false);
    }

    public void setAssetUri(String assetUri) {
      Matcher matcher = urlprintPattern.matcher(assetUri);
      if (matcher.find()) {
//        put("assetUri", matcher.group(1) + "://" + matcher.group(2));
        return;
      }
//      put("assetUri", assetUri);
    }

  }

//  /**
//   *  Wait until the boolean becomes either true or false.
//   */
//  public boolean waitFor(Job job, String key, int timeout) {
//    int countDown = 20 * timeout;
//
//    while (job.isUnknown(key)) {
//      host.sleep(50);
//
//      // Don't get stuck in an infinite loop
//      if (countDown-- < 0) {
//        host.log_w(TAG, "Waiting too long for " + key);
//        if (countDown < -(20 * 60)) {
//          return false;
//        }
//      }
//    }
//
//    return job.getBoolean(key);
//  }
//
//
//
//  // ---------- TODO: These two are needed only by the desktop_debug_platform ----------
//  public String status(String ip) {
//    return controller.status(ip, "");
//  }
//
//  public void scan(final boolean justScanning_) {
//    controller.scan(justScanning_);
//  }
//
//
//
//
//  // ---------- private TODO: remove ----------
//
//  /**
//   *  Print a local photo
//   */
////  public boolean printPhoto(InputStream inStream, String filename) {
////    return controller.printPhoto(inStream, filename);
////  }
//
//
////  public boolean print(String assetUri, JSONObject attributes) {
////    return controller.print(assetUri, attributes);
////  }
//
////  public boolean printPhoto(InputStream inStream) {
////    return controller.printPhoto(inStream);
////  }
//
////  public boolean printPhoto(InputStream inStream, JSONObject attributes) {
////    return controller.printPhoto(inStream, attributes);
////  }
//
////  public boolean printPhoto(InputStream inStream, String filename, JSONObject attributes) {
////    return controller.printPhoto(inStream, filename, attributes);
////  }
//
////  public boolean print(int index, String assetUri) {
////    return controller.print(index, assetUri);
////  }
//
////  public boolean print(String ip, String assetUri, JSONObject attributes) {
////    return controller.print(ip, assetUri, attributes);
////  }
//
////  public boolean printPhoto(String ip, InputStream inStream, String filename) {
////    return controller.printPhoto(ip, inStream, filename);
////  }
//
////  public boolean printPhoto(int index, InputStream inStream, String filename) {
////    return controller.printPhoto(index, inStream, filename);
////  }
//
////  private void onScanDone() {
////    controller.onScanDone();
////  }
////
////  private void addPrinter(XJSONObject o) {
////    controller.addPrinter(o);
////  }
////
////  private void addPrinter(String ip, int port, String name) {
////    controller.addPrinter(ip, port, name);
////  }
////
////  private void addPrinter(Printer printer) {
////    controller.addPrinter(printer);
////  }

}



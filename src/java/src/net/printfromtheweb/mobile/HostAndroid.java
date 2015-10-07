/**
 *  Android-centric host.
 *
 *  Notable:
 *
 *  * JSON that is sent to httpPost is not POSTed immediately.  It is enqueued, and 
 *    posted in a known-to-be-a-non-UI thread.
 */

package net.printfromtheweb.mobile;

import net.mobilewebprint.Client;

//import net.printfromtheweb.mobile.Util;
//import net.printfromtheweb.mobile.Util.Js;
//import net.printfromtheweb.mobile.Util.Js.*;
//
//import org.json.*;
//
//import org.apache.http.*;
//import org.apache.http.util.*;
//import org.apache.http.client.methods.*;
//import org.apache.http.Header;
//import org.apache.http.entity.*;
//import org.apache.http.entity.mime.*;
////import org.apache.http.entity.mime.Header;
//import org.apache.http.entity.mime.content.*;
//import org.apache.http.impl.client.DefaultHttpClient;
//
//import android.os.*;
//import android.net.*;
import android.net.wifi.*;
//import android.net.wifi.WifiManager.MulticastLock;
import android.content.Context;
import android.net.ConnectivityManager.*;
//import android.util.Log;
//import android.accounts.*;
//import android.support.v4.content.AsyncTaskLoader;
//import android.widget.*;
//import android.content.*;
//
//import java.net.*;
//import java.io.*;
//import java.util.*;
//import java.util.concurrent.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
//import java.util.regex.*;

public class HostAndroid extends Host {

//  private static final String                       TAG             = "jMobileWebPrint";
//
//  protected boolean                                 usingUploader;
//  protected DefaultHttpClient                       client;

  protected Context                                 context;
  protected WifiManager                             wifiManager;
  private   ExecutorService                         executor;

////  private   Host.PrinterListObserver                printerListChangedObserver;
//
//  private   JSONObject                              httpExtraInfo;
//  private   ConcurrentLinkedQueue<HttpJsonPostData> postPayloads;
//
//  private   MulticastLock                           lock;

  /**
   *  Constructor.
   */
  public HostAndroid(Context context) {
//  public HostAndroid(Context context) {
//    Log.d(TAG, "HostAndroid ctor");

//    this.client                     = new DefaultHttpClient();
//    this.usingUploader              = false;
//
    this.context                    = context;
    this.wifiManager                = (WifiManager) context.getSystemService(android.content.Context.WIFI_SERVICE);
    this.executor                   = Executors.newFixedThreadPool(8);
////    this.printerListChangedObserver = null;
//    this.lock                       = null;
//
//    getPrimaryAccount();
//
//    httpExtraInfo = new JSONObject();
//    Js.put(httpExtraInfo, "platform", "rawAndroid");
//    Js.put(httpExtraInfo, "version", "0.0.1");
//    Js.put(httpExtraInfo, "user", primaryAccount);
//
//    this.postPayloads = new ConcurrentLinkedQueue<HttpJsonPostData>();
    //start();

//    log_d(TAG, "TempDir: " + context.getCacheDir());
//    log_d(TAG, "XTempDir: " + context.getExternalCacheDir());
//    log_d(TAG, "TempDirs: " + context.getExternalFilesDir(null));
//    //log_d(TAG, "TempDirss: " + context.getExternalFilesDirs(null));
  }

  /**
   *  Start the host up.
   *
   *  Starts the thread that uploads in the background.
   */
  public void start() {

    // Start the base Host
    super.start();

    // Start a new thread for the background post of JSON
    getThreadPool().execute(new Runnable() {
      public void run() {

//        turnWiFiOn();
//
//        HttpJsonPostData data = null;
//
//        while (true) {
//          if ((data = postPayloads.poll()) == null) {
//            sleep(100);
//            continue;
//          }
//
//          /* otherwise */
//          _httpPost(data);
//        }
      }
    });
  }

//  // Override this, so nothing gets sent
//  public void sendTelemetry() {
//    // Dont do anything for now
//  }

  public static class PrintJobAndroid extends CoreApi.PrintJob {

//    protected              Uri                          uri;
    public                 String                       intentType;

    public PrintJobAndroid() {
      super();
      _android_init();
    }

//    public PrintJobAndroid(Uri uri) {
//      super();
//      _android_init();
//
//      this.uri    = uri;
//    }

    public PrintJobAndroid(String str) {
      super(str);
      _android_init();
    }

    public PrintJobAndroid(String str, String intentType) {
      super(str);
      _android_init();

      this.intentType = intentType;
    }

//    public boolean prePrint(ContentResolver resolver) {
//
//      if (haveDonePrePrint) {
//        return true;
//      }
//
//      // Is it a URI?  -- a binary file on the device
//      if (uri != null) {
//        job = prePrintPhoto(resolver);
//        haveDonePrePrint = true;
//        return true;
//      }
//
//      /* otherwise */
//      return super.prePrint();
//    }

    public boolean print(CoreApi.PrinterStats item) throws Exception {

      if (!prePrint()) { return false; }

//      // Is it a URI?  -- a binary file on the device
//      if (uri != null) {
//        return SecureAssetPrintingApi.api.printPhoto(item.getIp(), job);
//      }

      /* otherwise */
      return super.print(item);
    }

//    protected CoreApi.Job prePrintPhoto(ContentResolver resolver) {
//      String filename = Util.fname(uri.toString());
//
//      try {
//        return SecureAssetPrintingApi.api.prePrintPhoto(resolver.openInputStream(uri), filename);
//      } catch (Exception e) {
//        Log.e(TAG, "Error getting uri", e);
//      }
//      return null;
//    }

    protected void _android_init() {
//      this.uri                  = null;
      this.intentType           = "";
    }
  }

//  public CoreApi.PrintJobInterface mkPrintJob() {
//    return new PrintJobAndroid();
//  }
//
//  public CoreApi.PrintJobInterface mkPrintJob(String str) {
//    return new PrintJobAndroid(str);
//  }
//
//  public CoreApi.PrintJobInterface mkPrintJob(String str, String intentType) {
//    return new PrintJobAndroid(str, intentType);
//  }
//
//  public CoreApi.PrintJobInterface mkPrintJob(Uri uri) {
//    return new PrintJobAndroid(uri);
//  }
//
//  HttpResponse uploaderResult;
//  private HttpResponse waitForUploader(final HttpUriRequest verb) {
//    while (usingUploader) {
//      log_v(TAG, "waiting for uploader so I can upload: " + verb.getURI());
//      sleep(250);
//    }
//
//    usingUploader = true;
//    uploaderResult = null;
//    SecureAssetPrintingApi.api.hoggingNetwork();
//
//    log_v(TAG, "Starting upload of: " + verb.getURI());
//    getThreadPool().execute(new Runnable() {
//      public void run() {
//
//        try {
//          uploaderResult = client.execute(verb);
//        } catch(IOException e) {
//          //log_e(TAG, "Error posting", e);
//        } finally {
//          usingUploader = false;
//          SecureAssetPrintingApi.api.hoggingNetwork(false);
//        }
//      }
//    });
//
//    while (usingUploader) {
//      log_v(TAG, "waiting for upload to finish for: " + verb.getURI());
//      sleep(250);
//    }
//    log_v(TAG, "Upload finished for: " + verb.getURI());
//
//    HttpResponse result_ = uploaderResult;
//    uploaderResult = null;
//    return result_;
//  }
//
//  /**
//   *  Post the JSON to the path.
//   *
//   *  In reality, this just enqueues the JSON.
//   */
//  public void httpPost(String path, JSONObject callerData) {
//
//    turnWiFiOn();
//
//    // Augment JSON
//    JSONObject reqBody = Js.newJSONObject(callerData);
//    Js.put(reqBody, "meta", httpExtraInfo);
//
//    postPayloads.add(new HttpJsonPostData(path, reqBody));
//  }
//
//  /**
//   *  Post the JSON to the path.
//   *
//   *  In reality, this just enqueues the JSON.
//   */
//  public void httpPost(String path, JSONArray callerData) {
//
//    turnWiFiOn();
//
//    // Augment JSON
//    JSONObject reqBody = Js.o("items", callerData);
//    Js.put(reqBody, "meta", httpExtraInfo);
//
//    postPayloads.add(new HttpJsonPostData(path, reqBody));
//  }
//
//  /**
//   *  post the JSON.
//   *
//   *  This is the function that actually does the POST.
//   */
//  private void _httpPost(HttpJsonPostData data) {
//
//    turnWiFiOn();
//
//    try {
//
//      // Prep for HTTP
//      String url = pclServerUrl(data.path);
//
//      HttpPost httppost = new HttpPost(url);
//      httppost.setEntity(new StringEntity(data.body));
//      httppost.setHeader("Content-type", "application/json");
//      waitForUploader(httppost).getEntity();
//    } catch(IOException e) {
//      log_e(TAG, "Error posting to " + data.path, e);
//    } catch(Exception e){
//      log_e(TAG, e.getMessage(), e);
//    }
//  }
//
//  /**
//   *  Holds the path and JSON while it is in the POST queue.
//   */
//  private static class HttpJsonPostData {
//    public String path;
//    public String body;
//
//    public HttpJsonPostData(String path, JSONObject body) {
//      this.path = path;
//      this.body = body.toString();
//    }
//  }
//
//  /**
//   *  Send a JSON object to the server, get JSON back.
//   *
//   *  OK, that's not really RPC, but you get the idea.
//   */
//  public JSONObject httpJsonRpc(String protocol_, String pclServerName_, int port_, String path, JSONObject callerData) {
//
//    turnWiFiOn();
//
//    // Augment JSON
//    JSONObject reqBody = Js.newJSONObject(callerData);
//    Js.put(reqBody, "meta", httpExtraInfo);
//
//    try {
//
//      // Prep for HTTP
//      String url = protocol_ + "://" + pclServerName_ + ":" + port_ + path;
//      HttpPost httppost = new HttpPost(url);
//      httppost.setEntity(new StringEntity(reqBody.toString()));
//      httppost.setHeader("Content-type", "application/json");
//
//      HttpResponse response = waitForUploader(httppost);
//      if (response == null) { return null; }
//
//      /* otherwise */
//      int statusCode = response.getStatusLine().getStatusCode();
//      JSONObject result = Js.o("_http_response_code_", statusCode, "_success_", false);
//
//      if (statusCode >= 200 && statusCode < 300) {
//
//        Js.extend(result, Js.o("_success_", true));
//
//        HttpEntity body = response.getEntity();
//        String jsonBody = EntityUtils.toString(body);
//        if (body != null) {
//          Js.extend(result, new JSONObject(jsonBody));
//        }
//        //log_v(TAG, "result " + result.toString() + " :: " + jsonBody);
//
//        return result;
//      }
//
//      /* otherwise */
//
//      // We got a response from the server, but not a success response code (we
//      // probably got a 404)
//      return result;
//
//    } catch(Exception e) {
//      log_e(TAG, "Error posting to " + path, e);
//    }
//
//    return null;
//  }
//
//  /**
//   *  Send a JSON object to the server, get JSON back.
//   *
//   *  OK, that's not really RPC, but you get the idea.
//   */
//  public JSONObject httpJsonRpc(String path, JSONObject callerData) {
//    String url = pclServerUrl(path);// ****** This line is needed to ensure the vars: protocol, pclServerName, and port ******
//    return httpJsonRpc(protocol, pclServerName, port, path, callerData);
//  }
//
//  /**
//   *  Send a JSON object to the server, get JSON back.
//   *
//   *  OK, that's not really RPC, but you get the idea.
//   */
//  public JSONObject httpJsonRpc(String path) {
//    return httpJsonRpc(path, new JSONObject());
//  }
//
//  /**
//   *  Send a JSON object to the server, get JSON back, using a new non-shared HttpClient.
//   *
//   *  OK, that's not really RPC, but you get the idea.
//   */
//  public JSONObject httpJsonRpcSync(String path, JSONObject callerData) {
//
//    turnWiFiOn();
//
//    // Augment JSON
//    JSONObject reqBody = Js.newJSONObject(callerData);
//    Js.put(reqBody, "meta", httpExtraInfo);
//
//    try {
//
//      SecureAssetPrintingApi.api.hoggingNetwork();
//      DefaultHttpClient   client   = new DefaultHttpClient();
//
//      // Prep for HTTP
//      String url = pclServerUrl(path);
//      HttpPost httppost = new HttpPost(url);
//      httppost.setEntity(new StringEntity(reqBody.toString()));
//      httppost.setHeader("Content-type", "application/json");
//
//      HttpResponse response = client.execute(httppost);
//      int statusCode = response.getStatusLine().getStatusCode();
//
//      if (statusCode >= 200 && statusCode < 300) {
//        HttpEntity body = response.getEntity();
//        if (body != null) {
//          return new JSONObject(EntityUtils.toString(body));
//        }
//      }
//
//    } catch(Exception e) {
//      log_e(TAG, "Error posting to " + path, e);
//
//    } finally {
//      SecureAssetPrintingApi.api.hoggingNetwork(false);
//    }
//
//    return null;
//  }
//
//  /**
//   *  Upload a file as if it was being uploaded from a HTML form via multi-part file upload.
//   *
//   *  Taken from http://stackoverflow.com/questions/18964288/upload-a-file-through-an-http-form-via-multipartentitybuilder-with-a-progress
//   *
//   *  I took this code from the above-mentioned SO article.  I do not understand the need for the extra objects, but this code works well.
//   *
//   *  TODO: also upload otherFormData
//   */
//  public JSONObject httpUploadMultipartForm(String path, String fileName, InputStream is, JSONObject otherFormData) throws Exception {
//
//    turnWiFiOn();
//
//    String url = pclServerUrl(path);
//    HttpPost post = new HttpPost(url);
//    MultipartEntityBuilder builder = MultipartEntityBuilder.create();
//    builder.setMode(HttpMultipartMode.BROWSER_COMPATIBLE);
//
//    builder.addPart("file", new InputStreamBody(is, fileName));
//    //builder.addTextBody("userName", userName);
//    //builder.addTextBody("password", password);
//    //builder.addTextBody("macAddress",  macAddress);
//    final HttpEntity yourEntity = builder.build();
//
//    class ProgressiveEntity implements HttpEntity {
//      @Override
//      public void consumeContent() throws IOException {
//        yourEntity.consumeContent();
//      }
//      @Override
//      public InputStream getContent() throws IOException,
//            IllegalStateException {
//        return yourEntity.getContent();
//      }
//      @Override
//      public Header getContentEncoding() {
//        return yourEntity.getContentEncoding();
//      }
//      @Override
//      public long getContentLength() {
//        return yourEntity.getContentLength();
//      }
//      @Override
//      public Header getContentType() {
//        return yourEntity.getContentType();
//      }
//      @Override
//      public boolean isChunked() {
//        return yourEntity.isChunked();
//      }
//      @Override
//      public boolean isRepeatable() {
//        return yourEntity.isRepeatable();
//      }
//      @Override
//      public boolean isStreaming() {
//        return yourEntity.isStreaming();
//      } // CONSIDER put a _real_ delegator into here!
//
//      @Override
//      public void writeTo(OutputStream outstream) throws IOException {
//
//        class ProxyOutputStream extends FilterOutputStream {
//          /**
//           * @author Stephen Colebourne
//           */
//
//          public ProxyOutputStream(OutputStream proxy) {
//            super(proxy);    
//          }
//          public void write(int idx) throws IOException {
//            out.write(idx);
//          }
//          public void write(byte[] bts) throws IOException {
//            out.write(bts);
//          }
//          public void write(byte[] bts, int st, int end) throws IOException {
//            out.write(bts, st, end);
//          }
//          public void flush() throws IOException {
//            out.flush();
//          }
//          public void close() throws IOException {
//            out.close();
//          }
//        } // CONSIDER import this class (and risk more Jar File Hell)
//
//        class ProgressiveOutputStream extends ProxyOutputStream {
//          public ProgressiveOutputStream(OutputStream proxy) {
//            super(proxy);
//          }
//          public void write(byte[] bts, int st, int end) throws IOException {
//
//            // FIXME  Put your progress bar stuff here!
//            Log.d(TAG, "Uploaded " + (end - st) + " bytes.");
//
//            out.write(bts, st, end);
//          }
//        }
//
//        yourEntity.writeTo(new ProgressiveOutputStream(outstream));
//      }
//
//    };
//
//    HttpEntity    body       = null;
//    String        bodyString = null;
//    HttpResponse  response   = null;
//    try {
//
//      ProgressiveEntity myEntity = new ProgressiveEntity();
//
//      post.setEntity(myEntity);
//      response = waitForUploader(post);
//
//      int statusCode = response.getStatusLine().getStatusCode();
//
//      if (statusCode >= 200 && statusCode < 300) {
//        body = response.getEntity();
//        if (body != null) {
//          bodyString = EntityUtils.toString(body);
//          return new JSONObject(bodyString);
//        }
//      }
//    } catch (Exception e) {
//      log_e(TAG, "Error uploading file: " + fileName + " to: " + path, e);
//      log_e(TAG, "Response: " + response);
//      log_e(TAG, "Response: " + body);
//      log_e(TAG, "Response: " + bodyString);
//    }
//    return null;
//  } 
//
//  /**
//   *  Do an HTTP GET, and write the response into the file stream.
//   *
//   *  Does not use a shared HttpClient, has its own.
//   */
//  public boolean httpGet_and_writeTo(String path, FileOutputStream stream) throws IOException {
//
//    turnWiFiOn();
//
//    SecureAssetPrintingApi.api.hoggingNetwork();
//
//    String jobUrl = pclServerUrl(path);
//    DefaultHttpClient   client   = new DefaultHttpClient();
//
//    HttpResponse response = client.execute(new HttpGet(jobUrl));
//
//    SecureAssetPrintingApi.api.hoggingNetwork(false);
//
//    int statusCode = response.getStatusLine().getStatusCode();
//    if (statusCode >= 200 && statusCode < 300) {
//      HttpEntity body = response.getEntity();
//      body.writeTo(stream);
//      return true;
//    }
//
//    return false;
//  }
//
//  /**
//   *  What is this device's primary account.
//   *
//   *  Used for ID.
//   */
//  private void getPrimaryAccount() {
//    primaryAccount = Util.randomString(64);
//
//    try {
//      for (Account account: AccountManager.get(context).getAccounts()) {
//        if (account.name.matches(".*@gmail\\.com") && account.type.matches(".*google.*")) {
//          primaryAccount = account.name;
//          break;
//        } else if (account.name.matches(".*@.*\\.(com|net|org|gov|us)")) {
//          primaryAccount = account.name;
//        }
//      }
//    } catch (Exception e) {}
//  }
//
//  public String getTempDir() {
//    return context.getFileStreamPath("").toString();
//  }
//
//  /**
//   *  Get a temp file (stream) to write to.
//   */
//  public FileOutputStream getTempFileForWriting(String tmpDir, String name) throws FileNotFoundException {
//    return context.openFileOutput(name, Context.MODE_PRIVATE);
//  }
//
//  /**
//   *  Get a read-stream from a file.
//   *
//   *  Should be used in conjunction with getTempFileForWriting()
//   */
//  public FileInputStream getTempFileForReading(String tmpDir, String name) throws FileNotFoundException {
//    return context.openFileInput(name);
//  }
//
//  /**
//   *  What is this host's IP address?
//   */
//  public InetAddress getDeviceIpAddress() {
//
//    InetAddress result = null;
//    try {
//      // default to Android localhost
//      result = InetAddress.getByName("10.0.0.2");
//
//      // figure out our wifi address, otherwise bail
//      WifiInfo wifiinfo = wifiManager.getConnectionInfo();
//      int intaddr = wifiinfo.getIpAddress();
//      byte[] byteaddr = new byte[] { (byte) (intaddr & 0xff), (byte) (intaddr >> 8 & 0xff), (byte) (intaddr >> 16 & 0xff), (byte) (intaddr >> 24 & 0xff) };
//      result = InetAddress.getByAddress(byteaddr);
//    } catch (UnknownHostException ex) {
//      log_e(TAG, String.format("getDeviceIpAddress Error: %s", ex.getMessage()), ex);
//    }
//
//    //log_v(TAG, "getting device IP: " + result.toString());
//    return result;
//  }
//
//  /**
//   *  A unique string for this user.
//   */
//  private String userUnique(String str) {
//    return primaryAccount + "-" + str;
//  }

  /**
   *  Get the thread pool, so caller can start a thread.
   */
  public ExecutorService getThreadPool() {
    return executor;
  }

//  /**
//   * Try to extract a hardware MAC address from a given IP address using the
//   * ARP cache (/proc/net/arp).<br>
//   * <br>
//   * We assume that the file has this structure:<br>
//   * <br>
//   * IP address       HW type     Flags       HW address            Mask     Device
//   * 192.168.18.11    0x1         0x2         00:04:20:06:55:1a     *        eth0
//   * 192.168.18.36    0x1         0x2         00:22:43:ab:2a:5b     *        eth0
//   *
//   * @return the MAC from the ARP cache
//   */
//  public Map<String, String> getMacsFromArpCache() {
//
//    Map<String, String> map = new HashMap<String, String>();
//
//    String    search = "[^0-9]*([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+).*([0-9a-fA-F][0-9a-fA-F]:..:..:..:..:[0-9a-fA-F][0-9a-fA-F]).*";
//    Pattern   macPattern = Pattern.compile(search);
//
//    String          line;
//    BufferedReader  br = null;
//    try {
//      br = new BufferedReader(new FileReader("/proc/net/arp"));
//      while ((line = br.readLine()) != null) {
//        Log.d(TAG, "-------------" + line);
//
//        Matcher m = macPattern.matcher(line);
//        if (m.find()) {
//          map.put(m.group(1), m.group(2));
//        }
//      }
//    } catch (Exception e) {
//      e.printStackTrace();
//    } finally {
//      try {
//        br.close();
//      } catch (IOException e) {
//        e.printStackTrace();
//      }
//    }
//
//    return map;
//  }
//
//  /**
//   *  Sleep.
//   *
//   *  Allow other threads execution time, while the caller is waiting for something to
//   *  happen.
//   *
//   *  TODO: Should be moved to Host?
//   */
//  public void sleep(int ms) {
//    try {
//      Thread.sleep(ms);
//    } catch (InterruptedException e) {}
//  }
//
//  /**
//   *  The app is going to scan.
//   *
//   *  Android must be kicked into multi-cast mode.
//   */
//  public void prepForScan(String name) {
//    turnWiFiOn();
//
//    // Then, lock us into multicast mode
//    lock = wifiManager.createMulticastLock(name);
//    lock.setReferenceCounted(true);
//    lock.acquire();
//  }
//
//  /**
//   *  The app is done scanning.
//   *
//   *  Release the multi-cast lock.
//   */
//  public void scanFinished() {
//    if (lock.isHeld()) {
//      lock.release();
//      lock = null;
//    }
//  }
//
//  public void turnWiFiOn() {
//    SupplicantState sstate    = null;
//    InetAddress     myAddress = null;
//
//    // First, make sure we are on the WiFi
//    boolean ok = false, haveCalledEnable = false;
//    while (!ok) {
//
//      if (!wifiManager.isWifiEnabled() && !haveCalledEnable) {
//        //Toast.makeText(context, "Starting WiFi...", Toast.LENGTH_SHORT).show();
//        wifiManager.setWifiEnabled(true);
//        haveCalledEnable = true;
//      }
//
//      if (wifiManager.isWifiEnabled()) {
//        sstate = wifiManager.getConnectionInfo().getSupplicantState();
//        if (sstate.equals(SupplicantState.ASSOCIATED) || sstate.equals(SupplicantState.COMPLETED)) {
//
//          myAddress = getDeviceIpAddress();
//          if (!myAddress.isAnyLocalAddress() &&    // The "any" address 0.0.0.0
//              !myAddress.isLoopbackAddress())      // A loopback address, like 127.0.0.1
//          {
//            ok = true;
//            break;
//          }
//        }
//      }
//
//      log_v(TAG, "Waiting for WiFi network: enabled: " + wifiManager.isWifiEnabled() + " " + wifiManager.getConnectionInfo());
//      sleep(1000);
//    }
//  }
//
//  /**
//   *  Are we on a background thread?
//   */
//  public boolean isBgThread() {
//    return isBgThread(false);
//  }
//
//  /**
//   *  Are we on a background thread?
//   */
//  public boolean isBgThread(boolean quiet) {
//    boolean isBg = (Looper.getMainLooper().getThread() != Thread.currentThread());
//    if (quiet) {
//      return isBg;
//    }
//
//    //Log.v(TAG, "Checking isBgThread: " + Thread.currentThread() + " | " + Looper.getMainLooper().getThread());
//    if (!isBg) {
//      try {
//        throw new Exception("You're in the UI thread!!!!");
//      }catch(Exception e) {
//        Log.e(TAG, "Not bg", e);
//      }
//    }
//
//    return isBg;
//  }
//
//
//  // Logging: Send to Android logging
//  public void log_d(String tag, String msg, String shortMsg) {
//    Log.d(tag, msg);
//  }
//
//  public void log_d(String tag, String msg) {
//    Log.d(tag, msg);
//  }
//
//  public void log_e(String tag, String msg, Exception ex) {
//    Log.e(tag, msg, ex);
//  }
//
//  public void log_e(String tag, String msg) {
//    Log.e(tag, msg);
//  }
//
//  public void log_w(String tag, String msg) {
//    Log.w(tag, msg);
//  }
//
//  public void log_v(String tag, String msg) {
//    Log.v(tag, msg);
//  }
//
//  public void displayPrinter(String pre, Printer printer) {
//    println(printer.toString());
//  }
//
//  public void println(String msg) {
//    log_d(TAG, msg);
//  }
//
//  public void print(String msg) {
//    log_d(TAG, msg);
//  }
//
//  public void showStack(String msg) {
//    try {
//      throw new Exception("Showing stack");
//    }catch(Exception e) {
//      Log.e(TAG, msg, e);
//    }
//  }
}


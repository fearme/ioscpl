package net.mobilewebprint;

import java.util.Properties;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.Map;
import java.util.HashMap;

import android.os.Handler;
import android.os.Looper;
import android.os.Build;
import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.*;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.net.wifi.WifiInfo;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

public class Client {

  private static final String   TAG = "jMobileWebPrint.Client";

  public               Context  context;
  public  NetworkStateReceiver  networkStateReceiver;
  public       ExecutorService  threadPool;
  public           WifiManager  wifiManager;
  public         MulticastLock  lock;

  public Client(net.mobilewebprint.Application application_)
  {
    this.application            = application_;
    this.application.mwp_client = this;
    this.context                = null;
    this.networkStateReceiver   = new NetworkStateReceiver();
    this.threadPool             = Executors.newFixedThreadPool(2);
    this.wifiManager            = (WifiManager) context.getSystemService(android.content.Context.WIFI_SERVICE);
    this.lock                   = null;

    initJni(application);
  }

  public boolean RegisterPrinterAttributeChangesListener(PrinterAttributeChangesListener listener)
  {
    return application.RegisterPrinterAttributeChangesListener(listener);
  }

  public boolean RegisterPrinterListChangesListener(PrinterListChangesListener listener)
  {
    return application.RegisterPrinterListChangesListener(listener);
  }

  public boolean RegisterPrintProgressChangesListener(PrintProgressChangesListener listener)
  {
    return application.RegisterPrintProgressChangesListener(listener);
  }

  public ArrayList<Properties> getSortedPrinterList(boolean filterUnsupported)
  {
    return application.getSortedPrinterList(filterUnsupported);
  }

  public boolean start()
  {

    context.registerReceiver(networkStateReceiver, new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION));
    context.registerReceiver(networkStateReceiver, new IntentFilter("android.net.wifi.WIFI_STATE_CHANGED"));

    // Then, lock us into multicast mode
    lock = wifiManager.createMulticastLock("mwp_multicast_lock");
    lock.setReferenceCounted(true);
    lock.acquire();

    // This is what to do if you ever stop scanning
    //if (lock != null && lock.isHeld()) {
    //  lock.release();
    //  lock = null;
    //}

    boolean result = startUp();

    // Do some work 5 seconds after startup
    runDelayed(5000, new Runnable() {
      public void run() {
        ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo[] nis = cm.getAllNetworkInfo();

        Map<String, String> item;
        JSONObject infos = new JSONObject();

        for (int i = 0; i < nis.length; ++i) {
          item = new HashMap<String, String>();

          _setItemProperty(item, "state",          nis[i].getState().toString());
          _setItemProperty(item, "detailedState",  nis[i].getDetailedState().toString());
          //_setItemProperty(item, "extraInfo",      nis[i].getExtraInfo());
          _setItemProperty(item, "type",           nis[i].getTypeName());
          _setItemProperty(item, "subtype",        nis[i].getSubtypeName());
          _setItemProperty(item, "available",      nis[i].isAvailable() ? "true" : "false");
          _setItemProperty(item, "connected",      nis[i].isConnected() ? "true" : "false");

          sendImmediately("telemetry/network/wifiState", new JSONObject(item).toString().replace("\"", "'"));
        }

        WifiManager  wifiManager = (WifiManager) context.getSystemService(android.content.Context.WIFI_SERVICE);
        sendImmediately("telemetry/network/wifiIsEnabled", "" + wifiManager.getWifiState());

        WifiInfo info = wifiManager.getConnectionInfo();
        sendImmediately("telemetry/network/bssid", "" + info.getBSSID());
        sendImmediately("telemetry/network/ssid", "" + info.getSSID());
        sendImmediately("telemetry/network/clientMac", "" + info.getMacAddress());
        sendImmediately("telemetry/network/rssi", "" + info.getRssi());

        JSONObject build = new JSONObject();
        try {
          build.put("MANUFACTURER", Build.MANUFACTURER);
          build.put("MODEL", Build.MODEL);
          build.put("PRODUCT", Build.PRODUCT);

          build.put("CPU_ABI", Build.CPU_ABI);
          build.put("CPU_ABI2", Build.CPU_ABI2);

//          for (int i = 0; i < Build.SUPPORTED_ABIS; ++i) {
//            build.put("ABI" + i, Build.SUPPORTED_ABIS[i]);
//          }
        } catch (JSONException e) {
        }
        sendImmediately("telemetry/network/build", build.toString().replace("\"", "'"));

      }
    });

    return result;
  }

  public static void _setItemProperty(Map<String, String> item, String name, String value)
  {
    if (value != null) {
      item.put(name, value);
    }
  }

  public void runDelayed(final long delayMillis, final Runnable r)
  {
    threadPool.execute(new Runnable() {
      public void run() {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(r, delayMillis);
      }
    });

  }

  public native boolean startUp();
  public native boolean reScan();
  public native boolean sendJob(String url, String printer_ip);

  public native boolean sendImmediately(String msgName, String payload);

  public native void        setOption(String name, String value);
  public native void     setIntOption(String name, int value);
  public native void          setFlag(String name, boolean value);
  public native void        clearFlag(String name);

  public static native void    logD(String tag, String message);
  public static native void    logV(String tag, String message);
  public static native void    logW(String tag, String message);
  public static native void    logE(String tag, String message);

  // -----------------------------------------------------------------------------
  protected native void setSecureMode();

  // -----------------------------------------------------------------------------
  protected net.mobilewebprint.Application application;

  private native boolean initJni(net.mobilewebprint.Application application);
  static {
    try {
      System.loadLibrary("mwp");
      //System.loadLibrary("mobilewebprint");
    } catch (UnsatisfiedLinkError e) {
      // Do not ignore on Android
      if (System.getProperty("java.vm.name").equals("Dalvik")) { throw e; }
    }
  }
}


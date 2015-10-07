package net.mobilewebprint;

import java.util.Properties;
import java.util.ArrayList;

import android.content.Context;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.*;

public class Client {

  private static final String   TAG = "jMobileWebPrint.Client";
  public               Context  context;
  public  NetworkStateReceiver  networkStateReceiver;

  public Client(net.mobilewebprint.Application application_)
  {
    this.application            = application_;
    this.application.mwp_client = this;
    this.context                = null;
    this.networkStateReceiver   = new NetworkStateReceiver();

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

    return startUp();
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


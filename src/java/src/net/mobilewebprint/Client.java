package net.mobilewebprint;

import java.util.Properties;
import java.util.ArrayList;

public class Client {

  private static final String TAG = "MobileWebPrint.Client";

  public Client(net.mobilewebprint.Application application_)
  {
    this.application = application_;
    this.application.mwp_client = this;
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

  public native boolean start();
  public native boolean sendJob(String url, String printer_ip);


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


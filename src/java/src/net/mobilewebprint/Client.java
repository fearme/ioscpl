package net.mobilewebprint;

import java.util.Map;
import java.util.HashMap;
import java.util.Set;
import java.util.HashSet;
import java.util.Properties;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

public class Client {

  // The printer list
  protected Semaphore               printersMutex;
  protected Map<String, Properties> printers;
  protected Set<String>             changedPrinterIps;

  public Client() {
  }

  // ---------------------------------------------------------------------------------
  public native boolean setSecureMode();
  public native boolean start();

  public void onBootstrap() {
    setOption("http_proxy_name", System.getProperty("http.proxyHost"));
    setOption("http_proxy_port", System.getProperty("http.proxyPort"));
  }

  public native void        setOption(String name, String value);

  // ---------------------------------------------------------------------------------
  // The application is about to be sent the printer list (or changes to one printer)
  public void onNewPrinterList() {
    //Client.logD("MobileWebPrint", "+++onNewPrinterList");

    _initializeNewPrintersList();
    changedPrinterIps = new HashSet<String>();
  }

  public void onBeginPrinterChanges() {
    try {
      //Client.logD("MobileWebPrint", "+++onBeginPrinterChanges");
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
      changedPrinterIps = new HashSet<String>();
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  public void onPrinterAttribute(String ip, String name, String value) {
    try {
      //Client.logD("MobileWebPrint", "+++onPrinterAttribute " + ip + " " + name + " " + value);
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
      Properties printer = _ensure(ip);

      printer.setProperty(name, value);
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  public void onRemovePrinterAttribute(String ip, String name) {
    try {
      //Client.logD("MobileWebPrint", "+++onRemovePrinterAttribute " + ip + " " + name);
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
      if (printers.containsKey(ip)) {

        Properties printer = printers.get(ip);
        printer.remove(name);
      }
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  public void onRemovePrinter(String ip) {
    try {
      //Client.logD("MobileWebPrint", "+++onRemovePrinter " + ip);
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
      printers.remove(ip);
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  // The enumeration of printers is done
  public void onEndPrinterEnumeration() {
    try {
      //Client.logD("MobileWebPrint", "+++onEndPrinterEnumeration");
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
//      for (PrinterListChangesListener listener : printerListChangesListeners) {
        _onEndPrinterEnumeration();
//      }
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  public ArrayList<Properties> getSortedPrinterList(boolean filterUnsupported) /*throws InterruptedException*/ {
    ArrayList<Properties> result = new ArrayList<Properties>();

    //Client.logD("MobileWebPrint", "******************** gspl");
    //printersMutex.acquire();
    try {
      // TODO: Ensure we are in the right thread

      int score = 1000000;
      while ((score = _nextBestScore(score)) != -1) {
        _addPrintersWithScore(result, score);
      }
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      //printersMutex.release();
      //Client.logD("MobileWebPrint", "88888888888888888888 gspl");
    }

    return result;
  }

  // -----------------------------------------------------------------------------
  // ----------- private ---------------------------------------------------------
  // -----------------------------------------------------------------------------

  private native boolean initJni(Object lifeline);

  static {
    try {
      System.loadLibrary("mwp");
      //System.loadLibrary("mobilewebprint");
    } catch (UnsatisfiedLinkError e) {
      // Do not ignore on Android
      if (System.getProperty("java.vm.name").equals("Dalvik")) { throw e; }
    }
  }

  public void _onEndPrinterEnumeration() {
    if (changedPrinterIps != null) {
      //listener.onBeginPrinterListChanges();

//      listener.onBeginPrinterListChanges();
      for (String ip:changedPrinterIps) {
        if (printers.containsKey(ip)) {
//          listener.onPrinterChanged(ip, printers.get(ip));
        } else {
//          listener.onPrinterRemoved(ip);
        }
      }
//      listener.onEndPrinterListEnumeration();
      changedPrinterIps = null;
    }
  }

  protected void _initializeNewPrintersList() {
    printersMutex = new Semaphore(1, true);
    printers = new HashMap<String, Properties>();
  }

  protected Properties _ensure(String ip) {
    Properties printer;
    if (printers.containsKey(ip)) {
      printer = printers.get(ip);
    } else {
      printer = new Properties();
      printers.put(ip, printer);
    }

    return printer;
  }

  protected int _nextBestScore(int bestScore) {
    int nextBest = -1, currScore = 0;
    for (Map.Entry<String, Properties> ent: printers.entrySet()) {
      currScore = atoi(ent.getValue().getProperty("score", "0"));
      if (currScore > nextBest && currScore < bestScore) {
        nextBest = currScore;
      }
    }

    //Client.logD("MobileWebPrint", "best: " + nextBest);
    return nextBest;
  }

  protected int _addPrintersWithScore(ArrayList<Properties> list, int score) {
    int count = 0;
    int currScore = 0;

    for (Map.Entry<String, Properties> ent: printers.entrySet()) {
      if ((currScore = atoi(ent.getValue().getProperty("score", "0"))) == score) {
        list.add(ent.getValue());
        count += 1;
      }
    }

    return count;
  }

  public static int atoi(String str) {
    if (str == null || str.length() == 0) { return 0; }

    str = str.trim();

    char flag = '+';

    int i = 0;
    if (str.charAt(i) == '-') {
      flag = '-';
      i += 1;
    } else if (str.charAt(i) == '+') {
      i += 1;
    }

    double result = 0.0;

    for (; str.length() > i && str.charAt(i) >= '0' && str.charAt(i) <= '9'; ++i) {
      result = result * 10 + (str.charAt(i) - '0');
    }

    if (flag == '-') {
      result = -result;
    }

    if (result > Integer.MAX_VALUE) { return Integer.MAX_VALUE; }
    if (result < Integer.MIN_VALUE) { return Integer.MIN_VALUE; }

    return (int)result;
  }
}


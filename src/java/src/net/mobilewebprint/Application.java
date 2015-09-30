package net.mobilewebprint;

import java.util.Set;
import java.util.HashSet;
import java.util.Map;
import java.util.HashMap;
import java.util.Properties;
import java.util.ArrayList;
import java.util.concurrent.Semaphore;

public class Application implements PrinterAttributeChangesListener, PrintProgressChangesListener {

  public static final int         JOB_STATE_SUCCESS            = 1;
  public static final int         JOB_STATE_CANCELED           = 2;
  public static final int         JOB_STATE_FAILED             = 3;

  public static final String      STATUS_WAITING0              = "WAITING0";
  public static final String      STATUS_WAITING1              = "WAITING1";
  public static final String      STATUS_PRINTING              = "PRINTING";
  public static final String      STATUS_WAITING2              = "WAITING2";
  public static final String      STATUS_CANCELLED             = "CANCELLED";
  public static final String      STATUS_CANCELLING            = "CANCELLING";
  public static final String      STATUS_SUCCESS               = "SUCCESS";

  public static final String      RAW_STATUS_CANCELLING        = "CANCELING";
  public static final String      RAW_STATUS_IDLE              = "IDLE";
  public static final String      RAW_STATUS_PRINTING          = "PRINTING";
  public static final String      RAW_STATUS_VERY_LOW_ON_INK   = "VERY LOW ON INK";

  public static       net.mobilewebprint.Client               mwp_client;

  // Listeners
  protected ArrayList<PrinterAttributeChangesListener>  printerAttributeChangesListeners;
  protected ArrayList<PrinterListChangesListener>       printerListChangesListeners;
  protected ArrayList<PrintProgressChangesListener>     printProgressChangesListeners;

  // The printer list
  protected Semaphore               printersMutex;
  protected Map<String, Properties> printers;
  protected Set<String>             changedPrinterIps;

  // The string dictionary
  protected Map<Integer, String>    stringDictionary;

  public Application() {
    printerAttributeChangesListeners  = new ArrayList<PrinterAttributeChangesListener>();
    printerListChangesListeners       = new ArrayList<PrinterListChangesListener>();
    printProgressChangesListeners     = new ArrayList<PrintProgressChangesListener>();

    stringDictionary                  = new HashMap<Integer, String>();

    _initializeNewPrintersList();
  }

  public boolean RegisterPrinterAttributeChangesListener(PrinterAttributeChangesListener listener)
  {
    printerAttributeChangesListeners.add(listener);
    return true;
  }

  public boolean RegisterPrinterListChangesListener(PrinterListChangesListener listener)
  {
    printerListChangesListeners.add(listener);
    return true;
  }

  public boolean RegisterPrintProgressChangesListener(PrintProgressChangesListener listener)
  {
    printProgressChangesListeners.add(listener);
    return true;
  }

  public void onBootstrap() {
    mwp_client.setOption("http_proxy_name", System.getProperty("http.proxyHost"));
    mwp_client.setOption("http_proxy_port", System.getProperty("http.proxyPort"));
  }

  // The application is about to be sent the printer list (or changes to one printer)
  public void onNewPrinterList() {
    //Client.logD("MobileWebPrint", "+++onNewPrinterList");

    _initializeNewPrintersList();
    changedPrinterIps = new HashSet<String>();
  }

  // ---------------------------------------------------------------------------------
  // Option 1 - more granular.  Receive individual attributes of the printers
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
      Client.logD("MobileWebPrint", "+++onPrinterAttribute " + ip + " " + name + " " + value);
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
      Client.logD("MobileWebPrint", "+++onRemovePrinter " + ip);
      printersMutex.acquire();
    } catch (InterruptedException e) {
      return;
    }

    try {
      printers.remove(ip);
    } finally {
      Client.logD("MobileWebPrint", "-------------------- release onRemovePrinter");
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
      for (PrinterListChangesListener listener : printerListChangesListeners) {
        _onEndPrinterEnumeration(listener);
      }
    } finally {
      //Client.logD("MobileWebPrint", "-------------------- release");
      printersMutex.release();
    }
  }

  public void _onEndPrinterEnumeration(PrinterListChangesListener listener) {
    if (changedPrinterIps != null) {
      //listener.onBeginPrinterListChanges();

      listener.onBeginPrinterListChanges();
      for (String ip:changedPrinterIps) {
        if (printers.containsKey(ip)) {
          listener.onPrinterChanged(ip, printers.get(ip));
        } else {
          listener.onPrinterRemoved(ip);
        }
      }
      listener.onEndPrinterListEnumeration();
      changedPrinterIps = null;
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

  public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId, String jobStatus)
  {
    for (PrintProgressChangesListener listener : printProgressChangesListeners) {
      listener.onPrintJobProgress(state, numerator, denominator, message, rawState, jobId, jobStatus);
    }
  }

  public void setDictionaryItems(String items)
  {
    for (String line: items.split("\n")) {
      //Client.logD("MobileWebPrint", line);

      int    index  = 0;
      String str    = "";
      int    i      = 0;
      for (String part: line.split("~~~")) {
        if (i == 0) {
          if (!part.equals("")) {
            index = Integer.parseInt(part);
          }
        } else if (i == 1) {
          str = part;
        }

        i += 1;
      }
      stringDictionary.put(index, str);
    }
  }

  public boolean sendMessage(int message, int numStrings, int n1, int n2, int n3, int n4, int n5, int n6, int n7, int n8)
  {
    //Client.logD("MobileWebPrint", "num" + numStrings);
    //Client.logD("MobileWebPrint", "n1" + " -> " + n1);
    //Client.logD("MobileWebPrint", "n2" + " -> " + n2);
    //Client.logD("MobileWebPrint", "n3" + " -> " + n3);
    //Client.logD("MobileWebPrint", "n4" + " -> " + n4);
    //Client.logD("MobileWebPrint", "n5" + " -> " + n5);
    //Client.logD("MobileWebPrint", "n6" + " -> " + n6);
    //Client.logD("MobileWebPrint", "n7" + " -> " + n7);
    //Client.logD("MobileWebPrint", "n8" + " -> " + n8);

    String fName, s1="", s2="", s3="", s4="", s5="", s6="", s7="", s8="";
    if ((fName = stringDictionary.get(message)) == null) { return false; }
    Client.logD("MobileWebPrint", "fName: " + fName + " numStrings: " + numStrings);

    if (numStrings >= 1) { if ((s1 = stringDictionary.get(n1)) == null) { return false; } }
    if (numStrings >= 2) { if ((s2 = stringDictionary.get(n2)) == null) { return false; } }
    if (numStrings >= 3) { if ((s3 = stringDictionary.get(n3)) == null) { return false; } }
    if (numStrings >= 4) { if ((s4 = stringDictionary.get(n4)) == null) { return false; } }
    if (numStrings >= 5) { if ((s5 = stringDictionary.get(n5)) == null) { return false; } }
    if (numStrings >= 6) { if ((s6 = stringDictionary.get(n6)) == null) { return false; } }
    if (numStrings >= 7) { if ((s7 = stringDictionary.get(n7)) == null) { return false; } }
    if (numStrings >= 8) { if ((s8 = stringDictionary.get(n8)) == null) { return false; } }

    // Do the individual functions
    if (fName.equals("onPrintJobProgress")) {
      //Client.logD("MobileWebPrint", "pjp " + s1 + ", " + n6 + ", " + n7 + ", " + s2 + ", " + s3 + ", " + s4 + ", " + s5);
      if (numStrings != 5) { return false; }
      onPrintJobProgress(s1, n6, n7, s2, s3, s4, s5);
      return true;
    }

    if (fName.equals("onPrinterAttribute")) {
      if (numStrings != 3) { return false; }
      onPrinterAttribute(s1, s2, s3);
      return true;
    }

    Client.logD("MobileWebPrint", "fName2: " + fName);
    return false;
  }

  // -------------------------------------------------------------------------------

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
      currScore = Utils.atoi(ent.getValue().getProperty("score", "0"));
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
      if ((currScore = Utils.atoi(ent.getValue().getProperty("score", "0"))) == score) {
        list.add(ent.getValue());
        count += 1;
      }
    }

    return count;
  }

}



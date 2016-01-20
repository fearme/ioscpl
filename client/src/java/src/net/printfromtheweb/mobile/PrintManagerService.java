package net.mobilewebprint.secureassetprinting;

import net.printfromtheweb.mobile.*;
//
//import android.os.*;
//import android.content.*;
//import java.util.*;
//
//
////--------------------------------------------------------------------------------------------
//// This is just a stub to make this project compile.
////
//// TODO: This should be an interface in the Api object
////--------------------------------------------------------------------------------------------

public class PrintManagerService implements Host.PrintManager {

//  public  static final int    MSG_REGISTER_CLIENT   = 1;
//  public  static final int    MSG_UNREGISTER_CLIENT = 2;
//  public  static final int    MSG_PRINTER_LIST      = 3;
//  public  static final int    MSG_PRINT_PROGRESS    = 4;
//
//  private static final String TAG                   = "jMobileWebPrint";
//
//  public static        PrintManagerService  printManagerService;
//
//  private Map<String, Client>               clients;
//
//  public PrintManagerService() {
//    this.printManagerService  = this;
//
//    clients             = new HashMap<String, Client>();
//
//    SecureAssetPrintingApi.api.host.registerAsPrintManager(this);
//  }
//
//  public void registerClient(String name, Client client) {
//    clients.put(name, client);
//  }
//
//  public void unRegisterClient(String name) {
//    clients.remove(name);
//  }
//
    public void onPrinterListReset() {}
    public void onPrinterListClear() {}
    public void onPrinterScanDone() {}

    @Override
    public void onPrinterListChanged() {
//    ArrayList<Printer> printers = (ArrayList<Printer>)SecureAssetPrintingApi.api.getSortedPrinters();
//
//    Bundle printersBundle = new Bundle();
//    for (Printer printer: printers) {
//      Bundle b = new Bundle();
//      b.putString("name", printer.name);
//      b.putString("ip", printer.ip);
//      b.putInt("score", printer.displayScore());
//
//      printersBundle.putBundle(printer.ip, b);
//    }
//
//    for (Map.Entry<String, Client> entry : clients.entrySet()) {
//      try {
//        Message msg = Message.obtain(null, MSG_PRINTER_LIST);
//        msg.setData(printersBundle);
//        entry.getValue().messenger.send(msg);
//      } catch (Exception e) {
//        SecureAssetPrintingApi.api.host.log_e(TAG, "Caught error in onPrinterListChanged: ", e);
//      }
//    }
    }
//
//  private String jobState;
//  private String jobMessage;
//  private String jobRawState;
//  private int    jobNumerator;
//  private int    jobDenominator;
//
    @Override
    public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId) {
//
//    Bundle progress = new Bundle();
//    progress.putString("state",    jobState = state);
//    progress.putInt("numerator",   jobNumerator = numerator);
//    progress.putInt("denominator", jobDenominator = denominator);
//    progress.putString("message",  jobMessage = message);
//    progress.putString("rawState", jobRawState = rawState);
//
//    dispatchPrintJobProgress(progress);
    }
//
//  public void dispatchPrintJobProgress(Bundle progress_) {
//
//    Bundle progress = progress_;
//    if (progress_ == null) {
//      if (jobState == null || jobState.equals("")) {
//        return;
//      }
//
//      progress = new Bundle();
//      progress.putString("state",    jobState);
//      progress.putInt("numerator",   jobNumerator);
//      progress.putInt("denominator", jobDenominator);
//      progress.putString("message",  jobMessage);
//      progress.putString("rawState", jobRawState);
//    }
//
//    for (Map.Entry<String, Client> entry : clients.entrySet()) {
//      try {
//        Message msg = Message.obtain(null, MSG_PRINT_PROGRESS);
//        msg.setData(progress);
//        entry.getValue().messenger.send(msg);
//      } catch (Exception e) {
//        SecureAssetPrintingApi.api.host.log_e(TAG, "Caught error in dispatchPrintJobProgress: ", e);
//      }
//    }
//
//    // When done, clear out our global data
//    jobState = "";
//  }
//
//  public class Client {
//
//    private Context that;
//    private String  name;
//
//    final Messenger messenger;
//
//    public Client(Context that, String name) {
//      this.that = that;
//      this.name = name;
//
//      messenger = new Messenger(new IncomingHandler());
//    }
//
//    public void onPrinterListChanged(Bundle printers) {}
//    public void onPrintJobProgress(Bundle progress) {}
//    public void onServiceConnected(ComponentName className, IBinder service) {}
//    public void onServiceDisconnected(ComponentName className) {}
//
//    public void onStart() {
//      printManagerService.registerClient(name, this);
//      printManagerService.dispatchPrintJobProgress(null);
//    }
//
//    protected void onStop() {
//      printManagerService.unRegisterClient(name);
//    }
//
//
//    class IncomingHandler extends Handler {
//      @Override
//      public void handleMessage(Message msg) {
//
//        switch (msg.what) {
//          case MSG_PRINTER_LIST:
//            onPrinterListChanged(msg.getData());
//            break;
//
//          case MSG_PRINT_PROGRESS:
//            onPrintJobProgress(msg.getData());
//            break;
//        }
//      }
//    }
//
//  }
}


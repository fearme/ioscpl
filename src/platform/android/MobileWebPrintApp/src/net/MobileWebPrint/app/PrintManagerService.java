
package net.mobilewebprint.app;

import net.mobilewebprint.app.MwpApplication;

import net.mobilewebprint.Client;
import net.mobilewebprint.Utils;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Service;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Messenger;
import android.os.Message;
import android.os.Handler;
import android.os.RemoteException;
import android.content.Intent;
import android.content.Context;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.provider.Settings.Secure;
import android.widget.Toast;
import android.util.Log;

import java.util.ArrayList;
import java.util.Properties;
import java.util.Random;

import java.io.UnsupportedEncodingException;

import java.security.NoSuchAlgorithmException;
import java.security.MessageDigest;
import java.security.SecureRandom;


public class PrintManagerService extends Service implements net.mobilewebprint.PrinterListChangesListener, net.mobilewebprint.PrintProgressChangesListener {

  public  static final int    MSG_REGISTER_CLIENT   = 1;
  public  static final int    MSG_UNREGISTER_CLIENT = 2;
  public  static final int    MSG_PRINTER_LIST      = 3;
  public  static final int    MSG_PRINT_PROGRESS    = 4;
  public  static final int    MSG_SEND_JOB          = 5;

  private static final String TAG                   = "MobileWebPrint";


  public static net.mobilewebprint.Client               mwp_client;
  public static net.mobilewebprint.app.MwpApplication   mwp_application;

  protected     Messenger                               messenger;
  protected     ArrayList<Messenger>                    clients;

  @Override
  public void onCreate() {
    Log.v(TAG, "lifetime:- PrintManagerService.onCreate");

    this.clients    = new ArrayList<Messenger>();
    this.messenger  = new Messenger(new IncomingHandler());

    mwp_application = new net.mobilewebprint.app.MwpApplication();
    mwp_client      = new net.mobilewebprint.Client(mwp_application);

    getPrimaryAccount();

    mwp_client.RegisterPrinterListChangesListener(this);
    mwp_client.RegisterPrintProgressChangesListener(this);
    mwp_client.start();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {

    Log.v(TAG, "lifetime:- PrintManagerService.onStartCommand");

    Toast.makeText(this, "service starting", Toast.LENGTH_SHORT).show();
    return super.onStartCommand(intent, flags, startId);
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.v(TAG, "lifetime:- PrintManagerService.onBind");
    Toast.makeText(getApplicationContext(), "binding", Toast.LENGTH_SHORT).show();
    return messenger.getBinder();
  }

  class IncomingHandler extends Handler {
    @Override
    public void handleMessage(Message msg) {
      Log.d(TAG, "Handling message: " + msg);

      switch (msg.what) {
        case MSG_REGISTER_CLIENT:
          PrintManagerService.this.clients.add(msg.replyTo);
          PrintManagerService.this.sendPrinterListToClients();
          break;

        case MSG_UNREGISTER_CLIENT:
          PrintManagerService.this.clients.remove(msg.replyTo);
          break;

        case MSG_SEND_JOB:
          mwp_client.sendJob(msg.getData().getString("url"), msg.getData().getString("ip"));
          break;

        default:
          super.handleMessage(msg);
          break;
      }
    }
  }

  // PrinterListChangesListener
  @Override
  public void onNewPrinterList() {}

  @Override
  public void onBeginPrinterListChanges() {}

  @Override
  public void onPrinterChanged(String ip, Properties printer) {}

  @Override
  public void onPrinterRemoved(String ip) {}

  @Override
  public void onEndPrinterListEnumeration() {
    sendPrinterListToClients();
  }

  public void sendPrinterListToClients() {
    ArrayList<Properties> printers = mwp_application.getSortedPrinterList(/*boolean filterUnsupportedPrinters*/ true);

    Bundle printersBundle = new Bundle();
    for (Properties printer: printers) {
      Bundle b = new Bundle();
      String ip = printer.getProperty("ip");
      b.putString("ip", ip);
      b.putString("name", printer.getProperty("name"));

      String score = printer.getProperty("score", "0");
      b.putInt("score", Utils.atoi(score));

      printersBundle.putBundle(score + ip, b);
    }

    sendMessageToClients(MSG_PRINTER_LIST, printersBundle);
  }

  public void sendMessageToClients(int messageType, Bundle bundle) {

    for (int i = clients.size() -1; i >= 0; i--) {
      try {
        Message msg = Message.obtain(null, messageType);
        msg.setData(bundle);
        clients.get(i).send(msg);
      } catch (RemoteException e) {
        clients.remove(i);
      }
    }
  }

  @Override
  public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId, String jobStatus) {

    //Log.d(TAG, "PMS::onprintJobprogres1: " + state);

    Bundle progress = new Bundle();
    progress.putString("state",        state);
    progress.putInt("numerator",       numerator);
    progress.putInt("denominator",     denominator);
    progress.putString("message",      message);
    progress.putString("rawState",     rawState);
    progress.putString("jobId",        jobId);
    progress.putString("jobStatus",    jobStatus);

    //Log.d(TAG, "PMS::onprintJobprogres: " + progress);
    dispatchPrintJobProgress(progress);
  }

  public void dispatchPrintJobProgress(Bundle progress) {

//    Bundle progress = progress_;
//    if (progress == null) {
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
//      progress.putString("jobId",    jobId);
//    }

    //Log.d(TAG, "dispatchprogress: " + progress);
    for (int i = clients.size() -1; i >= 0; i--) {
      try {
        Message msg = Message.obtain(null, MSG_PRINT_PROGRESS);
        msg.setData(progress);
        clients.get(i).send(msg);
      } catch (RemoteException e) {
        clients.remove(i);
      }
    }
  }

  @Override
  public void onDestroy() {
    Log.v(TAG, "lifetime:- PrintManagerService.onDestroy");
  }

  /**
   *  What is this device's primary account.
   *
   *  Used for ID.
   */
  private void getPrimaryAccount() {

    String primaryAccount = "";

    // Get the hardware ID, if we have one
    Context context = getApplicationContext();
    String uniqueId = getUniqueHardwareId();
    mwp_client.setOption("hardwareid", uniqueId);
    mwp_client.setOption("deviceid", uniqueId);
    Log.d(TAG, "hardware_id " + uniqueId);

    if(uniqueId != null && uniqueId.length() > 0){
      mwp_client.setOption("hardwareid", uniqueId);
    }

    try {
        for (Account account: AccountManager.get(context).getAccounts()) {
            if (account.name.matches(".*@gmail\\.com") && account.type.matches(".*google.*")) {
                primaryAccount = account.name;
                break;
            } else if (account.name.matches(".*@.*\\.(com|net|org|gov|us)")) {
                primaryAccount = account.name;
            }
        }

        if (primaryAccount.length() > 0) {
          mwp_client.setOption("username", primaryAccount);
        }

    } catch (Exception e) {
        Log.e(TAG, "getprimaryaccount", e);
    }
  }

  private String getUniqueHardwareId() {

    // Get the hardware ID
    Context context = getApplicationContext();
    String hardware_id = Secure.getString(context.getContentResolver(), Secure.ANDROID_ID);

    try {
      if(hardware_id != null && hardware_id.length() > 0) {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] hash = md.digest(hardware_id.getBytes("UTF-8"));
        StringBuffer hexString = new StringBuffer();
        for (int i = 0; i< hash.length; i++){
            String hex = Integer.toHexString(0xff & hash[i]);
            if(hex.length() == 1) hexString.append('0');
            hexString.append(hex);
        }
        hardware_id =  hexString.toString();
      }
      else{
        hardware_id = randomString(64);
      }
    } catch (NoSuchAlgorithmException e) {
    } catch (UnsupportedEncodingException e) {
    }

    return hardware_id;
  }

  private static char[] charSet = "abcdefghijklmnopqrstuvwxyz0123456789".toCharArray();
  private static String randomString(int length) {
    Random random = new SecureRandom();
    char[] result = new char[length];

    for (int i = 0; i < result.length; i++) {
      result[i] = charSet[random.nextInt(charSet.length)];
    }

    return new String(result);
  }


}


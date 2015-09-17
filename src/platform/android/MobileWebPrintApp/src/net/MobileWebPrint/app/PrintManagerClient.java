
/**
 *  Acts as a client to the PrintManager.
 */

package net.mobilewebprint.app;

import net.mobilewebprint.app.PrintManagerService;

import android.content.Context;
import android.content.ServiceConnection;
import android.content.ComponentName;
import android.content.Intent;

import android.os.Handler;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Messenger;
import android.os.Message;
import android.os.RemoteException;

import android.util.Log;

public abstract class PrintManagerClient extends Handler implements ServiceConnection, PrintManagerClientInterface {

//  // The interface that sub-classes must implement
//  public void onPrinterListChanged(Bundle printers);
//  public void onPrintJobProgress(Bundle progress);

  private static final String TAG                   = "jMobileWebPrint";

  protected String    name;
  protected Context   that;
  protected Messenger messenger;
  protected Messenger service;
  protected boolean   bound;

  // -------------------- Constructor and early-invoked methods --------------------------

  public PrintManagerClient(Context that, String name) {
    this.name       = name;
    this.that       = that;
    this.messenger  = new Messenger(this);
    this.service    = null;
    this.bound      = false;
  }

  public void onStart() {
    that.bindService(new Intent(that, PrintManagerService.class), this, Context.BIND_AUTO_CREATE);
  }

  // -------------------- Service Connection implementation --------------------------

  @Override
  public void onServiceConnected(ComponentName className, IBinder service_) {

    Log.v(TAG, "lifetime:- PrintManagerClient(" + name + ").onServiceConnected");

    service = new Messenger(service_);
    bound = true;

    // Tell service we want info!
    Message msg = Message.obtain(null, PrintManagerService.MSG_REGISTER_CLIENT);
    msg.replyTo = messenger;
    sendMessageToService(msg);
  }

  @Override
  public void onServiceDisconnected(ComponentName className) {

    Log.v(TAG, "lifetime:- PrintManagerClient(" + name + ").onServiceDisconnected");

    service = null;
    bound   = false;
  }

  public void sendMessageToService(Message message) {

    try {
      service.send(message);
    } catch (RemoteException e) {
    }
  }

  public void sendMessageToService(int messageType, Bundle bundle) {

    Message msg = Message.obtain(null, messageType);
    msg.setData(bundle);
    sendMessageToService(msg);
  }


  // -------------------- Handler implementation --------------------------

  @Override
  public void handleMessage(Message msg) {
    switch (msg.what) {
      case PrintManagerService.MSG_PRINTER_LIST:
        //Log.v(TAG, "Handling printer list");
        onPrinterListChanged(msg.getData());
        break;

      case PrintManagerService.MSG_PRINT_PROGRESS:
        //Log.v(TAG, "Handling job progress");
        onPrintJobProgress(msg.getData());
        break;

      default:
        super.handleMessage(msg);
        break;
    }
  }

  // -------------------- Late-invoked methods --------------------------

  protected void onStop() {
    if (bound) {
      if (service != null) {
        Message msg = Message.obtain(null, PrintManagerService.MSG_UNREGISTER_CLIENT);
        msg.replyTo = messenger;
        sendMessageToService(msg);
      }

      that.unbindService(this);
      bound = false;
    }
  }
}


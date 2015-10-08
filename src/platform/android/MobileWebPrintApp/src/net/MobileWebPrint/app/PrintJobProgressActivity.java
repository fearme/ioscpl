package net.mobilewebprint.app;

import net.mobilewebprint.Application;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.lang.RuntimeException;

public class PrintJobProgressActivity extends Activity {

  private static final String               TAG = "jMobileWebPrint";

//  private              CoreApi              api;

  private              ProgressBar          progressBar;
  private              TextView             label;

  public               PrintManagerService          printManagerService;
  private              ServiceBridge                serviceBridge;

  public PrintJobProgressActivity() {
    super();

//    this.api = CoreApi.api;
//    if ((this.printManagerService = (PrintManagerService)api.host.printManager) == null) {
//      this.printManagerService = new PrintManagerService();
//    }

  }

  // ------------------------------------------------------------------------------------------------------------------
  //         ServiceBridge to PrintManagerService
  // ------------------------------------------------------------------------------------------------------------------

  class ServiceBridge extends PrintManagerClient {
    ServiceBridge(Context that, String name) {
      super(that, name);
    }

    public void onPrinterListChanged(Bundle printers) {
      //PrintJobProgressActivity.this.onPrinterListChanged(printers);
    }

    public void onPrintJobProgress(Bundle progress) {
      PrintJobProgressActivity.this.onPrintJobProgress(progress);
    }
  }

  // ------------------------------------------------------------------------------------------------------------------
  //         Activity implementation
  // ------------------------------------------------------------------------------------------------------------------

  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onCreate: " + savedInstanceState);
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_print_job_progress);

    serviceBridge = new ServiceBridge(this, "PrintJobProgressActivity");

    progressBar = (ProgressBar)findViewById(R.id.print_progress_bar);
    label       = (TextView)findViewById(R.id.print_progress_message);

    progressBar.setMax(200);
    progressBar.setProgress(0);
  }

  @Override
  public void onStart() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onStart");
    super.onStart();
    serviceBridge.onStart();
  }

  @Override
  public void onResume() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onResume");
    super.onResume();
  }

  public void onPrintJobProgress(Bundle progress) {

    //Log.d(TAG, "progress::onprogress " + progress);

    String state     = progress.getString("state");
    int numerator    = progress.getInt("numerator");
    int denominator  = progress.getInt("denominator");
    String message   = progress.getString("message");
    String rawState  = progress.getString("rawState");
    String jobId     = progress.getString("jobId");
    String jobStatus = progress.getString("jobStatus");

    progressBar.setMax(denominator);
    progressBar.setProgress(numerator);

    String display = message;

    // Most times, we know a good message to show the user, but sometimes we need to add the
    // string that the printer sent as status to clue the user into what is truly going on.
    if (jobStatus.equals(net.mobilewebprint.Application.STATUS_CANCELLING)) {
    } else if (rawState.equals(net.mobilewebprint.Application.RAW_STATUS_PRINTING) || rawState.equals(net.mobilewebprint.Application.RAW_STATUS_IDLE) || rawState.equals("")) {
    } else {
      display += " -- " + rawState;
    }

//    Log.v(TAG, "*********************************************************rawState/printerState  " + rawState + " &**************************************");
//    Log.v(TAG, "*********************************************************state/state            " + state + " &**************************************");
//    Log.v(TAG, "*********************************************************jobStatus/jobStatus    " + jobStatus + " &**************************************");
//    Log.v(TAG, "*********************************************************message/message        " + message + " &**************************************");
//    Log.v(TAG, "*********************************************************display                " + display + " &**************************************");
    label.setText(display);

    // If the job is done, send a finish
    if (jobStatus.equals(net.mobilewebprint.Application.STATUS_SUCCESS) ||
        jobStatus.equals(net.mobilewebprint.Application.STATUS_NETWORK_ERROR) ||
        jobStatus.equals(net.mobilewebprint.Application.STATUS_UPSTREAM_ERROR))
    {
      setResult(Activity.RESULT_OK);
      finish();
      return;
    } else if (jobStatus.equals(net.mobilewebprint.Application.STATUS_CANCELLED)) {
      setResult(Activity.RESULT_CANCELED);
      finish();
    }
  }

  @Override
  public void onPause() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onPause");
    super.onPause();
  }

  @Override
  public void onStop() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onStop");
    super.onStop();
    serviceBridge.onStop();
  }

  @Override
  public void onRestart() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onRestart");
    super.onRestart();
  }

  @Override
  public void onDestroy() {
    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onDestroy");
    super.onDestroy();
  }

}




package net.mobilewebprint.app;

//import org.json.*;

import android.app.Activity;
import android.os.Bundle;
import android.os.IBinder;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.ContentResolver;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.net.Uri;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.ListView;
import android.util.Log;

import java.net.URLDecoder;
import java.io.UnsupportedEncodingException;

import java.util.ArrayList;
import java.util.Properties;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class PrintActivity extends Activity {

  // Basically, this Activity can be used in two ways:
  //
  //  1.  Someone calls this activity just to get the user to pick
  //      a printer.  The caller will do whatever is needed with the printer,
  //      and this Activity is done once it returns the chosen printer
  //      to the caller.
  //
  //  2.  Someone calls this activity to be the 'manager' of the print.  For
  //      example, if the user picks a photo on their mobile, this Activity
  //      gets launched.  In this case, we launch the print progress Activity,
  //      and wait for it to complete.
  //
  //      Note that in this case, this Activity doesn't really do much.  Mario
  //      core does the heavy-lifting, this Activity just watches for progress
  //      messages, and for when to finish.

  private static final int                            ACTIVITY_PROGRESS_DONE = 210;

  private static final Pattern                        printPathRe = Pattern.compile("(http|https|printit):[/][/][^/]+[/]social[/]print[/][^/]+[/]([^/]+)[/](.*)$");
  private static final Pattern                        urlPrintPathRe = Pattern.compile("(http|https|urlprint):[/][/]urlprint\\.mobile(...)print\\.net(:[0-9]+)?[/]([^/]+)[/](.*)$");

  protected            boolean                        userIsPickingPrinter;
  protected            boolean                        userIsPrinting;

  public               String                         assetString;
  public               String                         assetProvider;
  public               String                         assetProtocol;

  public  static final String   EXTRA_ASSET_URL = "net.mobilewebprint.app.ASSET_URL";
  public  static final String   EXTRA_PIN       = "net.mobilewebprint.app.PIN";
  public  static final String   EXTRA_PAIR_ID   = "net.mobilewebprint.app.PAIR_ID";
  public  static final String   EXTRA_PICKER    = "net.mobilewebprint.app.PICKER";

  private static final String   TAG             = "MobileWebPrintApp";

  // The printer list
  protected ArrayList<Properties>                     printerList;
  protected ArrayAdapter                              printerListAdapter;

  private              ServiceBridge                  serviceBridge;

  public               String                         pinPrinterIp;

  private ConcurrentLinkedQueue<DestinationJobStats>  jobs;
  private              DestinationJobStats            currentJob;

  private              boolean                        creating;
  private              ExecutorService                executor;

  private              PrintActivity                  myself;

  public PrintActivity() {
    super();

    this.myself = this;

    this.executor   = Executors.newFixedThreadPool(8);
    this.jobs       = new ConcurrentLinkedQueue<DestinationJobStats>();
    this.currentJob = null;
    this.pinPrinterIp=null;

    userIsPrinting        = false;
    userIsPickingPrinter  = true;
  }

  // ------------------------------------------------------------------------------------------------------------------
  //         ServiceBridge to PrintManagerService
  // ------------------------------------------------------------------------------------------------------------------

  class ServiceBridge extends PrintManagerClient {
    ServiceBridge(Context that, String name) {
      super(that, name);
    }

    public void onPrinterListChanged(Bundle printers) {
      PrintActivity.this.onPrinterListChanged(printers);
      Log.d(TAG, "PrintActivity.ServiceBridge onPrinterListChanged");
    }

    public void onPrintJobProgress(Bundle progress) {
      PrintActivity.this.onPrintJobProgress(progress);
      Log.d(TAG, "PrintActivity.ServiceBridge onPrintJobProgress");
    }
  }

  // ------------------------------------------------------------------------------------------------------------------
  //         Activity implementation
  // ------------------------------------------------------------------------------------------------------------------

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    // Do the normal Activity lifetime stuff
    super.onCreate(savedInstanceState);
    Log.v(TAG, "lifetime:- PrintActivity.onCreate: " + savedInstanceState);
    setContentView(R.layout.activity_print);

    creating = true;

    printerList   = new ArrayList<Properties>();
    serviceBridge = new ServiceBridge(this, "PrintActivity");

    ListView listview   = (ListView)findViewById(R.id.printerlistview);
    printerListAdapter  = new ArrayAdapter(this, android.R.layout.simple_list_item_1, printerList);
    listview.setAdapter(printerListAdapter);

    // Register click handler -- for when the user clicks a printer
    listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {

      // -------------------------------------------------------------------------
      // The click handler -- the user picked a printer

      @Override
      public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {

        Properties item = (Properties)parent.getItemAtPosition(position);
        Log.d(TAG, "User chose: " + item.getProperty("ip"));

        if (userIsPickingPrinter) {
          // TODO: untested
          Bundle bundle = new Bundle();
          bundle.putString("ip", item.getProperty("ip"));
          Intent intent = new Intent();
          intent.putExtras(bundle);
          setResult(Activity.RESULT_OK, intent);
          finish();
          return;
        } else if (userIsPrinting) {
          Bundle bundle = new Bundle();
          bundle.putString("ip", item.getProperty("ip"));
          bundle.putString("url", assetString);

          serviceBridge.sendMessageToService(PrintManagerService.MSG_SEND_JOB, bundle);
          showProgress();
        }

//        CoreApi.PrinterStats item = (CoreApi.PrinterStats)parent.getItemAtPosition(position);
//        Log.d(TAG, "User chose: " + item.toString());
//
//        if (currentJob != null) {
//
//          if (currentJob.intentType.equals(EXTRA_PICKER)) {
//            Bundle bundle = new Bundle();
//            bundle.putString("ip", item.getIp());
//            Intent intent = new Intent();
//            intent.putExtras(bundle);
//            setResult(Activity.RESULT_OK, intent);
//            finish();
//            return;
//          }
//
//          /* otherwise */
//          currentJob.print(item);
//          showProgress();
//        }
      }
    });

    // -------------------------------------------------------------------------
    // Discovery of what is being asked of us

    Intent intent = getIntent();
    Log.v(TAG, "lifetime:- PrintActivity.onCreate " + savedInstanceState + ", intent: " + intent);
    if (intent == null) {
      return;
    }

    String action = intent.getAction();
    String type   = intent.getType();

    Log.v(TAG, "lifetime:- PrintActivity.onCreate action: " + action + ", type: " + type);
    Log.v(TAG, "lifetime:- PrintActivity.onCreate EXTRA_ASSET_URL: " + intent.getStringExtra(EXTRA_ASSET_URL));
    Log.v(TAG, "lifetime:- PrintActivity.onCreate EXTRA_PIN: " + intent.getStringExtra(EXTRA_PIN));
    Log.v(TAG, "lifetime:- PrintActivity.onCreate EXTRA_PAIR_ID: " + intent.getStringExtra(EXTRA_PAIR_ID));
    Log.v(TAG, "lifetime:- PrintActivity.onCreate EXTRA_PICKER: " + intent.getStringExtra(EXTRA_PICKER));


    DestinationJobStats newJob = null;

    String temp = "";

    if (action != null && action.equals(Intent.ACTION_SEND)) {
      Log.v(TAG, "lifetime:- PrintActivity.onCreate(ACTION_SEND) " + action + ", type: " + type);

      if (type.startsWith("image/")) {

        //
        // The user chose an image from their gallery.
        //
        Uri  uri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
        newJob = new DestinationJobStats(uri);
        dispatchJob(newJob);
        return;

      } else if (type.startsWith("text/")) {

        //
        // Some apps send the link as "text/*" over the generic "EXTRA_TEXT" intent
        //
        temp = intent.getStringExtra(Intent.EXTRA_TEXT).toLowerCase();  // temp is lower-cased, need to re-fetch EXTRA_TEXT to get real URL
        Log.d(TAG, "Extra text: " + temp);

        if (temp.startsWith("http")) {

          // PDF?
          if (temp.contains(".pdf")) {

          // Photo?
          } else if (temp.contains(".jpg") || temp.contains(".jpeg") || temp.contains(".png")) {
            newJob = new DestinationJobStats(intent.getStringExtra(Intent.EXTRA_TEXT), Intent.EXTRA_TEXT);
//            Log.d(TAG, "Printing: " + newJob.urlStr);
            dispatchJob(newJob);
            return;

          // Unknown... Let server guess
          } else {
            newJob = new DestinationJobStats(intent.getStringExtra(Intent.EXTRA_TEXT), Intent.EXTRA_TEXT);
//            Log.d(TAG, "Unknown.  Let server guess: " + newJob.urlStr);
            dispatchJob(newJob);
            return;
          }
        }
      }
      return;

    } else if (action != null && action.equals(Intent.ACTION_VIEW)) {

      //
      // Share intent
      //
      Log.v(TAG, "lifetime:- PrintActivity.onCreate(ACTION_VIEW) " + action + ", uri: " + intent.getDataString());
      try {
        String dataString = URLDecoder.decode(intent.getDataString(), "UTF-8");

        Matcher matcher = urlPrintPathRe.matcher(dataString);
        Log.d(TAG, "PrintActivity, urlPrintPathRe: " + matcher);

        if (matcher.find(0)) {

          // urlprint://urlprint.mobilewebprint.net/PROVIDER/whatever
          assetProtocol = matcher.group(1);
          assetProvider = matcher.group(4);
          assetString   = matcher.group(5);

          String stack  = matcher.group(2);
          String port   = matcher.group(3);

          Log.d(TAG, "PrintActivity, string: "   + assetString);
          assetString = assetString.replaceFirst("^urlprint", "");
          assetString = assetString.replaceFirst("^/", "");

          Log.d(TAG, "PrintActivity, protocol: " + assetProtocol);
          Log.d(TAG, "PrintActivity, provider: " + assetProvider);
          Log.d(TAG, "PrintActivity, string: "   + assetString);
          Log.d(TAG, "PrintActivity, stack: "    + stack);
          Log.d(TAG, "PrintActivity, port: "     + port);

          userIsPrinting        = true;
          userIsPickingPrinter  = false;
        }

        // TODO: Handle urlprint://PROVIDER-domain/urlprint/whatever

        matcher = printPathRe.matcher(dataString);
        Log.d(TAG, "PrintActivity, printPathRe: " + matcher);

        if (matcher.find(0)) {

          // printit://clickit.couldpublish.com/social/print/v2/PROVIDER/whatever
          assetProtocol = matcher.group(1);
          assetProvider = matcher.group(2);
          assetString   = matcher.group(3);

          Log.d(TAG, "PrintActivity, protocol: " + assetProtocol);
          Log.d(TAG, "PrintActivity, provider: " + assetProvider);
          Log.d(TAG, "PrintActivity, url: "      + assetString);

          userIsPrinting        = true;
          userIsPickingPrinter  = false;
        }

      } catch(UnsupportedEncodingException e) {
      }

      return;
    }

    // The caller sent a server-side URL to print
    if (intent.getStringExtra(EXTRA_ASSET_URL) != null) {

      //
      // Mario's "share a URL" intent
      //
      newJob = new DestinationJobStats(intent.getStringExtra(EXTRA_ASSET_URL), EXTRA_ASSET_URL);
      dispatchJob(newJob);
      return;
    }

    // The caller sent a pin.  Fetch the job
    if (intent.getStringExtra(EXTRA_PIN) != null) {

      //
      // Mario's "share to a PIN" intent
      //
      newJob = new DestinationJobStats(intent.getStringExtra(EXTRA_PIN), EXTRA_PIN);
      dispatchJob(newJob);
      return;
    }

    // The caller sent a "pair print" ID.
    if (intent.getStringExtra(EXTRA_PAIR_ID) != null) {

      //
      // Mario's "share to a paired ID" intent
      //
      newJob = new DestinationJobStats(intent.getStringExtra(EXTRA_PAIR_ID), EXTRA_PAIR_ID);
      dispatchJob(newJob);
      return;
    }

    // The caller just wants the user to pick a printer
    if (intent.getStringExtra(EXTRA_PICKER) != null) {

      //
      // Just let the user pick a printer
      //
      newJob = new DestinationJobStats(intent.getStringExtra(EXTRA_PICKER), EXTRA_PICKER);
      dispatchJob(newJob);
      return;
    }

    Log.w(TAG, "Something sent to PrintActivity, but not handled: " + intent);
  }

  @Override
  protected void onStart() {
    super.onStart();
    Log.v(TAG, "lifetime:- PrintActivity.onStart");

    serviceBridge.onStart();
  }

  // The Uri to handle
  private class DestinationJobStats /* extends HostAndroid.PrintJobAndroid */ {

    protected ContentResolver resolver;

    public DestinationJobStats(Uri uri) {
//      super(uri);
      this.resolver = getContentResolver();
    }

    public DestinationJobStats(String str, String intentType) {
//      super(str, intentType);
      this.resolver = getContentResolver();
    }

    public boolean prePrint() {

//      if (intentType.equals(EXTRA_PICKER)) {
//        haveDonePrePrint = true;
//        return true;
//      }

//      return super.prePrint(resolver);
      return false;
    }

//    public boolean print(CoreApi.PrinterStats item) {
//
//      if (!prePrint()) { return false; }
//
//      if (intentType.equals(EXTRA_PICKER)) {
//        return true;
//      }
//
//      /* otherwise */
//      return super.print(item);
//    }
  }

  protected void dispatchJob(DestinationJobStats job) {

    int numJobsOnEnter = jobs.size();

    jobs.add(job);

    // If the queue was empty when I entered, its up to me to drain the queue
    if (numJobsOnEnter == 0) {
      runDispatchLoop();
    }
  }

  protected void runDispatchLoop() {

    if (jobs.size() > 0) {

      executor.execute(new Runnable() {
        public void run() {

          // Wait for all dependencies to start and be available
          boolean missingDep = true;
          while (missingDep) {
            missingDep = false;
//            missingDep = missingDep || (api() == null);
//            missingDep = missingDep || (api().host == null);

            if (missingDep) {
              Log.d(TAG, "Waiting for deps");
              try {
                Thread.sleep(100);
              } catch (InterruptedException e) {}
            }
          }

          while (jobs.size() > 0) {

            // Wait for any current jobs to finish
            while (currentJob != null) {
//              api().host.sleep(100);
            }

            if ((currentJob = jobs.poll()) == null) {
//              api().host.sleep(100);
              continue;
            }

            /* otherwise */
            currentJob.prePrint();
          }

        }
      });
    }
  }

  protected void showProgress() {
    Intent intent = new Intent(myself, PrintJobProgressActivity.class);
    startActivityForResult(intent, ACTIVITY_PROGRESS_DONE);
  }

  protected void onActivityResult(int reqCode, int resCode, Intent data) {
    if (reqCode == ACTIVITY_PROGRESS_DONE && resCode == RESULT_OK) {
      finish();
    }
  }

  public void onPrinterListChanged(Bundle ps) {

    Log.v(TAG, "PrintActivity - Handling printer list");

    int count = 0;
    printerList.clear();
    for (String key: ps.keySet()) {
      Bundle p = ps.getBundle(key);
      Properties properties = new Properties();

      if (p.containsKey("ip"))    { properties.setProperty("ip", p.getString("ip", "")); }
      if (p.containsKey("name"))  { properties.setProperty("name",  p.getString("name", "")); }
      if (p.containsKey("score")) { properties.setProperty("score", "" + p.getInt("score", 1)); }

      printerList.add(properties);

      count += 1;
    }

    printerListAdapter.notifyDataSetChanged();
    Log.v(TAG, "PrintActivity - Handled printer list (" + count + ")");
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu items for use in the action bar
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.print_activity, menu);
    return super.onCreateOptionsMenu(menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    // Handle presses on the action bar items
    switch (item.getItemId()) {
      case R.id.action_death:
        System.exit(1);
        return true;

      default:
        return super.onOptionsItemSelected(item);
    }
  }

  public void onPrintJobProgress(Bundle progress) {
    Log.d(TAG, "PrintActivity: on progress: " + progress.getString("state"));

    if (!creating /* && api().printJobIsDone(progress.getString("state"), progress.getString("rawState")) */) {
      Log.d(TAG, "PrintActivity: finishing: " + progress.getString("state"));
      currentJob = null;

      if (jobs.size() == 0) {
        //finish();
      }
    }

    creating = false;
  }

  @Override
  protected void onStop() {
    super.onStop();
    Log.v(TAG, "lifetime:- PrintActivity.onStop");

    serviceBridge.onStop();
  }


}


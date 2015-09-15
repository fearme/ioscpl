package net.mobilewebprint.app;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.widget.EditText;
import android.widget.Toast;
import android.util.Log;

public class MainActivity extends Activity {

  private static final String TAG = "MobileWebPrint";

  private static final int    ACTIVITY_PICK_TO_PRINT = 88;
  private static final int    ACTIVITY_PICK_PRINTER  = 89;
  private static final int    ACTIVITY_PRINT         = 90;
  private static final int    ACTIVITY_PROGRESS_DONE = 110;

  private             ServiceBridge    serviceBridge;
  private             int              count;
  private             String[]         assets;

  private             String           url;

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Log.v(TAG, "lifetime:- MainActivity.onCreate");
    setContentView(R.layout.main);

    serviceBridge = new ServiceBridge(this, "MainActivity");
    count = 0;

    assets = new String[20];
    assets[0] = "http://cayman-ext.cloudpublish.com/filestore/files/3/data";
    assets[1] = "http://cayman-ext.cloudpublish.com/filestore/files/4/data";
    //assets[1] = "http://demo.mobilewebprint.net/mario-test-page.pdf";

    // Tron -- Problem: should be landscape -- colors good -- 7 pages -- 51 sec to rip
    assets[2] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/04/tron-sam-cycle-printable-0710_FDCOM2.pdf";

    // Tinker bell -- problem: colors -- -- 45 sec to rip
    assets[3] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2014/07/disney-peter-pan-tinker-bell-silhouette-wall-decoration-printable-0113_FDCOM.pdf";

    // Snow -- Problem: colors -- -- 40 sec to rip
    assets[4] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/04/evil-queen-snow-white-papercraft-printable-0911_FDCOM.pdf";

    // Alice -- Problem: should be landscape -- colors good -- 7 pages -- 70 sec to rip
    assets[5] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/03/Aliceinwonderland-shadowbox-printable_FDCOM2.pdf";
    assets[6] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/10/wreck-it-ralph-pixel-placemat-craft-template-1012_FDCOM.pdf";
    assets[7] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/03/cars2-tow-mater-papercraft-printable-0511_FDCOM.pdf";
    assets[8] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2012/03/aurora-papercraft-printable-0210_FDCOM.pdf";

    // 104 days of summer -- Problem: colors -- -- 33 sec to rip
    assets[9] = "http://a.dilcdn.com/bl/wp-content/uploads/sites/9/2014/07/phineas-ferb-agent-p-3d-papercraft-0611_FDCOM.pdf";
  }

  @Override
  protected void onStart() {
    super.onStart();
    Log.v(TAG, "lifetime:- MainActivity.onStart");

    serviceBridge.onStart();
  }

  /** Called when the user clicks the button */
  public void send(View view) {
    launchSend();
  }

  private void launchSend() {
    Intent intent = new Intent(this, PrintActivity.class);

    EditText editText = (EditText)findViewById(R.id.url);
    url = editText.getText().toString();
    intent.putExtra(PrintActivity.EXTRA_ASSET_URL, url);
    Log.d(TAG, "Launching print " + url + "| " + intent);

    startActivityForResult(intent, ACTIVITY_PICK_TO_PRINT);
  }

  public void pair(View view) {
    Intent intent = new Intent(this, PrintActivity.class);

    EditText editText = (EditText)findViewById(R.id.url);
    String pin = editText.getText().toString();

    // Is this a numeric pin?
    if (pin.matches("[0-9]+")) {
      intent.putExtra(PrintActivity.EXTRA_PIN, pin);
      Log.d(TAG, "Launching print " + pin + "| " + intent);
      startActivityForResult(intent, ACTIVITY_PICK_PRINTER);
      return;
    }

    /* otherwise */

    if (pin.equals("")) {
//      pin = api().host.config.optString("uid", "");
    }

    if (pin.equals("")) {
//      pin = api().host.config.optString("username", "");
    }

    intent.putExtra(PrintActivity.EXTRA_PAIR_ID, pin);
    Log.d(TAG, "Launching pair print " + pin + "| " + intent);

    startActivity(intent);
  }

  // ------------------------------------------------------------------------------------------------------------------
  //         ServiceBridge to PrintManagerService
  // ------------------------------------------------------------------------------------------------------------------

  class ServiceBridge extends PrintManagerClient {
    ServiceBridge(Context that, String name) {
      super(that, name);
    }

    public void onPrinterListChanged(Bundle printers) {}
    public void onPrintJobProgress(Bundle progress) {}
  }

  protected void onActivityResult(int reqCode, int resCode, Intent data) {
    if (resCode == RESULT_OK) {
      if (reqCode == ACTIVITY_PICK_TO_PRINT) {
        Bundle bundle = new Bundle();
        bundle.putString("ip", data.getExtras().getString("ip"));
        bundle.putString("url", url);

        serviceBridge.sendMessageToService(PrintManagerService.MSG_SEND_JOB, bundle);

        Intent intent = new Intent(this, PrintJobProgressActivity.class);
        startActivityForResult(intent, ACTIVITY_PROGRESS_DONE);
      } else if (reqCode == ACTIVITY_PICK_PRINTER) {
        data.getExtras().getString("ip");
      }
    }

    if (reqCode == ACTIVITY_PROGRESS_DONE) {
      if (resCode == RESULT_OK) {
        Toast.makeText(this, "Print has finished.", Toast.LENGTH_SHORT).show();
      } else if (resCode == RESULT_CANCELED) {
        Toast.makeText(this, "Print canceled.", Toast.LENGTH_SHORT).show();
      }
    }
  }

  public void asset(View view) {
    EditText editText = (EditText)findViewById(R.id.url);
    editText.setText(assets[count]);
    if (count++ > 9) {
      count = 0;
    }
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu items for use in the action bar
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.main_activity_actions, menu);
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

  @Override
  protected void onStop() {
    super.onStop();
    Log.v(TAG, "lifetime:- MainActivity.onStop");

    serviceBridge.onStop();
  }

//  private CoreApi api() {
//    return CoreApi.api;
//  }

}

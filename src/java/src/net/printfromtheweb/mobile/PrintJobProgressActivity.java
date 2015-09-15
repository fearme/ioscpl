package net.mobilewebprint.secureassetprinting;
//import net.printfromtheweb.mobile.*;
//
//
//import android.app.*;
//import android.os.*;
//import android.content.*;
//import android.util.*;
//import android.view.*;
//import android.widget.*;
//
//import java.util.*;
//import java.lang.RuntimeException;

//public class PrintJobProgressActivity extends Activity {
public class PrintJobProgressActivity {

//  private static final String               TAG = "PrintFromTheWebMobile";
//
//  public               PrintJobProgressActivity myself = this;
//  private              CoreApi              api;
//
//  private              ProgressBar          progressBar;
//  private              TextView             label;
//
//  public               PrintManagerService          printManagerService;
//  private              PrintManagerService.Client   serviceBridge;
//
//  public PrintJobProgressActivity() {
//    super();
//
//    //this.myself = this;
//
//    this.api = CoreApi.api;
//    if ((this.printManagerService = (PrintManagerService)api.host.printManager) == null) {
//      this.printManagerService = new PrintManagerService();
//    }
//
//  }
//
//  @Override
//  public void onCreate(Bundle savedInstanceState) {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onCreate: " + savedInstanceState);
//    super.onCreate(savedInstanceState);
//    setContentView(R.layout.activity_print_job_progress);
//
//    final PrintJobProgressActivity myself = this;
//    serviceBridge = printManagerService.new Client(this, "PrintJobProgressActivity") {
//      public void onPrintJobProgress(Bundle progress) {
//        myself._onPrintJobProgress(progress);
//      }
//    };
//
//    progressBar = (ProgressBar)findViewById(R.id.print_progress_bar);
//    label       = (TextView)findViewById(R.id.print_progress_label);
//
//    progressBar.setMax(200);
//    progressBar.setProgress(0);
//  }
//
//  @Override
//  public void onStart() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onStart");
//    super.onStart();
//    serviceBridge.onStart();
//  }
//
//  @Override
//  public void onResume() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onResume");
//    super.onResume();
//  }
//
//  public void _onPrintJobProgress(Bundle progress) {
//    onPrintJobProgress_(progress.getString("state"),
//                        progress.getInt("numerator"),
//                        progress.getInt("denominator"),
//                        progress.getString("message"),
//                        progress.getString("rawState"));
//  }
//
//  public void onPrintJobProgress_(String state, int numerator, int denominator, String message, String rawState) {
//    progressBar.setMax(denominator);
//    progressBar.setProgress(numerator);
//
//    String display = message;
//
//    // Most times, we know a good message to show the user, but sometimes we need to add the
//    // string that the printer sent as status to clue the user into what is truly going on.
//    if (state.equals(api.STATUS_CANCELLING)) {
//    } else if (rawState.equals(api.RAW_STATUS_PRINTING) || rawState.equals(api.RAW_STATUS_IDLE) || rawState.equals("")) {
//    } else {
//      display += " -- " + rawState;
//    }
//
//    label.setText(display);
//
//    if (api.printJobIsDone(state, rawState)) {
//      int result = Activity.RESULT_CANCELED;
//      if (api.printJobSuccess(state, rawState)) {
//        result = Activity.RESULT_OK;
//      }
//      setResult(result);
//      finish();
//    }
//  }
//
//  @Override
//  public void onPause() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onPause");
//    super.onPause();
//  }
//
//  @Override
//  public void onStop() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onStop");
//    super.onStop();
//    serviceBridge.onStop();
//  }
//
//  @Override
//  public void onRestart() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onRestart");
//    super.onRestart();
//  }
//
//  @Override
//  public void onDestroy() {
//    Log.v(TAG, "lifetime:- PrintJobProgressActivity.onDestroy");
//    super.onDestroy();
//  }

}



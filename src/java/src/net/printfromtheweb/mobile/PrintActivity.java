package net.mobilewebprint.secureassetprinting;
//import net.printfromtheweb.mobile.*;
//
//import org.json.*;
//
//import java.util.ArrayList;
//
//import android.app.*;
//import android.os.*;
//import android.content.Context;
//import android.view.LayoutInflater;
//import android.content.*;
//import android.view.*;
//import android.util.*;
//import android.widget.*;
//
//import java.lang.RuntimeException;

//public class PrintActivity extends Activity {
public class PrintActivity {

//  private static final String               TAG = "PrintFromTheWebMobile";
  public  static final String   EXTRA_ASSET_URL = "net.printfromtheweb.mobile.ASSET_URL";

//  protected                             String      url;
//
//  // The printer list
//  ArrayList<SecureAssetPrintingApi.PrinterStats>    printerList;
//  ArrayAdapter                                      printerListAdapter;
//
//  public               PrintManagerService          printManagerService;
//  private              PrintManagerService.Client   serviceBridge;
//
//  // The Uri to handle
//  private class DestinationJobStats extends HostAndroid.PrintJobAndroid {
//
//    public DestinationJobStats(String str) {
//      super(str);
//    }
//  }
//
//  public PrintActivity() {
//    super();
//
//    if ((this.printManagerService = (PrintManagerService)api().host.printManager) == null) {
//      this.printManagerService = new PrintManagerService();
//    }
//  }
//
//  @Override
//  public void onCreate(Bundle savedInstanceState) {
//
//    try {
//      super.onCreate(savedInstanceState);
//    } catch (RuntimeException e) {
//      Log.e(TAG, "PrintActivity die", e);
//      SecureAssetPrintingApi.api.host.sleep(1000);
//      System.exit(2);
//    }
//
//    final PrintActivity myself = this;
//    serviceBridge = printManagerService.new Client(this, "PrintActivity") {
//      public void onPrinterListChanged(Bundle printers) {
//        myself.onPrinterListChanged(printers);
//      }
//    };
//
//    setContentView(R.layout.activity_print);
//
//    printerList = new ArrayList<SecureAssetPrintingApi.PrinterStats>();
//    final ListView listview = (ListView)findViewById(R.id.printerlistview);
//
//    printerListAdapter = new ArrayAdapter(this, android.R.layout.simple_list_item_1, printerList);
//    listview.setAdapter(printerListAdapter);
//
//    // Register click handler -- for when the user clicks a printer
//    listview.setOnItemClickListener(new AdapterView.OnItemClickListener() {
//
//      // -------------------------------------------------------------------------
//      // The click handler -- the user picked a printer
//
//      @Override
//      public void onItemClick(AdapterView<?> parent, final View view, int position, long id) {
//
//        final SecureAssetPrintingApi.PrinterStats item = (SecureAssetPrintingApi.PrinterStats)parent.getItemAtPosition(position);
//        Log.d(TAG, "User chose: " + item.toString());
//
//        final DestinationJobStats job = new DestinationJobStats(url);
//        api().host.getThreadPool().execute(new Runnable() {
//          public void run() {
//            if (job.print(item)) {
//              Intent intent = new Intent(myself, PrintJobProgressActivity.class);
//              startActivityForResult(intent, 110);
//            }
//          }
//        });
//      }
//    });
//
//    // -------------------------------------------------------------------------
//    // Discovery of what is being asked of us
//    Intent intent = getIntent();
//    url = intent.getStringExtra(EXTRA_ASSET_URL);
//    Log.v(TAG, "lifetime:- PrintActivity.onCreate " + savedInstanceState + ", intent: " + intent + ", url: " + url);
//  }
//
//  @Override
//  public void onStart() {
//    Log.v(TAG, "lifetime:- PrintActivity.onStart");
//    super.onStart();
//    serviceBridge.onStart();
//  }
//
//  protected void onActivityResult(int reqCode, int resCode, Intent data) {
//    if (reqCode == 110 /* && resCode == RESULT_OK*/) {
//      finish();
//    }
//  }
//
//  public void onPrinterListChanged(Bundle ps) {
//    printerList.clear();
//    for (String key: ps.keySet()) {
//      Bundle p = ps.getBundle(key);
//      printerList.add(new CoreApi.PrinterStats(p.getString("ip"), p.getString("name"), p.getInt("score")));
//    }
//
//    api().sort(printerList);
//
//    printerListAdapter.notifyDataSetChanged();
//  }
//
//  @Override
//  protected void onStop() {
//    super.onStop();
//    Log.v(TAG, "lifetime:- PrintActivity.onStop");
//
//    serviceBridge.onStop();
//  }
//
//  @Override
//  protected void onDestroy() {
//    super.onDestroy();
//  }
//
//  private static SecureAssetPrintingApi api() {
//    return (SecureAssetPrintingApi)SecureAssetPrintingApi.api;
//  }

}



package net.mobilewebprint;

import net.mobilewebprint.Client;

import android.os.Bundle;
import android.net.wifi.*;
import android.net.wifi.WifiManager.*;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.*;
import android.net.NetworkInfo;
import android.util.Log;

public class NetworkStateReceiver extends BroadcastReceiver {

  //private static final String                       TAG             = "PrintFromTheWebMobile";
  protected static final String             TAG             = "jMobileWebPrint";

  public NetworkStateReceiver() {
    super();
    //Log.v(TAG, "-------------------------  NetworkStateReceiver ctor");
  }

  public net.mobilewebprint.Client mwp() {
    return net.mobilewebprint.Application.mwp_client;
  }

  public static void dumpNi(String message, NetworkInfo ni) {
    if (ni == null) { return; }

    Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " " + ni);
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " state: " + ni.getState());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " detailed-state: " + ni.getDetailedState());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " extra-info: " + ni.getExtraInfo());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " reason: " + ni.getReason());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " subtype: " + ni.getSubtype());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " subtypename: " + ni.getSubtypeName());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " type: " + ni.getType());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " typename: " + ni.getTypeName());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " available: " + ni.isAvailable());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " connected: " + ni.isConnected());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " connected-or-connecting: " + ni.isConnectedOrConnecting());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " failover: " + ni.isFailover());
    //Log.v(TAG, "****^^^^!!!!^^^^********* " + message + " roaming: " + ni.isRoaming());
  }

  public void onReceive(Context context, Intent intent) {

    //Log.v(TAG, "-------------------------  Network connectivity change " + intent);

//    Bundle bundle = intent.getExtras();
//    for (String key : bundle.keySet()) {
//      Object value = bundle.get(key);
//      Log.v(TAG, String.format("%s %s (%s)", key, value.toString(), value.getClass().getName()));
//    }

    if (mwp() == null) {
      return;
    }

    /* otherwise */
    if (intent.getAction() == WifiManager.WIFI_STATE_CHANGED_ACTION) {

      int wifiStateInt = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, -99);

      String wifiState;
      if      (wifiStateInt == WifiManager.WIFI_STATE_ENABLED)   { wifiState = "WIFI_STATE_ENABLED"; }
      else if (wifiStateInt == WifiManager.WIFI_STATE_ENABLING)  { wifiState = "WIFI_STATE_ENABLING"; }
      else if (wifiStateInt == WifiManager.WIFI_STATE_DISABLED)  { wifiState = "WIFI_STATE_DISABLED"; }
      else if (wifiStateInt == WifiManager.WIFI_STATE_DISABLING) { wifiState = "WIFI_STATE_DISABLING"; }
      else if (wifiStateInt == WifiManager.WIFI_STATE_UNKNOWN)   { wifiState = "WIFI_STATE_UNKNOWN"; }
      else                                                       { wifiState = "WIFI_STATE_NOT_EVEN_UNKNOWN"; }

      Log.v(TAG, "  -- " + wifiState);

      mwp().sendImmediately("WIFI_STATE_CHANGED", wifiState);
      return;

    } else if (intent.getAction() == ConnectivityManager.CONNECTIVITY_ACTION) {

      //Log.v(TAG, "  -- conn: " + intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, false));
      //Log.v(TAG, "  -- conn: " + intent.getBooleanExtra(ConnectivityManager.EXTRA_NO_CONNECTIVITY, true));

      // Ignore the intent.  It provides different semantics on different HW and Android versions.
      // Simply see if any WiFi networks have connectivity
      boolean isConnected = false;

      ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
      NetworkInfo[] nis = cm.getAllNetworkInfo();
      for (int i = 0; i < nis.length && !isConnected; ++i) {
//        dumpNi("Network change[" + i + "]", nis[i]);
        if (nis[i].getType() == ConnectivityManager.TYPE_WIFI) {
          isConnected = nis[i].isConnected();
        }
      }

      mwp().sendImmediately("CONNECTIVITY_ENABLED", isConnected ? "true" : "false");
      return;
    }
  }
}


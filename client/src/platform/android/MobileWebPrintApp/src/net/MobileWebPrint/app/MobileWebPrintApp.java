package net.mobilewebprint.app;



import android.app.Application;
import android.content.Intent;
import android.content.res.Configuration;
import android.util.Log;

// ** The application implements a SecureAssetPrintingApp
public class MobileWebPrintApp extends /*android.app.*/ Application {

  private static final String           TAG              = "MobileWebPrintApp";

  @Override
  public void onCreate() {
    super.onCreate();
    Log.v(TAG, "lifetime:- Application.onCreate");
    startService(new Intent(this, PrintManagerService.class));
  }

  @Override
  public void onConfigurationChanged(Configuration newConfig) {
    Log.v(TAG, "lifetime:- Application.onConfigurationChanged" + newConfig.toString());
  }

}



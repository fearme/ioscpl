/**
 *  The Secure Print API.
 *
 */

package net.printfromtheweb.mobile;

//import net.printfromtheweb.mobile.*;
//import org.json.*;
//import java.io.*;

public class SecureAssetPrintingApi extends CoreApi {

  protected static SecureAssetPrintingApi instance = null;

  public static SecureAssetPrintingApi getInstance(SecureAssetPrintingApp app, Host host) {
//    if (instance != null) {
//      host.log_d(TAG, "SAP-API getInstance2-1: " + instance);
//    }

    if (instance == null) {
      instance = new SecureAssetPrintingApi(app, host);
    }

//    if (instance != null) {
//      instance.host.log_d(TAG, "SAP-API getInstance2-2: " + instance);
//    }
    return (SecureAssetPrintingApi)instance;
  }

  public static SecureAssetPrintingApi getInstance() {
//    if (instance != null) {
//      instance.host.log_d(TAG, "SAP-API getInstance: " + instance);
//    }

    return (SecureAssetPrintingApi)instance;
  }

  protected SecureAssetPrintingApi(SecureAssetPrintingApp app, Host host) {
    super(app, host);
//    this.controller = new Controller(this);
  }

//  protected CoreApi.PrintJobInterface mkPrintJob(String str) {
//    return host.mkPrintJob(str);
//  }
//
//  public boolean printJobIsDone(String state, String rawState) {
//    return controller.printJobIsDone(state, rawState);
//  }
//
//  public Job prePrint(String assetUri) {
//    return controller.prePrint(assetUri);
//  }
//
//  public boolean print(String ip, Job job) {
//    return controller.print(ip, job);
//  }
//
//  public boolean print(String ip, Job job, JSONObject attributes) {
//    return controller.print(ip, job, attributes);
//  }
//
//
//  // ---------- quasi-private ----------
//  public Job prePrintPhoto(InputStream inStream, String filename) { return null; }

}



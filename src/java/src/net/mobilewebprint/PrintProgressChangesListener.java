package net.mobilewebprint;


import java.util.Properties;

public interface PrintProgressChangesListener {

  public void onPrintJobProgress(String state, int numerator, int denominator, String message, String rawState, String jobId, String jobStatus);

}






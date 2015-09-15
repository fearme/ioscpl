
package net.mobilewebprint.app;

import android.os.Bundle;

public interface PrintManagerClientInterface {

  public void onPrinterListChanged(Bundle printers);
  public void onPrintJobProgress(Bundle progress);
}


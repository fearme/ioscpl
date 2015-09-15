package net.mobilewebprint;


import java.util.Properties;

public interface PrinterListChangesListener {

  public void onNewPrinterList();

  public void onBeginPrinterListChanges();
  public void onPrinterChanged(String ip, Properties printer);
  public void onPrinterRemoved(String ip);
  public void onEndPrinterListEnumeration();

}





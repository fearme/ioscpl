package net.mobilewebprint;


public interface PrinterAttributeChangesListener {


  // The application is about to be sent the printer list (or changes to one printer)
  public void onNewPrinterList();
  public void onBeginPrinterChanges();

  public void onPrinterAttribute(String ip, String name, String value);
  public void onRemovePrinterAttribute(String ip, String name);
  public void onRemovePrinter(String ip);

  // The enumeration of printers is done
  public void onEndPrinterEnumeration();
}




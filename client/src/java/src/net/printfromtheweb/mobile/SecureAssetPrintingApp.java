/**
 *  Abstract class that an app implements.
 *
 */

package net.printfromtheweb.mobile;
import  net.printfromtheweb.mobile.*;

import java.util.Map;
import java.util.Map.Entry;
import java.util.ArrayList;

public class SecureAssetPrintingApp {
  protected ArrayList<String> preFlight(Map<String, Printer> printers) {

    ArrayList<String> ips = new ArrayList<String>();

    for (Entry<String, Printer> entry : printers.entrySet()) {
      ips.add(entry.getKey());
    }

    return ips;
  }
}



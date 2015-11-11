/**
 *  A printer.
 *
 */

package net.printfromtheweb.mobile;

//import  net.printfromtheweb.mobile.*;
//import  net.printfromtheweb.mobile.Util.Js;
//import  net.printfromtheweb.mobile.Util.Js.*;
//
//import org.json.*;
//
//import java.util.Map;
//import java.util.HashMap;
//import java.util.regex.*;
//import java.util.ArrayList;

public class Printer {

//  private static final String             TAG             = "jMobileWebPrint";
//  public  static       Host               host            = null;
//  public  static       SendSnmp           snmp            = null;
//
//  // Attributes
  public               String             ip;
  public               int                port;
  public               String             name;
  public               String             ppdModel;
  public               String             manufacturer;
  public               String             mac;

  public               String             knownStatus;
  public               int                knownEzStatus;
  public               boolean            is_supported;

  public enum Supportedness {
    UNKNOWN,
    UNSUPPORTED,
    SUPPORTED
  }

  public               Supportedness      supported;

//  // Other attributes -- ones that aren't above.
//  public               XJSONObject        extraInfo;
//
//  // If the user has not renamed thier printer, it ends in [1A3B3C] or maybe (1D6CBA)
//  private static final Pattern nameRe    = Pattern.compile(".*[\\[({](......)[\\])}]$");
//
//  // A pattern to find printers I use a lot for development
//  private              ArrayList<Pattern>                 favNameRes;
//
//  Printer(String ip, int port, String name, Map<String, String> stats) {
//    init(ip, port, name, stats);
//  }
//
//  Printer(String ip, int port, String name) {
//    init(ip, port, name, new HashMap<String, String>());
//  }
//
//  private void init(String ip, int port, String name, Map<String, String> stats) {
//    this.ip           = ip;
//    this.port         = port;
//    this.name         = name;
//    this.mac          = "";
//    this.ppdModel     = "";
//    this.knownStatus  = "";
//    this.knownEzStatus = -1;
//    this.manufacturer = "hp";
//    this.extraInfo    = new XJSONObject();
//
//    if (stats.containsKey("MDL")) {
//      this.ppdModel     = stats.get("MDL");
//    }
//
////    if (port >= 8611 && port <= 8612) {
////      if (stats.containsKey("DES")) {
////        this.ppdModel     = stats.get("DES");
////      }
////    }
//
//    if (stats.containsKey("MFG")) {
//      this.manufacturer = stats.get("MFG").toLowerCase();
//    }
//
//    if (this.manufacturer.equals("hewlett-packard")) {
//      this.manufacturer = "hp";
//    }
//
//    this.favNameRes         = new ArrayList<Pattern>();
//
//    this.favNameRes.add(Pattern.compile(".*309.*"));        // Vader
//    this.favNameRes.add(Pattern.compile(".*F2E30E.*"));     // Whitney on Brians desk
//    //this.favNameRes.add(Pattern.compile(".*CE2BB4.*"));     // An officejet in the San Diego CEC
//  }
//
//  public boolean isHP() {
//    return manufacturer.equals("hp");
//  }
//
//  public boolean canDoStatus() {
//    return isHP();
//  }
//
  public boolean getIsSupported(){
    return this.is_supported;
  }

  public Supportedness getSupported() {
    return supported;
  }

//
//  public void setIsSupported(boolean value){
//    if(value){
//      this.is_supported = value;
//    }
//  }
//
//  public int ezStatus() {
//    return knownEzStatus;
//  }
//
//  public int setEzStatus(int st) {
//    if (knownEzStatus == 1 && st == 0) {
//      st = 2;
//    }
//
//    return (knownEzStatus = st);
//  }
//
//  public String status(String def) {
//    if (isHP()) {
//      host.isBgThread();
//      return snmp.status(ip, def);
//    }
//
//    /* otherwise */
//    if (!knownStatus.equals("")) {
//      return knownStatus;
//    }
//
//    return def;
//  }
//
//  public void setStatus(String str) {
//    knownStatus = str;
//  }
//
//  public Printer extra(XJSONObject extra) {
//    extraInfo = Js.extend(extraInfo, extra);
//
//    // If we don't already have a mac, but the extra data contains one, use it
//    if (mac.equals("")) {
//      String snmpMacAddress = "";
//      try {
//        snmpMacAddress = extra.getXJSONObject("snmp").get("macAddress");
//      } catch(Exception e) {
//        //host.log_e(TAG, "Trouble (not an error) extra getting snmpMac for: " + ip.toString() + "-- the situation is handled, and probably indicates a network issue, but heres the stack, anyway:", e);
//      }
//
//      if (!snmpMacAddress.equals("")) {
//        mac = snmpMacAddress;
//      }
//    }
//
//    return this;
//  }
//
//  public XJSONObject toJsObject() {
//    return Js.extend(Js.xo("ip", ip, "port", port, "name", name, "ppdModel", ppdModel, "mac", mac), extraInfo);
//  }
//
//  public String toString() {
//    //return name + "-" + ip + "-" + displayScore() + "-" + manufacturer; 
//    //return name;
//    return name + " - " + ip;
//  }
//
//  public String toCmdLineString() {
//    // This one is good for command-line
//    return String.format("%-15s     %-25s", ip, name);
//  }
//
//  /**
//   *  The score of the display string (for sorting).
//   *
//   *  Higher is "better".
//   */
//  public int displayScore() {
//    int result = 100;
//
//    if (!this.manufacturer.equals("hp")) {
//      return 0;
//    }
//
//    // Has the user renamed the printer?  If not, it ends in [0A4C1D]
//    Matcher m = nameRe.matcher(name);
//    if (m.find()) {
//      result = result / 2;
//    }
//
//    for (Pattern favNameRe : favNameRes) {
//      if (favNameRe.matcher(name).find()) {
//        result += 100;
//      }
//    }
//
//    return result;
//  }
//
//  public static XJSONObject toJsObject(String key, Map<String, Printer> printers) {
//    return Js.xo(key, toJsObject(printers));
//  }
//
//  public static XJSONObject toJsObject(Map<String, Printer> printers) {
//    XJSONObject o = new XJSONObject();
//
//    for (Printer printer: printers.values()) {
//      o.put(printer.ip, printer.toJsObject());
//    }
//
//    return o;
//  }

}


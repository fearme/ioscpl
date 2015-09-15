
package net.printfromtheweb.mobile;

//import org.json.*;

//import java.util.*;
//import java.security.SecureRandom;
//import java.util.regex.*;
//import java.util.*;

//import android.util.Log;

public class Util {

  public static class JSONAble {
  }

  public static class JsObject {
  }


  public static class Js {

    public static class XJSONObject {
    }

//    public static XJSONObject extend(XJSONObject a, XJSONObject b) {
//      return extend(a, b, null, null, null);
//    }
//
//    public static XJSONObject extend(XJSONObject a, XJSONObject b, XJSONObject c) {
//      return extend(a, b, c, null, null);
//    }
//
//    public static XJSONObject extend(XJSONObject a, XJSONObject b, XJSONObject c, XJSONObject d) {
//      return extend(a, b, c, d, null);
//    }
//
//    public static XJSONObject extend(XJSONObject a, XJSONObject b, XJSONObject c, XJSONObject d, XJSONObject e) {
//      XJSONObject o = (a != null) ? a : new XJSONObject();
//      Iterator<String> i;
//
//      if (b != null) {
//        for (i = b.object.keys(); i.hasNext();) {
//          String key = i.next();
//          o.put(key, b.get(key, null, true)); 
//        }
//      }
//
//      if (c != null) {
//        for (i = c.object.keys(); i.hasNext();) {
//          String key = i.next();
//          o.put(key, c.get(key, null, true)); 
//        }
//      }
//
//      if (d != null) {
//        for (i = d.object.keys(); i.hasNext();) {
//          String key = i.next();
//          o.put(key, d.get(key, null, true)); 
//        }
//      }
//
//      if (e != null) {
//        for (i = e.object.keys(); i.hasNext();) {
//          String key = i.next();
//          o.put(key, e.get(key, null, true)); 
//        }
//      }
//
//      return o;
//    }

    public static class XJSONArray {
    }

//    public static JSONObject newJSONObject(JSONObject src) {
//
//      JSONObject ret = new JSONObject();
//
//      String key;
//      Iterator<String> keys = src.keys();
//      try {
//        while (keys.hasNext()) {
//          key = (String)keys.next();
//          ret.put(key, src.get(key));
//        }
//      } catch (JSONException e) {}
//
//      return ret;
//    }
//
//    public static <T> XJSONObject xo(String key, T value) {
//      XJSONObject obj = new XJSONObject();
//      put(obj, key, value);
//      return obj;
//    }
//
//    public static <T, T2> XJSONObject xo(String key, T value, String key2, T2 value2) {
//      return put(xo(key, value), key2, value2);
//    }
//
//    public static <T, T2, T3> XJSONObject xo(String key, T value, String key2, T2 value2, String key3, T3 value3) {
//      return put(xo(key, value, key2, value2), key3, value3);
//    }
//
//    public static <T, T2, T3, T4> XJSONObject xo(String key, T value, String key2, T2 value2, String key3, T3 value3, String key4, T4 value4) {
//      return put(xo(key, value, key2, value2, key3, value3), key4, value4);
//    }
//
//    public static <T, T2, T3, T4, T5> XJSONObject xo(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5)
//    {
//      return put(xo(key, value, key2, value2, key3, value3, key4, value4), key5, value5);
//    }
//
//    public static <T, T2, T3, T4, T5, T6> XJSONObject xo(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5,
//        String key6, T6 value6)
//    {
//      return put(xo(key, value, key2, value2, key3, value3, key4, value4, key5, value5), key6, value6);
//    }
//
//    public static <T, T2, T3, T4, T5, T6, T7> XJSONObject xo(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5,
//        String key6, T6 value6,
//        String key7, T7 value7)
//    {
//      return put(xo(key, value, key2, value2, key3, value3, key4, value4, key5, value5, key6, value6), key7, value7);
//    }
//
//    public static <T, T2, T3, T4, T5, T6, T7, T8> XJSONObject xo(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5,
//        String key6, T6 value6,
//        String key7, T7 value7,
//        String key8, T8 value8)
//    {
//      return put(xo(key, value, key2, value2, key3, value3, key4, value4, key5, value5, key6, value6, key7, value7), key8, value8);
//    }
//
//    public static <T> XJSONObject put(XJSONObject xo, String key, T value) {
//      return xo.put(key, value);
//    }
//
//    public static JSONObject o() {
//      return new JSONObject();
//    }
//
//    public static <T> JSONObject o(String key, T value) {
//      JSONObject obj = new JSONObject();
//      put(obj, key, value);
//      return obj;
//    }
//
//    public static <T, T2> JSONObject o(String key, T value, String key2, T2 value2) {
//      return put(o(key, value), key2, value2);
//    }
//
//    public static <T, T2, T3> JSONObject o(String key, T value, String key2, T2 value2, String key3, T3 value3) {
//      return put(o(key, value, key2, value2), key3, value3);
//    }
//
//    public static <T, T2, T3, T4> JSONObject o(String key, T value, String key2, T2 value2, String key3, T3 value3, String key4, T4 value4) {
//      return put(o(key, value, key2, value2, key3, value3), key4, value4);
//    }
//
//    public static <T, T2, T3, T4, T5> JSONObject o(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5)
//    {
//      return put(o(key, value, key2, value2, key3, value3, key4, value4), key5, value5);
//    }
//
//    public static <T, T2, T3, T4, T5, T6> JSONObject o(
//        String key,  T  value,
//        String key2, T2 value2, 
//        String key3, T3 value3,
//        String key4, T4 value4,
//        String key5, T5 value5,
//        String key6, T6 value6)
//    {
//      return put(o(key, value, key2, value2, key3, value3, key4, value4, key5, value5), key6, value6);
//    }
//
//    public static JSONObject extend(JSONObject a, JSONObject b) {
//      return extend(a, b, null, null, null);
//    }
//
//    public static JSONObject extend(JSONObject a, JSONObject b, JSONObject c) {
//      return extend(a, b, c, null, null);
//    }
//
//    public static JSONObject extend(JSONObject a, JSONObject b, JSONObject c, JSONObject d) {
//      return extend(a, b, c, d, null);
//    }
//
//    /**
//     *  Copy a key-value from b onto a, recursing, if necessary.
//     */
//    public static void extend_it(String key, JSONObject a, JSONObject b) {
//      boolean done = false;
//      Iterator<String> i;
//      String value;
//
//      try {
//        boolean isNewA = false;
//        JSONObject subB = b.getJSONObject(key);
//
//        JSONObject subA;
//        try {
//          subA = a.getJSONObject(key);
//        } catch(JSONException dontcare) {
//          subA = new JSONObject();
//          isNewA = true;
//        }
//
//        for (i = subB.keys(); i.hasNext();) {
//          String subKey = i.next();
//          extend_it(subKey, subA, subB);
//        }
//
//        if (isNewA) {
//          a.put(key, subA);
//        }
//
//        return;
//      } catch(JSONException dontcare) {}
//
//      try {
//        a.put(key, b.getDouble(key));
//        return;
//      } catch(JSONException dontcare) {}
//
//      try {
//        a.put(key, b.getLong(key));
//        return;
//      } catch(JSONException dontcare) {}
//
//      try {
//        a.put(key, b.getInt(key));
//        return;
//      } catch(JSONException dontcare) {}
//
//      try {
//        value = b.getString(key);
//
//        // If we get here, we may be a string, or may be "true" or "false"
//        if (value.equals("true"))         { a.put(key, true); }
//        else if (value.equals("false"))   { a.put(key, false); }
//        else                              { a.put(key, value); }
//
//        return;
//      } catch(JSONException dontcare) {}
//
//      try {
//        a.put(key, b.get(key));
//      } catch(JSONException dontcare) {}
//
//      try {
//        a.put(key, b.getBoolean(key));
//        return;
//      } catch(JSONException dontcare) {}
//    }
//
//    public static JSONObject extend(JSONObject a, JSONObject b, JSONObject c, JSONObject d, JSONObject e) {
//      JSONObject o = (a != null) ? a : new JSONObject();
//      Iterator<String> i;
//
//      if (b != null) {
//        for (i = b.keys(); i.hasNext();) {
//          String key = i.next();
//          extend_it(key, o, b);
//        }
//      }
//
//      if (c != null) {
//        for (i = c.keys(); i.hasNext();) {
//          String key = i.next();
//          extend_it(key, o, c);
//        }
//      }
//
//      if (d != null) {
//        for (i = d.keys(); i.hasNext();) {
//          String key = i.next();
//          extend_it(key, o, d);
//        }
//      }
//
//      if (e != null) {
//        for (i = e.keys(); i.hasNext();) {
//          String key = i.next();
//          extend_it(key, o, e);
//        }
//      }
//
//      return o;
//    }
//
//    public static <T> JSONObject put(JSONObject o, String key, T value) {
//      try {
//        o.put(key, value);
//      } catch (JSONException e) {}
//      return o;
//    }
//
//    public static String get(JSONObject o, String key) {
//      try {
//        return o.getString(key);
//      } catch (JSONException e) {}
//      return "";
//    }
//
//    public static int getInt(JSONObject o, String key) {
//      try {
//        return o.getInt(key);
//      } catch (JSONException e) {}
//      return 0;
//    }
//
//    public static boolean getBoolean(JSONObject o, String key) {
//
//      if (o == null) { return false; }
//
//      try {
//        return o.getBoolean(key);
//      } catch (JSONException e) {}
//      return false;
//    }
//
//    public static Object getObject(JSONObject o, String key) {
//      try {
//        return o.get(key);
//      } catch(JSONException e) {}
//
//      return null;
//    }
//
//    public static JSONObject getJSONObject(JSONObject o, String key) {
//      return getJSONObject(o, key, new JSONObject());
//    }
//
//    public static JSONObject getJSONObject(JSONObject o, String key, JSONObject def) {
//      try {
//        return (JSONObject)o.get(key);
//      } catch (JSONException e) {}
//      return def;
//    }
//
//
//
//    public static <T> JSONArray a(T value) {
//      JSONArray arr = new JSONArray();
//      push(arr, value);
//      return arr;
//    }
//
//    public static <T, T2> JSONArray a(T value, T2 value2) {
//      return push(a(value), value2);
//    }
//
//    public static <T, T2, T3> JSONArray a(T value, T2 value2, T3 value3) {
//      return push(a(value, value2), value3);
//    }
//
//    public static <T, T2, T3, T4> JSONArray a(T value, T2 value2, T3 value3, T4 value4) {
//      return push(a(value, value2, value3), value4);
//    }
//
//    public static <T, T2, T3, T4, T5> JSONArray a(T value, T2 value2, T3 value3, T4 value4, T5 value5) {
//      return push(a(value, value2, value3, value4), value5);
//    }
//
//    public static <T> JSONArray push(JSONArray a, T value) {
//      a.put(value);
//      return a;
//    }
//
//    public static String at(JSONArray a, int index) {
//      try {
//        return a.getString(index);
//      } catch (JSONException e) {}
//
//      return "";
//    }
//
//    public static int at_int(JSONArray a, int index) {
//      return atInt(a, index);
//    }
//
//    public static int atInt(JSONArray a, int index) {
//      try {
//        return a.getInt(index);
//      } catch (JSONException e) {}
//
//      return 0;
//    }
//
//    public static JSONObject atJSONObject(JSONArray a, int index) {
//      try {
//        return a.getJSONObject(index);
//      } catch (JSONException e) {}
//
//      return new JSONObject();
//    }
//
  }
//
//  public static <T> T or(T a, T def) {
//    if (a != null) {
//      return a;
//    }
//
//    return def;
//  }
//
//  public static Map<String, String> toMap(JSONObject o) {
//    Map<String, String> map = new HashMap<String, String>();
//
//    if (o != null) {
//      for (Iterator<String> keys = o.keys(); keys.hasNext();) {
//        String key = (String)keys.next();
//        if (o.has(key) && Js.get(o, key) instanceof String) {
//          map.put(key, (String)Js.get(o, key));
//        }
//      }
//    }
//
//    return map;
//  }
//
//  public static Map<String, Map<String, String>> toDeepMap(JSONObject o) {
//    Map<String, Map<String, String>> map = new HashMap<String, Map<String, String>>();
//
//    if (o != null) {
//      for (Iterator<String> keys = o.keys(); keys.hasNext();) {
//        String key = (String)keys.next();
//        if (o.has(key) && Js.getObject(o, key) instanceof JSONObject) {
//          map.put(key, toMap(Js.getJSONObject(o, key)));
//        }
//      }
//    }
//
//    return map;
//  }
//
//  private static char[] charSet = "abcdefghijklmnopqrstuvwxyz0123456789".toCharArray();
//  public static String randomString(int length) {
//    Random random = new SecureRandom();
//    char[] result = new char[length];
//
//    for (int i = 0; i < result.length; i++) {
//      result[i] = charSet[random.nextInt(charSet.length)];
//    }
//
//    return new String(result);
//  }
//
//  private static final Pattern             filenamePattern   = Pattern.compile(".*/([^/]+)");
//  public static String fname(String path) {
//    Matcher m = filenamePattern.matcher(path);
//    if (m.find()) {
//      return m.group(1);
//    }
//
//    return "";
//  }

}


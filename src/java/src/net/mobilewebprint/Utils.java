package net.mobilewebprint;


public class Utils {

  public static int atoi(String str) {
    if (str == null || str.length() == 0) { return 0; }

    str = str.trim();

    char flag = '+';

    int i = 0;
    if (str.charAt(i) == '-') {
      flag = '-';
      i += 1;
    } else if (str.charAt(i) == '+') {
      i += 1;
    }

    double result = 0.0;

    for (; str.length() > i && str.charAt(i) >= '0' && str.charAt(i) <= '9'; ++i) {
      result = result * 10 + (str.charAt(i) - '0');
    }

    if (flag == '-') {
      result = -result;
    }

    if (result > Integer.MAX_VALUE) { return Integer.MAX_VALUE; }
    if (result < Integer.MIN_VALUE) { return Integer.MIN_VALUE; }
    
    return (int)result;
  }
}


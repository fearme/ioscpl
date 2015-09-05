
#include <jni.h>
#include <android/log.h>
//#include "hp_mwp.h"
#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include <string>

using std::string;
//using net_mobilewebprint::get_flag;

////using namespace net_mobilewebprint;

static bool      secure                     = false;
static JavaVM *  g_jvm                      = NULL;
static jobject   jClient                    = NULL;
static jobject   jLifeline                  = NULL;

static jmethodID onBootstrapId              = NULL;
static jmethodID onNewPrinterListId         = NULL;
static jmethodID onBeginPrinterChangesId    = NULL;
static jmethodID onPrinterAttributeId       = NULL;
static jmethodID onRemovePrinterAttributeId = NULL;
static jmethodID onRemovePrinterId          = NULL;
static jmethodID onEndPrinterEnumerationId  = NULL;
static jmethodID onPrintJobProgressId       = NULL;

static jmethodID setDictionaryItemsId       = NULL;
static jmethodID sendMessageId              = NULL;

////jclass findClass(char const * name);
////static JNIEnv *             g_env = NULL;

////static jstring to_jstring(JNIEnv *env, string const & str);
////static jstring to_jstring(JNIEnv *env, char const * str);
static string  to_string(JNIEnv * env, jstring jstr);
static jobject storeJObject(JNIEnv *, jobject client, jobject lifeline);

// ===============================================================================================
// ========================= Init and setup ======================================================
// ===============================================================================================
extern "C" JNIEXPORT jstring JNICALL Java_net_mobilewebprint_Client_initJni(JNIEnv *env, jobject self, jobject lifeline)
{
  // Store our lifeline into the Java "heap", so we don't get released
  storeJObject(env, self, lifeline);
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_setSecureMode(JNIEnv *env, jobject self)
{
  secure = true;
  return true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_start(JNIEnv *env, jobject self)
{
  return true;
//  log_d("Client::start");
//  storeClient(env, self, NULL);
//
//  get_api()->set_option("quiet", false);
//  get_api()->set_option("vvverbose", true);
//  //get_api()->set_option("fast-fail", true);
//
//  if (!secure) {
//    core_api()->register_handler("printer_list", NULL, mwp_app_callback);
//  } else {
//    sap_api()->register_handler("printer_list", NULL, sap_app_callback);
//  }
//
//  return get_api()->start(true, false);
}

// ===================================================================================================================================
//      Client JNI methods
// ===================================================================================================================================

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_setOption(JNIEnv *env, jobject self, jstring name, jstring value)
{
//  get_api()->set_option(to_string(env, name).c_str(), to_string(env, value).c_str());
}

// ===================================================================================================================================
//             JNI helpers
// ===================================================================================================================================


JNIEnv * getEnv()
{
  JNIEnv * env = NULL;
  if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) >= 0) { return env; }

  if (g_jvm->AttachCurrentThread(&env, NULL) >= 0)       { return env; }

  return NULL;
}

jobject storeJObject(JNIEnv * env, jobject client, jobject lifeline)
{
  jmethodID methodId  = NULL;
  jclass    jClass    = NULL;
  if (!jClient) {
    jClient       = env->NewGlobalRef(client);
  }

  if (lifeline && !jLifeline) {
    jLifeline  = env->NewGlobalRef(lifeline);

    if ((jClass = env->FindClass("net/mobilewebprint/Client")) != NULL) {
      // public void onNewPrinterList();
      onBootstrapId               = env->GetMethodID(jClass, "onBootstrap", "()V");
      onNewPrinterListId          = env->GetMethodID(jClass, "onNewPrinterList", "()V");
      onBeginPrinterChangesId     = env->GetMethodID(jClass, "onBeginPrinterChanges", "()V");
      onPrinterAttributeId        = env->GetMethodID(jClass, "onPrinterAttribute", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
      onRemovePrinterAttributeId  = env->GetMethodID(jClass, "onRemovePrinterAttribute", "(Ljava/lang/String;Ljava/lang/String;)V");
      onRemovePrinterId           = env->GetMethodID(jClass, "onRemovePrinter", "(Ljava/lang/String;)V");
      onEndPrinterEnumerationId   = env->GetMethodID(jClass, "onEndPrinterEnumeration", "()V");
      onPrintJobProgressId        = env->GetMethodID(jClass, "onPrintJobProgress", "(Ljava/lang/String;IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

      setDictionaryItemsId        = env->GetMethodID(jClass, "setDictionaryItems", "(Ljava/lang/String;)V");
      sendMessageId               = env->GetMethodID(jClass, "sendMessage", "(IIIIIIIIII)Z");

      // Now do the bootstrap
      if (onBootstrapId != NULL) {
        getEnv()->CallVoidMethod(jClass, onBootstrapId);
      }
    }
  }

  return jClient;
}

jstring to_jstring(JNIEnv *env, string const & str)
{
  return to_jstring(env, str.c_str());
}

jstring to_jstring(JNIEnv *env, char const * str)
{
  if (str == NULL) { return env->NewStringUTF(""); }
  return env->NewStringUTF(str);
}

jstring to_jstring(string const & str)
{
  return to_jstring(getEnv(), str.c_str());
}

jstring to_jstring(char const * str)
{
  if (str == NULL) { return getEnv()->NewStringUTF(""); }
  return getEnv()->NewStringUTF(str);
}

string to_string(JNIEnv *env, jstring jstr) {
  string result;

  if (jstr) {
    char const * pch = env->GetStringUTFChars(jstr, 0);
    result = string(pch);
    env->ReleaseStringUTFChars(jstr, pch);
  }

  return result;
}

// ===============================================================================================
// ===============================================================================================
// ===============================================================================================
extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logD(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
////  if (get_flag("quiet")) { return; }

  __android_log_write(ANDROID_LOG_DEBUG, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logV(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
////  if (!get_flag("verbose")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logE(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  __android_log_write(ANDROID_LOG_ERROR, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

//void net_mobilewebprint::log_d(char const * msg, log_param_t x)
//{
//  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
//}

//void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
//{
//////  if (get_flag("quiet")) { return; }
//
//  __android_log_write(ANDROID_LOG_DEBUG, tag, msg);
//}

//void net_mobilewebprint::log_w(char const * msg, log_param_t)
//{
//////  if (get_flag("no_warn")) { return; }
//
//  __android_log_write(ANDROID_LOG_WARN, "MobileWebPrint", msg);
//}

//void net_mobilewebprint::log_v(char const * msg, log_param_t x)
//{
//  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
//}

//void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
//{
//////  if (!get_flag("verbose")) { return; }
//
//  __android_log_write(ANDROID_LOG_VERBOSE, tag, msg);
//}

//void net_mobilewebprint::log_e(char const * msg, log_param_t)
//{
//  __android_log_write(ANDROID_LOG_ERROR, "MobileWebPrint", msg);
//}


////jstring to_jstring(JNIEnv *env, string const & str)
////{
////  return to_jstring(env, str.c_str());
////}
////
////jstring to_jstring(JNIEnv *env, char const * str)
////{
////  return env->NewStringUTF(str);
////}

////// From: http://stackoverflow.com/questions/13263340/findclass-from-any-thread-in-android-jni
//////
////
////JNIEnv * getEnv();
////
////JavaVM * g_jvm = NULL;
////static jobject g_classLoader;
////static jmethodID g_findClassMethod;
////
////extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * pjvm, void * /*reserved*/)
////{
////  g_jvm = pjvm;
////  JNIEnv * env = getEnv();
////  jclass controllerClass = env->FindClass("net/printfromtheweb/mobile/Controller");
////  jclass classClass = env->GetObjectClass(controllerClass);
////
////  jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
////  jmethodID getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
////
////  g_classLoader = env->CallObjectMethod(controllerClass, getClassLoaderMethod);
////  g_findClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
////
////  return JNI_VERSION_1_6;
////}
////
////jclass findClass(char const * name)
////{
////  jclass result = static_cast<jclass>(getEnv()->CallObjectMethod(g_classLoader, g_findClassMethod, getEnv()->NewStringUTF(name)));
////
////  if (getEnv()->ExceptionCheck()) {
////    // TODO: tell the developer.
////    getEnv()->ExceptionClear();
////    return NULL;
////  }
////
////  return result;
////}
////
////JNIEnv * getEnv()
////{
////  JNIEnv * env = NULL;
////  if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) >= 0) { return env; }
////
////  if (g_jvm->AttachCurrentThread(&env, NULL) >= 0)       { return env; }
////
////  return NULL;
////}




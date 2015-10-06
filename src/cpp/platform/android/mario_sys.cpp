/**
 *  Implementations of functions that need the system APIs -- Android implementation.
 *
 *  Mario core has all the network functionality that Mario-client needs, and is
 *  centered on the BSD-socket APIs, which are "universally" implemented.  However,
 *  Mario needs a handful of other system APIs (like a function to start a new 
 *  thread).  This file supplies that functionality for this above-named platform.
 *
 *  The C/C++ side of the JNI layer is also implemented in this file.
 */


#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include "mwp_core_api.hpp"
#include "mwp_secure_asset_printing_api.hpp"

#include <string>
//#include <stdio.h>
//#include <pthread.h>
//#include <unistd.h>
//#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include <jni.h>
#include <android/log.h>

using std::string;
using namespace net_mobilewebprint;

static bool secure = false;

static int mwp_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, mwp_params const * params);
static int sap_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, sap_params const * params);
static core_api_t * get_api();

static string  to_string(JNIEnv * env, jstring jstr);
static jstring to_jstring(JNIEnv *env, string const & str);
static jstring to_jstring(JNIEnv *env, char const * str);
static jstring to_jstring(string const & str);
static jstring to_jstring(char const * str);

static JavaVM * g_jvm                       = NULL;
static JNIEnv * g_env                       = NULL;
static jobject jClient                      = NULL;
static jobject jApplication                 = NULL;

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

static JNIEnv * getEnv();
static jobject  storeClient(JNIEnv *, jobject client, jobject application);

// ===================================================================================================================================
//             Client JNI methods
// ===================================================================================================================================

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * pjvm, void * /*reserved*/)
{
  g_jvm = pjvm;
  getEnv();
  return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_initJni(JNIEnv *env, jobject self, jobject application)
{
  // Grab a reference to "self" to keep everything alive and reference-counted.
  storeClient(env, self, application);

  return true;
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_setSecureMode(JNIEnv *env, jobject self)
{
  secure = true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_start(JNIEnv *env, jobject self)
{
  log_d("Client::start");
  storeClient(env, self, NULL);

  //get_api()->set_option("quiet", false);
  //get_api()->set_option("vvverbose", true);
  //get_api()->set_option("fast-fail", true);

  if (!secure) {
    core_api()->register_handler("printer_list", NULL, mwp_app_callback);
  } else {
    sap_api()->register_handler("printer_list", NULL, sap_app_callback);
  }

  return get_api()->start(true, false);
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_reScan(JNIEnv *env, jobject self)
{
  log_d("Client::reScan");

  return get_api()->reScan();
}

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_sendJob(JNIEnv *env, jobject self, jstring url, jstring printer_ip)
{
  return get_api()->send_job(to_string(env, url).c_str(), to_string(env, printer_ip).c_str());
}


extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_sendImmediately(JNIEnv *env, jobject self, jstring msg_name, jstring payload)
{
  return get_api()->send_immediately(to_string(env, msg_name).c_str(), to_string(env, payload).c_str());
}



extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_setOption(JNIEnv *env, jobject self, jstring name, jstring value)
{
  get_api()->set_option(to_string(env, name).c_str(), to_string(env, value).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_setIntOption(JNIEnv *env, jobject self, jstring name, jint value)
{
  get_api()->set_option(to_string(env, name).c_str(), value);
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_setFlag(JNIEnv *env, jobject self, jstring name, jboolean value)
{
  get_api()->set_option(to_string(env, name).c_str(), value);
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_clearFlag(JNIEnv *env, jobject self, jstring name)
{
  get_api()->set_option(to_string(env, name).c_str(), false);
}

// ===================================================================================================================================
//             End Client JNI methods
// ===================================================================================================================================

int next_string_dict_index = 1;
std::map<std::string, int> string_dictionary;
int ensure_is_in(string const & str, string & diffs, int & num_diffs)
{
  int result = -1;

  std::map<std::string, int>::iterator it = string_dictionary.find(str);
  if (it == string_dictionary.end()) {
    result = next_string_dict_index++;
    string_dictionary.insert(std::make_pair(str, result));
    diffs += mwp_itoa(result);
    diffs += "~~~";
    diffs += str + "\n";
    num_diffs += 1;
    return result;
  }

  /* otherwise */
  return (*it).second;
}

template <typename T>
int app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, T const * params)
{

  if        (strcmp(msg, HP_MWP_BEGIN_NEW_PRINTER_LIST_MSG) == 0) {
    if (onNewPrinterListId != NULL) {
      getEnv()->CallVoidMethod(jApplication, onNewPrinterListId);
    }

  } else if (strcmp(msg, HP_MWP_BEGIN_PRINTER_CHANGES_MSG) == 0) {
    if (onBeginPrinterChangesId != NULL) {
      getEnv()->CallVoidMethod(jApplication, onBeginPrinterChangesId);
    }

  } else if (strcmp(msg, HP_MWP_PRINTER_ATTRIBUTE_MSG) == 0) {
    if (setDictionaryItemsId != NULL && sendMessageId != NULL) {
      string diffs;
      int num_diffs = 0;

      int n_msg = ensure_is_in("onPrinterAttribute", diffs, num_diffs);
      int n1    = ensure_is_in((char const *)p1, diffs, num_diffs);
      int n2    = ensure_is_in((char const *)(params != NULL && params->p2 != NULL ? (char const *)params->p2 : ""), diffs, num_diffs);
      int n3    = ensure_is_in((char const *)(params != NULL && params->p3 != NULL ? (char const *)params->p3 : ""), diffs, num_diffs);

      int n4    = 0;
      int n5    = 0;
      int n6    = 0;
      int n7    = 0;


      jstring jdiffs;
      if (num_diffs > 0) {
        jdiffs = to_jstring(diffs);
        getEnv()->CallVoidMethod(jApplication, setDictionaryItemsId, jdiffs);
      }

      getEnv()->CallBooleanMethod(jApplication, sendMessageId, n_msg, 3, n1, n2, n3, n4, n5, n6, n7);

      if (num_diffs > 0) {
        getEnv()->DeleteLocalRef(jdiffs);
      }
    }

  } else if (strcmp(msg, HP_MWP_RM_PRINTER_ATTRIBUTE_MSG) == 0) {
    if (onRemovePrinterAttributeId != NULL) {
      getEnv()->CallVoidMethod(jApplication, onRemovePrinterAttributeId);
    }

  } else if (strcmp(msg, HP_MWP_RM_PRINTER_MSG) == 0) {
    if (setDictionaryItemsId != NULL && sendMessageId != NULL) {
      string diffs;
      int num_diffs = 0;

      int n_msg = ensure_is_in("onRemovePrinter", diffs, num_diffs);
      int n1    = ensure_is_in((char const *)p1, diffs, num_diffs);
      int n2    = 0;
      int n3    = 0;

      int n4    = 0;
      int n5    = 0;
      int n6    = 0;
      int n7    = 0;
      int n8    = 0;


      jstring jdiffs;
      if (num_diffs > 0) {
        jdiffs = to_jstring(diffs);
        getEnv()->CallVoidMethod(jApplication, setDictionaryItemsId, jdiffs);
      }

      getEnv()->CallBooleanMethod(jApplication, sendMessageId, n_msg, 1, n1, n2, n3, n4, n5, n6, n7, n8);

      if (num_diffs > 0) {
        getEnv()->DeleteLocalRef(jdiffs);
      }
    }

  } else if (strcmp(msg, HP_MWP_END_PRINTER_ENUM_MSG) == 0) {
    if (onEndPrinterEnumerationId != NULL) {
      getEnv()->CallVoidMethod(jApplication, onEndPrinterEnumerationId);
    }

  } else if (strcmp(msg, HP_MWP_PRINT_PROGRESS_MSG) == 0) {
    if (setDictionaryItemsId != NULL && sendMessageId != NULL) {
      string diffs;
      int num_diffs = 0;

      int n_msg = ensure_is_in("onPrintJobProgress", diffs, num_diffs);
      int n1    = ensure_is_in((char const *)p1, diffs, num_diffs);
      int n2    = ensure_is_in((char const *)(params != NULL && params->p2 != NULL ? (char const *)params->p2 : ""), diffs, num_diffs);
      int n3    = ensure_is_in((char const *)(params != NULL && params->p3 != NULL ? (char const *)params->p3 : ""), diffs, num_diffs);
      int n4    = ensure_is_in((char const *)(params != NULL && params->p4 != NULL ? (char const *)params->p4 : ""), diffs, num_diffs);
      int n5    = ensure_is_in((char const *)(params != NULL && params->p5 != NULL ? (char const *)params->p5 : ""), diffs, num_diffs);
      int n6    = params->n1;
      int n7    = params->n2;
      int n8    = 0;


      jstring jdiffs;
      if (num_diffs > 0) {
        jdiffs = to_jstring(diffs);
        getEnv()->CallVoidMethod(jApplication, setDictionaryItemsId, jdiffs);
      }

      getEnv()->CallBooleanMethod(jApplication, sendMessageId, n_msg, 5, n1, n2, n3, n4, n5, n6, n7, n8);

      if (num_diffs > 0) {
        getEnv()->DeleteLocalRef(jdiffs);
      }
    }
  }
}

int mwp_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, mwp_params const * params)
{
  return app_callback<mwp_params>(app_data, msg, ident, transaction_id, p1, params);
}

int sap_app_callback(void * app_data, char const * msg, int ident, int32 transaction_id, uint8 const * p1, sap_params const * params)
{
  return app_callback<sap_params>(app_data, msg, ident, transaction_id, p1, params);
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logD(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  if (get_flag("quiet")) { return; }

  __android_log_write(ANDROID_LOG_DEBUG, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logV(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  if (!get_flag("verbose")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logW(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  if (get_flag("no_warn")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logE(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  __android_log_write(ANDROID_LOG_ERROR, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

core_api_t * get_api()
{
  if (secure) {
    return sap_api();
  }

  return core_api();
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

jobject storeClient(JNIEnv * env, jobject client, jobject application)
{
  jmethodID methodId  = NULL;
  jclass    jClass    = NULL;
  if (!jClient) {
    jClient       = env->NewGlobalRef(client);
  }

  if (application && !jApplication) {
    jApplication  = env->NewGlobalRef(application);

    if ((jClass = env->FindClass("net/mobilewebprint/Application")) != NULL) {
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
        getEnv()->CallVoidMethod(jApplication, onBootstrapId);
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

// ===================================================================================================================================
//             Implement system functions
// ===================================================================================================================================

char const * net_mobilewebprint::platform_name()
{
  return "android";
}

// Convert the system-preferred string type to US-ASCII -- the network APIs
// use US-ASCII.  For example, Windows prefers UNICODE as the string type
// that gets passed around.  This platform uses 'normal' C (utf-8) strings.
string net_mobilewebprint::platform_to_ascii_string(void const *str)
{
  return net_mobilewebprint::dumb_and_ok::platform_to_ascii_string(str);
}

// ----------------------------------------------------------------------------------
// Mutex
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::allocate_host_lock(char const * name, void ** result_lock_out)
{
  return net_mobilewebprint::POSIX::allocate_host_lock(name, result_lock_out);
}

bool net_mobilewebprint::host_lock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::host_lock(name, lock_);
}

bool net_mobilewebprint::host_unlock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::host_unlock(name, lock_);
}

void net_mobilewebprint::free_host_lock(char const * name, void * lock_)
{
  return net_mobilewebprint::POSIX::free_host_lock(name, lock_);
}

// ----------------------------------------------------------------------------------
// Threads
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::start_thread(void*(*pfn)(void*data), void*data)
{
  return net_mobilewebprint::POSIX::start_thread(pfn, data);
}

// ----------------------------------------------------------------------------------
// Time
// ----------------------------------------------------------------------------------
uint32 net_mobilewebprint::get_tick_count()
{
  return net_mobilewebprint::POSIX::get_tick_count();
}

bool net_mobilewebprint::interruptable_sleep(int msec)
{
  return net_mobilewebprint::POSIX::interruptable_sleep(msec);
}

// ----------------------------------------------------------------------------------
// Socket operations
// ----------------------------------------------------------------------------------
bool net_mobilewebprint::set_socket_non_blocking(int fd)
{
  return net_mobilewebprint::POSIX::set_socket_non_blocking(fd);
}

bool net_mobilewebprint::set_socket_blocking(int fd)
{
  return net_mobilewebprint::POSIX::set_socket_blocking(fd);
}

bool net_mobilewebprint::connect_in_progress()
{
  return net_mobilewebprint::POSIX::connect_in_progress();
}

int net_mobilewebprint::get_last_network_error()
{
  return net_mobilewebprint::POSIX::get_last_network_error();
}

// ----------------------------------------------------------------------------------
// Assert
// ----------------------------------------------------------------------------------
void * net_mobilewebprint::mwp_assert(void * x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}

int    net_mobilewebprint::mwp_assert(int x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}

bool   net_mobilewebprint::mwp_assert(bool x)
{
  return net_mobilewebprint::dumb_and_ok::mwp_assert(x);
}


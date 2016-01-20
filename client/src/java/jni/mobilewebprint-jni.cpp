
#include <jni.h>
#include <android/log.h>
#include "hp_mwp.h"
#include "mwp_types.hpp"
#include "mwp_controller.hpp"
#include <string>

using std::string;
using net_mobilewebprint::get_flag;

//using namespace net_mobilewebprint;

//jclass findClass(char const * name);
//static JNIEnv *             g_env = NULL;

//static jstring to_jstring(JNIEnv *env, string const & str);
//static jstring to_jstring(JNIEnv *env, char const * str);
static string  to_string(JNIEnv * env, jstring jstr);

extern "C" JNIEXPORT jboolean JNICALL Java_net_mobilewebprint_Client_initJni(JNIEnv *env, jobject self)
{
  // TODO: grab a reference to "self" to keep the everything alive and reference-counted.
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logD(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
//  if (get_flag("quiet")) { return; }

  __android_log_write(ANDROID_LOG_DEBUG, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logV(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
//  if (!get_flag("verbose")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_net_mobilewebprint_Client_logE(JNIEnv *env, jobject self, jstring tag, jstring msg)
{
  __android_log_write(ANDROID_LOG_ERROR, to_string(env, tag).c_str(), to_string(env, msg).c_str());
}

void net_mobilewebprint::log_d(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_d(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_d(char const * msg, char const * tag, log_param_t)
{
//  if (get_flag("quiet")) { return; }

  __android_log_write(ANDROID_LOG_DEBUG, tag, msg);
}

void net_mobilewebprint::log_w(char const * msg, log_param_t)
{
//  if (get_flag("no_warn")) { return; }

  __android_log_write(ANDROID_LOG_WARN, "MobileWebPrint", msg);
}

void net_mobilewebprint::log_v(char const * msg, log_param_t x)
{
  net_mobilewebprint::log_v(msg, "MobileWebPrint", x);
}

void net_mobilewebprint::log_v(char const * msg, char const * tag, log_param_t)
{
//  if (!get_flag("verbose")) { return; }

  __android_log_write(ANDROID_LOG_VERBOSE, tag, msg);
}

void net_mobilewebprint::log_e(char const * msg, log_param_t)
{
  __android_log_write(ANDROID_LOG_ERROR, "MobileWebPrint", msg);
}


//jstring to_jstring(JNIEnv *env, string const & str)
//{
//  return to_jstring(env, str.c_str());
//}
//
//jstring to_jstring(JNIEnv *env, char const * str)
//{
//  return env->NewStringUTF(str);
//}

string to_string(JNIEnv *env, jstring jstr) {
  string result;

  char const * pch = env->GetStringUTFChars(jstr, 0);
  result = string(pch);
  env->ReleaseStringUTFChars(jstr, pch);

  return result;
}

//// From: http://stackoverflow.com/questions/13263340/findclass-from-any-thread-in-android-jni
////
//
//JNIEnv * getEnv();
//
//JavaVM * g_jvm = NULL;
//static jobject g_classLoader;
//static jmethodID g_findClassMethod;
//
//extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * pjvm, void * /*reserved*/)
//{
//  g_jvm = pjvm;
//  JNIEnv * env = getEnv();
//  jclass controllerClass = env->FindClass("net/printfromtheweb/mobile/Controller");
//  jclass classClass = env->GetObjectClass(controllerClass);
//
//  jclass classLoaderClass = env->FindClass("java/lang/ClassLoader");
//  jmethodID getClassLoaderMethod = env->GetMethodID(classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
//
//  g_classLoader = env->CallObjectMethod(controllerClass, getClassLoaderMethod);
//  g_findClassMethod = env->GetMethodID(classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
//  
//  return JNI_VERSION_1_6;
//}
//
//jclass findClass(char const * name)
//{
//  jclass result = static_cast<jclass>(getEnv()->CallObjectMethod(g_classLoader, g_findClassMethod, getEnv()->NewStringUTF(name)));
//
//  if (getEnv()->ExceptionCheck()) {
//    // TODO: tell the developer.
//    getEnv()->ExceptionClear();
//    return NULL;
//  }
//
//  return result;
//}
//  
//JNIEnv * getEnv()
//{
//  JNIEnv * env = NULL;
//  if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) >= 0) { return env; }
//
//  if (g_jvm->AttachCurrentThread(&env, NULL) >= 0)       { return env; }
//
//  return NULL;
//}




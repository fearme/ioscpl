/**
 *
 * sap
 * SAP
 */

#ifndef __HP_SAP_H__
#define __HP_SAP_H__

/* What platform are we on? */
#ifdef __ANDROID__
  #define HAVE_SIOCGARP 1
#else
#endif

/* Startup flags */
#define HP_SAP_START_SCANNING 0x00000001
#define HP_SAP_BLOCK_START_FN 0x00000002

/* Startup flags that cannot be part of Secure Assert Printing */
#define HP_SAP_NON_SAP        0xff000000
#define HP_SAP_START_LOCAL    HP_SAP_NON_SAP & 0x01000000

/* Default startup flags: 1) start the scanner, 2) do not return from start function 'cuz
 * the app called the start function from a thread dedicated for SAP 
 */
#define HP_SAP_START_APP_DEFAULT_FLAGS (HP_SAP_START_SCANNING /*| HP_SAP_BLOCK_START_FN*/)
#define HP_SAP_START_CLI_DEFAULT_FLAGS (HP_SAP_START_SCANNING | HP_SAP_BLOCK_START_FN)

#define STATUS_WAITING0                   "WAITING0"
#define STATUS_WAITING1                   "WAITING1"
#define STATUS_PRINTING                   "PRINTING"
#define STATUS_WAITING2                   "WAITING2"
#define STATUS_CANCELLED                  "CANCELLED"
#define STATUS_CANCELLING                 "CANCELLING"
#define STATUS_SUCCESS                    "SUCCESS"

/**
 *  Messages that the app can listen for.
 */
#define HP_SAP_BEGIN_NEW_PRINTER_LIST_MSG "begin_new_printer_list"
#define HP_SAP_BEGIN_PRINTER_CHANGES_MSG  "begin_printer_changes"

#define HP_SAP_PRINTER_ATTRIBUTE_MSG      "printer_attribute"
  #define HP_SAP_ATTR_IP                    "ip"
  #define HP_SAP_ATTR_NAME                  "name"
  #define HP_SAP_ATTR_1284_DEVICE_ID        "1284_device_id"

#define HP_SAP_RM_PRINTER_ATTRIBUTE_MSG   "rm_printer_attribute"
#define HP_SAP_RM_PRINTER_MSG             "rm_printer"

#define HP_SAP_END_PRINTER_ENUM_MSG       "end_printer_enum"

#define HP_SAP_PRINT_PROGRESS_MSG         "print_progress"

/*
 *  Messages that SAP will respond to
 */
//#define HP_SAP_SEND_FULL_PRINTER_LIST     "send_full_printer_list"


/* Some basic type definitions */
#ifndef uint8
  #define  uint8      unsigned  char
#endif

#ifndef int8
  #define  int8                 char
#endif

#ifndef uint16
  #define  uint16     unsigned  short
#endif

#ifndef int16
  #define  int16                short
#endif

#ifndef uint32
  #define  uint32     unsigned  int
#endif

#ifndef int32
  #define  int32                int
#endif

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct sap_tag_callback_extra_params {
    uint8 const * p2;
    uint8 const * p3;
    uint8 const * p4;
    uint8 const * p5;

    uint32        n1;
    uint32        n2;
    uint32        n3;
    uint32        n4;
    uint32        n5;
  } sap_params;

  typedef int(*hp_sap_callback_t)(void * app_data, char const * message, int id, int32 transaction_id, uint8 const * p1, sap_params const * params);

  /* 1. ---------- Register your callback fn ---------- */
  bool hp_sap_register_handler(char const * name, void * app_data, hp_sap_callback_t callback);
  bool hp_sap_deregister_handler(char const * name);

  /* 2. ---------- get/set options ---------- */
  char const *  hp_sap_get_option          (char const *name, char * buffer_out, uint32 buf_size);
  char const *  hp_sap_get_option_def      (char const *name, char const *def, char * buffer_out, uint32 buf_size);
  int           hp_sap_get_int_option      (char const *name);
  int           hp_sap_get_int_option_def  (char const *name, int def);
  bool          hp_sap_get_flag            (char const *name);

  int           hp_sap_set_option(char const *name, char const *value);
  int           hp_sap_set_int_option(char const *name, int value);
  int           hp_sap_set_flag(char const *name, bool value);
  int           hp_sap_clear_flag(char const *name);

  int           hp_sap_parse_cli(int argc, void const * argv[]);

  /* 3. ---------- Start the service ---------- */
  bool hp_sap_start();
  bool hp_sap_start_ex(uint32 start_flags);

  // Is the event queue done?
  bool hp_sap_mq_is_done();

  /* 4. ---------- Do various other things (optional) ---------- */
  /* See "other things" below */

  /* 5. ---------- Invoke a job ---------- */

  /**
   *  Print the asset given by url to the printer at printer_ip.
   *
   *  This is the main API for printing, when the app shows the printer
   *  list.  Returns true if the print was started, not that it 
   *  completes.
   *
   *  This function returns immediately.
   */
  bool hp_sap_send_job(char const * url, char const * printer_ip);

  /**
   *  Print the URL.
   *
   *  Starts the print motion.  Returns if the job *starts* not
   *  if the job actually printed.
   *
   *  This function returns immediately.
   */
  bool hp_sap_print(char const * url);

  /* 6. ---------- Stop the service ---------- */
  void hp_sap_stop();

  /* ---------- Other things ---------- */
  bool hp_sap_send_full_printer_list();

  /* Send a message through the event queue */
  int  hp_sap_send(char const *message, char const *payload); // payload may be NULL
  int  hp_sap_set_timeout(char const * message_to_send, int msecs_to_wait);

#ifdef __cplusplus
}
#endif

#endif  /* __HP_SAP_H__ */


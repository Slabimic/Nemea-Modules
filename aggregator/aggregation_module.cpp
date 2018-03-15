/**
 * \file aggregation_module.cpp
 * \brief Aggregation NEMEA module based on UniRec.
 * \author Michal Slabihoudek <slabimic@fit.cvut.cz>
 * \date 2018
 */
/*
 * Copyright (C) 2013,2014,2015,2016 CESNET
 *
 * LICENSE TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is'', and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <csignal>
#include <getopt.h>
#include <libtrap/trap.h>
#include <unirec/unirec.h>
#include "fields.h"

#include <map>
#include <utility>
#include <pthread.h>

#include "output.h"
#include "configuration.h"
#include "agg_functions.h"


#define MAX_TIMEOUT_RETRY 3
#define TRAP_RECV_TIMEOUT 500000   // 0.5 second
//#define TRAP_RECV_TIMEOUT 4000000   // 4 seconds
#define TRAP_SEND_TIMEOUT 1000000   // 1 second

trap_module_info_t *module_info = NULL;
/**
 * COUNT, TIME_FIRST, TIME_LAST always used by module
 */
UR_FIELDS (
        uint32 COUNT,
        time TIME_FIRST,
        time TIME_LAST
)

/**
 * Definition of basic module information - module name, module description, number of input and output interfaces
 */
#define MODULE_BASIC_INFO(BASIC) \
  BASIC("Aggregation module", \
        "This module serves for UniRec records aggregation processing. " \
        "User has to specify parameters for processing including key fields and applied aggregation function. " \
        "It receives UniRec and sends UniRec containing the fields which take part in aggregation process. ", 1, 1)

/**
 * Definition of module parameters - every parameter has short_opt, long_opt, description,
 * flag whether an argument is required or it is optional and argument type which is NULL
 * in case the parameter does not need argument.
 * Module parameter argument types: int8, int16, int32, int64, uint8, uint16, uint32, uint64, float, string
 */
#define MODULE_PARAMS(PARAM) \
  PARAM('k', "key", "Defines received UniRec field name as part of aggregation key.", required_argument, "string") \
  PARAM('t', "time_window", "[G,A,P]:#seconds. Represents type of timeout and #seconds before sending output.", required_argument, "string") \
  PARAM('s', "sum", "Makes sum of UniRec field values identified by given name.", required_argument, "string") \
  PARAM('a', "avg", "Makes average of UniRec field values identified by given name.", required_argument, "string") \
  PARAM('m', "min", "Keep minimal value of UniRec field identified by given name.", required_argument, "string") \
  PARAM('M', "max", "Keep maximal value of UniRec field identified by given name.", required_argument, "string") \
  PARAM('f', "first", "Keep first value of UniRec field identified by given name.", required_argument, "string") \
  PARAM('l', "last", "Keep first value of UniRec field identified by given name.", required_argument, "string") \
  PARAM('o', "or", "Make bitwise OR of UniRec field identified by given name.", required_argument, "string") \
  PARAM('n', "and", "Make bitwise AND of UniRec field identified by given name.", required_argument, "string")

/**
 * To define positional parameter ("param" instead of "-m param" or "--mult param"), use the following definition:
 * PARAM('-', "", "Parameter description", required_argument, "string")
 * There can by any argument type mentioned few lines before.
 * This parameter will be listed in Additional parameters in module help output
 */


static int stop = 0;
static std::map<Key, void*> storage;                   // Need to be global because of trap_terminate
time_t time_last_from_record = time(NULL);             // Passive timeout time info set due to records time
pthread_mutex_t storage_mutex = PTHREAD_MUTEX_INITIALIZER;                 // For storage modifying sections
pthread_mutex_t time_last_from_record_mutex = PTHREAD_MUTEX_INITIALIZER;   // For modifying Passive timeout time info
int var_field_len;                                     // Length of variable field used by last_variable()
void flush_storage();

/**
 * Function to handle SIGTERM and SIGINT signals (used to stop the module)
 */
/*
TRAP_DEFAULT_SIGNAL_HANDLER(
        stop = 1;
        printf("Signal caught by handler!\n");
        flush_storage();
        trap_send(0, "", 1);
        sleep(1))
*/
void my_signal_handler(int signal)
{
   if (signal == SIGTERM || signal == SIGINT) {
      printf("Signal caught by handler!\n");
      stop = 1;
   }
}


/* ================================================================= */
/* ================== Develop/helper functions ===================== */
/* ================================================================= */

void print_all_defined_ur_fields()
{
   int16_t size = ur_field_specs.ur_last_id;
   for (int i = 0; i < size; i++) {
      const char* name = ur_get_name(i);
      printf("%d. name = %s\n", i, name);
   }
   printf("All data from global unirec structure printed.\n\n");
}
/* ----------------------------------------------------------------- */
void print_template_fields(ur_template_t* tmplt)
{
   int16_t size = tmplt->count;
   for (int i = 0; i < size; i++) {
      const char* name = ur_get_name(tmplt->ids[i]);
      printf("%d. name = %s\n", i, name);
   }
   printf("All data from given template printed.\n\n");
}
/* ----------------------------------------------------------------- */
void clean_memory(ur_template_t *in_tmplt, ur_template_t *out_tmplt, std::map<Key, void*> storage){
   std::map<Key, void*>::iterator it;
   for ( it = storage.begin(); it != storage.end(); it++) {
      ur_free_record(it->second);
   }
   storage.clear();

   TRAP_DEFAULT_FINALIZATION();
   ur_free_template(in_tmplt);
   if (out_tmplt)
      ur_free_template(out_tmplt);
   ur_finalize();
}
/* ----------------------------------------------------------------- */
void * create_record(ur_template_t *tmplt, int length)
{
   return ur_create_record(tmplt, length);
}
/* ----------------------------------------------------------------- */
void process_agg_functions(ur_template_t *in_tmplt, const void *src_rec, ur_template_t *out_tmplt, void *dst_rec)
{
   // Always increase count when processing functions
   ur_set(out_tmplt, dst_rec, F_COUNT, ur_get(out_tmplt, dst_rec, F_COUNT) + 1);

   // Modify time attributes TIME_FIRST:min
   uint64_t stored = ur_get(out_tmplt, dst_rec, F_TIME_FIRST);
   uint64_t record = ur_get(in_tmplt, src_rec, F_TIME_FIRST);
   if (record < stored)
      ur_set(out_tmplt, dst_rec, F_TIME_FIRST, ur_get(in_tmplt, src_rec, F_TIME_FIRST));

   // Modify time attributes TIME_LAST:max
   stored = ur_get(out_tmplt, dst_rec, F_TIME_LAST);
   record = ur_get(in_tmplt, src_rec, F_TIME_LAST);
   if (record > stored)
      ur_set(out_tmplt, dst_rec, F_TIME_LAST, ur_get(in_tmplt, src_rec, F_TIME_LAST));


   void *ptr_dst;
   void *ptr_src;
   // Process all registered fields with their agg function
   for (int i = 0; i < OutputTemplate::used_fields; i++) {
      if (ur_is_fixlen(i)) {
         ptr_dst = ur_get_ptr_by_id(out_tmplt, dst_rec, OutputTemplate::indexes_to_record[i]);
         ptr_src = ur_get_ptr_by_id(in_tmplt, src_rec, OutputTemplate::indexes_to_record[i]);
         OutputTemplate::process[i](ptr_src, ptr_dst);
      }
      else {
         var_params params = {dst_rec, i, ur_get_var_len(in_tmplt, src_rec, OutputTemplate::indexes_to_record[i])};
         ptr_src = ur_get_ptr_by_id(in_tmplt, src_rec, OutputTemplate::indexes_to_record[i]);
         OutputTemplate::process[i](ptr_src, (void*)&params);
      }
   }
}

/* ----------------------------------------------------------------- */
void init_record_data(ur_template_t * in_tmplt, const void *src_rec, ur_template_t *out_tmplt, void *dst_rec)
{
   ur_clear_varlen(in_tmplt, dst_rec);
   // Copy all fields which are part of output template
   ur_copy_fields(out_tmplt, dst_rec, in_tmplt, src_rec);
   // Set initial value of module field(s)
   ur_set(out_tmplt, dst_rec, F_COUNT, 1);
}
/* ----------------------------------------------------------------- */
void prepare_to_send(void *stored_rec)
{
   // Proces activities needed to be done before sending the record

   // Count Average function
   for (int i = 0; i < OutputTemplate::used_fields; i++) {
      if (OutputTemplate::avg_fields[i]) {
         OutputTemplate::avg_fields[i](ur_get_ptr_by_id(OutputTemplate::out_tmplt, stored_rec, OutputTemplate::indexes_to_record[i]), ur_get(OutputTemplate::out_tmplt, stored_rec, F_COUNT));
      }
   }

   /*
    * Add every new post processing of agg function here
    */

}
/* ----------------------------------------------------------------- */
bool send_record_out(ur_template_t *out_tmplt, void *out_rec)
{
   printf("Count of message to send is: %d\n", ur_get(out_tmplt, out_rec, F_COUNT));

   if(OutputTemplate::prepare_to_send) {
      prepare_to_send(out_rec);
   }

   // Send record to interface 0.
   int i = 0;
   for (; i < MAX_TIMEOUT_RETRY; i++) {
      printf("Trying to send..\n");
      int ret = trap_send(0, out_rec, ur_rec_fixlen_size(out_tmplt) + ur_rec_varlen_size(out_tmplt, out_rec));

      // Handle possible errors
      TRAP_DEFAULT_SEND_ERROR_HANDLING(ret, continue, printf("SEND ERR\n");break);
      return true;
   }

   fprintf(stderr, "Cannot send record due to error or time_out\n");
   return false;
}

/* ----------------------------------------------------------------- */
void flush_storage()
{
   // Send all stored data
   std::map<Key, void*>::iterator it;
   for ( it = storage.begin(); it != storage.end(); it++) {
      send_record_out(OutputTemplate::out_tmplt, it->second);
      ur_free_record(it->second);
   }
   storage.clear();
}
/* ----------------------------------------------------------------- */
void *check_timeouts(void *input)
{
   Config *configuration = (Config*)input;
   int timeout_type = configuration->get_timeout_type();

   if (timeout_type == TIMEOUT_GLOBAL) {
      int timeout = configuration->get_timeout(TIMEOUT_GLOBAL);
      while (!stop) {
         time_t start = time(NULL);

         // Lock the storage -- CRITICAL SECTION START
         pthread_mutex_lock(&storage_mutex);
         flush_storage();
         // Unlock the storage -- CRITICAL SECTION END
         pthread_mutex_unlock(&storage_mutex);
         time_t end = time(NULL);

         int elapsed = difftime(end, start);
         int sec_to_sleep = (timeout - elapsed);
         if (sec_to_sleep > 0)
            sleep(sec_to_sleep);
      }

      return NULL;
   }

   if ((timeout_type == TIMEOUT_PASSIVE) || (timeout_type == TIMEOUT_ACTIVE_PASSIVE)) {
      int timeout = configuration->get_timeout(TIMEOUT_PASSIVE);

      pthread_mutex_lock(&time_last_from_record_mutex);
      time_last_from_record += timeout;
      pthread_mutex_unlock(&time_last_from_record_mutex);

      while (!stop) {
         time_t start = time(NULL);

         /* Can happen that record accesed for timeout check is being processed by main thread, need to use lock
          * Only accessing and modifying different elements is thread safe in stl map container */
         // Lock the storage -- CRITICAL SECTION START
         pthread_mutex_lock(&storage_mutex);
         for (std::map<Key, void*>::iterator it = storage.begin(); it != storage.end(); ) {
            if (ur_time_get_sec(ur_get(OutputTemplate::out_tmplt, it->second, F_TIME_LAST)) < time_last_from_record - timeout) {
               // Send record out
               send_record_out(OutputTemplate::out_tmplt, it->second);
               ur_free_record(it->second);
               storage.erase(it++);
            }
            else {
               ++it;
            }
         }
         // Unlock the storage -- CRITICAL SECTION END
         pthread_mutex_unlock(&storage_mutex);

         time_t end = time(NULL);
         int elapsed = difftime(end, start);
         int sec_to_sleep = (timeout - elapsed);

         pthread_mutex_lock(&time_last_from_record_mutex);
         // Update the last record time with elapsed time interval
         time_last_from_record += (elapsed + sec_to_sleep);
         pthread_mutex_unlock(&time_last_from_record_mutex);

         // Assume regularly timeout period
         if (sec_to_sleep > 0) {
            sleep(sec_to_sleep);
         }

      }

      return NULL;
   }

   // Timeout is ACTIVE only, no thread checks needed, close the thread.
   return NULL;
}
/* ----------------------------------------------------------------- */

/* ================================================================= */
/* ========================= M A I N =============================== */
/* ================================================================= */
int main(int argc, char **argv)
{
   int ret;
   signed char opt;

   /* **** TRAP initialization **** */

   /*
    * Macro allocates and initializes module_info structure according to MODULE_BASIC_INFO and MODULE_PARAMS
    * definitions on the lines 71 and 84 of this file. It also creates a string with short_opt letters for getopt
    * function called "module_getopt_string" and long_options field for getopt_long function in variable "long_options"
    */
   INIT_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)

   /*
    * Let TRAP library parse program arguments, extract its parameters and initialize module interfaces
    */
   TRAP_DEFAULT_INITIALIZATION(argc, argv, *module_info);

   /*
    * Register signal handler.
    */
   //TRAP_REGISTER_DEFAULT_SIGNAL_HANDLER();
   signal(SIGTERM, my_signal_handler);
   signal(SIGINT, my_signal_handler);

   // Set TRAP_RECEIVE() timeout to TRAP_RECV_TIMEOUT/1000000 seconds
   trap_ifcctl(TRAPIFC_INPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_RECV_TIMEOUT);

   // Set TRAP_RECEIVE() timeout to TRAP_RECV_TIMEOUT/1000000 seconds
   trap_ifcctl(TRAPIFC_OUTPUT, 0, TRAPCTL_SETTIMEOUT, TRAP_SEND_TIMEOUT);

   Config config;

   /*
    * Parse program arguments defined by MODULE_PARAMS macro with getopt() function (getopt_long() if available)
    * This macro is defined in config.h file generated by configure script
    */
   while ((opt = TRAP_GETOPT(argc, argv, module_getopt_string, long_options)) != -1) {
      switch (opt) {
      case 'k':
         config.add_member(KEY, optarg);
         break;
      case 't':
         config.set_timeout(optarg);
         break;
      case 's':
         config.add_member(SUM, optarg);
         break;
      case 'a':
         config.add_member(AVG, optarg);
         break;
      case 'm':
         config.add_member(MIN, optarg);
         break;
      case 'M':
         config.add_member(MAX, optarg);
         break;
      case 'f':
         config.add_member(FIRST, optarg);
         break;
      case 'l':
         config.add_member(LAST, optarg);
         break;
      case 'o':
         fprintf(stderr, "Develop: Option \'%c\' currently being implemented.\n", opt);
         break;
      case 'n':
         fprintf(stderr, "Develop: Option \'%c\' currently being implemented.\n", opt);
         break;
      default:
         fprintf(stderr, "Invalid argument %c, skipped...\n", opt);
      }
   }

   // DEVEL: print configuration
   config.print();



   /* **** Create UniRec templates **** */
   ur_template_t *in_tmplt = ur_create_input_template(0, "TIME_FIRST,TIME_LAST", NULL);
   if (in_tmplt == NULL){
      fprintf(stderr, "Error: Input template could not be created.\n");
      TRAP_DEFAULT_FINALIZATION();
      FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
      return -1;
   }

   /* **** Create new thread for checking timeouts **** */
   pthread_t timeout_thread;
   pthread_create(&timeout_thread, NULL, &check_timeouts, (void*)&config);

   /* **** Main processing loop **** */

   // Read data from input, process them and write to output
   while (!stop) {
      const void *in_rec;
      uint16_t in_rec_size;

      // Receive data from input interface 0.
      // Block if data are not available immediately (unless a timeout is set using trap_ifcctl)
      ret = TRAP_RECEIVE(0, in_rec, in_rec_size, in_tmplt);

      // Handle possible errors
      TRAP_DEFAULT_RECV_ERROR_HANDLING(ret, continue, break);


      // Check for end-of-stream message, close only when signal caught
      if (in_rec_size <= 1) {
         continue;
      }

      // Change of UniRec input template -> sanity check and templates creation
      if (ret == TRAP_E_FORMAT_CHANGED ) {
         fprintf(stderr, "Format change, setting new module configuration\n");
         // Internal structures cleaning because of possible redefinition
         std::map<Key, void*>::iterator it;

         // Lock the storage -- CRITICAL SECTION START
         pthread_mutex_lock( &storage_mutex );

         for ( it = storage.begin(); it != storage.end(); it++) {
            send_record_out(OutputTemplate::out_tmplt, it->second);
            ur_free_record(it->second);
         }
         storage.clear();

         OutputTemplate::reset();
         KeyTemplate::reset();

         //print_all_defined_ur_fields();
         //print_template_fields(in_tmplt);

         int id;
         for (int i = 0; i < config.get_used_fields(); i++) {
            id = ur_get_id_by_name(config.get_name(i));

            if (id == UR_E_INVALID_NAME) {
               fprintf(stderr, "Requested field %s not in input records, cannot continue.\n", config.get_name(i));
               clean_memory(in_tmplt, NULL, storage);
               FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
               return 1;
            }

            if (ur_is_varlen(id) && !config.is_variable())
               config.set_variable(true);

            if (config.is_key(i)) {
               KeyTemplate::add_field(id, ur_get_size(id));
            }
            else {
               OutputTemplate::add_field(id, config.get_function_ptr(i, ur_get_type(id)), config.is_func(i, AVG), config.get_avg_ptr(i, ur_get_type(id)));
            }
         }
         char *tmplt_def = config.return_template_def();
         OutputTemplate::out_tmplt = ur_create_output_template(0, tmplt_def, NULL);
         delete [] tmplt_def;

         if (OutputTemplate::out_tmplt == NULL){
            fprintf(stderr, "Error: Output template could not be created.\n");
            clean_memory(in_tmplt, NULL, storage);
            FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
            return -1;
         }
         // Unlock the storage -- CRITICAL SECTION END
         pthread_mutex_unlock(&storage_mutex);

         // Lock the time variable -- CRITICAL SECTION START
         pthread_mutex_lock(&time_last_from_record_mutex);
         // Set the global last time with first received record
         time_last_from_record = ur_time_get_sec(ur_get(in_tmplt, in_rec, F_TIME_LAST));
         // Unlock the time variable -- CRITICAL SECTION END
         pthread_mutex_unlock(&time_last_from_record_mutex);

         //print_template_fields(OutputTemplate::out_tmplt);
      }

      /* Start message processing */
      time_t record_first = ur_time_get_sec(ur_get(in_tmplt, in_rec, F_TIME_FIRST));

      // Generate key
      Key rec_key;
      for (uint i = 0; i < KeyTemplate::used_fields; i++) {
         rec_key.add_field(ur_get_ptr_by_id(in_tmplt, in_rec, KeyTemplate::indexes_to_record[i]),
                           ur_get_size(KeyTemplate::indexes_to_record[i]));
      }

      void *init_ptr = NULL;
      std::pair<std::map<Key, void*>::iterator, bool> inserted;
      // Lock the storage -- CRITICAL SECTION START
      pthread_mutex_lock( &storage_mutex );
      inserted = storage.insert(std::make_pair(rec_key, init_ptr));

      if (inserted.second == false) {
         // Element already exists

         bool new_time_window = false;
         void *stored_rec = inserted.first->second;
         // Main thread checks time window only when active timeout set
         if ( (config.get_timeout_type() == TIMEOUT_ACTIVE) || (config.get_timeout_type() == TIMEOUT_ACTIVE_PASSIVE)) {
            // Check time window for active timeout
            time_t stored_first = ur_time_get_sec(ur_get(OutputTemplate::out_tmplt, stored_rec, F_TIME_FIRST));
            // Record is not in current time window
            if (stored_first + config.get_timeout(TIMEOUT_ACTIVE) < record_first ) {
               new_time_window = true;
            }
         }
         if (new_time_window) {
            if(!send_record_out(OutputTemplate::out_tmplt, stored_rec)) {
               break;
            }

            init_record_data(in_tmplt, in_rec, OutputTemplate::out_tmplt, stored_rec);
         }
         else {
            process_agg_functions(in_tmplt, in_rec, OutputTemplate::out_tmplt, stored_rec);
         }
      }
      else {
         // New element
         // If there should be place for variable length field in record reserve it
         int var_length = config.is_variable() == false ? 0 : 2048;
         void * out_rec = create_record(OutputTemplate::out_tmplt, var_length);
         if (!out_rec) {
            clean_memory(in_tmplt, OutputTemplate::out_tmplt, storage);
            fprintf(stderr, "Error: Memory allocation problem (output record).\n");
            FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS);
            return -1;
         }
         init_record_data(in_tmplt, in_rec, OutputTemplate::out_tmplt, out_rec);
         inserted.first->second = out_rec;
      }
      // Unlock the storage -- CRITICAL SECTION END
      pthread_mutex_unlock( &storage_mutex );
   }

   printf("Module canceled, waiting for running threads.\n");
   pthread_join(timeout_thread, NULL);
   printf("Other threads ended, cleaning storage and exiting.\n");
   // All other threads not running now, no need to use mutexes there

   flush_storage();
   trap_send(0, "", 1);
   sleep(1);
   trap_terminate();

   /* **** Cleanup **** */
   // Free unirec templates and stored records
   clean_memory(in_tmplt, OutputTemplate::out_tmplt, storage);
   // Release allocated memory for module_info structure
   FREE_MODULE_INFO_STRUCT(MODULE_BASIC_INFO, MODULE_PARAMS)


   return 0;
}


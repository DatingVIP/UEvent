/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2013 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: joe@firstbeatmedia.com                                       |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#ifdef HAVE_SPL
#include "ext/spl/spl_exceptions.h"
#endif
#include "zend_closures.h"
#include "zend_interfaces.h"
#include "php_uevent.h"

typedef void (*zend_executor) (zend_execute_data * TSRMLS_DC);

typedef struct _uevent {
	zval               name;
	zval              *handler;
	zval              *args;
} uevent_t;

#define UEVENT_EVENT_INIT {{0}, NULL, NULL}

zend_executor zend_executor_function = NULL;

zend_class_entry *UEvent_ce = NULL;
zend_class_entry *UEventArgs_ce = NULL;
zend_class_entry *UEventInput_ce = NULL;

ZEND_DECLARE_MODULE_GLOBALS(uevent);

/* {{{ php_uevent_init_globals
 */
static void php_uevent_globals_ctor(zend_uevent_globals *ug)
{}
/* }}} */

/* {{{ */
static inline void php_uevent_event_dtor(void *ev) {
	uevent_t *uevent = (uevent_t*) ev;
	
	zval_dtor(&uevent->name);

	if (uevent->handler) {
		zval_ptr_dtor(&uevent->handler);	
	}
	
	if (uevent->args) {
		zval_ptr_dtor(&uevent->args);
	}
} /* }}} */

/* {{{ */
static inline void php_uevent_events_dtor(void *evs) {
	zend_hash_destroy((HashTable*) evs);
} /* }}} */

/* {{{ proto boolean UEvent::addEvent(string name, callable where [, UEventInput input = null]) */
PHP_METHOD(UEvent, addEvent) {
	uevent_t uevent = UEVENT_EVENT_INIT;
	zval *name = NULL;
	zval *call = NULL;
	zval *input = NULL;
	char *cname = NULL;
	zend_fcall_info_cache fcc;
	HashTable *events;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|O", &name, &call, &input, UEventInput_ce) != SUCCESS) {
		return;
	}
	
	if (!name || Z_TYPE_P(name) != IS_STRING) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 
			"UEvent::addEvent expected (string name, callable call, UEventInput input = null), name is not a string");
		return;
	}
	
	if (!call || !zend_is_callable_ex(call, NULL, IS_CALLABLE_CHECK_SILENT, &cname, NULL, &fcc, NULL TSRMLS_CC)) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException, 
			"UEvent::addEvent expected (string name, callable call, UEventInput input = null), call is not a callable");
		if (cname)
			efree(cname);
		return;
	}
	
	uevent.name = *name;
	zval_copy_ctor(&uevent.name);
	if (input) {
		uevent.args = input;
		Z_ADDREF_P(uevent.args);
	}

	if (zend_hash_index_find(&UG(events), (zend_ulong) fcc.function_handler, (void**) &events) != SUCCESS) {
		HashTable evtable;

		zend_hash_init(&evtable, 8, NULL, php_uevent_event_dtor, 0);
		zend_hash_index_update(
			&UG(events),
			(zend_ulong) fcc.function_handler, 
			(void**)&evtable, sizeof(HashTable), 
			(void**)&events);
	}
	
	RETVAL_BOOL(zend_hash_next_index_insert(events, (void**) &uevent, sizeof(uevent_t), NULL) == SUCCESS);
	
	if (cname)
		efree(cname);
} /* }}} */

/* {{{ proto array UEvent::getEvents() */
PHP_METHOD(UEvent, getEvents) {
	HashPosition position[2];
	HashTable    *events;
	uevent_t     *uevent;
	
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}
	
	array_init(return_value);

	for (zend_hash_internal_pointer_reset_ex(&UG(events), &position[0]);
		zend_hash_get_current_data_ex(&UG(events), (void**) &events, &position[0]) == SUCCESS;
		zend_hash_move_forward_ex(&UG(events), &position[0])) {
		for (zend_hash_internal_pointer_reset_ex(events, &position[1]);
			zend_hash_get_current_data_ex(events, (void**)&uevent, &position[1]) == SUCCESS;
			zend_hash_move_forward_ex(events, &position[1])) {
			add_next_index_stringl(
				return_value, Z_STRVAL(uevent->name), Z_STRLEN(uevent->name), 1);
		}	
	}
} /* }}} */

/* {{{ proto boolean UEvent::addListener(string name, Closure handler [, UEventArgs args]) */
PHP_METHOD(UEvent, addListener) {
	uevent_t uevent = UEVENT_EVENT_INIT;
	zval *name = NULL;
	zval *handler = NULL;
	zval *args = NULL;
	char *cname = NULL;
	zend_fcall_info_cache fcc;
	HashTable *listeners = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zO|O", &name, &handler, zend_ce_closure, &args, UEventArgs_ce) != SUCCESS) {
		return;
	}
	
	if (!name || Z_TYPE_P(name) != IS_STRING) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addListener expected (string name, Closure listener [, UEventArgs args])");
		return;
	}

	uevent.name = *name;
	zval_copy_ctor
		(&uevent.name);
	uevent.handler = handler;
	Z_ADDREF_P(handler);
	if (args) {
		uevent.args = args;
		Z_ADDREF_P(args);
	}

	if (zend_hash_find(&UG(listeners), Z_STRVAL_P(name), Z_STRLEN_P(name), (void**) &listeners) != SUCCESS) {
		HashTable ltable;

		zend_hash_init(&ltable, 8, NULL, php_uevent_event_dtor, 0);
		zend_hash_update(
			&UG(listeners),
			Z_STRVAL_P(name), Z_STRLEN_P(name),
			(void**)&ltable, sizeof(HashTable), 
			(void**)&listeners);
	}
	
	RETVAL_BOOL(zend_hash_next_index_insert(listeners, (void**) &uevent, sizeof(uevent_t), NULL) == SUCCESS);
	
	if (cname)
		efree(cname);
} /* }}} */

ZEND_BEGIN_ARG_INFO_EX(uevent_addevent_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_TYPE_INFO(0, call, IS_CALLABLE, 0)
	ZEND_ARG_OBJ_INFO(0, input, UEventInput, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_getevents_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_addlistener_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_OBJ_INFO(0, handler, Closure, 0)
	ZEND_ARG_OBJ_INFO(0, input,   UEventArgs, 1)
ZEND_END_ARG_INFO()

static zend_function_entry uevent_methods[] = {
	PHP_ME(UEvent, addEvent, uevent_addevent_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(UEvent, getEvents, uevent_getevents_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(UEvent, addListener, uevent_addlistener_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(ueventargs_get_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ueventargs_set_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

zend_function_entry ueventargs_methods[] = {
	PHP_ABSTRACT_ME(UEventArgs, get, ueventargs_get_arginfo)
	PHP_FE_END
};

ZEND_BEGIN_ARG_INFO_EX(ueventinput_accept_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

zend_function_entry ueventinput_methods[] = {
	PHP_ABSTRACT_ME(UEventInput, accept, ueventinput_accept_arginfo)
	PHP_FE_END
};

/* {{{ */
static inline void uevent_execute(zend_execute_data *execute_data TSRMLS_DC) {
	HashTable *events;
	void      **top = NULL;
	int       stacked = 0;
	
	zend_executor_function(execute_data TSRMLS_CC);
	
	top = zend_vm_stack_top(TSRMLS_C) - 1;

	if (top) {
		stacked = (int)(zend_uintptr_t) *top;
	}
	
	if (zend_hash_index_find(&UG(events), (zend_ulong) execute_data->function_state.function, (void**)&events) == SUCCESS) {
		uevent_t *event;
		HashPosition position[2];
		
		for (zend_hash_internal_pointer_reset_ex(events, &position[0]);
			zend_hash_get_current_data_ex(events, (void**)&event, &position[0]) == SUCCESS;
			zend_hash_move_forward_ex(events, &position[0])) {

			HashTable *listeners;
			uevent_t *listener;

			if (event->args && Z_TYPE_P(event->args) == IS_OBJECT) {
				zend_fcall_info fci;
				zend_fcall_info_cache fcc;
				zval arguments;
				
				zval callable;
				zval *retval = NULL;
				void *bottom = NULL;
				zend_bool invoke = 0;
				
				array_init(&callable);
				add_next_index_zval(&callable, event->args);
				add_next_index_string(&callable, "accept", 1);
				array_init(&arguments);
				
				if (zend_fcall_info_init(&callable, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
					fci.retval_ptr_ptr = &retval;
					
					bottom = zend_vm_stack_top(TSRMLS_C);
					
					EG(argument_stack)->top = top + 1;
					
					if (zend_copy_parameters_array(stacked, &arguments TSRMLS_CC) == SUCCESS) {
						zend_fcall_info_args(&fci, &arguments TSRMLS_CC);
					}
					
					zend_call_function(&fci, &fcc TSRMLS_CC);
					
					EG(argument_stack)->top = bottom;
					
					zend_fcall_info_args_clear(&fci, 1);
					
					if (retval) {
						invoke = zend_is_true
							(retval TSRMLS_CC);
						zval_ptr_dtor(&retval);
					}			
				}
				
				zval_dtor(&callable);
				zval_dtor(&arguments);
				
				if (!invoke) {
					continue ;
				}
			}
			
			if (zend_hash_find(&UG(listeners), Z_STRVAL(event->name), Z_STRLEN(event->name), (void**)&listeners) == SUCCESS) {				
				
				for (zend_hash_internal_pointer_reset_ex(listeners, &position[1]);
					zend_hash_get_current_data_ex(listeners, (void**)&listener, &position[1]) == SUCCESS;
					zend_hash_move_forward_ex(listeners, &position[1])) {
					zend_fcall_info fci;
					zend_fcall_info_cache fcc;
					zval *retval = NULL;
					zval *argval = NULL;
					
					if (zend_fcall_info_init(listener->handler, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
						fci.retval_ptr_ptr = &retval;
						
						if (listener->args && Z_TYPE_P(listener->args) == IS_OBJECT) {
							if (zend_call_method_with_0_params(
								&listener->args,
								Z_OBJCE_P(listener->args), NULL, "get", &argval)) {
								zend_fcall_info_argn(
									&fci TSRMLS_CC, 1, &argval);
							}
						}

						zend_call_function(&fci, &fcc TSRMLS_CC);

						if (retval)
							zval_ptr_dtor(&retval);
						
						if (argval)
							zval_ptr_dtor(&argval);
						
						zend_fcall_info_args_clear(&fci, 1);
					}
				}
			}
		}
	}
} /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(uevent)
{
	zend_class_entry ce;
	
	ZEND_INIT_MODULE_GLOBALS(uevent, php_uevent_globals_ctor, NULL);
	
	INIT_CLASS_ENTRY(ce, "UEvent", uevent_methods);
	UEvent_ce = zend_register_internal_class(&ce TSRMLS_CC);
	
	INIT_CLASS_ENTRY(ce, "UEventArgs", ueventargs_methods);
	UEventArgs_ce = zend_register_internal_class(&ce TSRMLS_CC);
	UEventArgs_ce->ce_flags |= ZEND_ACC_INTERFACE;
	
	INIT_CLASS_ENTRY(ce, "UEventInput", ueventinput_methods);
	UEventInput_ce = zend_register_internal_class(&ce TSRMLS_CC);
	UEventInput_ce->ce_flags |= ZEND_ACC_INTERFACE;
	
	zend_executor_function = zend_execute_ex;
	zend_execute_ex        = uevent_execute;
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(uevent)
{
	zend_execute_ex = zend_executor_function;
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(uevent)
{
	zend_hash_init(&UG(events), 8, NULL, php_uevent_events_dtor, 0);
	zend_hash_init(&UG(listeners), 8, NULL, php_uevent_events_dtor, 0);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(uevent)
{
	zend_hash_destroy(&UG(events));
	zend_hash_destroy(&UG(listeners));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(uevent)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "uevent support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ uevent_module_entry
 */
zend_module_entry uevent_module_entry = {
	STANDARD_MODULE_HEADER,
	"uevent",
	NULL,
	PHP_MINIT(uevent),
	PHP_MSHUTDOWN(uevent),
	PHP_RINIT(uevent),
	PHP_RSHUTDOWN(uevent),
	PHP_MINFO(uevent),
	PHP_UEVENT_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_UEVENT
ZEND_GET_MODULE(uevent)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

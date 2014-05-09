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
	zval               handler;
	zval               args;
} uevent_t;

#define UEVENT_EVENT_INIT(ev) do { \
	ZVAL_NULL(&(ev).name); \
	ZVAL_NULL(&(ev).handler); \
	ZVAL_NULL(&(ev).args); \
} while (0)

zend_executor zend_executor_function = NULL;

zend_class_entry *UEvent_ce = NULL;
zend_class_entry *UEventArgs_ce = NULL;
zend_class_entry *UEventInput_ce = NULL;

ZEND_DECLARE_MODULE_GLOBALS(uevent);

/* {{{ */
#include "uevent.h" /* }}} */

/* {{{ php_uevent_init_globals
 */
static void php_uevent_globals_ctor(zend_uevent_globals *ug)
{}
/* }}} */

/* {{{ */
static inline void php_uevent_event_dtor(void *ev) {
	uevent_t *uevent = (uevent_t*) ev;
	
	zval_dtor(&uevent->name);
	zval_dtor(&uevent->handler);	
	zval_dtor(&uevent->args);
} /* }}} */

/* {{{ */
static inline void php_uevent_events_dtor(void *evs) {
	zend_hash_destroy((HashTable*) evs);
} /* }}} */

/* {{{ proto boolean UEvent::addEvent(string name, callable where [, UEventInput input = null]) */
PHP_METHOD(UEvent, addEvent) {
	uevent_t uevent;
	zval *name = NULL;
	zval *call = NULL;
	zval *input = NULL;
	char *lcname = NULL;
	zval **clazz = NULL;
	zval **method = NULL;
	zend_function *address;
	HashTable *events;
	
	UEVENT_EVENT_INIT(uevent);	
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|O", &name, &call, &input, UEventInput_ce) != SUCCESS) {
		return;
	}
	
	if (!name || Z_TYPE_P(name) != IS_STRING) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addEvent expected (string name, callable call, UEventInput input = null), name is not a string");
		return;
	}
	
	if (!call) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addEvent expected (string name, callable call, UEventInput input = null), call was not provided");
		return;
	}
	
	switch (Z_TYPE_P(call)) {
		case IS_ARRAY: {
			zend_class_entry **pce;
			
			if (zend_hash_index_find(Z_ARRVAL_P(call), 0, (void**) &clazz) != SUCCESS ||
				zend_hash_index_find(Z_ARRVAL_P(call), 1, (void**) &method) != SUCCESS) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class and method not provided");
				return;
			}
			
			if ((!clazz || Z_TYPE_PP(clazz) != IS_STRING) ||
				(!method || Z_TYPE_PP(method) != IS_STRING)) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class or method is invalid");
				return;
			}	

			if (zend_lookup_class(Z_STRVAL_PP(clazz), Z_STRLEN_PP(clazz), &pce TSRMLS_CC) != SUCCESS) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class %s cannot be found", Z_STRVAL_PP(clazz));
				return;
			}
			
			lcname = zend_str_tolower_dup(Z_STRVAL_PP(method), Z_STRLEN_PP(method)+1);
			if (zend_hash_find(&(*pce)->function_table, lcname, Z_STRLEN_PP(method)+1, (void**)&address) != SUCCESS) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), method %s could not be found in %s", 
						Z_STRVAL_PP(method), Z_STRVAL_PP(clazz));
				efree(lcname);
				return;
			}
			efree(lcname);
		} break;
		
		case IS_STRING: {
			lcname = zend_str_tolower_dup(Z_STRVAL_P(call), Z_STRLEN_P(call)+1);
			if (zend_hash_find(EG(function_table), lcname, Z_STRLEN_P(call)+1, (void**)&address) != SUCCESS) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), the function %s could not be found", 
						Z_STRVAL_P(call));
				efree(lcname);
				return;
			}
			efree(lcname);
		} break;
	}
	
	if (address->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(spl_ce_RuntimeException,
			"UEvent cannot bind to internal function addresses, it does not seem useful !", Z_STRVAL_PP(clazz));
		return;
	}
	
	uevent.name = *name;
	zval_copy_ctor(&uevent.name);
	if (input) {
		uevent.args = *input;
		zval_copy_ctor(&uevent.args);
	}

	if (zend_hash_index_find(&UG(events), (zend_ulong) address, (void**) &events) != SUCCESS) {
		HashTable evtable;

		zend_hash_init(&evtable, 8, NULL, php_uevent_event_dtor, 0);
		zend_hash_index_update(
			&UG(events),
			(zend_ulong) address, 
			(void**)&evtable, sizeof(HashTable), 
			(void**)&events);
	}
	
	RETURN_BOOL(zend_hash_next_index_insert(events, (void**) &uevent, sizeof(uevent_t), NULL) == SUCCESS);
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
	uevent_t uevent;
	zval *name = NULL;
	zval *handler = NULL;
	zval *args = NULL;
	char *cname = NULL;
	zend_fcall_info_cache fcc;
	HashTable *listeners = NULL;
	
	UEVENT_EVENT_INIT(uevent);
	
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
	uevent.handler = *handler;
	zval_copy_ctor(&uevent.handler);
	if (args) {
		uevent.args = *args;
		zval_copy_ctor(&uevent.args);
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
	ZEND_ARG_INFO(0, call)
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
#define EEX(el) (execute_data)->el
	HashTable *events, *listeners;
	uevent_t *event, *listener;
	HashPosition position[2] = {NULL, NULL};
	void      **top = NULL;
	int       stacked = 0;

	zend_executor_function(execute_data TSRMLS_CC);
	
	if ((EEX(op_array) && EEX(op_array)->fn_flags & ZEND_ACC_GENERATOR)) {
		/* cannot fire events in generators, too messy ... */
		return;
	}
	
	top = zend_vm_stack_top(TSRMLS_C) - 1;

	if (top)
		stacked = (int)(zend_uintptr_t) *top;

	if ((events = uevent_get_events(EEX(function_state).function, &position[0] TSRMLS_CC))) {
		while ((event = uevent_get_event(events, &position[0]))) {
			if (!uevent_accept(event, top, stacked TSRMLS_CC)) {
				continue;
			}
			
			if ((listeners = uevent_get_listeners(event, &position[1] TSRMLS_CC))) {
				while ((listener = uevent_get_listener(listeners, &position[1]))) {
					uevent_invoke(listener TSRMLS_CC);
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

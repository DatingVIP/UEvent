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

/* {{{ proto boolean UEvent::addEvent(string name, callable where) */
PHP_METHOD(UEvent, addEvent) {
	zval *name = NULL;
	zval *call = NULL;
	zval *input = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz", &name, &call) != SUCCESS) {
		return;
	}

	if (!name || Z_TYPE_P(name) != IS_STRING) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addEvent expected (string name, callable call), name is not a string");
		return;
	}

	if (!call) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addEvent expected (string name, callable call), call was not provided");
		return;
	}

	RETURN_BOOL(uevent_add_event(name, call));
} /* }}} */

/* {{{ proto array UEvent::getEvents() */
PHP_METHOD(UEvent, getEvents) {
	HashPosition position[2];
	zval         *bucket[2];
	
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}
	
	array_init(return_value);

	for (zend_hash_internal_pointer_reset_ex(&UG(events), &position[0]);
	    (bucket[0] = zend_hash_get_current_data_ex(&UG(events), &position[0]));
	    zend_hash_move_forward_ex(&UG(events), &position[0])) {
		HashTable *events = (HashTable*) Z_PTR_P(bucket[0]);
		for (zend_hash_internal_pointer_reset_ex(events, &position[1]);
		     (bucket[1] = zend_hash_get_current_data_ex(events, &position[1]));
			zend_hash_move_forward_ex(events, &position[1])) {
			uevent_t *uevent = 
			  (uevent_t*) Z_PTR_P(bucket[1]);
			add_next_index_stringl(
				return_value, Z_STRVAL(uevent->name), Z_STRLEN(uevent->name));
		}	
	}
} /* }}} */

/* {{{ proto boolean UEvent::addListener(string name, Closure handler) */
PHP_METHOD(UEvent, addListener) {
	zval *name = NULL;
	zval *handler = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zO", &name, &handler, zend_ce_closure) != SUCCESS) {
		return;
	}
	
	if (!name || Z_TYPE_P(name) != IS_STRING) {
		zend_throw_exception_ex(spl_ce_InvalidArgumentException,
			"UEvent::addListener expected (string name, Closure listener)");
		return;
	}

	RETURN_BOOL(uevent_add_listener(name, handler));
} /* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(uevent_addevent_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, call)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_getevents_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_addlistener_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_OBJ_INFO(0, handler, Closure, 0)
ZEND_END_ARG_INFO()
/* }}} */
 
/* {{{ */
static zend_function_entry uevent_methods[] = {
	PHP_ME(UEvent, addEvent, uevent_addevent_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(UEvent, getEvents, uevent_getevents_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(UEvent, addListener, uevent_addlistener_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_FE_END
}; /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(uevent)
{
	zend_class_entry ce;
	
	ZEND_INIT_MODULE_GLOBALS(uevent, php_uevent_globals_ctor, NULL);
	
	INIT_CLASS_ENTRY(ce, "UEvent", uevent_methods);
	UEvent_ce = zend_register_internal_class(&ce TSRMLS_CC);
	
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
	zend_hash_init(&UG(events), 8, NULL, uevent_events_dtor, 0);
	zend_hash_init(&UG(listeners), 8, NULL, uevent_events_dtor, 0);

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

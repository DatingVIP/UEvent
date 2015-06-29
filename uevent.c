/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
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

typedef void (*zend_executor) (zend_execute_data *);

zend_executor zend_executor_function = NULL;

zend_class_entry *UEvent_ce = NULL;
zend_object_handlers UEvent_handlers;

ZEND_DECLARE_MODULE_GLOBALS(uevent);

/* {{{ */
#include "uevent.h" /* }}} */

/* {{{ php_uevent_init_globals
 */
static void php_uevent_globals_ctor(zend_uevent_globals *ug)
{}
/* }}} */

/* {{{ proto UEvent UEvent::__construct(callable binding) */
PHP_METHOD(UEvent, __construct) {
	uevent_t	*uevent = php_uevent_fetch(getThis());
	zval            *callable = NULL;
	HashTable       *bindings;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &callable) != SUCCESS) {
		return;
	}
	
	uevent->binding = uevent_get_binding(callable);
	
	if (!(bindings = zend_hash_index_find_ptr(&UG(events), (zend_ulong) uevent->binding))) {
		bindings = 
			(HashTable*) emalloc(sizeof(HashTable));
		zend_hash_init(bindings, 8, NULL, ZVAL_PTR_DTOR, 0);
		zend_hash_index_update_ptr(
			&UG(events), (zend_ulong) uevent->binding, bindings);
	}
	
	zend_hash_next_index_insert(bindings, &EX(This));
	Z_ADDREF(EX(This));
} /* }}} */

/* {{{ proto void UEvent::__destruct(void) */
PHP_METHOD(UEvent, __destruct) {
	uevent_t *uevent = php_uevent_fetch(getThis());
	HashTable *bindings;
	
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}
	
	if ((bindings = zend_hash_index_find_ptr(&UG(events), (zend_ulong) uevent->binding))) {
		zval *event;
		HashPosition position;
		zend_ulong index;
		
		for (zend_hash_internal_pointer_reset_ex(bindings, &position);
		     event = zend_hash_get_current_data_ex(bindings, &position);
		     zend_hash_move_forward_ex(bindings, &position)) {
			if (php_uevent_fetch(event) == uevent) {
				if (zend_hash_get_current_key_ex(bindings, NULL, &index, &position) == HASH_KEY_IS_LONG) {
					zend_hash_index_del(bindings, index);
				}
			}
		}
	}
} /* }}} */

/* {{{ proto UEvent UEvent::add(Closure listener) */
PHP_METHOD(UEvent, add) {
	uevent_t	*uevent = php_uevent_fetch(getThis());
	zval            *listener;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "O", &listener, zend_ce_closure) != SUCCESS) {
		return;
	}
	
	if (zend_hash_next_index_insert(&uevent->listeners, listener)) {
		Z_ADDREF_P(listener);
	}
	
	php_uevent_chain(return_value, &EX(This));
} /* }}} */

/* {{{ proto array UEvent::list(void) */
PHP_METHOD(UEvent, list) {
	uevent_t *uevent = php_uevent_fetch(getThis());
	
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}
	
	array_init(return_value);
	zend_hash_copy(
		Z_ARRVAL_P(return_value), &uevent->listeners, uevent_copy_ctor);
} /* }}} */

/* {{{ proto bool UEvent::remove(integer index) */
PHP_METHOD(UEvent, remove) {
	uevent_t   *uevent = php_uevent_fetch(getThis());
	zend_ulong listener;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &listener) != SUCCESS) {
		return;
	}
	
	RETVAL_BOOL(zend_hash_index_del(&uevent->listeners, listener) == SUCCESS);
} /* }}} */

/* {{{ proto void UEvent::reset(void) */
PHP_METHOD(UEvent, reset) {
	uevent_t *uevent = php_uevent_fetch(getThis());
	
	if (zend_parse_parameters_none() != SUCCESS) {
		return;
	}
	
	zend_hash_clean(&uevent->listeners);
} /* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(uevent_no_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, binding)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_add_arginfo, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, listener, Closure, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(uevent_remove_arginfo, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()
/* }}} */
 
/* {{{ */
static zend_function_entry uevent_methods[] = {
	PHP_ME(UEvent, __construct,	uevent_construct_arginfo, 	ZEND_ACC_PUBLIC)
	PHP_ME(UEvent, add,		uevent_add_arginfo, 		ZEND_ACC_PUBLIC)
	PHP_ME(UEvent, remove,		uevent_remove_arginfo, 		ZEND_ACC_PUBLIC)
	PHP_ME(UEvent, list,		uevent_no_arginfo, 		ZEND_ACC_PUBLIC)
	PHP_ME(UEvent, reset,		uevent_no_arginfo, 		ZEND_ACC_PUBLIC)
	PHP_ME(UEvent, __destruct,	uevent_no_arginfo,		ZEND_ACC_PUBLIC)
	PHP_FE_END
}; /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(uevent)
{
	zend_class_entry ce;
	
	ZEND_INIT_MODULE_GLOBALS(uevent, php_uevent_globals_ctor, NULL);
	
	INIT_CLASS_ENTRY(ce, "UEvent", uevent_methods);
	UEvent_ce = zend_register_internal_class(&ce);
	UEvent_ce->create_object = uevent_event_create;
	
	memcpy(&UEvent_handlers, 
	       zend_get_std_object_handlers(), 
	       sizeof(zend_object_handlers));
	UEvent_handlers.free_obj = uevent_event_free;
	UEvent_handlers.offset = XtOffsetOf(uevent_t, std);
	
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

static inline void php_uevent_unbind(zval *p) {
	zend_hash_destroy((HashTable*) Z_PTR_P(p));
	efree(Z_PTR_P(p));
}

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(uevent)
{
	zend_hash_init(&UG(events), 8, NULL, php_uevent_unbind, 0);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(uevent)
{
	zend_hash_destroy(&UG(events));

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

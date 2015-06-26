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
#ifndef HAVE_UEVENT_H
#define HAVE_UEVENT_H
/* {{{ */
typedef struct _uevent {
	zval               name;
	zval               handler;
	zval               args;
} uevent_t; /* }}} */

/* {{{ */
#define UEVENT_EVENT_INIT(ev) do { \
	ZVAL_NULL(&(ev)->name); \
	ZVAL_NULL(&(ev)->handler); \
	ZVAL_NULL(&(ev)->args); \
} while (0) /* }}} */

/* {{{ */
static inline void uevent_event_dtor(zval *ev) {
	uevent_t *uevent = 
	  (uevent_t*) Z_PTR_P(ev);
	
	zval_dtor(&uevent->name);
	zval_dtor(&uevent->handler);
	zval_dtor(&uevent->args);
	
	efree(uevent);
} /* }}} */

/* {{{ */
static inline void uevent_events_dtor(zval *evs) {
	HashTable *events = 
	  (HashTable*) Z_PTR_P(evs);
	
	zend_hash_destroy(events);
	efree(events);
} /* }}} */

/* {{{ */
static zend_function* uevent_get_function(zval *call TSRMLS_DC) {
	char *lcname = NULL;
	zval *clazz = NULL;
	zval *method = NULL;
	zend_function *address;
	
	switch (Z_TYPE_P(call)) {
		case IS_ARRAY: {
			zend_class_entry *ce;
			
			if (!(clazz = zend_hash_index_find_ptr(Z_ARRVAL_P(call), 0)) ||
			    !(method = zend_hash_index_find_ptr(Z_ARRVAL_P(call), 1))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class and method not provided");
				return NULL;
			}
			
			if ((!clazz || Z_TYPE_P(clazz) != IS_STRING) || (!method || Z_TYPE_P(method) != IS_STRING)) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class or method is invalid");
				return NULL;
			}	

			if (!(ce = zend_lookup_class(Z_STR_P(clazz)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), class %s cannot be found", Z_STRVAL_P(clazz));
				return NULL;
			}
			
			lcname = zend_str_tolower_dup(Z_STRVAL_P(method), Z_STRLEN_P(method)+1);
			if (!(address = zend_hash_str_find_ptr(&(ce)->function_table, lcname, Z_STRLEN_P(method)+1))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), method %s could not be found in %s", 
						Z_STRVAL_P(method), Z_STRVAL_P(clazz));
				efree(lcname);
				return NULL;
			}
			efree(lcname);
		} break;
		
		case IS_STRING: {
			lcname = zend_str_tolower_dup(Z_STRVAL_P(call), Z_STRLEN_P(call)+1);
			if (!(address = zend_hash_str_find_ptr(EG(function_table), lcname, Z_STRLEN_P(call)+1))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addEvent expected (string name, callable call, UEventInput input = null), the function %s could not be found", 
						Z_STRVAL_P(call));
				efree(lcname);
				return NULL;
			}
			efree(lcname);
		} break;
		
		case IS_OBJECT: {
			zend_fcall_info_cache fcc;

			if (!zend_is_callable_ex(call, NULL, IS_CALLABLE_CHECK_SILENT, NULL, &fcc, NULL TSRMLS_CC)) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException,
					"UEvent::addListener expected (string name, closure listener, UEventArgs args = null), the closure passed was not valid");
				return NULL;
			}
			
			address = fcc.function_handler;
		} break;
	}
	
	if (address->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(spl_ce_RuntimeException,
			"UEvent cannot bind to internal function addresses, it does not seem useful !", Z_STRVAL_PP(clazz));
		return NULL;
	}
	
	return address;
} /* }}} */

/* {{{ */
static inline HashTable *uevent_get_events(zend_function *function, HashPosition *position TSRMLS_DC) {
	HashTable *events;
	if (!(events = zend_hash_index_find_ptr(&UG(events), (zend_ulong) function))) {
		return NULL;
	}
	if (position)
		zend_hash_internal_pointer_reset_ex(events, position);
	return events;
} /* }}} */

/* {{{ */
static inline zend_bool uevent_add_event(zval *name, zval *callable, zval *input TSRMLS_DC) {
	uevent_t *uevent;
	HashTable *events;
	zend_function *address = uevent_get_function(callable TSRMLS_CC);
		
	if (!address) {
		return 0;
	}
	
	UEVENT_EVENT_INIT(uevent);

	uevent = (uevent_t*) emalloc(sizeof(uevent_t));
	uevent->name = *name;
	zval_copy_ctor(&uevent->name);
	
	if (input) {
		uevent->args = *input;
		zval_copy_ctor(&uevent->args);
	}

	if (!(events = zend_hash_index_find_ptr(&UG(events), (zend_ulong) address))) {
		events = (HashTable*) emalloc(sizeof(HashTable));
		
		zend_hash_init(events, 8, NULL, uevent_event_dtor, 0);
		zend_hash_index_update_ptr(
			&UG(events), (zend_ulong) address, events);
	}
	
	return (zend_hash_next_index_insert_ptr(events, uevent) == SUCCESS);
} /* }}} */

/* {{{ */
static inline HashTable *uevent_get_listeners(uevent_t *event, HashPosition *position TSRMLS_DC) {
	HashTable *listeners;
	if(!(listeners = zend_hash_str_find_ptr(
		&UG(listeners),
		Z_STRVAL(event->name), Z_STRLEN(event->name)))) {
		return NULL;
	}
	if (position)
		zend_hash_internal_pointer_reset_ex(listeners, position);
	return listeners;
} /* }}} */

/* {{{ */
static inline zend_bool uevent_add_listener(zval *name, zval *handler, zval *args TSRMLS_DC) {
	uevent_t *uevent;
	HashTable *listeners = NULL;
	
	UEVENT_EVENT_INIT(uevent);
  
	uevent = (uevent_t*) emalloc(sizeof(uevent_t));
	uevent->name = *name;
	zval_copy_ctor
		(&uevent->name);
	uevent->handler = *handler;
	zval_copy_ctor(&uevent->handler);
	if (args) {
		uevent->args = *args;
		zval_copy_ctor(&uevent->args);
	}

	if (!(listeners = zend_hash_str_find_ptr(&UG(listeners), Z_STRVAL_P(name), Z_STRLEN_P(name)))) {
		listeners = (HashTable*) emalloc(sizeof(HashTable));
		zend_hash_init(listeners, 8, NULL, uevent_event_dtor, 0);
		zend_hash_str_update_ptr(
			&UG(listeners),
			Z_STRVAL_P(name), Z_STRLEN_P(name),
			listeners);
	}

	return (zend_hash_next_index_insert_ptr(listeners, uevent) == SUCCESS);
} /* }}} */

/* {{{ */
static inline uevent_t *uevent_get_event(HashTable *events, HashPosition *position) {
	zval *bucket;
	
	if (!(bucket = zend_hash_get_current_data_ex(events, position))) {
		return NULL;
	}
	zend_hash_move_forward_ex(events, position);
	
	return (uevent_t*) Z_PTR_P(bucket);
} /* }}} */

/* {{{ */
static inline uevent_t *uevent_get_listener(HashTable *listeners, HashPosition *position) {
	zval *bucket;
	
	if (!(bucket = zend_hash_get_current_data_ex(listeners, position))) {
		return NULL;
	}
	zend_hash_move_forward_ex(listeners, position);
	
	return (uevent_t*) Z_PTR_P(bucket);
} /* }}} */

/* {{{ */
static inline zend_bool uevent_accept(uevent_t *event, void **top, int stacked TSRMLS_DC) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval arguments;
	void **bottom;
	zval callable;
	zval *retval = NULL;
	zend_bool accept = 0;
	
	if (Z_TYPE(event->args) != IS_OBJECT) {
		return 1;
	}
	
	array_init(&callable);
	array_init(&arguments);
	
	add_next_index_zval(&callable, &event->args);
	Z_ADDREF(event->args);
	add_next_index_string(&callable, "accept");
	
	if (zend_fcall_info_init(&callable, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
		fci.retval = retval;
		
		if (stacked) {
			bottom = zend_vm_stack_top(TSRMLS_C);
		
			EG(argument_stack)->top = top + 1;
		
			if (zend_copy_parameters_array(stacked, &arguments TSRMLS_CC) == SUCCESS) {
				zend_fcall_info_args(&fci, &arguments TSRMLS_CC);
			}
		}
		
		zend_call_function(&fci, &fcc TSRMLS_CC);
		
		if (stacked) {
			EG(argument_stack)->top = bottom;
			
			zend_fcall_info_args_clear(&fci, 1);
		}
		
		if (retval) {
#if PHP_VERSION_ID >= 50600
			accept = zend_is_true(retval TSRMLS_CC);
#else
			accept = zend_is_true(retval);
#endif
			zval_ptr_dtor(&retval);
		}			
	}
	
	zval_dtor(&callable);
	zval_dtor(&arguments);
	
	return accept;	
} /* }}} */

/* {{{ */
static inline zend_bool uevent_verify_error(zend_function *listener, zend_function *address, char *reason TSRMLS_DC) {
	if (address->common.scope) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0 TSRMLS_CC,
		"%s::%s has invalid listener, %s",
		address->common.scope->name, address->common.function_name, reason);
	} else zend_throw_exception_ex(spl_ce_RuntimeException,
		"%s::%s has invalid listener, %s", 0 TSRMLS_CC,
		address->common.function_name, address->common.num_args, reason);
	return 0;
} /* }}} */

/* {{{ */
static inline zend_bool uevent_verify_listener(zend_function *listener, zend_function *address TSRMLS_DC) {
	zend_arg_info *info[2] = {NULL, NULL};
	
	if (address->common.num_args) {
		zend_uint arg = 0;
		
		if ((!listener->common.num_args) ||
			(listener->common.num_args != address->common.num_args)) {
			return uevent_verify_error
				(listener, address, "prototype mismatch, wrong number of arguments" TSRMLS_CC);
		}
		
		while (arg < address->common.num_args) {
			info[0] = &address->common.arg_info[arg];
			info[1] = &listener->common.arg_info[arg];
			
			if (info[0]->type_hint != info[1]->type_hint) {
				return uevent_verify_error
					(listener, address, "prototype mismatch, type hints incompatible" TSRMLS_CC);
			}
			
			arg++;
		}
	}
	
	return 1;
} /* }}} */

/* {{{ */
static inline void uevent_invoke(uevent_t *listener, zend_function *address TSRMLS_DC) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *retval = NULL;
	zval *argval = NULL;
	
	if (zend_fcall_info_init(&listener->handler, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
		fci.retval_ptr_ptr = &retval;

		if (!uevent_verify_listener(fcc.function_handler, address TSRMLS_CC)) {
			return;	
		}

		if (Z_TYPE(listener->args) == IS_OBJECT) {
			zval *object = &listener->args;
			if (zend_call_method_with_0_params(
				&object,
				Z_OBJCE(listener->args), NULL, "get", &argval)) {
				zend_fcall_info_args(
					&fci, argval TSRMLS_CC);
			}
		}

		zend_call_function(&fci, &fcc TSRMLS_CC);

		if (retval)
			zval_ptr_dtor(&retval);
		
		if (argval)
			zval_ptr_dtor(&argval);
		
		zend_fcall_info_args_clear(&fci, 1);
	}
} /* }}} */

/* {{{ */
static inline void uevent_execute(zend_execute_data *execute_data TSRMLS_DC) {
#define EEX(el) (execute_data)->el
	HashTable *events, *listeners;
	uevent_t *event,   *listener;
	HashPosition        position[2] = {NULL, NULL};
	void              **top = NULL;
	int                 stacked = 0;
	zend_function      *address = EEX(function_state).function;
	
	zend_executor_function(execute_data TSRMLS_CC);
	
	if ((EEX(op_array) && EEX(op_array)->fn_flags & ZEND_ACC_GENERATOR)) {
		/* cannot fire events in generators, too messy ... */
		return;
	}
	
	top = zend_vm_stack_top(TSRMLS_C) - 1;

	if (top)
		stacked = (int)(zend_uintptr_t) *top;

	if ((events = uevent_get_events(address, &position[0] TSRMLS_CC))) {
		while ((event = uevent_get_event(events, &position[0]))) {
			if (!uevent_accept(event, top, stacked TSRMLS_CC)) {
				continue;
			}
			
			if ((listeners = uevent_get_listeners(event, &position[1] TSRMLS_CC))) {
				while ((listener = uevent_get_listener(listeners, &position[1]))) {
					uevent_invoke(listener, address TSRMLS_CC);
				}
			}
		}
	}
} /* }}} */
#endif

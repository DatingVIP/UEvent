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
	ev = (uevent_t*) \
	  emalloc(sizeof(uevent_t)); \
	\
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
			
			if (!(clazz = zend_hash_index_find(Z_ARRVAL_P(call), 0)) ||
			    !(method = zend_hash_index_find(Z_ARRVAL_P(call), 1))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"UEvent::addEvent expected (string name, callable call), class and method not provided");
				return NULL;
			}
			
			if (!(ce = zend_lookup_class(Z_STR_P(clazz)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"UEvent::addEvent expected (string name, callable call), class %s cannot be found", Z_STRVAL_P(clazz));
				return NULL;
			}
			
			lcname = zend_str_tolower_dup(Z_STRVAL_P(method), Z_STRLEN_P(method)+1);
			if (!(address = zend_hash_str_find_ptr(&(ce)->function_table, lcname, Z_STRLEN_P(method)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"UEvent::addEvent expected (string name, callable call), method %s could not be found in %s", 
						Z_STRVAL_P(method), Z_STRVAL_P(clazz));
				efree(lcname);
				return NULL;
			}
			efree(lcname);
		} break;
		
		case IS_STRING: {
			lcname = zend_str_tolower_dup(Z_STRVAL_P(call), Z_STRLEN_P(call)+1);
			if (!(address = zend_hash_str_find_ptr(EG(function_table), lcname, Z_STRLEN_P(call)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"UEvent::addEvent expected (string name, callable call), the function %s could not be found", 
						Z_STRVAL_P(call));
				efree(lcname);
				return NULL;
			}
			efree(lcname);
		} break;
		
		case IS_OBJECT: {
			zend_fcall_info_cache fcc;

			if (!zend_is_callable_ex(call, NULL, IS_CALLABLE_CHECK_SILENT, NULL, &fcc, NULL TSRMLS_CC)) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"UEvent::addListener expected (string name, closure listener), the closure passed was not valid");
				return NULL;
			}
			
			address = fcc.function_handler;
		} break;
	}
	
	if (address->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
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
static inline zend_bool uevent_add_event(zval *name, zval *callable TSRMLS_DC) {
	uevent_t *uevent;
	HashTable *events;
	zend_function *address = uevent_get_function(callable TSRMLS_CC);

	if (!address) {
		return 0;
	}
	
	UEVENT_EVENT_INIT(uevent);
	uevent->name = *name;
	zval_copy_ctor(&uevent->name);

	if (!(events = zend_hash_index_find_ptr(&UG(events), (zend_ulong) address))) {
		events = (HashTable*) emalloc(sizeof(HashTable));
		
		zend_hash_init(events, 8, NULL, uevent_event_dtor, 0);
		zend_hash_index_update_ptr(
			&UG(events), (zend_ulong) address, events);
	}
	
	return (zend_hash_next_index_insert_ptr(events, uevent) == uevent);
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
static inline zend_bool uevent_add_listener(zval *name, zval *handler TSRMLS_DC) {
	uevent_t *uevent;
	HashTable *listeners = NULL;
	
	UEVENT_EVENT_INIT(uevent);
  	uevent->name = *name;
	zval_copy_ctor
		(&uevent->name);
	uevent->handler = *handler;
	zval_copy_ctor(&uevent->handler);

	if (!(listeners = zend_hash_str_find_ptr(&UG(listeners), Z_STRVAL_P(name), Z_STRLEN_P(name)))) {
		listeners = (HashTable*) emalloc(sizeof(HashTable));
		zend_hash_init(listeners, 8, NULL, uevent_event_dtor, 0);
		zend_hash_str_update_ptr(
			&UG(listeners),
			Z_STRVAL_P(name), Z_STRLEN_P(name),
			listeners);
	}

	return (zend_hash_next_index_insert_ptr(listeners, uevent) == uevent);
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
		uint32_t arg = 0;
		
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
static inline void uevent_invoke(uevent_t *listener, zend_function *address, int nparams, zval *params TSRMLS_DC) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval retval;
	zval *argval = NULL;
	
	if (zend_fcall_info_init(&listener->handler, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
		fci.retval = &retval;

		if (!uevent_verify_listener(fcc.function_handler, address TSRMLS_CC)) {
			return;	
		}

		fci.param_count = nparams;
		fci.params = params;

		zend_call_function(&fci, &fcc TSRMLS_CC);

		if (Z_TYPE(retval) != IS_UNDEF)
			zval_dtor(&retval);
		
		if (argval)
			zval_ptr_dtor(argval);
		
		//zend_fcall_info_args_clear(&fci, 1);
	}
} /* }}} */

/* {{{ */
static inline void uevent_execute(zend_execute_data *execute_data TSRMLS_DC) {
#define EEX(el) (execute_data)->el
	HashTable *events, *listeners;
	uevent_t *event,   *listener;
	HashPosition        position[2];
	zend_function      *address = EEX(func);
	zend_op_array      *ops = (zend_op_array*) address;
	zval               *params = ZEND_CALL_ARG(EG(current_execute_data), 1);
	int                 nparams = ZEND_CALL_NUM_ARGS(EG(current_execute_data));
	
	if (nparams) {
	  int nparam = 0;
	  zval *param = params;
	  while (nparam < nparams) {
	    if (Z_REFCOUNTED_P(param)) {
	      Z_ADDREF_P(param);
	    }
	    nparam++;
	    param++;
	  }
	}
	
	zend_executor_function(execute_data TSRMLS_CC);
	
	if ((ops && ops->fn_flags & ZEND_ACC_GENERATOR)) {
		/* cannot fire events in generators, too messy ... */
		return;
	}
	
	if ((events = uevent_get_events(address, &position[0] TSRMLS_CC))) {
		while ((event = uevent_get_event(events, &position[0]))) {
			if ((listeners = uevent_get_listeners(event, &position[1] TSRMLS_CC))) {
				while ((listener = uevent_get_listener(listeners, &position[1]))) {
					uevent_invoke(listener, address, nparams, params TSRMLS_CC);
				}
			}
		}
	}
} /* }}} */
#endif

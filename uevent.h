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
#ifndef HAVE_UEVENT_H
#define HAVE_UEVENT_H
/* {{{ */
typedef struct _uevent {
	zend_function   *binding;
	HashTable       listeners;
	zend_object	std;	
} uevent_t; /* }}} */

#define php_uevent_fetch(o) ((uevent_t*) (((char*)Z_OBJ_P(o)) - XtOffsetOf(uevent_t, std)))
#define php_uevent_from(o)  (uevent_t*)((char*)(o) - XtOffsetOf(uevent_t, std))
#define php_uevent_chain(r, o) ZVAL_ZVAL(r, o, 1, 0)

/* {{{ */
static inline void uevent_copy_ctor(zval *listener) {
	zval_copy_ctor(listener);
} /* }}} *

/* {{{ */
static inline zend_object* uevent_event_create(zend_class_entry *ce) {
	uevent_t *uevent = (uevent_t*) emalloc(sizeof(uevent_t));
	
	zend_object_std_init(&uevent->std, ce);
	zend_hash_init(&uevent->listeners, 8, NULL, ZVAL_PTR_DTOR, 0);
	uevent->std.handlers = &UEvent_handlers;
	
	return &uevent->std;
} /* }}} */

/* {{{ */
static inline void uevent_event_free(zend_object *zobject) {
	uevent_t *uevent = php_uevent_from(zobject);

	zend_hash_destroy(&uevent->listeners);
	zend_object_std_dtor(&uevent->std);
} /* }}} */

/* {{{ */
static zend_function* uevent_get_binding(zval *call) {
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
					"failed to find binding, class and method not provided");
				return NULL;
			}
			
			if (!(ce = zend_lookup_class(Z_STR_P(clazz)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"failed to find binding, class %s cannot be found", Z_STRVAL_P(clazz));
				return NULL;
			}
			
			lcname = zend_str_tolower_dup(Z_STRVAL_P(method), Z_STRLEN_P(method)+1);
			if (!(address = zend_hash_str_find_ptr(&(ce)->function_table, lcname, Z_STRLEN_P(method)))) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"failed to find binding, method %s could not be found in %s", 
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
					"failed to find binding, the function %s could not be found", 
						Z_STRVAL_P(call));
				efree(lcname);
				return NULL;
			}
			efree(lcname);
		} break;
		
		case IS_OBJECT: {
			zend_fcall_info_cache fcc;

			if (!zend_is_callable_ex(call, NULL, IS_CALLABLE_CHECK_SILENT, NULL, &fcc, NULL)) {
				zend_throw_exception_ex(spl_ce_InvalidArgumentException, 0,
					"failed to find binding, the closure passed was not valid");
				return NULL;
			}
			
			address = fcc.function_handler;
		} break;
	}
	
	if (address->type != ZEND_USER_FUNCTION) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
			"cannot bind to internal function addresses, it does not seem useful !", Z_STRVAL_PP(clazz));
		return NULL;
	}
	
	return address;
} /* }}} */

/* {{{ */
static inline HashTable *uevent_get_events(zend_function *function, HashPosition *position) {
	HashTable *events;
	if (!(events = zend_hash_index_find_ptr(&UG(events), (zend_ulong) function))) {
		return NULL;
	}
	if (position)
		zend_hash_internal_pointer_reset_ex(events, position);
	return events;
} /* }}} */

/* {{{ */
static inline uevent_t *uevent_get_event(HashTable *events, HashPosition *position) {
	zval *bucket;
	
	if (!(bucket = zend_hash_get_current_data_ex(events, position))) {
		return NULL;
	}
	zend_hash_move_forward_ex(events, position);
	
	return php_uevent_fetch(bucket);
} /* }}} */

/* {{{ */
static inline zval *uevent_get_listener(uevent_t *uevent, HashPosition *position) {
	zval *bucket;
	
	if (!(bucket = zend_hash_get_current_data_ex(&uevent->listeners, position))) {
		return NULL;
	}
	zend_hash_move_forward_ex(&uevent->listeners, position);
	
	return bucket;
} /* }}} */

/* {{{ */
static inline zend_bool uevent_verify_error(zend_function *listener, zend_function *address, char *reason) {
	if (address->common.scope) {
		zend_throw_exception_ex(spl_ce_RuntimeException, 0,
		"%s::%s has invalid listener, %s",
		address->common.scope->name, address->common.function_name, reason);
	} else zend_throw_exception_ex(spl_ce_RuntimeException, 0,
		"%s::%s has invalid listener, %s",
		address->common.function_name, address->common.num_args, reason);
	return 0;
} /* }}} */

/* {{{ */
static inline zend_bool uevent_verify_listener(zend_function *listener, zend_function *address) {
	zend_arg_info *info[2] = {NULL, NULL};
	
	if (address->common.num_args) {
		uint32_t arg = 0;
		
		if ((!listener->common.num_args) ||
			(listener->common.num_args != address->common.num_args)) {
			return uevent_verify_error
				(listener, address, "prototype mismatch, wrong number of arguments");
		}
		
		while (arg < address->common.num_args) {
			info[0] = &address->common.arg_info[arg];
			info[1] = &listener->common.arg_info[arg];
			
			if (info[0]->type_hint != info[1]->type_hint) {
				return uevent_verify_error
					(listener, address, "prototype mismatch, type hints incompatible");
			}
			
			arg++;
		}
	}
	
	return 1;
} /* }}} */

/* {{{ */
static inline void uevent_invoke(zval *listener, zend_function *address, int nparams, zval *params) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval retval;
	
	if (zend_fcall_info_init(listener, 0, &fci, &fcc, NULL, NULL) == SUCCESS) {
		fci.retval = &retval;

		if (!uevent_verify_listener(fcc.function_handler, address)) {
			return;	
		}

		fci.param_count = nparams;
		fci.params = params;

		zend_call_function(&fci, &fcc);

		if (Z_TYPE(retval) != IS_UNDEF)
			zval_dtor(&retval);
	}
} /* }}} */

/* {{{ */
static inline void uevent_execute(zend_execute_data *execute_data) {
#define ARGS(action) do {\
	int nparam = 0; \
	zval *param = params; \
	\
	while (nparam < nparams) { \
		if (Z_REFCOUNTED_P(param)) { \
			action(param); \
		} \
		nparam++; \
		param++; \
	}\
} while(0)

	HashPosition        position[2];
	zend_function      *address = execute_data->func;
	int                 nparams = ZEND_CALL_NUM_ARGS(execute_data);
	zval               *params = ZEND_CALL_ARG(execute_data, 1);
	HashTable 	   *events = uevent_get_events(address, &position[0]);

	if (nparams && events) ARGS(Z_ADDREF_P);
	
	zend_executor_function(execute_data);

	if (address->common.fn_flags & ZEND_ACC_GENERATOR) {
		/* cannot fire events in generators, too messy ... */
		goto cleanup;
	}

	if (events) {
		uevent_t	*event;
		zval		*listener;
		
		while ((event = uevent_get_event(events, &position[0]))) {
			while ((listener = uevent_get_listener(event, &position[1]))) {
				uevent_invoke(listener, address, nparams, params);
			}
			position[1] = 0;
		}
	}

cleanup:
	if (nparams && events) ARGS(zval_ptr_dtor);
#undef ARGS
} /* }}} */
#endif

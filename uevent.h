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
static inline HashTable *uevent_get_events(zend_function *function, HashPosition *position TSRMLS_DC) {
	HashTable *events;
	if (zend_hash_index_find(
			&UG(events), 
			(zend_ulong) function, 
			(void**) &events) != SUCCESS) {
		return NULL;
	}
	if (position)
		zend_hash_internal_pointer_reset_ex(events, position);
	return events;
} /* }}} */

/* {{{ */
static inline HashTable *uevent_get_listeners(uevent_t *event, HashPosition *position TSRMLS_DC) {
	HashTable *listeners;
	if(zend_hash_find(
		&UG(listeners), 
		Z_STRVAL(event->name), Z_STRLEN(event->name), 
		(void**)&listeners) != SUCCESS) {
		return NULL;
	}
	if (position)
		zend_hash_internal_pointer_reset_ex(listeners, position);
	return listeners;
} /* }}} */

/* {{{ */
static inline uevent_t *uevent_get_event(HashTable *events, HashPosition *position) {
	uevent_t *event;
	if (zend_hash_get_current_data_ex(
		events, 
		(void**)&event, position) != SUCCESS) {
		return NULL;
	}
	zend_hash_move_forward_ex(events, position);
	return event;
} /* }}} */

/* {{{ */
static inline uevent_t *uevent_get_listener(HashTable *listeners, HashPosition *position) {
	uevent_t *listener;
	if (zend_hash_get_current_data_ex(	
		listeners, 
		(void**)&listener, position) != SUCCESS) {
		return NULL;
	}
	zend_hash_move_forward_ex(listeners, position);
	return listener;
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
	add_next_index_string(&callable, "accept", 1);
	
	if (zend_fcall_info_init(&callable, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
		fci.retval_ptr_ptr = &retval;
		
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
static inline void uevent_invoke(uevent_t *listener TSRMLS_DC) {
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	zval *retval = NULL;
	zval *argval = NULL;
	
	if (zend_fcall_info_init(&listener->handler, 0, &fci, &fcc, NULL, NULL TSRMLS_CC) == SUCCESS) {
		fci.retval_ptr_ptr = &retval;
		
		if (Z_TYPE(listener->args) == IS_OBJECT) {
			zval *object = &listener->args;
			if (zend_call_method_with_0_params(
				&object,
				Z_OBJCE(listener->args), NULL, "get", &argval)) {
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
} /* }}} */
#endif

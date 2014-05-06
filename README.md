UEvent
======
*Userland events in PHP*

This here is an extension to allow users to decare and Zend to fire events at runtime, because delicious ...

Example
=======
*How ...*

```php
<?php
class foo {
	public static function bar() {}
	/* ... */
}

UEvent::addEvent("foo.bar", ["Foo", "bar"])
UEvent::addListener("foo.bar", function(array $args = []){
	echo "hello foo::bar\n";
});

/* ... */

foo::bar();
?>
```

Will output:

```
hello foo::bar
```

API
===
*The rest ... WIP!!*

```php
<?php
interface UEventInput {
/**
* Shall recieve the argument stack at call time
* @returns boolean
* Note: use func_get_args
*/
	public function accept();
}

interface UEventArgs {
/**
* Shall return arguments for event listener invocation
* @param string name
* @returns mixed
*/
	public function get();
}

class UEvent {
/**
* Shall call $handler($args->get()) when $name is fired by uevent 
* @param string name
* @param Closure handler
* @returns boolean
* @throws \RuntimeException
*/
	public static function addListener($name, Closure $handler, UEventArgs $args = null);

/**
* Shall find all listeners atttached to $name
* @param string name
* @returns array
*/
	public static function getListeners($name);
	
/**
* Shall create an event of the given $name:
*  $name shall be fired when $input->accept() returns true
* @param string name
* @param callable call
* @param UEventInput input
* @returns boolean
* @throws \RuntimeException
*/
	public static function addEvent($name, callable $call, UEventInput $input = null);
}
?>
```

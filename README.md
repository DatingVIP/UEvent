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

var_dump(UEvent::addEvent("foo.bar", ["Foo", "bar"]));

var_dump(UEvent::addListener("foo.bar", function(UEventArgs $args = null){
	echo "hello foo::bar\n";
}));

foo::bar();
?>
```

API
===
*The rest ... WIP!!*

```php
<?php
interface UEventInput {
/**
* Shall return boolean true if $params should be accepted
* If $params are accepted the current event will be fired
* @param array params
* @returns boolean
*/
	public function accept(array $params);
}

interface UEventArgs {
/**
* Shall return an argument by name, or all arguments
* @param string name
* @returns mixed
*/
	public function get($name = null);
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
	
/*
* Shall create an event of the given $name:
*  $name shall be fired when $input->match returns true
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

UEvent
======
*Userland events in PHP*

This here is an extension to allow users to decare and Zend to fire events at runtime, because delicious ...

Example
=======
*How ...*

The following code demonstrates how to attach events to the invokation of methods or functions:

```php
<?php
class foo {
	public static function bar(array $array, float $float) {
	  /* ... */
	}
	/* ... */
}

UEvent::addEvent("foo.bar", ["Foo", "bar"]);
UEvent::addListener("foo.bar", function(array $array = [], float $float){
	echo "hello foo::bar\n";
	var_dump($array, 
		 $float);
});

/* ... */

foo::bar(["first", "second"], 3.33);
?>
```

Will output:

```
hello foo::bar
array(2) {
  [0]=>
  string(5) "first"
  [1]=>
  string(6) "second"
}
float(3.33)
```

API
===
*Then names of things and what not ...*

```php
<?php
class UEvent {
/**
* Shall create an event of the given $name at $call location:
* @param string name
* @param callable call
* @returns boolean
* @throws \RuntimeException
*/
	public static function addEvent($event, callable $call);

/**
* Shall add a listening function to execute when the named event is fired
* @param string event
* @param Closure listener
* @returns boolean
* @throws \RuntimeException
*/
	public static function addListener($name, Closure $listener);
	
/**
* Shall return the names of all events
* @returns array
*/
	public static function getEvents();
}
?>
```

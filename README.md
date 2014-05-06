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
	public static function bar() {}
	/* ... */
}

UEvent::addEvent("foo.bar", ["Foo", "bar"])
UEvent::addListener("foo.bar", function(array $array = []){
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

Trigger events based on arguments
=================================
*A bit more complicated ...*

The following code demonstrates how to use ```UEventInput``` in combination with ```UEventArgs``` to capture
and pass the argument stack from call to listener

```php
<?php
class foo {
	public static function bar($foo) {}
	/* ... */
}

/* Will capture arguments at calltime and trigger event based on arguments
	also stores argument stack for passing to listener ... so ... voodoo ... */
class EventArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = func_get_args();
		if (count($this->args)) {
			return ($this->args[0] == "trigger");
		}
	}
	
	public function get() { return $this->args;	}

	protected $args;
}

$arguments = new EventArgs();
UEvent::addEvent("foo.bar", ["Foo", "bar"], $arguments);
UEvent::addListener("foo.bar", function($argv){
	echo "Foo::bar({$argv[0]}) called\n";
}, $arguments);

foo::bar('trigger');
foo::bar('no-trigger');
?>
```

Will output

```
Foo::bar(trigger) called
```

API
===
*Then names of things and what not ...*

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
* @returns array
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
* Shall create an event of the given $name:
*  $name shall be fired when $input->accept() returns true
* @param string name
* @param callable call
* @param UEventInput input
* @returns boolean
* @throws \RuntimeException
*/
	public static function addEvent($name, callable $call, UEventInput $input = null);
	
/**
* Shall return the names of all events
* @returns array
*/
	public static function getEvents();
}
?>
```

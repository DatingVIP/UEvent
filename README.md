UEvent
======
*Userland events in PHP*

This here is an extension to allow users to decare and Zend to fire events at runtime, because delicious ...

Example
=======
*How ...*

The following code demonstrates usage (see tests for more info):

```php
<?php
class foo {
	public static function bar(array $array, float $float) {
	  /* ... */
	}
	/* ... */
}

$event = new UEvent([Foo::class, "bar"]);
$event->add(function(array $array = [], float $float){
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
*The names of things and what not ...*

```php
<?php
class UEvent {
	public function __construct(callable binding);

	public function add(Closure listener);

	public function remove(integer index);

	public function reset(void);

	public function list(void);
}
?>
```

--TEST--
Check for removal of listener
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
class foo {
	public static function bar(string $arg) {
		return $arg;
	}
	/* ... */
}

$event = new UEvent([Foo::class, "bar"]);
$event->add(function(string $arg){
	var_dump($arg);
});
var_dump($event->remove(0));
?>
--EXPECT--
bool(true)

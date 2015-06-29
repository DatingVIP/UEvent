--TEST--
Check listeners reset
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
$event->reset();
var_dump($event->list());
?>
--EXPECT--
array(0) {
}


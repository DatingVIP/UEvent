--TEST--
Check for UEvent::addEvent on non-static method
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
class Foo {
	public function bar($arg) {
		return $arg;
	}
}

UEvent::addEvent("my.event", ["Foo", "bar"]); # don't care if not static
var_dump(UEvent::getEvents());
?>
--EXPECTF--
array(1) {
  [0]=>
  string(8) "my.event"
}


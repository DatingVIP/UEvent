--TEST--
Check for UEvent::getEvents functionality
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
class Foo {
	public static function bar($arg) {
		return $arg;
	}
}

UEvent::addEvent("my.event", ["Foo", "bar"]); # don't care if names clash
UEvent::addEvent("my.event", ["Foo", "bar"]); # each call creates an event
UEvent::addEvent("my.event", ["Foo", "bar"]); # if the callable passed is valid

var_dump(UEvent::getEvents());
?>
--EXPECTF--
array(3) {
  [0]=>
  string(8) "my.event"
  [1]=>
  string(8) "my.event"
  [2]=>
  string(8) "my.event"
}


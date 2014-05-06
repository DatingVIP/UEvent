--TEST--
Check for UEventInput functionality
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php 
class Foo {
	public static function bar($arg) {
		return $arg;
	}
}

class Selector implements UEventInput {
	public function accept() {
		$args = func_get_args();
		
		if (count($args)) {
			return ($args[0] == "trigger");
		}
	}
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"], new Selector()));
var_dump(UEvent::addListener("my.event", function(){
	echo "fired";
}));

Foo::bar("arg");
Foo::bar("trigger");
?>
--EXPECTF--
bool(true)
bool(true)
fired


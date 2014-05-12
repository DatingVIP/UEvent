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

class FooArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = 
			func_get_args();
		if (count($this->args)) {
			return 
				($this->args[0] == "trigger");
		}
	}
	
	public function get() {
		return $this->args;
	}
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"], $args = new FooArgs()));
var_dump(UEvent::addListener("my.event", function($arg){
	echo "fired";
}, $args));

Foo::bar("arg");
Foo::bar("trigger");
?>
--EXPECTF--
bool(true)
bool(true)
fired


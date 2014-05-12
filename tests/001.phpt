--TEST--
Check for basic uevent functionality
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
class FooArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = 
			func_get_args();
		if (count($this->args)) {
			return true;
		}
	}
	
	public function get() { return $this->args; }
	
	protected $args;
}

class Foo {
	public static function bar($arg) {
		return $arg;
	}
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"], $args = new FooArgs()));
var_dump(UEvent::addListener("my.event", function($arg){
	echo "fired";
}, $args));

Foo::bar("arg");
?>
--EXPECTF--
bool(true)
bool(true)
fired


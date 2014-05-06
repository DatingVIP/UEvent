--TEST--
Check for basic uevent functionality
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php 
class Foo {
	public static function bar($arg) {
		return $arg;
	}
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"]));
var_dump(UEvent::addListener("my.event", function(){
	echo "fired";
}));

Foo::bar("arg");
?>
--EXPECTF--
bool(true)
bool(true)
fired


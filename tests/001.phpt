--TEST--
Check basic
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
class Foo {
	public static function bar(string $arg) {
		return $arg;
	}
}

$event = new UEvent([Foo::class, "bar"]);
$event->add(function(string $arg){
	echo "fired";
});

Foo::bar("arg");
?>
--EXPECTF--
fired


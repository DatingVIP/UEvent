--TEST--
Check arguments
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
	var_dump($arg);
});

Foo::bar("arg");
?>
--EXPECT--
string(3) "arg"


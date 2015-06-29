--TEST--
Check listener list
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
var_dump($event->list());
var_dump($event->remove(0));
var_dump($event->list());
?>
--EXPECT--
array(1) {
  [0]=>
  object(Closure)#2 (1) {
    ["parameter"]=>
    array(1) {
      ["$arg"]=>
      string(10) "<required>"
    }
  }
}
bool(true)
array(0) {
}


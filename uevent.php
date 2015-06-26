<?php
/* dummy */
class Foo {
	public static function bar($qux = null) {
		return __METHOD__;
	}
}

UEvent::addEvent("onFooBar", ["Foo", "bar"]);
UEvent::addListener("onFooBar", function($qux = null) {
	echo "in listener\n";
	var_dump(func_get_args());
});

Foo::bar(new stdClass(__FILE__));

$foo = new Foo();
$foo->bar(__FILE__, __FILE__);
?>

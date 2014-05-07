<?php
class foo {
	public function bar($foo) {}
	protected function qux($bar) {}
	/* ... */
}

UEvent::addEvent("foo.bar", ["Foo", "bar"]);
UEvent::addEvent("foo.qux", ["Foo", "qux"]);
UEvent::addListener("foo.bar", function(){
	echo "Foo::bar() called\n";
});

$foo = new foo();
$foo->bar('trigger');

var_dump(UEvent::getEvents());
?>

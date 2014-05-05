<?php
class foo {
	public static function bar() {}
	/* ... */
}

var_dump(UEvent::addEvent("foo.bar", ["Foo", "bar"]));

var_dump(UEvent::addListener("foo.bar", function(UEventArgs $args = null){
	echo "hello foo::bar\n";
}));

foo::bar();
?>

<?php
class foo {
	public static function bar() {}
	/* ... */
}

UEvent::addEvent("foo.bar", ["Foo", "bar"]);
UEvent::addListener("foo.bar", function(array $array = [], float $float){
	echo "hello foo::bar\n";
	var_dump($array, 
		 $float);
});

/* ... */

foo::bar(["first", "second"], 3.23);
?>

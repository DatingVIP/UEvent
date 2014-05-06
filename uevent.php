<?php
class foo {
	public static function bar($foo) {}
	/* ... */
}

class EventSelector implements UEventInput {
	public function accept() {
		$args = func_get_args();
		
		if (count($args)) {
			return ($args[0] == "trigger");
		}
	}
}

var_dump(UEvent::addEvent("foo.bar", ["Foo", "bar"], new EventSelector()));
var_dump(UEvent::addListener("foo.bar", function(UEventArgs $args = null){
	echo "hello foo::bar\n";
}));

foo::bar('no-trigger');
foo::bar('trigger');
?>

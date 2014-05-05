<?php
class foo {
	public static function bar() {
	
	}
}

class EventArgs implements UEventArgs {
	public function get($offset = null) {
		return ["hello"];
	}
}

var_dump(UEvent::addEvent("foo.bar", ["Foo", "bar"]));

var_dump(UEvent::addListener("foo.bar", function(array $args = []){	
	echo "foo::bar with UEventArgs:\n";
	var_dump($args);
},  new EventArgs()));

var_dump(UEvent::addListener("foo.bar", function(UEventArgs $args = null){
	echo "hello again foo::bar\n";
}));

foo::bar();
?>

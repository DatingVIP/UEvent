--TEST--
Check for UEventArgs functionality
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php 
class Foo {
	public static function bar($arg) {
		return $arg;
	}
}

class Selector implements UEventInput {
	public function accept() {
		$args = func_get_args();
		
		if (count($args)) {
			return ($args[0] == "trigger");
		}
	}
}

class Arguments implements UEventArgs {
	public function __construct($key) {
		$this->key = $key;
	}
	
	public function get($offset = null) { return $this->key; }
	
	protected $key;
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"], new Selector()));
var_dump(UEvent::addListener("my.event", function($args){
	echo "fired\n";
	var_dump($args);
}, new Arguments("qux")));

Foo::bar("arg"); # will not fire event
Foo::bar("trigger"); # will fire event
?>
--EXPECTF--
bool(true)
bool(true)
fired
string(3) "qux"

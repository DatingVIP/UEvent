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

class FooArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = func_get_args();
		
		if (count($this->args)) {
			return 
				($this->args[0] == "trigger");
		}
	}
	
	public function get() { return $this->args; }
	
	protected $args;
}

var_dump(UEvent::addEvent("my.event", ["Foo", "bar"], $args = new FooArgs()));
var_dump(UEvent::addListener("my.event", function($arg){
	echo "fired\n";
	var_dump($arg);
}, $args));

Foo::bar("arg"); # will not fire event
Foo::bar("trigger"); # will fire event
?>
--EXPECTF--
bool(true)
bool(true)
fired
string(7) "trigger"

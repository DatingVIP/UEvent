<?php
class foo {
	public static function bar($foo) {}
	/* ... */
}

/* Will capture arguments at calltime and trigger event based on arguments
	also stores argument stack for passing to listener ... so ... voodoo ... */
class EventArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = func_get_args();
		if (count($this->args)) {
			return ($this->args[0] == "trigger");
		}
	}
	
	public function get() { return $this->args;	}

	protected $args;
}

$arguments = new EventArgs();
UEvent::addEvent("foo.bar", ["Foo", "bar"], $arguments);
UEvent::addListener("foo.bar", function($argv){
	echo "Foo::bar({$argv[0]}) called\n";
}, $arguments);

foo::bar('trigger');
foo::bar('no-trigger');
?>

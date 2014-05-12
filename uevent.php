<?php
/* dummy */
class Foo {
	public static function bar($qux = null) {
		return __METHOD__;
	}
}

/* move args around */
class FooArgs implements UEventInput, UEventArgs {
	public function accept() {
		$this->args = 
			func_get_args();

		if (count($this->args)) {
			return true;
		}
	}
	
	public function get() { return $this->args; }
	
	protected $args;
}

UEvent::addEvent("onFooBar", ["Foo", "bar"], $args = new FooArgs());
UEvent::addListener("onFooBar", function($qux) {
	var_dump($qux);
}, $args);

Foo::bar(new stdClass(__FILE__));

$foo = new Foo();
$foo->bar(__FILE__);
?>

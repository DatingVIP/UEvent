<?php
class foo {
	public static function bar(string $arg) {
		return $arg;
	}
	/* ... */
}

$event = new UEvent([Foo::class, "bar"]);
$event
	->add(function(string $arg){
		echo "listened for $arg\n";
	})->add(function(string $arg){
		echo "also listened for $arg\n";
	});

$nev = new UEvent([Foo::class, "bar"]);
$nev->add(function(string $arg){
	echo "new event listened for $arg too\n";
});

Foo::bar("arg");
?>

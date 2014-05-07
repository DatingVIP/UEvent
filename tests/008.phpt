--TEST--
Check for UEvent::addEvent on function
--SKIPIF--
<?php include "skip-if.inc" ?>
--FILE--
<?php
function foobar() {}
UEvent::addEvent("my.event", "foobar"); # don't care if function
var_dump(UEvent::getEvents());
UEvent::addListener("my.event", function(){
	echo "fired";
});
foobar();
?>
--EXPECT--
array(1) {
  [0]=>
  string(8) "my.event"
}
fired


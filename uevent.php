<?php
/* dummy */
class DB {
	public static function fetch($event) {
		/* SELECT * FROM events WHERE event = '$event' && enabled = '1' */
		foreach (self::$events[$event] as $interface) {
			yield $interface;
		}
	}
	
	protected static $events = [
		"UserRegisteredEvent" => [
			"SendEmailHandler",
			"BurnADogHandler"
		]
	];
}

class User {
	/* this defines the UserRegisteredEvent invoke method */
	public function register($user, $email) {
		printf("registering %s -> %s\n", $user, $email);
		return true;
	}
}

/* defined by the method the event was registered on */
interface UserRegisteredEvent {
	public static function invoke($user, $email);
}

/* move args around */
class UserRegisteredEventArgs implements UEventInput, UEventArgs {
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

/* I can run on User::register, present me in interface with checkbox
	under the User::register column for events to enable ? */
class SendEmailHandler implements UserRegisteredEvent {
	public static function invoke($user, $email) {
		printf("send email to %s -> %s\n", $user, $email);
	}
}

/* I can run on User::register, present me in interface with checkbox
	under the User::register column for events to enable ?
	also, I burn dogs ... */
class BurnADogHandler implements UserRegisteredEvent {
	public static function invoke($user, $email) {
		printf("burning the dog of %s ...\n", $user);
	}
}

$ua = new UserRegisteredEventArgs();
UEvent::addEvent("UserRegisteredEvent", ["User", "register"], $ua);
UEvent::addListener("UserRegisteredEvent", function($args){
	foreach (DB::fetch("UserRegisteredEvent") as $handler) {
		call_user_func_array([$handler, "invoke"], $args);
	}
}, $ua);

$user = new User();
$user->register("krakjoe", "krakjoe@php.net");
?>

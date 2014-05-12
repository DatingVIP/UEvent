<?php
/* dummy */
class DB {
	public static function fetch($event) {
		foreach (self::$events[$event] as $event => $method) {
			yield $event => $method;
		}
	}
	
	protected static $events = [
		"ControllerEvents" => [
			"onRegister" => ["User", "register"],
			"onLogin"=>		["User", "login"]
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

interface UEvents {
	public function getEvents();
}

interface ControllerEvent {
	public function invoke(Controller $controller, array $argv = []);
}

/* move args around */
class ControllerEventArgs implements UEventInput, UEventArgs {
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

class Controller implements UEvents {
	public function getEvents() {
		return [
			"onRegister" => ["User", "register"],
			"onLogin"=>		["User", "login"]
		];
	}
}

/* I can run on User::register, present me in interface with checkbox
	under the User::register column for events to enable ? */
class SendEmail implements ControllerEvent {
	public static function invoke(Controller $controller, array $argv = []) {
		printf(
			"send email to %s -> %s\n", $argv[0], $argv[1]);
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

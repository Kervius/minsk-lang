# minsk-lang
Minimalistic Scripting Language

Purpose
-------

It is a barebone implementation of a primitive dynamic interpreted langauge, inteded as a testbed for ideas.

Interpreter is written in C++ and is about 200-300 ELOC (not including the barebone std library).

To keep it simple, it borrows the most basic syntax features from Tcl, Lisp and Unix shell. Execution is conventional: the command are executed as soon as they are fully parsed (and they arguments are expanded). Similar to Unix shell, everything is a string.

In the current state, the standard library contains only the functions required to implement the factorial example.

Example - Factorial
-------------------

	proc fact n {
		if (eq $n 0) {
			return 1
		} else {
			var m (fact (add $n -1))
			return (mul $n $m)
		}
	}
	println (fact 6)

TODO
----
* [ ] object and object-orientation
* [ ] containers (possibly based on OO; basic array can be implemented already using the string splices)
* [ ] a basic type checking system

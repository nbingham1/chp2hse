# chp2hse

**Usage**: `chp2hse [options] files...`

This program takes in files written in CHP and decomposes each process into two HSE's: an HSE that defines the process itself and an HSE that defines the most basic environment. To compile a group of files, you only need specify the top most level file. All included files will automatically be
 pulled into the compilation.

**Options**:
 - `-h`,`--help` Display some basic information about these options
 - `-v`,`--version` Display compiler version information
 - `-s`,`--stream` Print HSE to standard out. If this option is not specified, then each process will be placed into its own file named <process name>.dot or <process name>.hse
 - `-f`,`--flat` Print the human readable HSE. If this is specified the file extension is ".hse". Otherwise, its ".dot" and the file will contain a graphical representation of the HSE that may be rendered using graphviz's dot program. The reason to have a graphical representation of the HSE is to allow handling of non-proper nesting.
 - `-p`,`--process <process name>,...` Only print the HSE of the specified processes. If this option is not used, the HSE for all processes are printed

## CHP Language

There are four basic structures to this version of chp: records, channels, processes, and operators. By definition the most basic and the only hardcoded data type is the node. A node represents a group of wires and must always be specified with a bit width. The syntax for specifying an N bit node named a is as follows:

```
node<N> a := expression
```

The expression is optional. If this declaration is in the middle of a CHP block, then a new variable 'a' will be created with no reset value, and the declaration will be changed into an assignment a := expression. Otherwise, the expression must be constant and a new variable 'a' will be created with a reset value given by the expression. To declare the variable a without an expression, the syntax is as follows:

```
node<N> a
```

Nodes are the only type in which you may specify a bit width. Variables of all other types are declared as follows:

```
type a := expression
```

### Records

Records are used to group nodes, channels, and other records into a single named data type. The basic template for records is as follows:

```
record typename
{
	declaration;
	declaration;
	...
}
```

For example:

```
record dualrail
{
	node<1> t := 0;
	node<1> f := 0;
}
```

Creates a dualrail type with two variables t and f that each reset to ground. To create a new variable 'r' of type dualrail one would simply type:

```
dualrail r
```

You may also create records of records. For example

```
record byte
{
	dualrail b0;
	dualrail b1;
	dualrail b2;
	dualrail b3;
	dualrail b4;
	dualrail b5;
	dualrail b6;
	dualrail b7;
}
```

### Channels

Channels allow you to create data types that have the send, receive, and probe operators associated with them. They are defined very similarly to records except for those operators. The template channel definition is as follows:

```
channel typename
{
	declaration;
	declaration;
	...
	operator!(declaration)
	{
		...
	}

	operator!(declaration)
	{
		...
	}
	
	...

	operator?(declaration)
	{
		...
	}

	operator?(declaration)
	{
		...
	}
	
	...

	operator#(declaration)
	{
		...
	}
}
```

When defining a channel you may define multiple send and receive operators given different input/output types, but only one probe operator. In all of these operators, the channel is automatically included in the variable space with the name 'this' as the first argument. An example channel definition of a four phase active enable channel is as follows:

```
channel e1of1
{
	node<1> r;
	node<1> e;

	operator!()
	{
		r+;[~e];r-;[e]
	}

	operator?()
	{
		[r];e-;[~r];e+
	}

	operator#(node<1> result)
	{
		result := r;
	}
}
```

### Processes

Processes are the only structures that are ultimately compiled into production rules. they consist of a name, a list of connecting variables, a block of CHP that represents that process's internal circuitry, and a block of CHP that represents the simplest possible environment for that process. Their syntax is as follows:

```
process myproc(variable declarations...)
{
	main CHP...
}
{
	environment CHP...
}
```

The environment CHP is optional. If it is not specified, the compiler will generate an environment using the send and receive operators defined in the channels. The syntax for leaving the environment unspecified is as follows:

```
process myproc(variable declarations...)
{
	main CHP...
}
```

### Operators

Operators are the backbone of this language. They are very similar to processes except that they are only instantiated by using them in an expression. The one bit Boolean operators (&, |, ~, and :=) are the only operators that are pre defined, all other operators must be defined before they are used. An example operator definition is as follows:

```
operator^(dualrail a, dualrail b, dualrail o)
{
	o := a & ~b | b & ~a
}
```

In an operator definition, the last connection is always the output while all others are always inputs. The only operator that does not follow this rule is channel send (!). 

Upon instantiation, expressions or sub expressions using only one bit Boolean variables are treated like a variable name and passed directly onto the HSE. Other operators are treated like macros and instantiated as the data flow requires. For example, if you have an assignment of dualrails as follows:

```
a := (x & y) ^ (u & v)
```

It will be expanded into:

```
(operator&(x, y, v1) || operator&(u, v, v2)); operator^(v1, v2, v3); a := v3
```

All assignment operators are handled automatically. Given an assignment a := b where a and b are the same type, that assignment will be split up into N conditionals where N is the number of wires contained within that type. The conditionals are as follows:

```
[  b  -> a+
[] ~b -> a-
]
```

### Misc

All other CHP syntax is the same except:

 - `~` is boolean not
 - `+` is up arrow
 - `-` is down arrow
 - `#A` is a probe on the channel A
 - `:` is non-deterministic selection
 - `->` is the guard arrow

IMPORTANT!!!

Because of this expansion framework, probes and operators on shared variables are handled dead wrong, creating unstable guards. For example: consider the conditional `[#A -> A?]` where A is the `e1of1` defined above. This will be expanded into the following:

```
[  A.r  -> v0+
[] ~A.r -> v0-
]; [v0 -> [A.r]; A.e-; [~A.r]; A.e+]
```

Creating the unstable guard ~A.r -> ... and a deadlocking guard at v0 -> ...

Also, non-deterministic selection statements are not yet supported. They are currently handled exactly the same way deterministic selection statements are, so that no error will be thrown in chp2hse, but it will cause problems later in hse2prs.


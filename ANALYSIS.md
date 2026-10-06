1. Pick 3 of the 10 categories. For each, pick a language that gives it to you for free and say what that language pays for it. "Python has dictionaries" isn't an answer. What does Python's dictionary cost in memory or in speed compared to what you built, and where would you notice?

	c. Python lists vs dt_array
	In Python, you never have to declare a size up front or manage a lower bound, unlike the fixed-size dt_array we implemented. Python's list grows and shrinks dynamically, and the language handles all the resizing for you. In exchange for this convenience, Python pays for it in memory, since each element in a Python list isn't actually stored inline the way our dt_value elements are. Instead, the list only stores pointers, and each element is a separate, heap-allocated object elsewhere in memory, carrying its own type pointer and reference count, even for something as small as an integer which for lists containing 100s of elements can very quickly occupy a big chunk of memory. For Python, this also costs them speed, since iterating a list means chasing a pointer to a different part of memory for every single element, rather than reading straight through one contiguous block the way our array allows.


2. You wrote the tag check in dt_value_as_int by hand. Some languages don't let you. They make the tagged union a language construct, so the compiler writes the check for you, refuses to compile a read that skips it, and refuses to compile a set of cases that misses one. Rust's enum and match work this way, and so do ML's datatypes and Swift's enumerations with associated values. What does the C version let you do that a compiler enforcing the check wouldn't, and is any of it worth wanting?

C’s tag system lets you skip the check entirely by reading .as.string or other union member instead of going through dt_value_as_str. 

For example:

dt_value v = dt_value_int(42); 
dt_str *s = v.as.string;
printf("%s", s->bytes);   // dereferences address 42


The code above compiles without the compiler throwing an error even though v.tag is DT_INT. The union's bytes hold the integer 42's bit pattern, which is now treated as a pointer. Dereferencing it reads from address 42, which is almost certainly not a valid memory address, making this undefined behavior.

C’s version also lets you add new tags without making the compiler force readers to handle the new tag, unlike other languages like Rust, whose compiler does. For example, if a new tag DT_FLOAT were to be added and a function print_value() builds output like:

void print_value(dt_value v)
{
    switch (v.tag) {
        case DT_NIL:   printf("nil"); break;
        case DT_INT:   printf("%lld", v.as.integer); break;
        case DT_STR:   printf("\"%s\"", v.as.string->bytes); break;
        // no case DT_FLOAT, no default
    }
}


If the function above were to be called with an argument tagged DT_FLOAT, it would still compile but would print nothing.

Neither are worth wanting in this context: Both of these capabilities that C allows you to do only increase the risk of undetected errors slipping past compilation and surfacing at runtime and are more of a lack of a safety net rather than a feature that is unique to the language. The tag check in dt_value_as_int costs one integer comparison, so the cost of safety here is negligible, while the cost of skipping it is undefined behavior. And there is no legitimate use case for wanting new tag variants to be silently unhandled.


4. Compare access after release with an allocation that remains unreleased at the driver's final check. What damage can each cause in a long-running server? How does that answer change for a command-line tool that exits in a second?
	a. Access after release (use-after-free) 
		i. In a long-running server: Extremely dangerous since the freed memory doesn't disappear; it becomes available for the next allocation, which, in a server handling many concurrent or sequential requests, is very likely to be a completely unrelated object. Reading through the dangling pointer can return another user's data; writing through it can corrupt that unrelated object's state.
		ii. For a short CLI tool: Far less dangerous but not harmless, since a UAF read can still produce wrong output or crash the single invocation,  but the range is one process, one run, with no other users or requests sharing that memory space to corrupt.
	b. An allocation that remains unreleased at exit (a leak)
		i. In a long-running server: The process doesn't crash immediately; it just accumulates unreachable memory for every request sent to the server. If left running for a long enough time, it will eventually exhaust available memory, which can cause degradation in the performance or even crashes during traffic spikes.
		ii. For a short CLI tool: Close to inconsequential since the operating system reclaims the entire process's memory the instant it exits so anything leaked never outlives the program itself.

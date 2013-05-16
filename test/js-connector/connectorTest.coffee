chai = require "chai"
should = chai.should()
expect = chai.expect
chai.Assertion.includeStack = true

module = require "../../build/harfbuzz-test"
[struct, array, ptr, string, define, callback, typedef, free] = [module.struct, module.array, module.ptr, module.string, module.define, module.callback, module.typedef, module.free]
[SelfPtr, Void] = [module.SelfPtr, module.Void]
[allocate, _free, intArrayFromString] = [module.allocate, module._free, module.intArrayFromString]

using = (tests..., func) ->
	func()
	for test in tests
		free test
	return

describe "Empty struct", ->
	test_t = struct {
		# Empty struct
	}

	it "should be a function", ->
		should.exist test_t
		test_t.should.be.a "function"

	it "should have a size of 0", ->
		test_t.resolve()
		test_t.size.should.equal 0

	it "should create an empty struct instance", ->
		using test = new test_t(), ->
			test.toString().should.equal "{ }"
			expect(test.field).not.to.exist

	it "should have a non-null pointer", ->
		using test = new test_t(), ->
			test.$ptr.should.be.above 0

	it "should throw an error if it is freed for a second time", ->
		test = new test_t()
		free test
		expect(-> free test).to.throw # something


describe "Simple struct of a single i32 field", ->
	test_t = struct {
		field: "i32",
	}
	test_t.resolve()

	it "should have a size of 4", ->
		test_t.size.should.equal 4

	it "should create an instance initialized with zeroes", ->
		using test = new test_t(), ->
			test.toString().should.equal "{ field: 0 }"
			expect(test.get("field")).to.exist

	it "should have a r/w number field called 'field'", ->
		using test = new test_t(), ->
			expect(test.get("field")).to.be.a "number"
			test.get("field").should.equal 0
			module.getValue(test.$ptr, "i32").should.equal 0
			test.set "field", 12
			test.get("field").should.equal 12
			module.getValue(test.$ptr, "i32").should.equal 12
			test.set "field", -12
			test.get("field").should.equal -12
			module.getValue(test.$ptr, "i32").should.equal -12

	it "should truncate non-integers", ->
		using test = new test_t(), ->
			test.set "field", -12.8
			test.get("field").should.equal -12
			module.getValue(test.$ptr, "i32").should.equal -12

	it "should have a toString()", ->
		using test = new test_t(), ->
			test.set "field", 123
			test.toString().should.equal "{ field: 123 }"
			test.set "field", -443
			test.toString().should.equal "{ field: -443 }"


describe "Complex struct of i32, i8, i16, i1", ->
	test_t = struct {
		f1: "i8",
		f2: "i16",
		f3: "i32",
		f4: "i64",
		f5: "i1",
	}
	test_t.resolve()

	it "should have a size of 24 (i.e. 4 x 32 bit + 1 x 64 bit)", ->
		test_t.size.should.equal 4 * 4 + 1 * 8

	it "should have the right fields initialized to 0", ->
		using test = new test_t(), ->
			test.get("f1").should.equal 0
			test.get("f2").should.equal 0
			test.get("f3").should.equal 0
			test.get("f4").should.equal 0
			test.get("f5").should.equal 0

	it "should be assignable", ->
		using test = new test_t(), ->
			test.set "f1", 1
			test.set "f2", 2
			test.set "f3", 3
			test.set "f4", 4
			test.set "f5", 0

			test.get("f1").should.equal 1
			test.get("f2").should.equal 2
			test.get("f3").should.equal 3
			test.get("f4").should.equal 4
			test.get("f5").should.equal 0

			test.set "f2", 123
			test.get("f1").should.equal 1
			test.get("f2").should.equal 123
			test.get("f3").should.equal 3
			test.get("f4").should.equal 4
			test.get("f5").should.equal 0

	it "should truncate non-integers", ->
		using test = new test_t(), ->
			test.set "f1", 1.2
			test.set "f2", 2.9
			test.set "f3", 3.1
			test.set "f4", 4.5
			test.set "f5", 17.9

			test.get("f1").should.equal 1
			test.get("f2").should.equal 2
			test.get("f3").should.equal 3
			test.get("f4").should.equal 4
			# "i1" is actually the same as "i8"
			test.get("f5").should.equal 17

	it "should handle overflows", ->
		using test = new test_t(), ->
			test.set "f1", Math.pow(2, 8) + 1
			test.set "f2", Math.pow(2, 16) + 2
			test.set "f3", Math.pow(2, 32) + 3
			test.set "f4", Math.pow(2, 32) + 4
			test.set "f5", Math.pow(2, 8) + 7

			test.get("f1").should.equal 1
			test.get("f2").should.equal 2
			test.get("f3").should.equal 3
			# "i64" is basically "i32" for us
			# TODO is this okay?
			test.get("f4").should.equal 4
			test.get("f5").should.equal 7


describe "Complex struct of float, double", ->
	test_t = struct {
		f1: "float",
		f2: "double",
	}
	test_t.resolve()

	it "should have a size of 12", ->
		test_t.size.should.equal 4 + 8

	it "should handle floating point values", ->
		using test = new test_t(), ->
			test.set "f1", 1.2
			test.set "f2", 2.9
			# Float is broken with i386-linux target, beware!
			# Only use doubles
			# test.get("f1").should.be.closeTo 1.2, 0.001
			test.get("f2").should.equal 2.9


describe "Nested struct", ->
	child_t = struct {
		c1: "i32",
		c2: "i32",
	}
	parent_t = struct {
		p0: "i32",
		p1: child_t,
		p2: "i32",
		p3: child_t,
		p4: "i32",
	}
	parent_t.resolve()

	it "should have the correct sizes", ->
		child_t.size.should.equal 4 + 4
		parent_t.size.should.equal 4 + 8 + 4 + 8 + 4

	it "should be created and initialized to all zero", ->
		using test = new parent_t(), ->
			test.get("p0").should.be.a "number"
			test.get("p1").should.be.an "object"
			test.get("p1").get("c1").should.be.a "number"
			test.get("p1").get("c2").should.be.a "number"
			test.get("p2").should.be.a "number"
			test.get("p3").should.be.an "object"
			test.get("p3").get("c1").should.be.a "number"
			test.get("p3").get("c2").should.be.a "number"
			test.get("p4").should.be.a "number"

			test.get("p0").should.equal 0
			test.get("p1").get("c1").should.equal 0
			test.get("p1").get("c2").should.equal 0
			test.get("p2").should.equal 0
			test.get("p3").get("c1").should.equal 0
			test.get("p3").get("c2").should.equal 0
			test.get("p4").should.equal 0

	it "should have children that can be freed any number of times", ->
		using test = new parent_t(), ->
			# Apparently Emscripten has no problem with
			# freeing non-malloc'd data multiple times
			free test.get("p1")
			free test.get("p1")

	it "should have working nested properties", ->
		using test = new parent_t(), ->
			test.set "p0", 11
			test.get("p1").set "c1", 7
			test.get("p1").get("c1").should.equal 7
			test.get("p1").set "c2", -12
			test.get("p1").get("c2").should.equal -12
			module.getValue(test.$ptr + 0, "i32").should.equal 11
			module.getValue(test.$ptr + 4, "i32").should.equal 7
			module.getValue(test.$ptr + 8, "i32").should.equal -12
			module.getValue(test.get("p1").$ptr + 0, "i32").should.equal 7
			module.getValue(test.get("p1").$ptr + 4, "i32").should.equal -12

	it "should have a toString()", ->
		using test = new parent_t(), ->
			test.set "p0", 1
			test.get("p1").set "c1", 2
			test.get("p1").set "c2", 3
			test.set "p2", 4
			test.get("p3").set "c1", 5
			test.get("p3").set "c2", 6
			test.set "p4", 7
			test.toString().should.equal "{ p0: 1, p1: { c1: 2, c2: 3 }, p2: 4, p3: { c1: 5, c2: 6 }, p4: 7 }"

	it "should have nested properties assignable", ->
		using parent = new parent_t(), child = new child_t(), ->
			parent.set "p0", 1
			parent.get("p1").set "c1", 2
			parent.get("p1").set "c2", 3
			parent.set "p2", 4
			parent.get("p3").set "c1", 5
			parent.get("p3").set "c2", 6
			parent.set "p4", 7

			child.set "c1", 8
			child.set "c2", 10

			parent.set "p1", child
			parent.get("p1").should.not.equal child
			parent.get("p1").get("c1").should.equal 8
			parent.get("p1").get("c2").should.equal 10

	it "should refuse to set nested property to incompatible struct", ->
		other_t = struct {
			c1: "i32",
			c2: "i32",
		}
		using parent = new parent_t(), other = new other_t(), ->
			expect(-> parent.set("p1", other)).to.throw /Cannot load/


describe "Redefined struct", ->
	struct1_t = typedef "struct1", struct {}
	struct2_2 = typedef "struct2", struct {
		field1: struct1_t,
		field2: struct1_t
	}
	struct1_t.redefine {
		value: "i32"
	}

	it "should work", ->
		using s2 = new struct2_2(), \
			s1 = new struct1_t(), ->

			s1.set "value", 12.3
			s2.get("field1").set "value", 123.7

			# Rounding should prove that we are using our own properties
			s2.get("field1").get("value").should.equal 123
			s2.set "field2", s1
			s2.get("field2").get("value").should.equal 12


describe "Recursive struct", ->
	test_t = typedef "test_t", struct {
		parent: SelfPtr,
		value: "i32"
	}
	test_t.resolve()

	it "should have a toString()", ->
		test_t.toString().should.equal "{ parent: *test_t, value: i32 }"

describe "Empty array", ->
	test_t = array "i32", 0
	test_t.resolve()

	it "should be a function", ->
		should.exist test_t
		test_t.should.be.a "function"

	it "should have a size of 0", ->
		test_t.size.should.equal 0
		test_t.count.should.equal 0

	it "should create an empty array instance", ->
		using test = new test_t(), ->
			test.toString().should.equal "[ ]"

	it "should have a non-null pointer", ->
		using test = new test_t(), ->
			test.$ptr.should.be.above 0

	it "should throw an error if it is freed for a second time", ->
		test = new test_t()
		free test
		expect(-> free test).to.throw # something


describe "Simple array of a single i32 field", ->
	test_t = array "i32", 1
	test_t.resolve()

	it "should have a size of 4", ->
		test_t.size.should.equal 4

	it "should create an instance initialized with zeroes", ->
		using test = new test_t(), ->
			test.toString().should.equal "[ 0 ]"

	it "should have a r/w element", ->
		using test = new test_t(), ->
			expect(test.get(0)).to.be.a "number"
			test.get(0).should.equal 0
			module.getValue(test.$ptr, "i32").should.equal 0
			test.set 0, 12
			test.get(0).should.equal 12
			module.getValue(test.$ptr, "i32").should.equal 12
			test.set 0, -12
			test.get(0).should.equal -12
			module.getValue(test.$ptr, "i32").should.equal -12

	it "should truncate non-integers", ->
		using test = new test_t(), ->
			test.set 0, -12.8
			test.get(0).should.equal -12
			module.getValue(test.$ptr, "i32").should.equal -12

	it "should check index overflows", ->
		using test = new test_t(), ->
			expect(-> test.set 1, 1).to.throw /Index out of bounds: 0 <= 1 < 1/
			expect(-> test.set -1, 1).to.throw /Index out of bounds: 0 <= -1 < 1/
			module.getValue(test.$ptr, "i32").should.equal 0

	it "should have a toString()", ->
		using test = new test_t(), ->
			test.set 0, 123
			test.toString().should.equal "[ 123 ]"
			test.set 0, -443
			test.toString().should.equal "[ -443 ]"


describe "Size 5 array of i8", ->
	test_t = array "i8", 5
	test_t.resolve()

	it "should have a size of 20 (i.e. 5 * 8 bit)", ->
		test_t.size.should.equal 5

	it "should have been initialized to 0", ->
		using test = new test_t(), ->
			for i in [0...5]
				test.get(i).should.equal 0

	it "should be assignable", ->
		using test = new test_t(), ->
			for i in [0...5]
				test.set i, i + 1
			test.get(0).should.equal 1
			test.get(1).should.equal 2
			test.get(2).should.equal 3
			test.get(3).should.equal 4
			test.get(4).should.equal 5

			test.set 1, 123
			test.get(0).should.equal 1
			test.get(1).should.equal 123
			test.get(2).should.equal 3
			test.get(3).should.equal 4
			test.get(4).should.equal 5

	it "should truncate non-integers", ->
		using test = new test_t(), ->
			test.set 0, 1.2,
			test.set 1, 2.9
			test.set 2, 3.1
			test.set 3, 4.5
			test.set 4, 17.9
			test.get(0).should.equal 1
			test.get(1).should.equal 2
			test.get(2).should.equal 3
			test.get(3).should.equal 4
			test.get(4).should.equal 17

	it "should handle overflows", ->
		using test = new test_t(), ->
			test.set 0, Math.pow(2, 8) + 1
			test.set 1, Math.pow(2, 16) + 2
			test.set 2, Math.pow(2, 32) + 3
			test.set 3, Math.pow(2, 32) + 4
			test.set 4, Math.pow(2, 8) + 7

			test.get(0).should.equal 1
			test.get(1).should.equal 2
			test.get(2).should.equal 3
			# "i64" is basically "i32" for us
			# TODO is this okay?
			test.get(3).should.equal 4
			test.get(4).should.equal 7


describe "Array of float", ->
	test_t = array "float", 3
	test_t.resolve()

	it "should have a size of 12", ->
		test_t.size.should.equal 3 * 4

	it "should handle floating point values", ->
		using test = new test_t(), ->
			test.set 0, 1.2
			test.set 1, 2.9
			test.set 2, 14.1

			test.get(0).should.be.closeTo 1.2, 0.001
			test.get(1).should.be.closeTo 2.9, 0.001
			test.get(2).should.be.closeTo 14.1, 0.001


describe "Array of double", ->
	test_t = array "double", 3
	test_t.resolve()

	it "should have a size of 24", ->
		test_t.size.should.equal 3 * 8

	it "should handle floating point values", ->
		using test = new test_t(), ->
			test.set 0, 1.2
			test.set 1, 2.9
			test.set 2, 14.1

			test.get(0).should.equal 1.2
			test.get(1).should.equal 2.9
			test.get(2).should.equal 14.1


describe "Array of struct", ->
	child_t = struct {
		c1: "i32",
		c2: "i32",
	}
	parent_t = array child_t, 2
	parent_t.resolve()

	it "should have the correct sizes", ->
		child_t.size.should.equal 2 * 4
		parent_t.size.should.equal 2 * 2 * 4

	it "should be created and initialized to all zero", ->
		using test = new parent_t(), ->
			test.get(0).should.be.an "object"
			test.get(1).should.be.an "object"

			test.get(0).get("c1").should.equal 0
			test.get(0).get("c2").should.equal 0
			test.get(1).get("c1").should.equal 0
			test.get(1).get("c2").should.equal 0

	it "should throw error if element is freed", ->
		using test = new parent_t(), ->
			expect(-> free test.get 0).to.throw # something

	it "should have working nested structs", ->
		using test = new parent_t(), ->
			test.get(0).set "c1", 7
			test.get(0).get("c1").should.equal 7
			test.get(1).set "c2", -12
			test.get(1).get("c2").should.equal -12
			module.getValue(test.$ptr + 0, "i32").should.equal 7
			module.getValue(test.$ptr + 4, "i32").should.equal 0
			module.getValue(test.$ptr + 8, "i32").should.equal 0
			module.getValue(test.$ptr + 12, "i32").should.equal -12
			module.getValue(test.get(0).$ptr + 0, "i32").should.equal 7
			module.getValue(test.get(1).$ptr + 4, "i32").should.equal -12

	it "should be created and initialized with values", ->
		using test = new parent_t(), ->
			test.get(0).set "c1", 2
			test.get(0).set "c2", 3
			test.get(1).set "c1", 5
			test.get(1).set "c2", 6

			test.get(0).get("c1").should.equal 2
			test.get(0).get("c2").should.equal 3
			test.get(1).get("c1").should.equal 5
			test.get(1).get("c2").should.equal 6

	it "should have a toString()", ->
		using test = new parent_t(), ->
			test.get(0).set "c1", 2
			test.get(0).set "c2", 3
			test.get(1).set "c1", 5
			test.get(1).set "c2", 6

			test.toString().should.equal "[ { c1: 2, c2: 3 }, { c1: 5, c2: 6 } ]"

	it "should have nested properties assignable", ->
		using parent = new parent_t(), \
				child = new child_t(), ->
			parent.get(0).set "c1", 2
			parent.get(0).set "c2", 3
			parent.get(1).set "c1", 5
			parent.get(1).set "c2", 6
			child.set "c1", 8
			child.set "c2", 10

			parent.set 0, child
			parent.get(0).should.not.equal child
			parent.get(0).get("c1").should.equal 8
			parent.get(0).get("c2").should.equal 10

	it "should refuse to set nested property to incompatible struct", ->
		other_t = struct {
			c1: "i32",
			c2: "i32",
		}
		using parent = new parent_t(), \
				other = new other_t(), ->
			parent.get(0).set "c1", 2
			parent.get(0).set "c2", 3
			parent.get(1).set "c1", 5
			parent.get(1).set "c2", 6
			other.set "c1", 8
			other.set "c2", 10

			expect(-> parent.set 0, other).to.throw /Cannot load/


target_t = struct {
	field: "i32",
}
target_ptr = ptr(target_t)

describe "Null pointer", ->

	it "should point to NULL", ->
		using test = new target_ptr(), ->
			expect(test.get()).to.be.null

	it "should have a toString() that returns NULL", ->
		using test = new target_ptr(), ->
			test.toString().should.equal "NULL"

describe "Simple pointer", ->

	it "should describe itself", ->
		target_ptr.toString().should.equal "*{ field: i32 }"
	
	it "should point to the right object", ->
		heap = allocate 1, "i32", module.ALLOC_NORMAL
		using test = new target_ptr(heap), \
				target = new target_t(), ->
			test.setAddress target.$ptr
			test.get().$ptr.should.equal target.$ptr

			test.setAddress 0
			expect(test.get()).to.be.null
		#module._free heap

	it "should have a toString()", ->
		heap = allocate 1, "i32", module.ALLOC_NORMAL
		using test = new target_ptr(heap), \
				target = new target_t(), ->
			target.set "field", 12345
			test.setAddress target.$ptr
			test.toString().should.equal "@#{target.$ptr}->{ field: 12345 }"
		#module._free heap

describe "Local pointer", ->

	it "should describe itself", ->
		target_ptr.toString().should.equal "*{ field: i32 }"
	
	it "should point to the right object", ->
		test = new target_ptr()
		using target = new target_t(), ->
			test.setAddress target.$ptr
			test.get().$ptr.should.equal target.$ptr

			test.setAddress 0
			expect(test.get()).to.be.null

	it "should have a toString()", ->
		using test = new target_ptr(), \
				target = new target_t(), ->
		target.set "field", 12345
		test.setAddress target.$ptr
		test.toString().should.equal "@#{target.$ptr}->{ field: 12345 }"

###
testString = define string, "testString", prefix: string, len: "i32"

describe "Strings", ->
	it "should work as parameters and return values", ->
		using sWorld = new string("World"), ->
			sGreet = testString sWorld
			sGreet.toString().should.equal "World, hello"

doSomething = define "i32", "doSomething", a: "i32", b: "i32", op: "i32"
printSomething = define "i32", "printSomething"
addFunc = callback "i32", "addFunc", {a: "i32", b: "i32" }, (a, b) -> a + b

describe "External function", ->
	it "should be called", ->
		res = doSomething 10, 21, addFunc
		# console.log "Res: #{res}"
		res.should.equal 31

describe "String", ->
	it "when just allocated should have a toString() '(null)'", ->
		using test = new string(), ->
			test.toString().should.equal "(null)"

	it "should return the right string when address is pointed to a string", ->
		using test = new string(), ->
			test.set "address", allocate intArrayFromString("Lajos"), "i8", module.ALLOC_STACK
			test.toString().should.equal "Lajos"


describe "Local string", ->
	it "should have a toString() of (null) by default", ->
		using test = new localString(), ->
			test.toString().should.equal "(null)"

	it "should return the right string when allocated to a string", ->
		test = new localString(allocate intArrayFromString("Tibor"), "i8", module.ALLOC_STACK)
		test.toString().should.equal "Tibor"
###

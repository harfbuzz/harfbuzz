Module["free"] = free = (struct) ->
	_free struct["$ptr"]

remapCallbacks = []
Module["registerMemoryRemapCallback"] = registerMemoryRemapCallback = (remapCallback, userData) ->
	# console.log "Mapping memory from #{offset}, length: #{length}, first byte: #{getValue offset, 'i8'}"
	callbackData = "userData": userData, "callback": remapCallback
	remapCallbacks.push callbackData
	return callbackData

Module["unregisterMemoryRemapCallback"] = unregisterMemoryRemapCallback = (callbackData) ->
	index = remapCallbacks.indexOf callbackData
	remapCallbacks.splice index, 1 unless index is -1

# When memory size is increased, we need to re-map our arrays
__originalEnlargeMemory = enlargeMemory
enlargeMemory = ->
	__originalEnlargeMemory()

	for remapCallback in remapCallbacks
		remapCallback["callback"] Module["HEAPU8"], remapCallback["userData"]

Module["Void"] = Void = "i32"

Module["SelfPtr"] = SelfPtr = {}

NON_HEAP = -1
INVALID_SIZE = -1

class CObject
	constructor: (heap) ->
		# First give a chance for the type to resolve itself
		@["$type"]["resolve"]()

		if not heap?
			# console.log "Allocating #{@["$type"]["size"]} bytes for #{@["$type"].toString()}"
			heap = allocate @["$type"]["size"], "i8", ALLOC_NORMAL
		if heap != NON_HEAP
			# console.log "Setting $ptr to #{heap}"
			@["$ptr"] = heap

	@::["$offset"] = (index) ->
		if @["$ptr"] == NON_HEAP then throw new Error "Non-heap"
		return new @["$type"](@["$ptr"] + index * @["$type"]["size"])

	@::["$next"] = ->
		@["$offset"] 1

	@::["toString"] = (stack = []) ->
		dumpData @, @["$type"], stack

	@["resolve"] = ->
		throw new Error("Unknown type, cannot resolve")

	@["writeTo"] = (address, value) ->
		if value["$ptr"] == NON_HEAP then throw new Error "Non heap object"
		if value["$ptr"] == 0 then throw new Error "Null reference"
		_memcpy address, value["$ptr"], @["size"]

	@["fromNative"] = (value) ->
		new @(value)

	@["toNative"] = (value) ->
		if value is null then 0 else value["$ptr"]


Module["struct"] = struct = (fields) -> class CStruct extends CObject
	@::["$type"] = @

	constructor: (heap) ->
		super heap

	@::["toString"] = (stack = []) ->
		"{" + (" #{field}: #{dumpData @['get'](field), type, stack}" for own field, type of fields).join(",") + " }"

	@["toString"] = (stack = []) ->
		"{" + (" #{field}: #{dumpType type, stack}" for own field, type of fields).join(",") + " }"

	resolved = false

	@["redefine"] = (newFields) ->
		if resolved then throw new Error "Type #{@} is already resolved"
		fields = newFields

	getters = {}
	setters = {}
	@["resolve"] = ->
		if resolved then return

		fieldsNotToResolve = []
		for own field, type of fields
			if type is SelfPtr
				fields[field] = ptr(@)
				fieldsNotToResolve.push field

		# console.log "Defining fields:", fields

		# Define properties
		offset = 0
		for own field, type of fields
			# console.log "Creating property #{field} at #{offset} of type #{type} " + new Error("Trace").stack
			do (offset, field, type) ->
				if simpleType type
					# console.log "Creating simple property at #{offset} with type #{type}"
					getters[field] = (object) ->
						# console.log "Getting #{field} at #{object['$ptr']} + #{offset}"
						getValue object["$ptr"] + offset, type
					setters[field] = (object, value) ->
						# console.log "Setting #{field} to #{value} at #{object['$ptr']} + #{offset}"
						setValue object["$ptr"] + offset, value, type
						return
				else
					# console.log "Creating compound property at #{offset} with type #{type}"
					type["resolve"]() if fieldsNotToResolve.indexOf(field) is -1
					compoundProperty = null

					getters[field] = (object) ->
						# console.log "Getting compound #{field} at #{object['$ptr']} + #{offset}"
						if compoundProperty is null
							# console.log "Creating compound at #{object['ptr']} + #{offset} for #{field}"
							compoundProperty = new type(object["$ptr"] + offset)
						return compoundProperty
					setters[field] = (object, otherStruct) ->
						# console.log "Setting compound #{field} at #{object['$ptr']} + #{offset} to #{otherStruct}"
						if otherStruct["$type"] isnt type
							throw new Error "Cannot load incompatible data: #{type} vs. #{otherStruct['$type']}"
						writeTo object["$ptr"] + offset, otherStruct, type
						return
			offset += sizeof type

		@["size"] = offset

		resolved = true

	@::["get"] = (field) ->
		getters[field](@)

	@::["set"] = (field, value) ->
		setters[field](@, value)
		return


Module["array"] = array = (elemType, count) -> class CArray extends CObject
	if not elemType? then throw new Error "Element type is not specified"
	if typeof count isnt "number" or count < 0 then throw new Error "Array size must be non-negative: #{count}"

	@::["$type"] = @
	@["count"] = count
	@["elemType"] = elemType

	constructor: (heap) ->
		super heap

	checkIndex = (index) ->
		if not (0 <= index < count)
			throw new Error "Index out of bounds: 0 <= #{index} < #{count}"
		return index

	@::["get"] = (index) ->
		address = @["$ptr"] + checkIndex(index) * sizeofType elemType
		if simpleType elemType
			getValue address, elemType
		else
			new elemType address

	@::["set"] = (index, value) ->
		address = @["$ptr"] + checkIndex(index) * sizeofType elemType
		if simpleType elemType
			if typeof value isnt "number"
				throw new Error "Cannot load #{typeof value} to #{elemType}"
			setValue address, value, elemType
		else
			if value["$type"] isnt elemType
				throw new Error "Cannot load #{value["$type"]} to #{elemType}"
			writeTo address, value, elemType
		return

	@::["getAddress"] = ->
		@["ptr"]()

	@::["ptr"] = (index) ->
		type = if simpleType elemType then simplePointerTypes[elemType] else elemType
		new type @["$ptr"] + checkIndex(index) * sizeofType elemType

	@::["toArray"] = ->
		@["get"](i) for i in [0...count]

	@::["toString"] = (stack = []) ->
		"[" + (" #{dumpData @get(index), elemType, stack}" for index in [0...count]).join(",") + " ]"

	@["toString"] = (stack = []) ->
		"#{dumpType elemType, stack}[#{count}]"

	resolved = false
	@["resolve"] = ->
		if resolved then return
		elemType["resolve"]() unless simpleType elemType
		size = count * sizeofType elemType
		@["size"] = size
		resolved = true


Module["ptr"] = ptr = (targetType) -> class CPointer extends CObject
	if not targetType? then throw new Error "Target type is not specified"

	@::["$type"] = @
	size = sizeof "i32"
	@["size"] = size

	_address = 0

	constructor: (heap = NON_HEAP, target = null) ->
		super heap
		@nonHeap = heap == NON_HEAP
		if @nonHeap
			# console.log "Non-heap pointer pointing to #{target}"
			_address = addressof target

	@::["getAddress"] = ->
		if @nonHeap
			_address
		else
			getValue(@["$ptr"], "i32")

	@::["setAddress"] = (targetAddress) ->
		if @nonHeap
			_address = targetAddress
		else
			setValue @["$ptr"], targetAddress, "i32"

	@::["get"] = ->
		address = @["getAddress"]()
		if address == 0 then null
		else if simpleType targetType then getValue address, targetType
		else new targetType address

	@::["set"] = (target) ->
		if @["getAddress"]() == 0 then throw new Error "Null reference"
		writeTo @["getAddress"](), target, targetType
		return

	@::["toString"] = (stack = []) ->
		address = @["getAddress"]()
		if address == 0
			"NULL"
		else
			"@#{address}->#{dumpData @['get'](), targetType, stack}"

	@["toString"] = (stack = []) ->
		"*#{dumpType targetType, stack}"

	resolved = false
	@["resolve"] = ->
		if resolved then return
		resolved = true
		targetType["resolve"]() unless simpleType targetType

	@["fromNative"] = (value) ->
		result = new @()
		result["setAddress"] value
		return result

	@["toNative"] = (value) ->
		if value is null then 0 else value["getAddress"]()


Module["string"] = string = class CString extends CObject
	@::["$type"] = @

	constructor: (arg, alloc = ALLOC_NORMAL) ->
		if arg is null
			super 0
		else if typeof arg is "number"
			super arg
		else if typeof arg is "string"
			super allocate intArrayFromString(arg), "i8", alloc
		else
			throw new Error "Cannot create a string from #{arg}"

	@::["toString"] = ->
		Pointer_stringify(@["$ptr"])

	@["toString"] = ->
		"*char"

	@["resolve"] = ->

	@["allocate"] = (size, alloc = ALLOC_NORMAL) ->
		new @ allocate size, "i8", alloc


Module["define"] = define = (returnType, name, argumentsDef = {}) ->
	returnNative = nativeTypeOf returnType
	argumentTypes = (type for own argument, type of argumentsDef)
	argumentNativeTypes = (nativeTypeOf type for type in argumentTypes)

	# console.log "Defining #{returnType} #{name} ( " + ("#{arg}: #{type}" for own arg, type of argumentsDef).join(", ") + " )"
	cFunc = cwrap name, returnNative, argumentNativeTypes

	Module[name] = (args...) ->
		nativeArgs = new Array(argumentTypes.length)
		# console.log "Calling '#{name}' with args:"
		# for i in [0...args.length]
		# 	console.log "  -> ##{i}: #{args[i]}"
		for i in [0...argumentTypes.length]
			# console.log "Type #{i}: #{argumentTypes[i]}"
			nativeArgs[i] = toNative args[i], argumentTypes[i]
		# console.log "ccall '#{name}', '#{returnNative}',", argumentNativeTypes, ",", nativeArgs

		resultNative = cFunc.apply null, nativeArgs

		# console.log "  native result:", resultNative
		result = fromNative resultNative, returnType
		# console.log "That is: #{result}"
		return result


Module["callback"] = callback = (returnType, name, argumentsDef = {}, func) ->
	argumentTypes = (type for own argument, type of argumentsDef)

	callbackFunc = (nativeArgs...) ->
		# console.log "Calling callback #{name} with arguments:", nativeArgs
		# console.log "  -- argument defs:", argumentsDef
		args = new Array(argumentTypes.length)
		for i in [0...argumentTypes.length]
			args[i] = fromNative nativeArgs[i], argumentTypes[i]

		result = func.apply null, args

		# console.log "  result: #{result}"

		resultNative = toNative result, returnType
		# console.log "native result: ", resultNative
		return resultNative

	functionIndex = Runtime.addFunction callbackFunc, "x"

	if name? then Module[name] = functionIndex

	return functionIndex


Module["unregisterCallback"] = unregisterCallback = (functionIndex) ->
	Runtime.removeFunction functionIndex


Module["typedef"] = typedef = (name, type) ->
	# console.log "Defining #{name} = #{type}"
	type["typeName"] = name
	Module[name] = type


addressof = (value, type) ->
	if simpleType type then throw new Error "Simple types don't have an address"
	if value is null then 0 else value["$ptr"]

sizeof = (type) ->
	if simpleType type then Runtime.getNativeFieldSize type else type["size"]

sizeofType = (type) ->
	if simpleType type then Runtime.getNativeTypeSize type else type["size"]

writeTo = (address, value, type) ->
	if simpleType type
		setValue address, value, type
	else
		type["writeTo"] address, value

simpleType = (type) ->
	typeof type is "string"

nativeTypeOf = (type) ->
	# TODO Add string support sometime
	return "number"

fromNative = (value, type) ->
	# console.log "Converting #{value} from native to #{if type then type.toString() else type}"
	if simpleType type then value else type["fromNative"] value

toNative = (value, type) ->
	# console.log "Converting #{value} to native from #{if type then type.toString() else type}"
	if simpleType type then value else type["toNative"] value

dumpType = (type, stack) ->
	if simpleType type
		s = type
	else if stack.indexOf(type) > -1
		s = if type["typeName"] then type["typeName"] else "<backreference>"
	else
		if not type["typeName"]?
			stack.push type
			s = type["toString"] stack
			stack.pop()
		else
			s = type["typeName"]
	return s

dumpData = (value, type, stack) ->
	s = null
	if simpleType type
		s = value
	else
		address = value["$ptr"]
		if address == 0
			s = "NULL"
		else
			if stack.indexOf(address) > -1
				s = "<backreference>"
			else 
				stack.push address
				s = value["toString"] stack
				stack.pop()
	return s

simplePointerTypes = {}
for type in ["i1", "i8", "i16", "i32", "i64", "float", "double"]
	simplePointerTypes[type] = ptr type

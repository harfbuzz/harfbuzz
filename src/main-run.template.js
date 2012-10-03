var fs = require("fs");

var file = process.argv[2];
console.log("Loading " + file);
var input = fs.readFileSync(file, "binary");
var Module = {
      input: input,
    };

{{{
// GENERATED_CODE
}}}

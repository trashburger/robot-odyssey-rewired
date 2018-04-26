const Bundler = require('parcel-bundler');
const Path = require('path');
const process = require('process');

const file = Path.join(__dirname, './src/index.html');

const options = {};

const bundler = new Bundler(file, options);

if (process.argv.includes("build")) {
	bundler.bundle().then(function () {
		process.exit(0);
	})

} else if (process.argv.includes("serve")) {
	bundler.serve(1234, false);

} else {
	console.log("unsupported");
	process.exit(1);
}

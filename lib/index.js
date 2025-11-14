const path = require('node:path');
const nodeGypBuild = require('node-gyp-build');

// Pass the **package root** to node-gyp-build, not lib/
module.exports = nodeGypBuild(path.resolve(__dirname, '..'));

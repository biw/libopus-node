const { resolve } = require('node:path');
const { find } = require('@mapbox/node-pre-gyp');

// eslint-disable-next-line import/no-dynamic-require
module.exports = require(find(resolve(__dirname, '..', 'package.json')));

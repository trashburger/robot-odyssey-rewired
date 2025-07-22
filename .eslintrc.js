module.exports = {
    'env': {
        'browser': true,
        'es6': true,
        'node': false
    },
    'extends': 'eslint:recommended',
    'parser': 'babel-eslint',
    'parserOptions': {
        'sourceType': 'module'
    },
    'rules': {
        'semi': [ 'error', 'always' ],
        'eqeqeq': [ 'error', 'always' ],
    }
};

const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const UglifyJsPlugin = require("uglifyjs-webpack-plugin");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const FaviconsWebpackPlugin = require('favicons-webpack-plugin')

const title = 'Robot Odyssey Rewired';

module.exports = {
    mode: 'production',
    entry: './src/main.js',
    output: {
        filename: '[name].[hash].js',
        path: path.resolve(__dirname, 'dist')
    },
    node: {
        fs: 'empty'
    },
    plugins: [
        new MiniCssExtractPlugin({
            filename: '[name].[hash].css'
        }),
        new HtmlWebpackPlugin({
            template: './src/main.html',
            title,
        }),
        new FaviconsWebpackPlugin({
            logo: './src/scanner-512px.png',
            prefix: './',
            persistentCache: false,
            title,
            background: '#000',
            icons: {
                favicons: true,
                appleIcon: true,
                appleStartup: false,
                android: true,
                firefox: true,
            }
        }),
    ],
    module: {
        rules: [
            { test: /\.css$/, use: [ MiniCssExtractPlugin.loader, 'css-loader' ] },
            { test: /\.png$/, loader: 'file-loader' },
        ]
    },
    optimization: {
        minimizer: [
            new UglifyJsPlugin({}),
            new OptimizeCSSAssetsPlugin({})
        ]
    },
};

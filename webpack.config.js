const path = require('path');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const UglifyJsPlugin = require("uglifyjs-webpack-plugin");
const OptimizeCSSAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const OfflinePlugin = require('offline-plugin');
const HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');
var FaviconsWebpackPlugin = require('favicons-webpack-plugin')
var WebpackPwaManifest = require('webpack-pwa-manifest')

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
    performance: {
        // The WASM is getting included in the entry point size, even though
        // webpack here is only processing the URL of the resource and we're still
        // downloading it asynchronously. Oh well.
        hints: false,
    },
    plugins: [
        new MiniCssExtractPlugin({
            filename: '[name].[hash].css'
        }),
        new HtmlWebpackPlugin({
            template: './src/main.html',
            inlineSource: '.css$',
            title,
        }),
        new HtmlWebpackInlineSourcePlugin(),
        new FaviconsWebpackPlugin({
            logo: path.resolve('./build/show/icon.png'),
            prefix: 'icon.[hash].',
            persistentCache: false,
            title,
            background: '#000',
            icons: {
                // No manifest outputs, only favicon and apple
                favicons: true,
                appleIcon: true,
                appleStartup: false,
                android: false,
                firefox: false,
            }
        }),
        new WebpackPwaManifest({
            name: title,
            background_color: '#000000',
            icons: [
                {
                    src: path.resolve('./build/show/icon.png'),
                    sizes: [32, 96, 128, 192, 256, 384, 512]
                }
            ],
        }),
        // Offline plugin should be last
        new OfflinePlugin(),
    ],
    module: {
        defaultRules: [
            {
                type: "javascript/auto",
                resolve: {}
            },
        ],
        rules: [
            {
                test: /\.css$/,
                use: [ MiniCssExtractPlugin.loader, 'css-loader' ]
            },
            {
                test: /\.js$/,
                exclude: /(node_modules|bower_components)/,
                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: ['babel-preset-env']
                    }
                }
            },
            {
                test: /\.png$/,
                loader: 'url-loader',
                options: {
                    limit: 8192,
                    fallback: 'file-loader',
                    name: '[name]-[hash].[ext]',
                },
            },
            {
                test: /\.wasm$/,
                loader: 'file-loader',
                options: {
                    name: '[name]-[hash].[ext]',
                },
            }
        ]
    },
    optimization: {
        minimizer: [
            new UglifyJsPlugin({}),
            new OptimizeCSSAssetsPlugin({})
        ]
    },
};
